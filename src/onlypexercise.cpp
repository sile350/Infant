#include "onlypexercise.h"

#include "exerciseassets.h"
#include "exerciseconfig.h"

#include <QMouseEvent>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QtMath>
#include <functional>

namespace {

constexpr int kPictureLeft = 700;
constexpr int kPictureTop = 240;
constexpr int kPictureTopOffset = 50;
constexpr int kPictureShiftLeft = 150;
constexpr int kPatientPictureShiftRight = 50;
constexpr int kPatientSecondScreenShiftLeft = 50;
constexpr int kSpecialistPictureShiftLeft = 15;
constexpr int kStopLeft = 80;
constexpr int kStopTop = 72;
constexpr int kRightLeft = 1280;
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

    m_rightButton = new ClickableLabel(this);
    m_rightButton->setGeometry(kRightLeft, kAnswerTop, 134, 29);
    m_rightButton->setScaledContents(false);
    m_rightButton->setAutoFillBackground(true);

    m_wrongButton = new ClickableLabel(this);
    m_wrongButton->setGeometry(kWrongLeft, kAnswerTop, 134, 29);
    m_wrongButton->setScaledContents(false);
    m_wrongButton->setAutoFillBackground(true);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, [this]() { ++m_elapsedSeconds; });

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
        const int availableW = qMax(40, width() - 2 * pictureMargin);
        const int availableH = qMax(40, height() - contentTop - pictureMargin);
        if (display.width() > availableW || display.height() > availableH) {
            display = m_pictureSource.scaled(
                availableW, availableH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_picture->setPixmap(display);
        m_picture->setFixedSize(display.size());

        int extraX = 0;
        int extraY = 0;
        if (m_exerciseId == QStringLiteral("1.1") || m_exerciseId == QStringLiteral("1.25")) {
            extraX = 100;
            extraY = -200;
        } else if (m_exerciseId == QStringLiteral("1.18") && m_stepId == QStringLiteral("3")) {
            extraX = 100;
            extraY = -200;
        } else if (m_exerciseId == QStringLiteral("4.1.2")) {
            // onlyp.cs: Пример → (1300,500), «1» → (1300,170)
            if (m_stepId == QStringLiteral("1")) {
                extraX = 600;
                extraY = -70;
            } else {
                extraX = 600;
                extraY = 260;
            }
        }

        if (m_settings.dualPicture) {
            // 1.13: 200/950 @300; 2.9: 200/1000 @240
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
        } else if (m_displayRole == DisplayRole::Patient) {
            int pictureX = pictureMargin + qMax(0, (width() - display.width()) / 2) + kPatientPictureShiftRight
                - kPatientSecondScreenShiftLeft + extraX;
            if (pictureX + display.width() > width() - pictureMargin) {
                pictureX = qMax(pictureMargin, width() - pictureMargin - display.width());
            }
            const int pictureY = qMax(pictureMargin, (height() - display.height()) / 2 + extraY);
            m_picture->move(pictureX, pictureY);
            m_picture->show();
            if (m_picture2) {
                m_picture2->hide();
            }
        } else if (m_displayRole == DisplayRole::Specialist) {
            int pictureX = pictureMargin + qMax(0, (width() - display.width()) / 2)
                - kSpecialistPictureShiftLeft + extraX;
            if (pictureX + display.width() > width() - pictureMargin) {
                pictureX = qMax(pictureMargin, width() - pictureMargin - display.width());
            }
            pictureX = qMax(pictureMargin, pictureX);
            const int pictureY = qMax(pictureMargin, (height() - display.height()) / 2 + extraY);
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
            const int pictureY = qMax(pictureMargin, baseTop + qRound(kPictureTopOffset * sy) + extraY);
            m_picture->move(pictureX, pictureY);
            m_picture->show();
            if (m_picture2) {
                m_picture2->hide();
            }
        }
    }

    m_stopButton->raise();
    m_rightButton->raise();
    m_wrongButton->raise();
    m_picture->raise();
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
    m_answers.clear();
    const int answerCount = qMax(1, m_settings.pictureCount);
    for (int i = 0; i < answerCount; ++i) {
        m_answers.append(false);
    }
    m_index = 0;
    m_picturesShown = 0;
    m_elapsedSeconds = 0;

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
    } else {
        loadPicture(1);
    }
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
    if (!m_stepId.isEmpty()) {
        return settings.imagePattern.arg(m_stepId);
    }
    return settings.imagePattern.arg(index);
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
        return;
    }
    loadPicture(index);
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
    if (!m_settings.answerButtons) {
        m_picturesShown = qMax(m_picturesShown, 1);
    }
    hide();
    emit finished(m_answers, m_elapsedSeconds);
}
