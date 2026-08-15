#include "e15canvas.h"

#include "exerciseassets.h"

#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

namespace {

constexpr int kDesignWidth = 1920;
constexpr int kDesignHeight = 1080;
constexpr int kPlaceX2[] = {1216, 1430, 1644, 1216, 1430, 1644};
constexpr int kPlaceX1[] = {200, 414, 628, 200, 414, 628};
constexpr char kAnswers16[] = "4041525102";

} // namespace

E15Canvas::E15Canvas(QWidget *parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    m_elapsedTimer.setInterval(1000);
    connect(&m_elapsedTimer, &QTimer::timeout, this, [this]() {
        if (m_finished) {
            return;
        }
        ++m_elapsed;
        if (m_elapsed >= kMaxSeconds) {
            failIncomplete();
        }
        update();
    });

    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(kMaxSeconds * 1000);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this]() {
        if (!m_finished) {
            failIncomplete();
        }
    });
}

double E15Canvas::scaleFactor() const {
    const double sx = width() > 0 ? static_cast<double>(width()) / kDesignWidth : 1.0;
    const double sy = height() > 0 ? static_cast<double>(height()) / kDesignHeight : 1.0;
    return qMin(1.0, qMin(sx, sy));
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

int E15Canvas::targetXForIndex(int index) const {
    return index >= 6 ? kTargetX1 : m_snapTargetX;
}

int E15Canvas::targetYForIndex(int index) const {
    return (index >= 6 ? kTargetY1 : m_snapTargetY) + kDeltaY;
}

void E15Canvas::startExercise(const QString &exerciseId, bool selectOnlyMode) {
    m_exerciseId = exerciseId;
    m_selectOnly = selectOnlyMode;
    m_completed = false;
    m_finished = false;
    m_readyOk = true;
    m_elapsed = 0;
    m_choose1 = 100;
    m_choose2 = 100;
    m_exerciseNumber = 1;
    m_snapTargetX = 1560;
    m_snapTargetY = 266;
    m_sprites.clear();
    m_nextPixmap = QPixmap();
    m_showNextButton = true;

    m_notReadyPixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), QStringLiteral("notready.png")));

    if (exerciseId == QStringLiteral("1.5")) {
        m_readyPixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), QStringLiteral("ready.png")));
        initExercise15();
    } else {
        m_readyPixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.6"), QStringLiteral("ready.png")));
        m_nextPixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.6"), QStringLiteral("next.png")));
        initExercise16(1);
    }

    m_elapsedTimer.start();
    m_timeoutTimer.start();
    update();
}

void E15Canvas::setSelectOnlyMode(bool selectOnlyMode) {
    if (m_finished || m_selectOnly == selectOnlyMode) {
        return;
    }
    m_selectOnly = selectOnlyMode;
    resetSelectionsForModeChange();
    setReadyVisual(true);
    update();
    emit stateChanged();
}

void E15Canvas::resetSelectionsForModeChange() {
    for (Sprite &sprite : m_sprites) {
        sprite.selected = false;
        sprite.done = false;
        sprite.x = sprite.homeX;
        sprite.y = sprite.homeY;
    }
    m_choose1 = 100;
    m_choose2 = 100;
}

void E15Canvas::abortSession() {
    m_completed = false;
    m_finished = true;
    m_elapsedTimer.stop();
    m_timeoutTimer.stop();
}

void E15Canvas::copyPlayStateFrom(const E15Canvas *peer) {
    if (!peer || peer == this) {
        return;
    }
    // 1.6 dual: смена номера задания → перегрузить картинки, затем позиции.
    if (m_exerciseId == QStringLiteral("1.6")
        && peer->m_exerciseNumber != m_exerciseNumber) {
        m_exerciseNumber = peer->m_exerciseNumber;
        if (m_exerciseNumber >= 1 && m_exerciseNumber <= 10) {
            loadSprites16(m_exerciseNumber);
        }
    }
    if (peer->m_sprites.size() != m_sprites.size()) {
        return;
    }
    for (int i = 0; i < m_sprites.size(); ++i) {
        m_sprites[i].x = peer->m_sprites.at(i).x;
        m_sprites[i].y = peer->m_sprites.at(i).y;
        m_sprites[i].selected = peer->m_sprites.at(i).selected;
        m_sprites[i].done = peer->m_sprites.at(i).done;
    }
    m_choose1 = peer->m_choose1;
    m_choose2 = peer->m_choose2;
    m_readyOk = peer->m_readyOk;
    m_completed = peer->m_completed;
    m_finished = peer->m_finished;
    m_selectOnly = peer->m_selectOnly;
    if (m_finished) {
        m_elapsedTimer.stop();
        m_timeoutTimer.stop();
    }
    setReadyVisual(m_readyOk);
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

    // Индексы 0..5 — правый коврик (21..26), 6..11 — левый (11..16).
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 3; ++col) {
            Sprite sprite;
            const QString file =
                QStringLiteral("2") + QString::number(row * 3 + col + 1) + QStringLiteral(".png");
            sprite.pixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), file));
            sprite.selectPixmap = selectRight;
            sprite.x = kPlaceX2[col + row * 3];
            sprite.y = (row == 0 ? kLine1 : kLine2) + kDeltaY;
            sprite.homeX = sprite.x;
            sprite.homeY = sprite.y;
            m_sprites.append(sprite);
        }
    }
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 3; ++col) {
            Sprite sprite;
            const QString file =
                QStringLiteral("1") + QString::number(row * 3 + col + 1) + QStringLiteral(".png");
            sprite.pixmap = QPixmap(ExerciseAssets::exerciseFile(QStringLiteral("1.5"), file));
            sprite.selectPixmap = selectLeft;
            sprite.x = kPlaceX1[col + row * 3];
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
    m_pole1 = QPixmap();
    updateSnapTarget16();

    for (int i = 0; i < 6; ++i) {
        Sprite sprite;
        const int row = i / 3;
        const int col = i % 3;
        sprite.pixmap = QPixmap(ExerciseAssets::exerciseFile(
            QStringLiteral("1.6"), folder + QString::number(i + 1) + QStringLiteral(".png")));
        sprite.selectPixmap = selectPixmap;
        sprite.x = kPlaceX2[col + row * 3];
        sprite.y = (row == 0 ? kLine1 : kLine2) + kDeltaY;
        sprite.homeX = sprite.x;
        sprite.homeY = sprite.y;
        m_sprites.append(sprite);
    }
}

void E15Canvas::updateSnapTarget16() {
    // Как f16p1_Click в e15.cs — цель «на месте» зависит от номера задания.
    switch (m_exerciseNumber) {
    case 2:
        m_snapTargetX = 1568;
        m_snapTargetY = 401;
        break;
    case 3:
        m_snapTargetX = 1516;
        m_snapTargetY = 400;
        break;
    case 4:
        m_snapTargetX = 1500;
        m_snapTargetY = 390;
        break;
    case 5:
        m_snapTargetX = 1556;
        m_snapTargetY = 390;
        break;
    case 6:
        m_snapTargetX = 1544;
        m_snapTargetY = 381;
        break;
    case 7:
        m_snapTargetX = 1559;
        m_snapTargetY = 407;
        break;
    case 8:
        m_snapTargetX = 1532;
        m_snapTargetY = 395;
        break;
    case 9:
        m_snapTargetX = 1562;
        m_snapTargetY = 302;
        break;
    case 10:
        m_snapTargetX = 1558;
        m_snapTargetY = 262;
        break;
    case 1:
    default:
        // Оригинал exInit("1.6"): targetx/targety = 1512/399 (не 1560/266 от 1.5).
        m_snapTargetX = 1512;
        m_snapTargetY = 399;
        break;
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
            if (!m_selectOnly) {
                snapSpriteHome(i);
            }
        }
    }
    if (sel >= 6 && sel <= 11) {
        for (int i = 6; i <= 11 && i < m_sprites.size(); ++i) {
            m_sprites[i].selected = false;
            if (!m_selectOnly) {
                snapSpriteHome(i);
            }
        }
    }
}

void E15Canvas::snapSpriteHome(int index) {
    if (index < 0 || index >= m_sprites.size()) {
        return;
    }
    Sprite &sprite = m_sprites[index];
    sprite.x = sprite.homeX;
    sprite.y = sprite.homeY;
    sprite.done = false;
}

void E15Canvas::snapSpriteToTarget(int index) {
    if (index < 0 || index >= m_sprites.size()) {
        return;
    }
    Sprite &sprite = m_sprites[index];
    sprite.x = targetXForIndex(index);
    sprite.y = targetYForIndex(index);
    sprite.done = true;
    sprite.selected = false;
}

void E15Canvas::spriteChosen(int index) {
    // Сначала вернуть предыдущий выбор той же доски домой (мгновенно).
    if (index <= 5 && m_choose2 != 100 && m_choose2 != index) {
        snapSpriteHome(m_choose2);
    }
    if (index >= 6 && m_choose1 != 100 && m_choose1 != index) {
        snapSpriteHome(m_choose1);
    }
    // Мгновенно на «своё место» — без анимации по наклонной.
    snapSpriteToTarget(index);
    if (index <= 5) {
        m_choose2 = index;
    } else {
        m_choose1 = index;
    }
}

void E15Canvas::advanceExercise16() {
    for (Sprite &sprite : m_sprites) {
        sprite.selected = false;
        sprite.done = false;
    }
    m_choose1 = 100;
    m_choose2 = 100;
    setReadyVisual(true);

    // Как оригинал: unumber++, затем если > 10 — закрыть (выполнено).
    ++m_exerciseNumber;
    if (m_exerciseNumber > 10) {
        m_completed = true;
        m_finished = true;
        m_elapsedTimer.stop();
        m_timeoutTimer.stop();
        emit exerciseCompleted();
        return;
    }
    loadSprites16(m_exerciseNumber);
    update();
}

void E15Canvas::skipToNextTask16() {
    if (m_finished || m_exerciseId != QStringLiteral("1.6")) {
        return;
    }
    advanceExercise16();
}

void E15Canvas::setReadyVisual(bool ready) {
    m_readyOk = ready;
}

void E15Canvas::failIncomplete() {
    if (m_finished) {
        return;
    }
    m_completed = false;
    m_finished = true;
    m_elapsedTimer.stop();
    m_timeoutTimer.stop();
    emit stopRequested();
}

void E15Canvas::onReadyClicked() {
    if (m_finished) {
        return;
    }
    if (m_exerciseId == QStringLiteral("1.5")) {
        // Верно: правый фрагмент index 2 (23.png) и левый index 10 (15.png).
        if (m_choose2 == 2 && m_choose1 == 10) {
            m_completed = true;
            m_finished = true;
            m_elapsedTimer.stop();
            m_timeoutTimer.stop();
            emit exerciseCompleted();
        } else {
            setReadyVisual(false);
            update();
        }
        return;
    }

    // 1.6
    if (m_choose2 != 100) {
        const int expected = QString(kAnswers16).at(m_exerciseNumber - 1).digitValue();
        if (m_choose2 == expected) {
            advanceExercise16();
        } else {
            setReadyVisual(false);
            update();
        }
    } else {
        setReadyVisual(false);
        update();
    }
}

bool E15Canvas::hitTest(int index, int x, int y) const {
    if (index < 0 || index >= m_sprites.size()) {
        return false;
    }
    const Sprite &sprite = m_sprites.at(index);
    if (sprite.pixmap.isNull()) {
        return false;
    }
    // В режиме перемещения «доехавшие» тоже кликабельны для смены выбора.
    return x >= sprite.x && y >= sprite.y && x < sprite.x + sprite.pixmap.width()
        && y < sprite.y + sprite.pixmap.height();
}

bool E15Canvas::hitReadyButton(int x, int y) const {
    return x >= kReadyX && x < kReadyX + kReadyW && y >= kReadyY && y < kReadyY + kReadyH;
}

bool E15Canvas::hitNextButton(int x, int y) const {
    if (!m_showNextButton || m_exerciseId != QStringLiteral("1.6") || m_nextPixmap.isNull()) {
        return false;
    }
    return x >= kNextX && x < kNextX + m_nextPixmap.width()
        && y >= kNextY && y < kNextY + m_nextPixmap.height();
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
        painter.drawPixmap(200, 59 + kDeltaY, m_pole2.isNull() ? m_pole1.width() : m_pole2.width(),
                           m_pole2.isNull() ? m_pole1.height() : m_pole2.height(), m_pole1);
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

    const QPixmap &readyPix = m_readyOk ? m_readyPixmap : m_notReadyPixmap;
    if (!readyPix.isNull()) {
        painter.drawPixmap(kReadyX, kReadyY, kReadyW, kReadyH, readyPix);
    }
    if (m_exerciseId == QStringLiteral("1.6")) {
        if (m_showNextButton && !m_nextPixmap.isNull()) {
            painter.drawPixmap(kNextX, kNextY, m_nextPixmap);
        }
        QFont labelFont(QStringLiteral("Microsoft Sans Serif"), 14);
        painter.setFont(labelFont);
        painter.setPen(Qt::black);
        painter.drawText(kLabelX, kLabelY + 20,
                         QStringLiteral("Упражнение %1 из 10").arg(m_exerciseNumber));
    }
}

void E15Canvas::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || m_finished) {
        return;
    }
    const QPoint design = mapToDesign(event->pos());

    if (hitNextButton(design.x(), design.y())) {
        skipToNextTask16();
        emit stateChanged();
        return;
    }

    if (hitReadyButton(design.x(), design.y())) {
        onReadyClicked();
        emit stateChanged();
        return;
    }

    for (int i = 0; i < m_sprites.size(); ++i) {
        if (!hitTest(i, design.x(), design.y())) {
            continue;
        }
        // Любое действие с фрагментами возвращает «Готово» (как bend ← ready.png).
        setReadyVisual(true);
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
        emit stateChanged();
        return;
    }
}

void E15Canvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        failIncomplete();
    }
}

void E15Canvas::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    update();
}
