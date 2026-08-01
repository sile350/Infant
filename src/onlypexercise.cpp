#include "onlypexercise.h"

#include "exerciseassets.h"
#include "exerciseconfig.h"

#include <QFont>
#include <QFrame>
#include <QMouseEvent>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTextEdit>
#include <QTimer>
#include <QtMath>
#include <functional>

namespace {

constexpr int kPictureLeft = 700;
constexpr int kPictureTop = 240;
constexpr int kPictureTopOffset = 50;
constexpr int kPictureShiftLeft = 150;
constexpr int kPatientPictureShiftRight = 0;
constexpr int kPatientSecondScreenShiftLeft = 80;
constexpr int kPatientPictureShiftDown = 40;
constexpr int kSpecialistPictureShiftLeft = 15;
constexpr int kStopLeft = 80;
constexpr int kStopTop = 72;
constexpr int kRightLeft = 1280;
// onlyp.cs f411 @ (12,151), size 800×800.
constexpr int kFairyTextLeft = 12;
constexpr int kFairyTextTop = 151;
constexpr int kFairyTextW = 800;
constexpr int kFairyTextH = 800;

QString fairyTaleText411() {
    return QStringLiteral(
        "    Жили-были дед да баба. И была у них Курочка Ряба. Снесла курочка яичко, да не простое - золотое. "
        "Дед бил - не разбил. Баба била - не разбила. А мышка бежала, хвостиком махнула, яичко упало и разбилось. "
        "Плачет дед, плачет баба и говорит им Курочка Ряба:\n"
        "- Не плачь, дед, не плачь, баба: снесу вам новое яичко не золотое, а простое!\n"
        "\n"
        "\n"
        "    Красная Шапочка дернула за веревочку-дверь и открылась. Вошла девочка в домик, а Волк спрятался "
        "под одеяло и говорит:\n"
        "- Положи - ка, внучка, пирожок на стол, горшочек на полку поставь, а сама приляг рядом со мной! "
        "Красная Шапочка прилегла рядом с Волком и спрашивает:\n"
        "  -Бабушка, почему у вас такие большие руки?\n"
        "-Это чтобы покрепче обнять тебя, дитя мое. \n"
        " - Бабушка, почему у вас такие большие уши? \n"
        " - Чтобы лучше слышать, дитя мое.\n"
        " - Бабушка, почему у вас такие большие глаза? \n"
        " - Чтобы лучше видеть, дитя мое.\n"
        " - Бабушка, почему у вас такие большие зубы? \n"
        "- А это чтоб скорее съесть тебя, дитя мое! Не успела Красная Шапочка и охнуть, как Волк бросился "
        "на нее и проглотил\n"
        "\n"
        "\n"
        "  Жил старик со своею старухой у самого синего моря. Они жили в ветхой землянке ровно тридцать лет "
        "и три года. Старик ловил неводом рыбу, старуха пряла свою пряжу.\n"
        "Раз он в море закинул невод — пришёл невод с одною тиной.\n"
        "Он в другой раз закинул невод — пришёл невод с травой морскою.\n"
        "В третий раз закинул он невод — пришёл невод с одною рыбкой, с не простою рыбкой — золотою.\n"
        "\n"
        "\n"
        "    Осёл тихонько поставил передние ноги на подоконник, собака взобралась на спину ослу, кот вскочил "
        "на спину собаке, а петух взлетел на голову коту. И тут они все разом закричали:осёл — по - ослиному, "
        "собака — по - собачьи, кот — по - кошачьи, а петух закукарекал. Закричали они и ввалились через окно "
        "в комнату. Испугались разбойники и убежали в лес. А осёл, собака, кот и петух сели вокруг стола и "
        "принялись за еду. Ели - ели, пили - пили — наелись, напились и спать легли. Осёл растянулся во дворе "
        "на сене, собака улеглась перед дверью, кот свернулся клубком на тёплой печи, а петух взлетел на ворота. "
        "Потушили они огонь в доме и заснули.");
}
constexpr int kWrongLeft = 1420;
constexpr int kAnswerTop = 72;
constexpr int kAnswerButtonGap = 75;

QPixmap flattenPixmapOnWhite(const QPixmap &source) {
    if (source.isNull()) {
        return source;
    }
    QPixmap target(source.size());
    target.fill(Qt::white);
    QPainter painter(&target);
    painter.drawPixmap(0, 0, source);
    painter.end();
    return target;
}

class ClickableLabel final : public QLabel {
public:
    using QLabel::QLabel;
    std::function<void()> onClick;

    void setWhiteBackedPixmap(const QPixmap &pixmap) {
        setPixmap(flattenPixmapOnWhite(pixmap));
        if (!pixmap.isNull()) {
            setFixedSize(pixmap.size());
        }
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        const QPixmap pixmap = this->pixmap(Qt::ReturnByValue);
        if (!pixmap.isNull()) {
            painter.drawPixmap(0, 0, pixmap);
        }
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick) {
            onClick();
        }
        QLabel::mouseReleaseEvent(event);
    }
};

ClickableLabel *asClickable(QLabel *label) {
    return static_cast<ClickableLabel *>(label);
}

void setAutoSizePixmap(QLabel *label, const QPixmap &pixmap) {
    if (pixmap.isNull()) {
        return;
    }
    label->setPixmap(pixmap);
    label->setFixedSize(pixmap.size());
}

void setWhiteBackedPixmap(QLabel *label, const QPixmap &pixmap) {
    auto *button = dynamic_cast<ClickableLabel *>(label);
    if (button) {
        button->setWhiteBackedPixmap(pixmap);
        return;
    }
    setAutoSizePixmap(label, flattenPixmapOnWhite(pixmap));
}

void applyButtonPixmap(QLabel *label, const QPixmap &source, int maxWidth, int maxHeight) {
    if (source.isNull() || !label) {
        return;
    }
    QPixmap pixmap = source;
    if (maxWidth > 0 && maxHeight > 0
        && (pixmap.width() > maxWidth || pixmap.height() > maxHeight)) {
        pixmap = source.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    setWhiteBackedPixmap(label, pixmap);
}

} // namespace

OnlyPExercise::OnlyPExercise(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral("background-color: #ffffff;"));

    m_picture = new QLabel(this);
    m_picture->setGeometry(kPictureLeft, kPictureTop, 1, 1);
    m_picture->setScaledContents(false);
    m_picture->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_picture->setStyleSheet(QStringLiteral("background: transparent;"));

    m_picture2 = new QLabel(this);
    m_picture2->setScaledContents(false);
    m_picture2->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_picture2->setStyleSheet(QStringLiteral("background: transparent;"));
    m_picture2->hide();

    m_stopButton = new ClickableLabel(this);
    m_stopButton->setGeometry(kStopLeft, kStopTop, 134, 29);
    m_stopButton->setScaledContents(false);
    m_stopButton->setAutoFillBackground(true);
    m_stopButton->hide();

    m_rightButton = new ClickableLabel(this);
    m_rightButton->setGeometry(kRightLeft, kAnswerTop, 134, 29);
    m_rightButton->setScaledContents(false);
    m_rightButton->setAutoFillBackground(true);
    m_rightButton->hide();

    m_wrongButton = new ClickableLabel(this);
    m_wrongButton->setGeometry(kWrongLeft, kAnswerTop, 134, 29);
    m_wrongButton->setScaledContents(false);
    m_wrongButton->setAutoFillBackground(true);
    m_wrongButton->hide();

    m_navBackButton = new ClickableLabel(this);
    m_navBackButton->setScaledContents(false);
    m_navBackButton->setAutoFillBackground(true);
    m_navBackButton->hide();

    m_navNextButton = new ClickableLabel(this);
    m_navNextButton->setScaledContents(false);
    m_navNextButton->setAutoFillBackground(true);
    m_navNextButton->hide();

    m_wordLabel = new QLabel(this);
    m_wordLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_wordLabel->setStyleSheet(QStringLiteral("color: #000000; background: transparent;"));
    m_wordLabel->setFont(QFont(QStringLiteral("Segoe UI"), 22));
    m_wordLabel->hide();

    // Как f311 в onlyp.cs: таймер появляется после порога (3.1.1/3.1.11/3.1.17 — 180с).
    m_overtimeLabel = new QLabel(this);
    m_overtimeLabel->setStyleSheet(QStringLiteral(
        "color: #000000; background: transparent; font-size: 16pt; font-weight: 700;"));
    m_overtimeLabel->hide();

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        ++m_elapsedSeconds;
        updateOvertimeTimer();
    });

    m_advanceTimer = new QTimer(this);
    m_advanceTimer->setInterval(10000);
    connect(m_advanceTimer, &QTimer::timeout, this, [this]() {
        if (!m_settings.autoAdvancePictures) {
            return;
        }
        // Как в onlyp.t14: пока acount <= 2, показываем следующий фрагмент p2..p4.
        if (m_picturesShown <= 3) {
            const int next = m_picturesShown + 1;
            if (next <= m_settings.pictureCount) {
                loadPicture(next);
            }
        }
    });

    asClickable(m_stopButton)->onClick = [this]() {
        if (m_mirrorMode) {
            emit mirrorStopRequested();
            return;
        }
        finishExercise();
    };
    asClickable(m_rightButton)->onClick = [this]() { recordAnswer(true); };
    asClickable(m_wrongButton)->onClick = [this]() { recordAnswer(false); };
    asClickable(m_navBackButton)->onClick = [this]() { browseBack(); };
    asClickable(m_navNextButton)->onClick = [this]() { browseNext(); };
}

void OnlyPExercise::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    QWidget::paintEvent(event);
}

void OnlyPExercise::setDisplayRole(DisplayRole role) {
    m_displayRole = role;
    updateWidgetLayout();
}

void OnlyPExercise::initAnswerButtons(const QString &exerciseId) {
    const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
    const QString rightPath = ExerciseAssets::exerciseFile(
        exerciseId, QStringLiteral("right.png"));
    const QString wrongPath = ExerciseAssets::exerciseFile(
        exerciseId, QStringLiteral("notright.png"));
    const QString fallbackId = QStringLiteral("1.2");
    if (!stopPath.isEmpty()) {
        m_stopSource = QPixmap(stopPath);
    }
    if (!rightPath.isEmpty()) {
        m_rightSource = QPixmap(rightPath);
    } else {
        const QString fallback = ExerciseAssets::exerciseFile(fallbackId, QStringLiteral("right.png"));
        if (!fallback.isEmpty()) {
            m_rightSource = QPixmap(fallback);
        }
    }
    if (!wrongPath.isEmpty()) {
        m_wrongSource = QPixmap(wrongPath);
    } else {
        const QString fallback = ExerciseAssets::exerciseFile(fallbackId, QStringLiteral("notright.png"));
        if (!fallback.isEmpty()) {
            m_wrongSource = QPixmap(fallback);
        }
    }
}

void OnlyPExercise::updateWidgetLayout() {
    if (m_displayRole == DisplayRole::Headless) {
        hide();
        return;
    }

    constexpr qreal kRefW = 1920.0;
    constexpr qreal kRefH = 1080.0;
    const qreal sx = width() > 0 ? width() / kRefW : 1.0;
    const qreal sy = height() > 0 ? height() / kRefH : 1.0;

    const bool showButtons =
        m_displayRole == DisplayRole::Primary || m_displayRole == DisplayRole::Specialist;
    const bool showAnswerButtons = showButtons && m_settings.answerButtons;

    m_stopButton->setVisible(showButtons);
    m_rightButton->setVisible(showAnswerButtons);
    m_wrongButton->setVisible(showAnswerButtons);

    int contentTop = 0;
    if (showButtons) {
        if (m_displayRole == DisplayRole::Specialist) {
            constexpr int kMargin = 12;
            constexpr int kGap = 18;
            int x = kMargin;
            const int y = kMargin;
            const QList<QLabel *> buttons = {m_stopButton, m_rightButton, m_wrongButton};
            const QList<QPixmap> sources = {m_stopSource, m_rightSource, m_wrongSource};
            int maxButtonH = 0;
            for (int i = 0; i < buttons.size(); ++i) {
                QLabel *button = buttons.at(i);
                if (i > 0 && !showAnswerButtons) {
                    button->hide();
                    continue;
                }
                if (sources.at(i).isNull()) {
                    button->hide();
                    continue;
                }
                setWhiteBackedPixmap(button, sources.at(i));
                button->move(x, y);
                button->show();
                maxButtonH = qMax(maxButtonH, button->height());
                if (i == 0) {
                    x += button->width() + kGap + kAnswerButtonGap;
                } else {
                    x += button->width() + kGap;
                }
            }
            contentTop = kMargin + maxButtonH + kMargin;
        } else {
            if (!m_stopSource.isNull()) {
                setWhiteBackedPixmap(m_stopButton, m_stopSource);
                m_stopButton->move(qRound(kStopLeft * sx), qRound(kStopTop * sy));
                m_stopButton->show();
            }
            if (!m_rightSource.isNull()) {
                setWhiteBackedPixmap(m_rightButton, m_rightSource);
                m_rightButton->move(qRound(kRightLeft * sx), qRound(kAnswerTop * sy));
                if (showAnswerButtons) {
                    m_rightButton->show();
                } else {
                    m_rightButton->hide();
                }
            }
            if (!m_wrongSource.isNull()) {
                setWhiteBackedPixmap(m_wrongButton, m_wrongSource);
                m_wrongButton->move(qRound(kWrongLeft * sx), qRound(kAnswerTop * sy));
                if (showAnswerButtons) {
                    m_wrongButton->show();
                } else {
                    m_wrongButton->hide();
                }
            }
            contentTop = qRound(kPictureTop * sy);
        }
    }

    if (!m_pictureSource.isNull()) {
        const int pictureMargin = 12;
        QPixmap display = m_pictureSource;
        const qreal layoutSx = width() > 0 ? width() / 1920.0 : 1.0;
        const bool fairySplitLayout = isFairyTaleExercise()
            && (m_displayRole == DisplayRole::Primary
                || m_displayRole == DisplayRole::Specialist);
        // 4.1.1: слева текст (~800px), справа картинка (один экран и dual specialist).
        const int leftReserve = fairySplitLayout
            ? qMax(pictureMargin, qMin(width() / 2, qRound(kFairyTextW * layoutSx) + 24))
            : pictureMargin;
        const int availableW = qMax(40, width() - leftReserve - pictureMargin);
        const int availableH = qMax(40, height() - contentTop - pictureMargin);
        if (display.width() > availableW || display.height() > availableH) {
            display = m_pictureSource.scaled(
                availableW, availableH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_picture->setPixmap(display);
        m_picture->setFixedSize(display.size());

        int extraX = 0;
        int extraY = 0;
        if (m_exerciseId == QStringLiteral("1.1")
            || m_exerciseId == QStringLiteral("1.25")
            || m_exerciseId == QStringLiteral("2.10")
            || m_exerciseId == QStringLiteral("3.1.2")
            || m_exerciseId == QStringLiteral("3.1.10")
            || m_exerciseId == QStringLiteral("3.1.12")) {
            // Центр картинки = центр экрана (один экран и dual).
            extraX = 0;
            extraY = 0;
        } else if (m_exerciseId == QStringLiteral("1.4")) {
            // Один экран и экран пациента: на 100px выше текущей позиции.
            if (m_displayRole == DisplayRole::Primary) {
                extraY = -140; // было −40
            } else if (m_displayRole == DisplayRole::Patient) {
                extraY = -100;
            }
        } else if (m_exerciseId == QStringLiteral("1.8")) {
            // Один экран: по центру по вертикали (опустить чуть ниже).
            if (m_displayRole == DisplayRole::Primary) {
                extraY = 40;
            }
        } else if (m_exerciseId == QStringLiteral("1.18")) {
            // Задание 3: центр картинки как в 1.1 (один экран и 2-й монитор).
            if (m_stepId == QStringLiteral("3")) {
                extraX = 0;
                extraY = 0;
            } else if (m_displayRole == DisplayRole::Primary) {
                extraX = 80;
            }
        } else if (m_exerciseId == QStringLiteral("3.1.1")) {
            // Один экран: центр как в 1.1. Dual patient: чуть вверх/правее (как было).
            if (m_displayRole == DisplayRole::Primary) {
                extraX = 0;
                extraY = 0;
            } else if (m_displayRole == DisplayRole::Patient) {
                extraX = 50;
                extraY = -50;
            } else {
                extraY = 40;
            }
        } else if (m_exerciseId == QStringLiteral("3.1.11")) {
            // По центру экрана; один экран — чуть правее.
            if (m_displayRole == DisplayRole::Primary) {
                extraX = 80;
                extraY = 40;
            } else {
                extraY = 40;
            }
        } else if (m_exerciseId == QStringLiteral("2.9")) {
            // 12.3: dual/full — чуть ниже, ближе к центру по вертикали.
            extraY = 80;
        } else if (m_exerciseId == QStringLiteral("3.1.18")) {
            // 20.2 один экран: ниже и правее → центр. 20.3 dual: только ниже на обоих.
            if (m_displayRole == DisplayRole::Primary) {
                extraX = 80;
                extraY = 40;
            } else {
                extraY = 40;
            }
        } else if (m_exerciseId == QStringLiteral("3.2.1")
                   || m_exerciseId == QStringLiteral("3.2.2")
                   || m_exerciseId == QStringLiteral("3.2.4")
                   || m_exerciseId == QStringLiteral("3.2.5")) {
            // Один экран: ниже и правее → центр. Dual: только ниже на обоих.
            if (m_displayRole == DisplayRole::Primary) {
                extraX = 80;
                extraY = 40;
            } else {
                extraY = 40;
            }
        } else if (m_exerciseId == QStringLiteral("3.2.3")) {
            // 23.10 один экран: чуть правее, по центру H/V.
            if (m_displayRole == DisplayRole::Primary) {
                extraX = 80;
            }
        } else if (m_exerciseId == QStringLiteral("3.2.11")) {
            // Задание 2: один экран — центр H/V; dual — ниже по вертикали на обоих.
            if (m_stepId == QStringLiteral("2")) {
                if (m_displayRole == DisplayRole::Primary) {
                    extraX = 80;
                    extraY = 40;
                } else {
                    extraY = 40;
                }
            }
        } else if (m_exerciseId == QStringLiteral("4.1.1")) {
            // Картинка справа; текст сказок слева (Primary / Specialist). Patient — по центру.
            if (m_displayRole == DisplayRole::Primary
                || m_displayRole == DisplayRole::Specialist) {
                extraX = 80;
                extraY = 40;
            } else if (m_displayRole == DisplayRole::Patient) {
                extraY = 40;
            }
        } else if (m_exerciseId == QStringLiteral("4.1.2")) {
            // 28.6 один экран («1»): выше и левее → центр H/V.
            // 28.7 dual («Пример»): выше и левее → центр правой половины / 2-го экрана.
            // 28.8 dual («1»): ниже и левее → центр правой половины / 2-го экрана.
            const bool isExample = m_stepId != QStringLiteral("1");
            if (m_displayRole == DisplayRole::Primary) {
                extraX = 80;
                extraY = 40;
            } else if (isExample) {
                extraX = -25;
                extraY = -20;
            } else {
                extraX = -25;
                extraY = 40;
            }
        }

        if (m_settings.dualPicture) {
            // 1.13: 200/950 @300; 2.9: 200/1000 @240 — на полном экране.
            // На панели специалиста (узкий rightPanel) абсолютные координаты
            // сжимаются и картинки накладываются — раскладываем рядом по ширине панели.
            if (m_displayRole == DisplayRole::Specialist) {
                constexpr int kGap = 24;
                const int halfW = qMax(40, (availableW - kGap) / 2);
                QPixmap leftDisplay = m_pictureSource;
                if (leftDisplay.width() > halfW || leftDisplay.height() > availableH) {
                    leftDisplay = m_pictureSource.scaled(
                        halfW, availableH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
                m_picture->setPixmap(leftDisplay);
                m_picture->setFixedSize(leftDisplay.size());
                m_picture->show();

                int maxH = leftDisplay.height();
                QPixmap rightDisplay;
                if (!m_picture2Source.isNull() && m_picture2) {
                    rightDisplay = m_picture2Source;
                    if (rightDisplay.width() > halfW || rightDisplay.height() > availableH) {
                        rightDisplay = m_picture2Source.scaled(
                            halfW, availableH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                    m_picture2->setPixmap(rightDisplay);
                    m_picture2->setFixedSize(rightDisplay.size());
                    maxH = qMax(maxH, rightDisplay.height());
                }

                const int pictureY =
                    contentTop + qMax(0, (availableH - maxH) / 2) + extraY;
                const int leftX =
                    pictureMargin + qMax(0, (halfW - leftDisplay.width()) / 2) + extraX;
                m_picture->move(qMax(pictureMargin, leftX), qMax(contentTop, pictureY));
                m_picture->raise();

                if (!rightDisplay.isNull() && m_picture2) {
                    const int rightX = pictureMargin + halfW + kGap
                        + qMax(0, (halfW - rightDisplay.width()) / 2) + extraX;
                    m_picture2->move(qMax(pictureMargin, rightX), qMax(contentTop, pictureY));
                    m_picture2->show();
                    m_picture2->raise();
                }
            } else {
                const int leftX = 200;
                const int rightX = (m_exerciseId == QStringLiteral("2.9")) ? 1000 : 950;
                const int dualY = (m_exerciseId == QStringLiteral("2.9")) ? 240 : 300;
                const int halfW = qMax(40, (availableW - 40) / 2);
                QPixmap leftDisplay = display;
                if (leftDisplay.width() > halfW || leftDisplay.height() > availableH) {
                    leftDisplay = m_pictureSource.scaled(
                        halfW, availableH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    m_picture->setPixmap(leftDisplay);
                    m_picture->setFixedSize(leftDisplay.size());
                }
                m_picture->move(qRound(leftX * sx) + extraX, qRound(dualY * sy) + extraY);
                m_picture->show();
                if (!m_picture2Source.isNull() && m_picture2) {
                    QPixmap display2 = m_picture2Source;
                    if (display2.width() > halfW || display2.height() > availableH) {
                        display2 = m_picture2Source.scaled(
                            halfW, availableH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                    m_picture2->setPixmap(display2);
                    m_picture2->setFixedSize(display2.size());
                    m_picture2->move(qRound(rightX * sx) + extraX, qRound(dualY * sy) + extraY);
                    m_picture2->show();
                    m_picture2->raise();
                }
            }
        } else if (m_displayRole == DisplayRole::Patient) {
            int pictureX = pictureMargin + qMax(0, (width() - display.width()) / 2) + kPatientPictureShiftRight
                - kPatientSecondScreenShiftLeft + extraX;
            if (pictureX + display.width() > width() - pictureMargin) {
                pictureX = qMax(pictureMargin, width() - pictureMargin - display.width());
            }
            int pictureY = qMax(
                pictureMargin,
                (height() - display.height()) / 2 + kPatientPictureShiftDown + extraY);
            // 1.1 / 1.25 / 2.10 / 3.1.2 / 3.1.10 / 3.1.12 / 1.18(№3): строго по центру второго экрана.
            if (m_exerciseId == QStringLiteral("1.1")
                || m_exerciseId == QStringLiteral("1.25")
                || m_exerciseId == QStringLiteral("2.10")
                || m_exerciseId == QStringLiteral("3.1.2")
                || m_exerciseId == QStringLiteral("3.1.10")
                || m_exerciseId == QStringLiteral("3.1.12")
                || (m_exerciseId == QStringLiteral("1.18") && m_stepId == QStringLiteral("3"))) {
                pictureX = qMax(pictureMargin, (width() - display.width()) / 2);
                pictureY = qMax(pictureMargin, (height() - display.height()) / 2);
            }
            m_picture->move(pictureX, pictureY);
            m_picture->show();
            if (m_picture2) {
                m_picture2->hide();
            }
        } else if (m_displayRole == DisplayRole::Specialist) {
            int pictureX = pictureMargin + qMax(0, (width() - display.width()) / 2)
                - kSpecialistPictureShiftLeft + extraX;
            if (fairySplitLayout) {
                pictureX = qMax(leftReserve, leftReserve + qMax(0, (availableW - display.width()) / 2));
            } else {
                if (pictureX + display.width() > width() - pictureMargin) {
                    pictureX = qMax(pictureMargin, width() - pictureMargin - display.width());
                }
                pictureX = qMax(pictureMargin, pictureX);
            }
            int pictureY = qMax(contentTop, (height() - display.height()) / 2 + extraY);
            // 1.1 / 1.25 / 2.10 / 3.1.2 / 3.1.10 / 3.1.12 / 1.18(№3 dual specialist): строго по центру панели.
            if ((m_exerciseId == QStringLiteral("1.1")
                 || m_exerciseId == QStringLiteral("1.25")
                 || m_exerciseId == QStringLiteral("2.10")
                 || m_exerciseId == QStringLiteral("3.1.2")
                 || m_exerciseId == QStringLiteral("3.1.10")
                 || m_exerciseId == QStringLiteral("3.1.12")
                 || (m_exerciseId == QStringLiteral("1.18") && m_stepId == QStringLiteral("3")))
                && !fairySplitLayout) {
                pictureX = qMax(pictureMargin, (width() - display.width()) / 2);
                pictureY = qMax(pictureMargin, (height() - display.height()) / 2);
            }
            m_picture->move(pictureX, pictureY);
            m_picture->show();
            if (m_picture2) {
                m_picture2->hide();
            }
        } else {
            int pictureX = qRound(kPictureLeft * sx) - kPictureShiftLeft + extraX;
            if (pictureX + display.width() > width() - pictureMargin) {
                pictureX = pictureMargin
                    + qMax(0, (width() - 2 * pictureMargin - display.width()) / 2)
                    - kPictureShiftLeft + extraX;
            }
            pictureX = qMax(pictureMargin, pictureX);
            const int baseTop = showButtons ? contentTop : qRound(kPictureTop * sy);
            int pictureY = qMax(pictureMargin, baseTop + qRound(kPictureTopOffset * sy) + extraY);
            // 1.1 / 1.25 / 2.10 / 3.1.1 / 3.1.2 / 3.1.10 / 3.1.12 / 1.18(№3): строго по центру экрана.
            if (m_exerciseId == QStringLiteral("1.1")
                || m_exerciseId == QStringLiteral("1.25")
                || m_exerciseId == QStringLiteral("2.10")
                || m_exerciseId == QStringLiteral("3.1.1")
                || m_exerciseId == QStringLiteral("3.1.2")
                || m_exerciseId == QStringLiteral("3.1.10")
                || m_exerciseId == QStringLiteral("3.1.12")
                || (m_exerciseId == QStringLiteral("1.18") && m_stepId == QStringLiteral("3"))) {
                pictureX = qMax(pictureMargin, (width() - display.width()) / 2);
                pictureY = qMax(pictureMargin, (height() - display.height()) / 2);
            } else if (m_exerciseId == QStringLiteral("3.1.11")
                || m_exerciseId == QStringLiteral("3.2.1")
                || m_exerciseId == QStringLiteral("3.2.2")
                || m_exerciseId == QStringLiteral("3.2.3")
                || m_exerciseId == QStringLiteral("3.2.4")
                || m_exerciseId == QStringLiteral("3.2.5")
                || m_exerciseId == QStringLiteral("4.1.1")
                || m_exerciseId == QStringLiteral("4.1.2")
                || (m_exerciseId == QStringLiteral("3.2.11")
                    && m_stepId == QStringLiteral("2"))) {
                // 3.1.x / 3.2.x / 4.1.1 один экран: по центру экрана по вертикали.
                pictureY = qMax(contentTop, (height() - display.height()) / 2 + extraY);
            }
            if (fairySplitLayout) {
                // Правее текста: центр правой половины.
                pictureX = qMax(leftReserve, leftReserve + qMax(0, (availableW - display.width()) / 2));
            }
            m_picture->move(pictureX, pictureY);
            m_picture->show();
            if (m_picture2) {
                m_picture2->hide();
            }
        }
    } else if (m_picture) {
        m_picture->hide();
        if (m_picture2) {
            m_picture2->hide();
        }
    }

    updateFairyTaleLayout();

    if (isCombineWordsExercise() && combineWordsUsesText() && m_wordLabel
        && m_displayRole != DisplayRole::Headless) {
        const qreal sx = width() > 0 ? width() / 1920.0 : 1.0;
        const qreal sy = height() > 0 ? height() / 1080.0 : 1.0;
        m_wordLabel->adjustSize();
        // onlyp.Designer: cWord @ (806, 275)
        m_wordLabel->move(qRound(806 * sx), qRound(275 * sy));
        m_wordLabel->show();
    } else if (m_wordLabel) {
        m_wordLabel->hide();
    }

    if (usesNavBrowseButtons() && m_displayRole != DisplayRole::Headless
        && m_displayRole != DisplayRole::Patient
        && m_navBackButton && m_navNextButton) {
        const qreal sx = width() > 0 ? width() / 1920.0 : 1.0;
        const qreal sy = height() > 0 ? height() / 1080.0 : 1.0;
        ensureCombineWordsUi();
        const int stopX = m_stopButton->isVisible() ? m_stopButton->x() : qRound(kStopLeft * sx);
        const int stopY = m_stopButton->isVisible() ? m_stopButton->y() : qRound(kStopTop * sy);
        // onlyp.cs: f312 = stop+240, ff312 = stop+350
        if (!m_navBackSource.isNull()) {
            setWhiteBackedPixmap(m_navBackButton, m_navBackSource);
            m_navBackButton->move(stopX + 240, stopY);
        }
        if (!m_navNextSource.isNull()) {
            setWhiteBackedPixmap(m_navNextButton, m_navNextSource);
            m_navNextButton->move(stopX + 350, stopY);
        }
        updateCombineWordsNavVisibility();
    } else {
        if (m_navBackButton) {
            m_navBackButton->hide();
        }
        if (m_navNextButton) {
            m_navNextButton->hide();
        }
    }

    if (m_wordLabel && m_wordLabel->isVisible()) {
        m_wordLabel->raise();
    } else if (m_picture) {
        m_picture->raise();
    }
    if (m_picture2 && m_picture2->isVisible()) {
        m_picture2->raise();
    }
    // Кнопки поверх картинки — иначе Стоп не кликается (3.1.11 dual и др.).
    m_stopButton->raise();
    m_rightButton->raise();
    m_wrongButton->raise();
    if (m_navBackButton) {
        m_navBackButton->raise();
    }
    if (m_navNextButton) {
        m_navNextButton->raise();
    }
    updateOvertimeTimer();
}

int OnlyPExercise::overtimeThresholdSeconds() const {
    // onlyp.cs: f311 появляется после порога.
    if (m_exerciseId == QStringLiteral("3.1.1")
        || m_exerciseId == QStringLiteral("3.1.11")
        || m_exerciseId == QStringLiteral("3.1.17")) {
        return 180;
    }
    if (m_exerciseId == QStringLiteral("3.1.18")) {
        return 120;
    }
    // 4.1.2 «Пример»: время не учитывается; порог только для задания «1».
    if (m_exerciseId == QStringLiteral("4.1.2")) {
        return (m_stepId == QStringLiteral("1")) ? 90 : -1;
    }
    return -1;
}

void OnlyPExercise::updateOvertimeTimer() {
    if (!m_overtimeLabel) {
        return;
    }
    const int threshold = overtimeThresholdSeconds();
    const bool show = threshold > 0
        && m_elapsedSeconds > threshold
        && m_timer && m_timer->isActive()
        && m_displayRole != DisplayRole::Headless
        && m_displayRole != DisplayRole::Patient;
    if (!show) {
        m_overtimeLabel->hide();
        return;
    }
    const int minutes = m_elapsedSeconds / 60;
    const int seconds = m_elapsedSeconds - minutes * 60;
    m_overtimeLabel->setText(QStringLiteral("%1:%2 сек").arg(minutes).arg(seconds));
    m_overtimeLabel->adjustSize();
    // onlyp.Designer: f311 @ (1162, 75)
    const qreal sx = width() / 1920.0;
    const qreal sy = height() / 1080.0;
    m_overtimeLabel->move(qRound(1162 * sx), qRound(75 * sy));
    m_overtimeLabel->show();
    m_overtimeLabel->raise();
}

void OnlyPExercise::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateWidgetLayout();
}

void OnlyPExercise::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (m_stopSource.isNull()) {
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stopSource = QPixmap(stopPath);
        }
    }
    updateWidgetLayout();
}

void OnlyPExercise::start(
    const QString &exerciseId,
    const OnlyPictureSettings &settings,
    const QString &stepId) {
    m_exerciseId = exerciseId;
    m_settings = settings;
    m_stepId = stepId;
    if (m_stepId.isEmpty() && !m_settings.stepIds.isEmpty()) {
        m_stepId = m_settings.stepIds.first();
    }
    setProperty("exerciseId", exerciseId);
    setProperty("stepId", m_stepId);
    m_answers.clear();
    const int answerCount = qMax(1, m_settings.pictureCount);
    for (int i = 0; i < answerCount; ++i) {
        m_answers.append(false);
    }
    m_index = 0;
    m_picturesShown = 0;
    m_elapsedSeconds = 0;
    m_stepElapsedSeconds.clear();
    if (!m_stepId.isEmpty()) {
        m_stepElapsedSeconds.insert(m_stepId, 0);
    }
    if (m_overtimeLabel) {
        m_overtimeLabel->hide();
        m_overtimeLabel->clear();
    }

    if (m_displayRole != DisplayRole::Headless) {
        initAnswerButtons(exerciseId);
        updateWidgetLayout();
    }

    if (m_settings.dualPicture) {
        // left = imagePattern, right = secondImagePattern (1.13: p2|p1; 2.9: 1|2)
        const QString leftName = m_settings.imagePattern.isEmpty()
            ? QStringLiteral("p2.png")
            : m_settings.imagePattern;
        const QString rightName = m_settings.secondImagePattern.isEmpty()
            ? QStringLiteral("p1.png")
            : m_settings.secondImagePattern;
        const QString leftPath = ExerciseAssets::exerciseFile(exerciseId, leftName);
        const QString rightPath = ExerciseAssets::exerciseFile(exerciseId, rightName);
        if (!leftPath.isEmpty()) {
            m_pictureSource.load(leftPath);
        }
        if (!rightPath.isEmpty()) {
            m_picture2Source.load(rightPath);
        }
        m_picturesShown = 1;
        updateWidgetLayout();
        if (m_picture) {
            m_picture->raise();
        }
        if (m_picture2) {
            m_picture2->raise();
        }
        // Чтобы dual-зеркала (Specialist/Patient) подхватили кадр после connect.
        emit pictureChanged(1);
    } else if (isCombineWordsExercise()) {
        m_browseIndex = 0;
        ensureCombineWordsUi();
        refreshCombineWordsStimulus();
    } else if (isPictureBrowseExercise()) {
        m_browseIndex = 0;
        ensureCombineWordsUi();
        loadPicture(1);
        updateCombineWordsNavVisibility();
    } else {
        loadPicture(1);
    }
    updateFairyTaleLayout();
    m_timer->start();
    if (m_settings.autoAdvancePictures) {
        m_advanceTimer->start();
    } else {
        m_advanceTimer->stop();
    }
    if (m_displayRole != DisplayRole::Headless) {
        show();
        raise();
    }
}

void OnlyPExercise::commitCurrentStepTime() {
    if (m_stepId.isEmpty()) {
        return;
    }
    m_stepElapsedSeconds.insert(m_stepId, m_elapsedSeconds);
}

void OnlyPExercise::switchStep(const QString &stepId) {
    const QString next = stepId.trimmed();
    if (next.isEmpty() || next == m_stepId) {
        return;
    }
    commitCurrentStepTime();
    m_stepId = next;
    setProperty("stepId", m_stepId);
    m_elapsedSeconds = m_stepElapsedSeconds.value(m_stepId, 0);
    if (m_settings.dualPicture) {
        return;
    }
    if (isCombineWordsExercise()) {
        m_browseIndex = 0;
        refreshCombineWordsStimulus();
        return;
    }
    loadPicture(1);
    if (m_picture) {
        m_picture->raise();
    }
}

QString OnlyPExercise::imageFileName(int index) const {
    OnlyPictureSettings settings = m_settings;
    if (settings.imagePattern.isEmpty()) {
        if (const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId)) {
            settings = definition->onlyPicture;
        }
    }
    // 4.1.2: «Пример» → 1.png, «1» → 2.png
    if (m_exerciseId == QStringLiteral("4.1.2")) {
        if (m_stepId == QStringLiteral("1")) {
            return QStringLiteral("2.png");
        }
        return QStringLiteral("1.png");
    }
    // 3.2.3: задание 1 → 1.png/2.png (листание), задание 2 → 3.png (картинка со словами).
    if (isCombineWordsExercise()) {
        if (m_stepId == QStringLiteral("2")) {
            return QStringLiteral("3.png");
        }
        return QStringLiteral("%1.png").arg(m_browseIndex + 1);
    }
    if (!m_stepId.isEmpty() && !settings.stepIds.isEmpty()) {
        // Numbered (1.17/1.18/…): файл по № задания, не по индексу слайда.
        return settings.imagePattern.arg(m_stepId);
    }
    return settings.imagePattern.arg(index);
}

bool OnlyPExercise::isCombineWordsExercise() const {
    return m_exerciseId == QStringLiteral("3.2.3");
}

bool OnlyPExercise::isPictureBrowseExercise() const {
    // 3.1.12: одно задание, картинки 1..N листаются Вперёд/Назад (без combo).
    return m_exerciseId == QStringLiteral("3.1.12");
}

bool OnlyPExercise::usesNavBrowseButtons() const {
    return isCombineWordsExercise() || isPictureBrowseExercise();
}

bool OnlyPExercise::isFairyTaleExercise() const {
    return m_exerciseId == QStringLiteral("4.1.1");
}

void OnlyPExercise::ensureFairyTaleUi() {
    if (m_fairyText) {
        return;
    }
    m_fairyText = new QTextEdit(this);
    m_fairyText->setReadOnly(true);
    m_fairyText->setFrameShape(QFrame::NoFrame);
    m_fairyText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fairyText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_fairyText->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 12));
    m_fairyText->setStyleSheet(QStringLiteral(
        "QTextEdit { background:#ffffff; color:#000000; border:none; padding:4px; }"));
    m_fairyText->hide();
}

void OnlyPExercise::updateFairyTaleLayout() {
    if (!isFairyTaleExercise()) {
        if (m_fairyText) {
            m_fairyText->hide();
        }
        return;
    }
    ensureFairyTaleUi();
    const bool showText = m_displayRole == DisplayRole::Primary
        || m_displayRole == DisplayRole::Specialist;
    if (!showText || m_displayRole == DisplayRole::Headless) {
        m_fairyText->hide();
        return;
    }

    m_fairyText->setPlainText(fairyTaleText411());
    const qreal sx = width() > 0 ? width() / 1920.0 : 1.0;
    const qreal sy = height() > 0 ? height() / 1080.0 : 1.0;
    int textX = qRound(kFairyTextLeft * sx);
    int textY = qRound(kFairyTextTop * sy);
    int textW = qRound(kFairyTextW * sx);
    int textH = qRound(kFairyTextH * sy);
    // Один экран и dual specialist: слева текст, справа картинка (оригинал 800×800).
    textW = qMin(textW, qMax(200, width() / 2 - 24));
    textH = qMin(textH, qMax(200, height() - textY - 12));
    m_fairyText->setGeometry(textX, textY, textW, textH);
    m_fairyText->show();
    m_fairyText->raise();
}

bool OnlyPExercise::combineWordsUsesText() const {
    // Задание 2 — картинка 3.png со словами (не текстовая метка cWord).
    return false;
}

void OnlyPExercise::ensureCombineWordsUi() {
    if (!isCombineWordsExercise()) {
        return;
    }
    if (m_navBackSource.isNull()) {
        const QString path = ExerciseAssets::exerciseFile(QStringLiteral("3.1.12"), QStringLiteral("back.png"));
        if (!path.isEmpty()) {
            m_navBackSource.load(path);
        }
    }
    if (m_navNextSource.isNull()) {
        const QString path = ExerciseAssets::exerciseFile(QStringLiteral("3.1.12"), QStringLiteral("next.png"));
        if (!path.isEmpty()) {
            m_navNextSource.load(path);
        }
    }
}

void OnlyPExercise::updateCombineWordsNavVisibility() {
    if (!m_navBackButton || !m_navNextButton) {
        return;
    }
    if (!usesNavBrowseButtons() || m_displayRole == DisplayRole::Headless
        || m_displayRole == DisplayRole::Patient
        || (isCombineWordsExercise() && m_stepId == QStringLiteral("2"))) {
        // 3.2.3 задание 2 — одна картинка, листание не нужно.
        m_navBackButton->hide();
        m_navNextButton->hide();
        return;
    }
    const int maxIndex = isPictureBrowseExercise()
        ? qMax(0, m_settings.pictureCount - 1)
        : 1;
    m_navBackButton->setVisible(m_browseIndex > 0 && !m_navBackSource.isNull());
    m_navNextButton->setVisible(m_browseIndex < maxIndex && !m_navNextSource.isNull());
}

void OnlyPExercise::refreshCombineWordsStimulus(bool notifyMirrors) {
    if (!isCombineWordsExercise()) {
        return;
    }
    if (m_wordLabel) {
        m_wordLabel->hide();
        m_wordLabel->clear();
    }
    if (m_stepId == QStringLiteral("2")) {
        m_browseIndex = 0;
        const QString path = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("3.png"));
        if (!path.isEmpty()) {
            m_pictureSource.load(path);
        } else {
            m_pictureSource = QPixmap();
        }
        m_picturesShown = qMax(m_picturesShown, 1);
        updateWidgetLayout();
        if (m_picture) {
            m_picture->raise();
        }
        if (notifyMirrors) {
            emit pictureChanged(3);
            emit browseStateChanged(0);
        }
        return;
    }
    m_browseIndex = qBound(0, m_browseIndex, 1);
    const QString path = ExerciseAssets::exerciseFile(
        m_exerciseId, QStringLiteral("%1.png").arg(m_browseIndex + 1));
    if (!path.isEmpty()) {
        m_pictureSource.load(path);
    }
    m_picturesShown = qMax(m_picturesShown, m_browseIndex + 1);
    updateWidgetLayout();
    if (m_picture) {
        m_picture->raise();
    }
    if (notifyMirrors) {
        emit pictureChanged(m_browseIndex + 1);
        emit browseStateChanged(m_browseIndex);
    }
}

void OnlyPExercise::browseNext() {
    if (m_mirrorMode) {
        emit mirrorBrowseNextRequested();
        return;
    }
    if (isPictureBrowseExercise()) {
        const int maxIndex = qMax(0, m_settings.pictureCount - 1);
        if (m_browseIndex >= maxIndex) {
            return;
        }
        ++m_browseIndex;
        loadPicture(m_browseIndex + 1);
        m_picturesShown = qMax(m_picturesShown, m_browseIndex + 1);
        updateWidgetLayout();
        emit pictureChanged(m_browseIndex + 1);
        emit browseStateChanged(m_browseIndex);
        return;
    }
    if (!isCombineWordsExercise() || m_stepId == QStringLiteral("2")) {
        return;
    }
    const int maxIndex = 1;
    if (m_browseIndex >= maxIndex) {
        return;
    }
    ++m_browseIndex;
    refreshCombineWordsStimulus(true);
}

void OnlyPExercise::browseBack() {
    if (m_mirrorMode) {
        emit mirrorBrowseBackRequested();
        return;
    }
    if (isPictureBrowseExercise()) {
        if (m_browseIndex <= 0) {
            return;
        }
        --m_browseIndex;
        loadPicture(m_browseIndex + 1);
        updateWidgetLayout();
        emit pictureChanged(m_browseIndex + 1);
        emit browseStateChanged(m_browseIndex);
        return;
    }
    if (!isCombineWordsExercise() || m_stepId == QStringLiteral("2") || m_browseIndex <= 0) {
        return;
    }
    --m_browseIndex;
    refreshCombineWordsStimulus(true);
}

void OnlyPExercise::applyBrowseIndex(int index) {
    if (isPictureBrowseExercise()) {
        const int maxIndex = qMax(0, m_settings.pictureCount - 1);
        m_browseIndex = qBound(0, index, maxIndex);
        loadPicture(m_browseIndex + 1);
        m_picturesShown = qMax(m_picturesShown, m_browseIndex + 1);
        updateWidgetLayout();
        return;
    }
    if (!isCombineWordsExercise() || m_stepId == QStringLiteral("2")) {
        return;
    }
    m_browseIndex = qBound(0, index, 1);
    refreshCombineWordsStimulus(false);
}

void OnlyPExercise::syncMirrorSession(
    const QString &exerciseId,
    const OnlyPictureSettings &settings,
    const QString &stepId) {
    m_exerciseId = exerciseId;
    m_settings = settings;
    m_stepId = stepId.trimmed().isEmpty() && !settings.stepIds.isEmpty()
        ? settings.stepIds.first()
        : stepId.trimmed();
    m_browseIndex = 0;
    setProperty("exerciseId", exerciseId);
    setProperty("stepId", m_stepId);
    if (m_settings.dualPicture) {
        const QString leftName = m_settings.imagePattern.isEmpty()
            ? QStringLiteral("p2.png")
            : m_settings.imagePattern;
        const QString rightName = m_settings.secondImagePattern.isEmpty()
            ? QStringLiteral("p1.png")
            : m_settings.secondImagePattern;
        const QString leftPath = ExerciseAssets::exerciseFile(exerciseId, leftName);
        const QString rightPath = ExerciseAssets::exerciseFile(exerciseId, rightName);
        if (!leftPath.isEmpty()) {
            m_pictureSource.load(leftPath);
        }
        if (!rightPath.isEmpty()) {
            m_picture2Source.load(rightPath);
        }
        m_picturesShown = 1;
        updateWidgetLayout();
        if (m_displayRole != DisplayRole::Headless) {
            show();
            raise();
        }
        return;
    }
    if (isCombineWordsExercise()) {
        ensureCombineWordsUi();
        refreshCombineWordsStimulus(false);
        return;
    }
    if (isPictureBrowseExercise()) {
        ensureCombineWordsUi();
        loadPicture(1);
        return;
    }
    loadPicture(1);
}

void OnlyPExercise::loadPicture(int index) {
    const QString path = ExerciseAssets::exerciseFile(m_exerciseId, imageFileName(index));
    if (path.isEmpty()) {
        return;
    }
    m_pictureSource.load(path);
    m_picturesShown = qMax(m_picturesShown, index);
    updateWidgetLayout();
    m_picture->raise();
    emit pictureChanged(index);
}

void OnlyPExercise::showPicture(int index) {
    if (m_settings.dualPicture) {
        // Зеркало: заново применить dual-картинки (Headless эмитит pictureChanged после start).
        const QString leftName = m_settings.imagePattern.isEmpty()
            ? QStringLiteral("p2.png")
            : m_settings.imagePattern;
        const QString rightName = m_settings.secondImagePattern.isEmpty()
            ? QStringLiteral("p1.png")
            : m_settings.secondImagePattern;
        const QString leftPath = ExerciseAssets::exerciseFile(m_exerciseId, leftName);
        const QString rightPath = ExerciseAssets::exerciseFile(m_exerciseId, rightName);
        if (!leftPath.isEmpty()) {
            m_pictureSource.load(leftPath);
        }
        if (!rightPath.isEmpty()) {
            m_picture2Source.load(rightPath);
        }
        m_picturesShown = 1;
        updateWidgetLayout();
        if (m_displayRole != DisplayRole::Headless) {
            show();
            raise();
        }
        return;
    }
    if (isCombineWordsExercise() || isPictureBrowseExercise()) {
        applyBrowseIndex(qMax(0, index - 1));
        return;
    }
    // Зеркало: не эмитить pictureChanged повторно (иначе петля / лишние сбросы).
    const QString path = ExerciseAssets::exerciseFile(m_exerciseId, imageFileName(index));
    if (path.isEmpty()) {
        return;
    }
    m_pictureSource.load(path);
    m_picturesShown = qMax(m_picturesShown, index);
    updateWidgetLayout();
    if (m_picture) {
        m_picture->raise();
    }
    if (m_displayRole != DisplayRole::Headless) {
        show();
    }
}

void OnlyPExercise::submitAnswer(bool correct) {
    recordAnswer(correct);
}

void OnlyPExercise::setMirrorMode(bool enabled) {
    m_mirrorMode = enabled;
}

void OnlyPExercise::prepareMirrorUi(const QString &exerciseId) {
    m_exerciseId = exerciseId;
    setProperty("exerciseId", exerciseId);
    if (m_displayRole == DisplayRole::Specialist) {
        initAnswerButtons(exerciseId);
    }
    updateWidgetLayout();
    show();
}

void OnlyPExercise::stopExercise() {
    if (!m_mirrorMode) {
        finishExercise();
    }
}

void OnlyPExercise::recordAnswer(bool correct) {
    if (m_mirrorMode) {
        emit mirrorAnswerRequested(correct);
        return;
    }
    if (m_index >= m_answers.size()) {
        return;
    }
    m_answers[m_index] = correct;
    emit answerRecorded(m_index, correct);
    ++m_index;
    if (m_index >= m_answers.size()) {
        finishExercise();
        return;
    }
    loadPicture(m_index + 1);
}

void OnlyPExercise::finishExercise() {
    m_timer->stop();
    m_advanceTimer->stop();
    if (m_overtimeLabel) {
        m_overtimeLabel->hide();
    }
    if (m_fairyText) {
        m_fairyText->hide();
    }
    commitCurrentStepTime();
    if (!m_settings.answerButtons) {
        m_picturesShown = qMax(m_picturesShown, 1);
    }
    hide();
    int totalElapsed = 0;
    for (auto it = m_stepElapsedSeconds.constBegin(); it != m_stepElapsedSeconds.constEnd(); ++it) {
        totalElapsed += it.value();
    }
    if (totalElapsed <= 0) {
        totalElapsed = m_elapsedSeconds;
    }
    m_elapsedSeconds = totalElapsed;
    emit finished(m_answers, m_elapsedSeconds);
}
