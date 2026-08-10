#include "remembercanvas.h"

#include "exerciseassets.h"
#include "puzzlelayout.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>

namespace {

constexpr int kDesignWidth = 1920;
constexpr int kDesignHeight = 1080;
constexpr int kDefaultSlots[] = {0, 200, 400, 600, 800, 1000, 1200, 1400, 1600};

} // namespace

RememberCanvas::RememberCanvas(QWidget *parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });
}

double RememberCanvas::scaleFactor() const {
    const double sx = width() > 0 ? static_cast<double>(width()) / kDesignWidth : 1.0;
    const double sy = height() > 0 ? static_cast<double>(height()) / kDesignHeight : 1.0;
    return qMin(1.0, qMin(sx, sy));
}

QPoint RememberCanvas::mapFromDesign(int x, int y) const {
    const double scale = scaleFactor();
    const int offsetX = (width() - static_cast<int>(kDesignWidth * scale)) / 2;
    const int offsetY = (height() - static_cast<int>(kDesignHeight * scale)) / 2;
    return QPoint(offsetX + static_cast<int>(x * scale), offsetY + static_cast<int>(y * scale));
}

bool RememberCanvas::loadRememberLayout(
    const QString &exerciseId,
    const QString &stepId,
    PuzzleLayout *layout,
    const QString &remPictureMask) {
    // Не брать autoGrid/puzzle JSON для remember-методик — ломает раскладку.
    if (exerciseId != QStringLiteral("1.27")
        && exerciseId != QStringLiteral("3.1.20")
        && exerciseId != QStringLiteral("3.1.21")
        && exerciseId != QStringLiteral("4.1.7")
        && loadPuzzleLayout(exerciseId, stepId, layout)) {
        return true;
    }
    PuzzleLayout built;
    const QString step = stepId.trimmed().isEmpty() ? QStringLiteral("1") : stepId.trimmed();

    if (exerciseId == QStringLiteral("3.1.21")) {
        // remember.cs: две карты столбцом, без shuffle.
        built.showTemplate = false;
        const QString a = step + QStringLiteral("1.png");
        const QString b = step + QStringLiteral("2.png");
        if (!ExerciseAssets::exerciseFile(exerciseId, a).isEmpty()) {
            PuzzleSpriteDef s;
            s.file = a;
            s.x = 100;
            s.y = 422;
            built.sprites.append(s);
        }
        if (!ExerciseAssets::exerciseFile(exerciseId, b).isEmpty()) {
            PuzzleSpriteDef s;
            s.file = b;
            s.x = 100;
            s.y = 0;
            built.sprites.append(s);
        }
        if (built.sprites.isEmpty()) {
            return false;
        }
        *layout = built;
        return true;
    }

    if (exerciseId == QStringLiteral("3.1.20")) {
        built.showTemplate = true;
        if (step == QStringLiteral("3")) {
            built.templateFile = QStringLiteral("traf2.png");
            built.templateX = 50;
            built.templateY = 100;
        } else {
            built.templateFile = QStringLiteral("traf1.png");
            built.templateX = 400;
            built.templateY = 50;
        }
        int linex = 40;
        const int spacing = step == QStringLiteral("3") ? 450 : 250;
        const int y = 350;
        for (int i = 1; i <= 4; ++i) {
            const QString file = step + QString::number(i) + QStringLiteral(".png");
            if (ExerciseAssets::exerciseFile(exerciseId, file).isEmpty()) {
                continue;
            }
            PuzzleSpriteDef s;
            s.file = file;
            s.x = linex;
            s.y = y;
            built.sprites.append(s);
            linex += spacing;
        }
        if (built.sprites.isEmpty()) {
            return false;
        }
        *layout = built;
        return true;
    }

    if (exerciseId == QStringLiteral("4.1.7")) {
        // 1.png…9.png по маске rem.state; template traf.png @ (10,300).
        built.showTemplate = true;
        built.templateFile = QStringLiteral("traf.png");
        built.templateX = 10;
        built.templateY = 300;
        const QStringList maskParts = remPictureMask.split(QLatin1Char(','));
        const bool useMask = !remPictureMask.trimmed().isEmpty();
        int linex = 10;
        for (int i = 1; i <= 9; ++i) {
            if (useMask
                && (i - 1 >= maskParts.size()
                    || maskParts.at(i - 1).trimmed() != QStringLiteral("1"))) {
                continue;
            }
            const QString file = QString::number(i) + QStringLiteral(".png");
            if (ExerciseAssets::exerciseFile(exerciseId, file).isEmpty()) {
                continue;
            }
            PuzzleSpriteDef s;
            s.file = file;
            s.name = QString::number(i);
            s.x = linex;
            s.y = 250;
            built.sprites.append(s);
            linex += 200;
        }
        if (built.sprites.isEmpty()) {
            return false;
        }
        *layout = built;
        return true;
    }

    const int linexStart = exerciseId == QStringLiteral("1.27") ? 370 : 40;
    int linex = linexStart;
    int liney = 450;
    int stepSpacing = 250;
    if (exerciseId == QStringLiteral("1.27")) {
        if (step == QStringLiteral("1")) {
            liney = 450;
            stepSpacing = 250;
        } else {
            liney = 250;
            stepSpacing = 300;
        }
    }
    // 1.27: по 5 карточек на серию (11–15 / 21–25 / 31–35), как в remember.cs.
    const int count = exerciseId == QStringLiteral("1.27") ? 5 : 4;
    for (int i = 1; i <= count; ++i) {
        QString file = step + QString::number(i) + QStringLiteral(".png");
        if (!ExerciseAssets::exerciseFile(exerciseId, file).isEmpty()) {
            PuzzleSpriteDef sprite;
            sprite.file = file;
            sprite.x = linex;
            sprite.y = liney;
            built.sprites.append(sprite);
            linex += stepSpacing;
        }
    }
    if (built.sprites.isEmpty()) {
        return false;
    }
    built.showTemplate = false;
    built.templateFile.clear();
    *layout = built;
    return true;
}

void RememberCanvas::startExercise(const QString &exerciseId, const QString &stepId) {
    startExercise(exerciseId, stepId, QString());
}

void RememberCanvas::startExercise(
    const QString &exerciseId,
    const QString &stepId,
    const QString &remPictureMask) {
    m_exerciseId = exerciseId;
    m_stepId = stepId;
    m_elapsed = 0;
    m_sprites.clear();
    m_hintSprites.clear();
    m_hintRecords.clear();
    m_template = QPixmap();
    m_showTemplate = true;
    m_phase = 0;
    m_moving = -1;
    m_dragging = false;
    m_removeButtonVisible = false;
    m_removeButtonImage.clear();

    m_slotPositions.clear();
    for (int slot : kDefaultSlots) {
        m_slotPositions.append(slot);
    }

    PuzzleLayout layout;
    if (!loadRememberLayout(exerciseId, stepId, &layout, remPictureMask)) {
        update();
        return;
    }

    m_showTemplate = layout.showTemplate;
    if (layout.showTemplate && !layout.templateFile.isEmpty()) {
        const QString path = ExerciseAssets::exerciseFile(exerciseId, layout.templateFile);
        if (!path.isEmpty()) {
            m_template = QPixmap(path);
        }
        m_templateX = layout.templateX;
        m_templateY = layout.templateY;
    } else if (exerciseId == QStringLiteral("3.1.20")) {
        m_template = QPixmap(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("traf1.png")));
        m_templateX = 400;
        m_templateY = 50;
        m_showTemplate = true;
    } else if (exerciseId == QStringLiteral("4.1.7")) {
        m_template = QPixmap(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("traf.png")));
        m_templateX = 10;
        m_templateY = 300;
        m_showTemplate = true;
    } else {
        // 1.27 / 3.1.21: без трафарета во время выполнения.
        m_template = QPixmap();
        m_showTemplate = false;
    }

    for (const PuzzleSpriteDef &def : layout.sprites) {
        Sprite sprite;
        const QString path = ExerciseAssets::exerciseFile(exerciseId, def.file);
        if (!path.isEmpty()) {
            sprite.pixmap = QPixmap(path);
        }
        sprite.x = def.x;
        sprite.y = def.y;
        sprite.name = def.file;
        if (exerciseId == QStringLiteral("4.1.7")) {
            if (!def.name.isEmpty()) {
                sprite.spriteId = def.name;
            } else {
                const QString baseName = def.file;
                sprite.spriteId = baseName.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)
                    ? baseName.left(baseName.size() - 4)
                    : baseName;
            }
        }
        m_sprites.append(sprite);
    }

    if (exerciseId == QStringLiteral("4.1.7")) {
        shuffleSprites(300);
        for (int i = 0; i < m_sprites.size(); ++i) {
            m_hintRecords.append(
                m_sprites[i].spriteId + QLatin1Char(';') + QString::number(m_sprites[i].homeSlotX));
        }
        m_removeButtonVisible = true;
        m_removeButtonImage = QStringLiteral("showp.png");
    } else if (exerciseId == QStringLiteral("1.27")) {
        // Как remember.cs: после перемешивания y = 650.
        shuffleSprites(650);
    } else if (exerciseId == QStringLiteral("3.1.21")) {
        // Две карты остаются на исходных координатах — без shuffle.
    } else if (exerciseId == QStringLiteral("3.1.20")) {
        // remember.cs: y = 400+dy; x = posx[mst]+dx (шаг1: dx=350,dy=50; шаг2: 350/80; шаг3: 30/70).
        const QString step = stepId.trimmed().isEmpty() ? QStringLiteral("1") : stepId.trimmed();
        int dx = 350;
        int dy = 50;
        if (step == QStringLiteral("2")) {
            dy = 80;
        } else if (step == QStringLiteral("3")) {
            dx = 30;
            dy = 70;
        }
        shuffleSprites(400 + dy, dx);
    } else {
        shuffleSprites(400);
    }

    m_timer.start();
    updateRemoveButton();
    setFocus(Qt::OtherFocusReason);
    update();
}

void RememberCanvas::shuffleSprites(int baseY, int offsetX) {
    // Перестановка слотов, как mst[] в remember.cs: sprite[i].x = posx[mst[i]] (+ dx).
    QVector<int> order;
    for (int i = 0; i < m_sprites.size(); ++i) {
        order.append(i);
    }
    for (int i = order.size() - 1; i > 0; --i) {
        const int j = QRandomGenerator::global()->bounded(i + 1);
        order.swapItemsAt(i, j);
    }
    for (int i = 0; i < m_sprites.size(); ++i) {
        const int slotIdx = order.at(i);
        const int slot = slotIdx < m_slotPositions.size() ? m_slotPositions.at(slotIdx) : slotIdx * 200;
        m_sprites[i].slotIndex = slotIdx;
        m_sprites[i].homeSlotX = slot + offsetX;
        m_sprites[i].x = slot + offsetX;
        m_sprites[i].y = baseY;
    }
}

QVector<RememberCanvas::SpritePose> RememberCanvas::spritePoses() const {
    QVector<SpritePose> poses;
    poses.reserve(m_sprites.size());
    for (const Sprite &sprite : m_sprites) {
        SpritePose pose;
        pose.x = sprite.x;
        pose.y = sprite.y;
        poses.append(pose);
    }
    return poses;
}

void RememberCanvas::applySpritePoses(const QVector<SpritePose> &poses) {
    const int n = qMin(poses.size(), m_sprites.size());
    for (int i = 0; i < n; ++i) {
        m_sprites[i].x = poses.at(i).x;
        m_sprites[i].y = poses.at(i).y;
    }
    update();
}

void RememberCanvas::updateRemoveButton() {
    emit removeButtonChanged();
}

void RememberCanvas::advanceRemovePhase() {
    if (m_exerciseId != QStringLiteral("4.1.7")) {
        return;
    }

    if (m_phase == 0) {
        m_phase = 1;
        shuffleSprites(570);
        m_hintSprites.clear();
        m_removeButtonImage = QStringLiteral("showp.png");
        update();
        updateRemoveButton();
        emit spritesChanged();
        return;
    }

    if (m_phase == 2) {
        m_phase = 1;
        m_hintSprites.clear();
        m_removeButtonImage = QStringLiteral("showp.png");
        update();
        updateRemoveButton();
        emit spritesChanged();
        return;
    }

    if (m_phase == 1) {
        m_phase = 2;
        m_hintSprites.clear();
        for (const QString &record : m_hintRecords) {
            const QStringList parts = record.split(QLatin1Char(';'));
            if (parts.size() < 2) {
                continue;
            }
            HintSprite hint;
            const QString path = ExerciseAssets::exerciseFile(m_exerciseId, parts.at(0) + QStringLiteral(".png"));
            if (path.isEmpty()) {
                continue;
            }
            hint.pixmap = QPixmap(path);
            hint.x = parts.at(1).toInt();
            hint.y = 35;
            m_hintSprites.append(hint);
        }
        m_removeButtonImage = QStringLiteral("removep.png");
        update();
        updateRemoveButton();
        emit spritesChanged();
    }
}

QString RememberCanvas::positionsSnapshot() const {
    QStringList parts;
    for (const Sprite &sprite : m_sprites) {
        parts.append(QStringLiteral("%1,%2").arg(sprite.x).arg(sprite.y));
    }
    return parts.join(QLatin1Char(';'));
}

bool RememberCanvas::hitTest(const Sprite &sprite, int x, int y) const {
    const QPoint topLeft = mapFromDesign(sprite.x, sprite.y);
    const double scale = scaleFactor();
    const int w = qRound(sprite.pixmap.width() * scale);
    const int h = qRound(sprite.pixmap.height() * scale);
    return x >= topLeft.x() && y >= topLeft.y() && x < topLeft.x() + w && y < topLeft.y() + h;
}

void RememberCanvas::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(240, 240, 240));

    const double scale = scaleFactor();
    const int offsetX = (width() - static_cast<int>(kDesignWidth * scale)) / 2;
    const int offsetY = (height() - static_cast<int>(kDesignHeight * scale)) / 2;
    painter.translate(offsetX, offsetY);
    painter.scale(scale, scale);

    if (m_showTemplate && !m_template.isNull()) {
        painter.drawPixmap(m_templateX, m_templateY, m_template);
    }
    for (const HintSprite &hint : m_hintSprites) {
        if (!hint.pixmap.isNull()) {
            painter.drawPixmap(hint.x, hint.y, hint.pixmap);
        }
    }
    for (const Sprite &sprite : m_sprites) {
        if (!sprite.pixmap.isNull()) {
            painter.drawPixmap(sprite.x, sprite.y, sprite.pixmap);
        }
    }
}

void RememberCanvas::mousePressEvent(QMouseEvent *event) {
    for (int i = m_sprites.size() - 1; i >= 0; --i) {
        if (hitTest(m_sprites[i], event->x(), event->y())) {
            m_moving = i;
            const QPoint topLeft = mapFromDesign(m_sprites[i].x, m_sprites[i].y);
            m_dragOffset = event->pos() - topLeft;
            m_dragging = true;
            return;
        }
    }
}

void RememberCanvas::mouseMoveEvent(QMouseEvent *event) {
    if (!m_dragging || m_moving < 0) {
        return;
    }
    const double scale = scaleFactor();
    const int offsetX = (width() - static_cast<int>(kDesignWidth * scale)) / 2;
    const int offsetY = (height() - static_cast<int>(kDesignHeight * scale)) / 2;
    m_sprites[m_moving].x = static_cast<int>((event->x() - offsetX - m_dragOffset.x()) / scale);
    m_sprites[m_moving].y = static_cast<int>((event->y() - offsetY - m_dragOffset.y()) / scale);
    update();
    emit spritesChanged();
}

void RememberCanvas::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    m_dragging = false;
    m_moving = -1;
    emit spritesChanged();
}

void RememberCanvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Space) {
        emit stopRequested();
        return;
    }
    if (event->key() == Qt::Key_H) {
        m_showTemplate = !m_showTemplate;
        update();
    }
}

void RememberCanvas::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    update();
}
