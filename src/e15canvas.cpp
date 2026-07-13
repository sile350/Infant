#include "e15canvas.h"

#include "exerciseassets.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

namespace {

constexpr int kDesignWidth = 1920;
constexpr int kDesignHeight = 1080;
constexpr int kPlaceX2[] = {1216, 1430, 1644, 1216, 1430, 1644};
constexpr int kPlaceX1[] = {200, 414, 628, 200, 414, 628};
constexpr double kSlopes[] = {-35.3, -35.7, -45.5, -35.5, -35.9, -36.1};
constexpr char kAnswers16[] = "4041525102";

double slopeRadians(int pairIndex) {
    const int idx = qBound(0, pairIndex, 5);
    return qDegreesToRadians(kSlopes[idx]);
}

} // namespace

E15Canvas::E15Canvas(QWidget *parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    m_elapsedTimer.setInterval(1000);
    connect(&m_elapsedTimer, &QTimer::timeout, this, [this]() { ++m_elapsed; });

    m_moveTimer.setInterval(16);
    connect(&m_moveTimer, &QTimer::timeout, this, [this]() {
        if (m_selected < 0 || m_selected >= m_sprites.size()) {
            m_moveTimer.stop();
            return;
        }
        Sprite &sprite = m_sprites[m_selected];
        const bool slow = (m_selected == 5 || m_selected == 2 || m_selected == 8 || m_selected == 11);
        const int dx = slow ? 2 : 23;
        sprite.x += dx;
        sprite.y = static_cast<int>(sprite.x * qTan(m_k) + m_b);

        const int targetY = (m_selected >= 6) ? kTargetY1 + kDeltaY : kTargetY + kDeltaY;
        const int targetX = (m_selected >= 6) ? kTargetX1 : kTargetX;
        if (sprite.y < targetY) {
            sprite.x = targetX;
            sprite.y = targetY;
            sprite.done = true;
            m_moveTimer.stop();
        }
        update();
    });

    m_backTimer.setInterval(16);
    connect(&m_backTimer, &QTimer::timeout, this, [this]() {
        if (m_selBack < 0 || m_selBack >= m_sprites.size()) {
            m_backTimer.stop();
            return;
        }
        Sprite &sprite = m_sprites[m_selBack];
        const bool slow = (m_selBack == 2 || m_selBack == 5 || m_selBack == 8 || m_selBack == 11);
        sprite.x -= slow ? 2 : 15;
        sprite.y = static_cast<int>(sprite.x * qTan(m_kBack) + m_bBack);

        bool arrived = false;
        if (m_selBack >= 0 && m_selBack <= 2 && sprite.y > kLine1 + kDeltaY) {
            sprite.x = kPlaceX2[m_selBack];
            sprite.y = kLine1 + kDeltaY;
            arrived = true;
        } else if (m_selBack >= 3 && m_selBack <= 5 && sprite.y > kLine2 + kDeltaY) {
            sprite.x = kPlaceX2[m_selBack - 3];
            sprite.y = kLine2 + kDeltaY;
            arrived = true;
        } else if (m_selBack >= 6 && m_selBack <= 8 && sprite.y > kLine1 + kDeltaY) {
            sprite.x = kPlaceX1[m_selBack - 6];
            sprite.y = kLine1 + kDeltaY;
            arrived = true;
        } else if (m_selBack >= 9 && m_selBack <= 11 && sprite.y > kLine2 + kDeltaY) {
            sprite.x = kPlaceX1[m_selBack - 9];
            sprite.y = kLine2 + kDeltaY;
            arrived = true;
        }

        if (arrived) {
            sprite.done = false;
            m_backTimer.stop();
            m_choose1 = 100;
            m_choose2 = 100;
        }
        update();
    });

    m_redrawTimer.setInterval(50);
    connect(&m_redrawTimer, &QTimer::timeout, this, [this]() { update(); });
}

double E15Canvas::scaleFactor() const {
    const double sx = width() > 0 ? static_cast<double>(width()) / kDesignWidth : 1.0;
    const double sy = height() > 0 ? static_cast<double>(height()) / kDesignHeight : 1.0;
    return qMin(sx, sy);
}

QPoint E15Canvas::mapFromDesign(int x, int y) const {
    const double scale = scaleFactor();
    const int offsetX = (width() - static_cast<int>(kDesignWidth * scale)) / 2;
    const int offsetY = (height() - static_cast<int>(kDesignHeight * scale)) / 2;
    return QPoint(offsetX + static_cast<int>(x * scale), offsetY + static_cast<int>(y * scale));
}

QPoint E15Canvas::mapToDesign(const QPoint &pos) const {
    const double scale = scaleFactor();
    const int offsetX = (width() - static_cast<int>(kDesignWidth * scale)) / 2;
    const int offsetY = (height() - static_cast<int>(kDesignHeight * scale)) / 2;
    return QPoint(
        static_cast<int>((pos.x() - offsetX) / scale),
        static_cast<int>((pos.y() - offsetY) / scale));
}

double E15Canvas::slopeForIndex(int index) const {
    return slopeRadians(index % 6);
}

void E15Canvas::startExercise(const QString &exerciseId, bool selectOnlyMode) {
    m_exerciseId = exerciseId;
    m_selectOnly = selectOnlyMode;
    m_completed = false;
    m_elapsed = 0;
    m_choose1 = 100;
    m_choose2 = 100;
    m_selected = -1;
    m_exerciseNumber = 1;
    m_sprites.clear();

    m_readyPixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), QStringLiteral("ready.png")));
    m_notReadyPixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), QStringLiteral("notready.png")));

    if (exerciseId == QStringLiteral("1.5")) {
        initExercise15();
    } else {
        initExercise16(1);
    }

    m_elapsedTimer.start();
    m_redrawTimer.start();
    update();
}

QString E15Canvas::doneState() const {
    return m_completed ? QStringLiteral("true") : QStringLiteral("false");
}

void E15Canvas::initExercise15() {
    m_pole1 = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), QStringLiteral("carpet1.png")));
    m_pole2 = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), QStringLiteral("carpet2.png")));

    const QPixmap selectRight(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), QStringLiteral("select2.png")));
    const QPixmap selectLeft(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), QStringLiteral("select.png")));

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 3; ++col) {
            Sprite sprite;
            const QString file = QString::number(2 + row) + QString::number(col + 1) + QStringLiteral(".png");
            sprite.pixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), file));
            sprite.selectPixmap = selectRight;
            sprite.x = kPlaceX2[col];
            sprite.y = (row == 0 ? kLine1 : kLine2) + kDeltaY;
            sprite.homeX = sprite.x;
            sprite.homeY = sprite.y;
            m_sprites.append(sprite);
        }
    }
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 3; ++col) {
            Sprite sprite;
            const QString file = QStringLiteral("1") + QString::number(row * 3 + col + 1) + QStringLiteral(".png");
            sprite.pixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), file));
            sprite.selectPixmap = selectLeft;
            sprite.x = kPlaceX1[col];
            sprite.y = (row == 0 ? kLine1 : kLine2) + kDeltaY;
            sprite.homeX = sprite.x;
            sprite.homeY = sprite.y;
            m_sprites.append(sprite);
        }
    }
}

void E15Canvas::loadSprites16(int number) {
    m_sprites.clear();
    const QString folder = QString::number(number) + QLatin1Char('/');
    const QPixmap selectPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.6"), QStringLiteral("select.png")));
    m_pole2 = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.6"), folder + QStringLiteral("pole.png")));

    for (int i = 0; i < 6; ++i) {
        Sprite sprite;
        const int row = i / 3;
        const int col = i % 3;
        sprite.pixmap = QPixmap(ExerciseAssets::exerciseFile(
            QStringLiteral("1.6"), folder + QString::number(i + 1) + QStringLiteral(".png")));
        sprite.selectPixmap = selectPixmap;
        sprite.x = kPlaceX2[col];
        sprite.y = (row == 0 ? kLine1 : kLine2) + kDeltaY;
        sprite.homeX = sprite.x;
        sprite.homeY = sprite.y;
        m_sprites.append(sprite);
    }
}

void E15Canvas::initExercise16(int number) {
    m_exerciseNumber = number;
    loadSprites16(number);
}

void E15Canvas::clearOtherSelected(int sel) {
    if (sel >= 0 && sel <= 5) {
        for (int i = 0; i <= 5 && i < m_sprites.size(); ++i) {
            m_sprites[i].selected = false;
        }
    }
    if (sel >= 6 && sel <= 11) {
        for (int i = 6; i <= 11 && i < m_sprites.size(); ++i) {
            m_sprites[i].selected = false;
        }
    }
}

void E15Canvas::spriteChosen(int index) {
    backChosen();
    m_k = slopeForIndex(index);
    m_selected = index;
    m_b = m_sprites[index].y - static_cast<int>(m_sprites[index].x * qTan(m_k));
    if (index <= 5) {
        m_choose2 = index;
    } else {
        m_choose1 = index;
    }
    m_moveTimer.start();
}

void E15Canvas::backChosen() {
    const int index = m_choose1 != 100 ? m_choose1 : m_choose2;
    if (index == 100) {
        return;
    }
    m_kBack = slopeForIndex(index);
    if (index >= 0 && index <= 5 && m_choose2 != 100) {
        m_selBack = m_choose2;
    } else if (index >= 6 && index <= 11 && m_choose1 != 100) {
        m_selBack = m_choose1;
    } else {
        m_choose1 = 100;
        m_choose2 = 100;
        return;
    }
    m_bBack = m_sprites[m_selBack].y - static_cast<int>(m_sprites[m_selBack].x * qTan(m_kBack));
    m_backTimer.start();
}

void E15Canvas::advanceExercise16() {
    for (Sprite &sprite : m_sprites) {
        sprite.selected = false;
        sprite.done = false;
    }
    m_choose1 = 100;
    m_choose2 = 100;
    if (m_exerciseNumber >= 10) {
        m_completed = true;
        emit exerciseCompleted();
        return;
    }
    ++m_exerciseNumber;
    loadSprites16(m_exerciseNumber);
    update();
}

bool E15Canvas::hitTest(int index, int x, int y) const {
    if (index < 0 || index >= m_sprites.size()) {
        return false;
    }
    const Sprite &sprite = m_sprites.at(index);
    if (sprite.done || sprite.pixmap.isNull()) {
        return false;
    }
    return x >= sprite.x && y >= sprite.y && x < sprite.x + sprite.pixmap.width()
        && y < sprite.y + sprite.pixmap.height();
}

void E15Canvas::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    const double scale = scaleFactor();
    const int offsetX = (width() - static_cast<int>(kDesignWidth * scale)) / 2;
    const int offsetY = (height() - static_cast<int>(kDesignHeight * scale)) / 2;
    painter.translate(offsetX, offsetY);
    painter.scale(scale, scale);

    if (!m_pole2.isNull()) {
        painter.drawPixmap(1214, 59 + kDeltaY, m_pole2);
    }
    if (m_exerciseId == QStringLiteral("1.5") && !m_pole1.isNull()) {
        painter.drawPixmap(200, 59 + kDeltaY, m_pole2.width(), m_pole2.height(), m_pole1);
    }

    auto drawSprite = [&](const Sprite &sprite) {
        if (sprite.selected && !sprite.selectPixmap.isNull()) {
            painter.drawPixmap(sprite.x - 13, sprite.y - 16, sprite.selectPixmap);
        }
        if (!sprite.pixmap.isNull()) {
            painter.drawPixmap(sprite.x, sprite.y, sprite.pixmap);
        }
    };

    for (const Sprite &sprite : m_sprites) {
        if (sprite.done) {
            drawSprite(sprite);
        }
    }
    for (const Sprite &sprite : m_sprites) {
        if (!sprite.done) {
            drawSprite(sprite);
        }
    }

    if (!m_readyPixmap.isNull()) {
        painter.drawPixmap(1700, 900, m_readyPixmap);
    }
    if (m_exerciseId == QStringLiteral("1.6")) {
        painter.drawText(200, 80, QStringLiteral("Упражнение %1 из 10").arg(m_exerciseNumber));
    }
}

void E15Canvas::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const QPoint design = mapToDesign(event->pos());

    if (design.x() >= 1700 && design.x() <= 1900 && design.y() >= 900 && design.y() <= 980) {
        if (m_exerciseId == QStringLiteral("1.5")) {
            if (m_choose1 == 10 && m_choose2 == 2) {
                m_completed = true;
                emit exerciseCompleted();
            }
        } else if (m_choose2 != 100) {
            const int expected = QString(kAnswers16).at(m_exerciseNumber - 1).digitValue();
            if (m_choose2 == expected) {
                advanceExercise16();
            }
        }
        update();
        return;
    }

    for (int i = 0; i < m_sprites.size(); ++i) {
        if (!hitTest(i, design.x(), design.y())) {
            continue;
        }
        if (m_selectOnly) {
            clearOtherSelected(i);
            m_sprites[i].selected = true;
            if (i <= 5) {
                m_choose2 = i;
            } else {
                m_choose1 = i;
            }
        } else {
            spriteChosen(i);
        }
        update();
        return;
    }
}

void E15Canvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        emit stopRequested();
    }
}

void E15Canvas::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    update();
}

void E15Canvas::redraw() {
    update();
}
