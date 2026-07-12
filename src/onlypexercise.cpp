#include "onlypexercise.h"

#include "exerciseassets.h"

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
constexpr int kNativePictureW = 847;
constexpr int kNativePictureH = 550;
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
    const QString rightPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("right.png"));
    const QString wrongPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("notright.png"));
    if (!stopPath.isEmpty()) {
        m_stopSource = QPixmap(stopPath);
    }
    if (!rightPath.isEmpty()) {
        m_rightSource = QPixmap(rightPath);
    }
    if (!wrongPath.isEmpty()) {
        m_wrongSource = QPixmap(wrongPath);
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

    m_stopButton->setVisible(showButtons);
    m_rightButton->setVisible(showButtons);
    m_wrongButton->setVisible(showButtons);

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
                m_rightButton->show();
            }
            if (!m_wrongSource.isNull()) {
                setWhiteBackedPixmap(m_wrongButton, m_wrongSource);
                m_wrongButton->move(qRound(kWrongLeft * sx), qRound(kAnswerTop * sy));
                m_wrongButton->show();
            }
            contentTop = qRound(kPictureTop * sy);
        }
    }

    if (!m_pictureSource.isNull()) {
        const int pictureMargin = 12;
        QPixmap scaled = m_pictureSource;
        if (m_displayRole == DisplayRole::Patient) {
            scaled = m_pictureSource.scaled(
                kNativePictureW, kNativePictureH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            m_picture->setPixmap(scaled);
            m_picture->setFixedSize(scaled.size());
            int pictureX = pictureMargin + qMax(0, (width() - scaled.width()) / 2) + kPatientPictureShiftRight
                - kPatientSecondScreenShiftLeft;
            if (pictureX + scaled.width() > width() - pictureMargin) {
                pictureX = qMax(pictureMargin, width() - pictureMargin - scaled.width());
            }
            const int pictureY = qMax(pictureMargin, (height() - scaled.height()) / 2);
            m_picture->move(pictureX, pictureY);
            m_picture->show();
        } else if (m_displayRole == DisplayRole::Specialist) {
            scaled = m_pictureSource.scaled(
                kNativePictureW, kNativePictureH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            m_picture->setPixmap(scaled);
            m_picture->setFixedSize(scaled.size());
            int pictureX = pictureMargin + qMax(0, (width() - scaled.width()) / 2);
            if (pictureX + scaled.width() > width() - pictureMargin) {
                pictureX = qMax(pictureMargin, width() - pictureMargin - scaled.width());
            }
            pictureX = qMax(pictureMargin, pictureX);
            const int pictureY = qMax(pictureMargin, (height() - scaled.height()) / 2);
            m_picture->move(pictureX, pictureY);
            m_picture->show();
        } else {
            int targetW = kNativePictureW;
            int targetH = kNativePictureH;
            const int availableW = qMax(40, width() - 2 * pictureMargin);
            const int availableH = qMax(40, height() - contentTop - pictureMargin);
            if (targetW > availableW || targetH > availableH) {
                scaled = m_pictureSource.scaled(
                    availableW, availableH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            } else {
                scaled = m_pictureSource.scaled(
                    targetW, targetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            }
            m_picture->setPixmap(scaled);
            m_picture->setFixedSize(scaled.size());
            int pictureX = qRound(kPictureLeft * sx) - kPictureShiftLeft;
            if (pictureX + scaled.width() > width() - pictureMargin) {
                pictureX = pictureMargin
                    + qMax(0, (width() - 2 * pictureMargin - scaled.width()) / 2)
                    - kPictureShiftLeft;
            }
            pictureX = qMax(pictureMargin, pictureX);
            const int baseTop = showButtons ? contentTop : qRound(kPictureTop * sy);
            const int pictureY = baseTop + qRound(kPictureTopOffset * sy);
            m_picture->move(pictureX, pictureY);
            m_picture->show();
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

void OnlyPExercise::start(const QString &exerciseId) {
    m_exerciseId = exerciseId;
    setProperty("exerciseId", exerciseId);
    m_answers.clear();
    for (int i = 0; i < 5; ++i) {
        m_answers.append(false);
    }
    m_index = 0;
    m_elapsedSeconds = 0;

    if (m_displayRole != DisplayRole::Headless) {
        initAnswerButtons(exerciseId);
        updateWidgetLayout();
    }

    loadPicture(1);
    m_timer->start();
    if (m_displayRole != DisplayRole::Headless) {
        show();
        raise();
    }
}

void OnlyPExercise::loadPicture(int index) {
    const QString path = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("p%1.png").arg(index));
    if (path.isEmpty()) {
        return;
    }
    m_pictureSource.load(path);
    updateWidgetLayout();
    m_picture->raise();
    emit pictureChanged(index);
}

void OnlyPExercise::showPicture(int index) {
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
    hide();
    emit finished(m_answers, m_elapsedSeconds);
}
