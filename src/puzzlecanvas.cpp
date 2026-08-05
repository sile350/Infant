#include "puzzlecanvas.h"

#include "exerciseassets.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTransform>
#include <QtMath>

namespace {

constexpr int kDesignWidth = 1920;
constexpr int kDesignHeight = 1080;

QString stepSlug(const QString &stepId) {
    QString slug = stepId;
    slug.remove(QLatin1Char(' '));
    return slug.toLower();
}

QString hintFileForExercise(const QString &exerciseId, const QString &stepId) {
    if (exerciseId == QStringLiteral("1.14") && stepId == QStringLiteral("2")) {
        return QStringLiteral("p2.png");
    }
    if (exerciseId == QStringLiteral("1.24")) {
        return QStringLiteral("ex") + stepId + QStringLiteral(".png");
    }
    if (exerciseId == QStringLiteral("1.19") || exerciseId == QStringLiteral("1.20")
        || exerciseId == QStringLiteral("1.21")) {
        return QStringLiteral("p") + stepSlug(stepId) + QStringLiteral(".png");
    }
    return QString();
}

} // namespace

PuzzleCanvas::PuzzleCanvas(QWidget *parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });
}

QSize PuzzleCanvas::designSize() const {
    return QSize(kDesignWidth, kDesignHeight);
}

QPoint PuzzleCanvas::mapFromDesign(int x, int y) const {
    return QPoint(qRound(x * m_scale) + m_offset.x(), qRound(y * m_scale) + m_offset.y());
}

void PuzzleCanvas::loadExercise(const QString &exerciseId, const QString &stepId, const PuzzleLayout &layout) {
    m_exerciseId = exerciseId;
    m_stepId = stepId;
    m_layout = layout;
    m_sprites.clear();
    m_rotateAllowed = layout.rotateAllowed;
    m_selectMode = layout.selectMode;
    m_scale = 1.0;
    m_offset = QPoint(0, 0);
    m_elapsed = 0;
    m_moving = -1;
    m_dragging = false;
    m_movedDuringDrag = false;

    if (!layout.backgroundFile.isEmpty()) {
        const QString path = ExerciseAssets::exerciseFile(exerciseId, layout.backgroundFile);
        if (!path.isEmpty()) {
            m_background = QPixmap(path);
        }
    }
    if (layout.showTemplate && !layout.templateFile.isEmpty()) {
        const QString path = ExerciseAssets::exerciseFile(exerciseId, layout.templateFile);
        if (!path.isEmpty()) {
            m_template = QPixmap(path);
        }
    }

    const bool hidePreviewHint =
        exerciseId == QStringLiteral("1.11") && stepId == QStringLiteral("2");
    m_hintX = 200;
    m_hintY = 100;
    if (exerciseId == QStringLiteral("1.14") && stepId == QStringLiteral("2")) {
        m_hintX = 200;
        m_hintY = 230;
    } else if (exerciseId == QStringLiteral("1.24")) {
        m_hintX = 200;
        m_hintY = stepId == QStringLiteral("4") ? 231 : 131;
    }
    if (hidePreviewHint) {
        m_hintPixmap = QPixmap();
        m_showHint = false;
    } else {
        const QString namedHint = hintFileForExercise(exerciseId, stepId);
        QString hintPath;
        if (!namedHint.isEmpty()) {
            hintPath = ExerciseAssets::exerciseFile(exerciseId, namedHint);
        }
        if (hintPath.isEmpty()) {
            const QString hintName = QStringLiteral("p") + stepId.toLower().remove(QLatin1Char(' '));
            hintPath = ExerciseAssets::exerciseFile(exerciseId, hintName + QStringLiteral(".png"));
        }
        if (hintPath.isEmpty()) {
            hintPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("f") + stepId + QStringLiteral(".png"));
        }
        if (!hintPath.isEmpty()) {
            m_hintPixmap = QPixmap(hintPath);
        } else {
            m_hintPixmap = QPixmap();
        }
        m_showHint = !m_hintPixmap.isNull();
    }
    m_storyVisible = true;
    m_showTemplate = true;

    if (!layout.template2File.isEmpty()) {
        const QString path2 = ExerciseAssets::exerciseFile(exerciseId, layout.template2File);
        m_template2 = path2.isEmpty() ? QPixmap() : QPixmap(path2);
    } else {
        m_template2 = QPixmap();
    }

    for (const PuzzleSpriteDef &def : layout.sprites) {
        if (def.file.startsWith(QStringLiteral("et"), Qt::CaseInsensitive)) {
            continue;
        }
        const QString path = ExerciseAssets::exerciseFile(exerciseId, def.file);
        if (path.isEmpty()) {
            continue;
        }
        Sprite sprite;
        sprite.pixmap = QPixmap(path);
        sprite.x = def.x;
        sprite.y = def.y;
        sprite.targetX = def.targetX;
        sprite.targetY = def.targetY;
        sprite.name = def.name;
        sprite.clickable = def.clickable;
        sprite.closed = def.closed;
        sprite.closedFile = def.closedFile;
        sprite.openFile = def.openFile;
        sprite.returnable = def.returnable;
        if (def.returnable && def.targetX >= 0 && def.targetY >= 0) {
            sprite.targetX = def.targetX;
            sprite.targetY = def.targetY;
        } else if (def.returnable) {
            sprite.targetX = def.x;
            sprite.targetY = def.y;
        }
        if (!def.selectFile.isEmpty()) {
            const QString selectPath = ExerciseAssets::exerciseFile(exerciseId, def.selectFile);
            if (!selectPath.isEmpty()) {
                sprite.selectPixmap = QPixmap(selectPath);
            }
        }
        m_sprites.append(sprite);
    }

    const QSize design = designSize();
    const double scaleX = width() > 0 ? static_cast<double>(width()) / design.width() : 1.0;
    const double scaleY = height() > 0 ? static_cast<double>(height()) / design.height() : 1.0;
    m_scale = qMin(1.0, qMin(scaleX, scaleY));
    const int contentW = qRound(design.width() * m_scale);
    const int contentH = qRound(design.height() * m_scale);
    m_offset = QPoint((width() - contentW) / 2, (height() - contentH) / 2);

    m_timer.start();
    update();
}

QString PuzzleCanvas::positionsSnapshot() const {
    QStringList parts;
    for (const Sprite &sprite : m_sprites) {
        parts.append(QStringLiteral("%1,%2").arg(sprite.x).arg(sprite.y));
    }
    return parts.join(QLatin1Char(';'));
}

void PuzzleCanvas::rotateSpriteC(Sprite &sprite) {
    if (sprite.done || sprite.pixmap.isNull()) {
        return;
    }
    QTransform transform;
    transform.rotate(-90);
    sprite.pixmap = sprite.pixmap.transformed(transform, Qt::SmoothTransformation);
    ++sprite.rotateState;
}

void PuzzleCanvas::rotateSpriteTo(Sprite &sprite) {
    if (sprite.done || sprite.pixmap.isNull()) {
        return;
    }
    QTransform transform;
    transform.rotate(90);
    sprite.pixmap = sprite.pixmap.transformed(transform, Qt::SmoothTransformation);
    --sprite.rotateState;
}

void PuzzleCanvas::applySessionOptions(const ExerciseSessionOptions &options) {
    m_showHint = options.showHint;
    const bool namedTemplateExercise = m_exerciseId == QStringLiteral("1.14")
        || m_exerciseId == QStringLiteral("1.19") || m_exerciseId == QStringLiteral("1.20")
        || m_exerciseId == QStringLiteral("1.21");
    if (namedTemplateExercise) {
        QString templateFile;
        if (m_exerciseId == QStringLiteral("1.14") && m_stepId == QStringLiteral("2")) {
            templateFile = options.showTemplate ? QStringLiteral("t2.png") : QStringLiteral("et2.png");
        } else {
            const QString slug = stepSlug(m_stepId);
            templateFile = (options.showTemplate ? QStringLiteral("t") : QStringLiteral("et")) + slug
                + QStringLiteral(".png");
        }
        const QString path = ExerciseAssets::exerciseFile(m_exerciseId, templateFile);
        m_template = path.isEmpty() ? QPixmap() : QPixmap(path);
        m_showTemplate = !m_template.isNull();
    } else {
        m_showTemplate = options.showTemplate;
        if (options.showTemplate && !m_layout.templateFile.isEmpty()) {
            const QString path = ExerciseAssets::exerciseFile(m_exerciseId, m_layout.templateFile);
            m_template = path.isEmpty() ? QPixmap() : QPixmap(path);
        } else if (!options.showTemplate) {
            m_template = QPixmap();
        }
    }
    if (m_exerciseId == QStringLiteral("1.22")) {
        m_selectMode = options.e15SelectMode;
    }
    m_rotateAllowed = options.rotateEnabled && m_layout.rotateAllowed;
    if (options.rotateEnabled && (options.rotateW > 0 || options.rotateCW > 0)) {
        applyRotates(options.rotateW, options.rotateCW);
    }
    update();
}

void PuzzleCanvas::setSelectHighlightMode(bool highlight) {
    if (m_exerciseId == QStringLiteral("1.22")) {
        m_selectMode = highlight;
        for (Sprite &sprite : m_sprites) {
            sprite.selected = false;
        }
        update();
    }
}

void PuzzleCanvas::setStoryVisible(bool visible) {
    m_storyVisible = visible;
    update();
}

void PuzzleCanvas::setSpriteVisible(const QString &name, bool visible) {
    for (Sprite &sprite : m_sprites) {
        if (sprite.name == name) {
            sprite.hidden = !visible;
        }
    }
    update();
}

void PuzzleCanvas::setShowTemplate(bool show) {
    m_showTemplate = show;
    update();
}

void PuzzleCanvas::setShowHint(bool show) {
    m_showHint = show;
    update();
}

void PuzzleCanvas::applyRotates(int rotateW, int rotateCW) {
    bool direction = false;
    for (int i = 0; i < m_sprites.size(); ++i) {
        if (rotateW > 0) {
            if (!direction) {
                rotateSpriteC(m_sprites[i]);
                direction = true;
            } else {
                rotateSpriteTo(m_sprites[i]);
                direction = false;
            }
            --rotateW;
            if (rotateCW > 0) {
                ++i;
            }
        }
        if (rotateCW > 0) {
            rotateSpriteTo(m_sprites[i]);
            rotateSpriteTo(m_sprites[i]);
            --rotateCW;
        }
    }
}

bool PuzzleCanvas::hitTest(const Sprite &sprite, int x, int y) const {
    if (sprite.done || sprite.pixmap.isNull()) {
        return false;
    }
    const QPoint topLeft = mapFromDesign(sprite.x, sprite.y);
    const int w = qRound(sprite.pixmap.width() * m_scale);
    const int h = qRound(sprite.pixmap.height() * m_scale);
    if (x < topLeft.x() || y < topLeft.y() || x >= topLeft.x() + w || y >= topLeft.y() + h) {
        return false;
    }
    const int localX = qRound((x - topLeft.x()) / m_scale);
    const int localY = qRound((y - topLeft.y()) / m_scale);
    if (localX < 0 || localY < 0 || localX >= sprite.pixmap.width() || localY >= sprite.pixmap.height()) {
        return false;
    }
    const QRgb pixel = sprite.pixmap.toImage().pixel(localX, localY);
    return qAlpha(pixel) > 16 && !(qRed(pixel) < 8 && qGreen(pixel) < 8 && qBlue(pixel) < 8);
}

void PuzzleCanvas::snapSprite(Sprite &sprite) {
    if (sprite.targetX < 0 || sprite.targetY < 0) {
        return;
    }
    const double rotationTurns = static_cast<double>(sprite.rotateState) / 4.0;
    if (qAbs(sprite.x - sprite.targetX) < 12 && qAbs(sprite.y - sprite.targetY) < 12
        && qAbs(rotationTurns - qRound(rotationTurns)) < 0.01) {
        sprite.x = sprite.targetX;
        sprite.y = sprite.targetY;
        sprite.done = true;
    }
}

void PuzzleCanvas::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    if (!m_background.isNull()) {
        const QPoint origin = mapFromDesign(0, 0);
        painter.drawPixmap(
            origin.x(),
            origin.y(),
            qRound(m_background.width() * m_scale),
            qRound(m_background.height() * m_scale),
            m_background);
    }

    if (!m_template.isNull() && m_showTemplate) {
        const QPoint origin = mapFromDesign(m_layout.templateX, m_layout.templateY);
        painter.drawPixmap(
            origin.x(),
            origin.y(),
            qRound(m_template.width() * m_scale),
            qRound(m_template.height() * m_scale),
            m_template);
    }
    if (!m_template2.isNull() && m_showTemplate) {
        const QPoint origin = mapFromDesign(m_layout.template2X, m_layout.template2Y);
        painter.drawPixmap(
            origin.x(),
            origin.y(),
            qRound(m_template2.width() * m_scale),
            qRound(m_template2.height() * m_scale),
            m_template2);
    }

    for (const Sprite &sprite : m_sprites) {
        if (sprite.pixmap.isNull() || sprite.hidden) {
            continue;
        }
        const QPoint origin = mapFromDesign(sprite.x, sprite.y);
        const QPixmap &drawPixmap =
            sprite.selected && !sprite.selectPixmap.isNull() ? sprite.selectPixmap : sprite.pixmap;
        painter.save();
        painter.translate(origin.x() + qRound(drawPixmap.width() * m_scale) / 2,
                          origin.y() + qRound(drawPixmap.height() * m_scale) / 2);
        painter.rotate(sprite.rotateState * 90.0);
        painter.translate(-qRound(drawPixmap.width() * m_scale) / 2,
                          -qRound(drawPixmap.height() * m_scale) / 2);
        painter.drawPixmap(
            0,
            0,
            qRound(drawPixmap.width() * m_scale),
            qRound(drawPixmap.height() * m_scale),
            drawPixmap);
        painter.restore();
        if (sprite.selected) {
            painter.setPen(QPen(Qt::red, 2));
            painter.drawRect(
                origin.x(),
                origin.y(),
                qRound(drawPixmap.width() * m_scale),
                qRound(drawPixmap.height() * m_scale));
        }
    }

    if (!m_hintPixmap.isNull() && m_showHint
        && (m_exerciseId == QStringLiteral("1.14") || m_storyVisible)) {
        const QPoint hintOrigin = mapFromDesign(m_hintX, m_hintY);
        painter.drawPixmap(
            hintOrigin.x(),
            hintOrigin.y(),
            qRound(m_hintPixmap.width() * m_scale),
            qRound(m_hintPixmap.height() * m_scale),
            m_hintPixmap);
    }
}

void PuzzleCanvas::mousePressEvent(QMouseEvent *event) {
    if (m_selectMode) {
        for (int i = 0; i < m_sprites.size(); ++i) {
            if (hitTest(m_sprites[i], event->x(), event->y())) {
                m_sprites[i].selected = !m_sprites[i].selected;
                update();
                return;
            }
        }
        return;
    }

    for (int i = m_sprites.size() - 1; i >= 0; --i) {
        if (hitTest(m_sprites[i], event->x(), event->y())) {
            m_moving = i;
            const QPoint topLeft = mapFromDesign(m_sprites[i].x, m_sprites[i].y);
            m_dragOffset = event->pos() - topLeft;
            m_dragging = true;
            m_movedDuringDrag = false;
            return;
        }
    }
}

void PuzzleCanvas::mouseMoveEvent(QMouseEvent *event) {
    if (!m_dragging || m_moving < 0 || m_selectMode) {
        return;
    }
    const QPoint designPos(
        qRound((event->x() - m_offset.x() - m_dragOffset.x()) / m_scale),
        qRound((event->y() - m_offset.y() - m_dragOffset.y()) / m_scale));
    m_sprites[m_moving].x = designPos.x();
    m_sprites[m_moving].y = designPos.y();
    m_movedDuringDrag = true;
    update();
}

void PuzzleCanvas::mouseReleaseEvent(QMouseEvent *event) {
    if (m_moving < 0 || !m_dragging) {
        return;
    }
    Sprite &sprite = m_sprites[m_moving];

    if (m_exerciseId == QStringLiteral("2.11") && sprite.clickable && !sprite.name.isEmpty()
        && event->button() == Qt::LeftButton && !m_movedDuringDrag) {
        if (sprite.closed && !sprite.openFile.isEmpty()) {
            const QString path = ExerciseAssets::exerciseFile(m_exerciseId, sprite.openFile);
            if (!path.isEmpty()) {
                sprite.pixmap = QPixmap(path);
                sprite.closed = false;
                sprite.clickable = false;
            }
        } else if (!sprite.closed && !sprite.closedFile.isEmpty()) {
            const QString path = ExerciseAssets::exerciseFile(m_exerciseId, sprite.closedFile);
            if (!path.isEmpty()) {
                sprite.pixmap = QPixmap(path);
                sprite.closed = true;
            }
        }
        m_dragging = false;
        m_moving = -1;
        update();
        return;
    }

    if (m_movedDuringDrag) {
        snapSprite(sprite);
    } else if (sprite.returnable && sprite.targetX >= 0 && sprite.targetY >= 0) {
        sprite.x = sprite.targetX;
        sprite.y = sprite.targetY;
    } else if (m_rotateAllowed && event->button() == Qt::RightButton) {
        QTransform transform;
        transform.rotate(-90);
        sprite.pixmap = sprite.pixmap.transformed(transform, Qt::SmoothTransformation);
        --sprite.rotateState;
    } else if (m_rotateAllowed && event->button() == Qt::LeftButton) {
        QTransform transform;
        transform.rotate(90);
        sprite.pixmap = sprite.pixmap.transformed(transform, Qt::SmoothTransformation);
        ++sprite.rotateState;
    } else {
        snapSprite(sprite);
    }

    m_dragging = false;
    m_moving = -1;
    m_movedDuringDrag = false;
    update();
}

void PuzzleCanvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        emit stopRequested();
    }
}

void PuzzleCanvas::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_sprites.isEmpty()) {
        return;
    }
    const QSize design = designSize();
    const double scaleX = width() > 0 ? static_cast<double>(width()) / design.width() : 1.0;
    const double scaleY = height() > 0 ? static_cast<double>(height()) / design.height() : 1.0;
    m_scale = qMin(1.0, qMin(scaleX, scaleY));
    const int contentW = qRound(design.width() * m_scale);
    const int contentH = qRound(design.height() * m_scale);
    m_offset = QPoint((width() - contentW) / 2, (height() - contentH) / 2);
    update();
}

void PuzzleCanvas::redraw() {
    update();
}
