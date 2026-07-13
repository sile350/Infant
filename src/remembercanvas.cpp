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

bool RememberCanvas::loadRememberLayout(const QString &exerciseId, const QString &stepId, PuzzleLayout *layout) {
    if (loadPuzzleLayout(exerciseId, stepId, layout)) {
        return true;
    }
    PuzzleLayout built;
    const int linexStart = exerciseId == QStringLiteral("1.27") ? 370 : 40;
    int linex = linexStart;
    const int liney = exerciseId == QStringLiteral("3.1.20") ? 350 : 450;
    const int count = exerciseId == QStringLiteral("4.1.7") ? 9 : 4;
    for (int i = 1; i <= count; ++i) {
        QString file = stepId + QString::number(i) + QStringLiteral(".png");
        if (!ExerciseAssets::exerciseFile(exerciseId, file).isEmpty()) {
            PuzzleSpriteDef sprite;
            sprite.file = file;
            sprite.x = linex;
            sprite.y = liney;
            built.sprites.append(sprite);
            linex += 250;
        }
    }
    if (built.sprites.isEmpty()) {
        return false;
    }
    *layout = built;
    return true;
}

void RememberCanvas::startExercise(const QString &exerciseId, const QString &stepId) {
    m_exerciseId = exerciseId;
    m_stepId = stepId;
    m_elapsed = 0;
    m_sprites.clear();
    m_hintSprites.clear();
    m_hintRecords.clear();
    m_showTemplate = true;
    m_phase = 0;

    m_slotPositions = QVector<int>(std::begin(kDefaultSlots), std::end(kDefaultSlots));

    PuzzleLayout layout;
    if (!loadRememberLayout(exerciseId, stepId, &layout)) {
        return;
    }

    if (!layout.templateFile.isEmpty()) {
        m_template = QPixmap(ExerciseAssets::exerciseFile(exerciseId, layout.templateFile));
        m_templateX = layout.templateX;
        m_templateY = layout.templateY;
    } else if (exerciseId == QStringLiteral("3.1.20")) {
        m_template = QPixmap(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("traf1.png")));
        m_templateX = 400;
        m_templateY = 50;
    } else if (exerciseId == QStringLiteral("4.1.7")) {
        m_template = QPixmap(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("traf.png")));
        m_templateX = 10;
        m_templateY = 300;
    }

    for (const PuzzleSpriteDef &def : layout.sprites) {
        Sprite sprite;
        sprite.pixmap = QPixmap(ExerciseAssets::exerciseFile(exerciseId, def.file));
        sprite.x = def.x;
        sprite.y = def.y;
        sprite.name = def.file;
        if (exerciseId == QStringLiteral("4.1.7")) {
            const QString baseName = def.file;
            sprite.spriteId = baseName.left(baseName.size() - 4);
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
    } else {
        shuffleSprites(400);
        m_removeButtonVisible = false;
        m_removeButtonImage.clear();
    }

    m_timer.start();
    updateRemoveButton();
    update();
}

void RememberCanvas::shuffleSprites(int baseY) {
    QVector<int> order;
    for (int i = 0; i < m_sprites.size(); ++i) {
        order.append(i);
    }
    for (int i = order.size() - 1; i > 0; --i) {
        const int j = QRandomGenerator::global()->bounded(i + 1);
        order.swapItemsAt(i, j);
    }
    for (int i = 0; i < m_sprites.size(); ++i) {
        const int slot = i < m_slotPositions.size() ? m_slotPositions.at(i) : i * 200;
        const int offset = m_exerciseId == QStringLiteral("3.1.20") ? 350 : 0;
        m_sprites[i].homeSlotX = slot + offset;
        m_sprites[i].x = slot + offset;
        m_sprites[i].y = baseY;
        m_sprites[i].slotIndex = order.at(i);
    }
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
        return;
    }

    if (m_phase == 2) {
        m_phase = 1;
        m_hintSprites.clear();
        m_removeButtonImage = QStringLiteral("showp.png");
        update();
        updateRemoveButton();
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
}

void RememberCanvas::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    m_dragging = false;
    m_moving = -1;
}

void RememberCanvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Space) {
        emit stopRequested();
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
