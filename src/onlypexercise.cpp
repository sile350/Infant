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
#include <functional>

namespace {

constexpr int kPictureLeft = 700;
constexpr int kPictureTop = 240;
constexpr int kStopLeft = 971;
constexpr int kStopTop = 72;
constexpr int kRightLeft = 1225;
constexpr int kWrongLeft = 1355;
constexpr int kAnswerTop = 72;

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

void OnlyPExercise::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    m_picture->move(kPictureLeft, kPictureTop);
    m_stopButton->move(kStopLeft, kStopTop);
    m_rightButton->move(kRightLeft, kAnswerTop);
    m_wrongButton->move(kWrongLeft, kAnswerTop);
}

void OnlyPExercise::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
    if (!stopPath.isEmpty()) {
        setWhiteBackedPixmap(m_stopButton, QPixmap(stopPath));
        m_stopButton->move(kStopLeft, kStopTop);
    }
    raise();
    m_stopButton->raise();
    m_rightButton->raise();
    m_wrongButton->raise();
    m_picture->raise();
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

    const QString rightPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("right.png"));
    const QString wrongPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("notright.png"));
    if (!rightPath.isEmpty()) {
        setWhiteBackedPixmap(m_rightButton, QPixmap(rightPath));
        m_rightButton->move(kRightLeft, kAnswerTop);
        m_rightButton->show();
    }
    if (!wrongPath.isEmpty()) {
        setWhiteBackedPixmap(m_wrongButton, QPixmap(wrongPath));
        m_wrongButton->move(kWrongLeft, kAnswerTop);
        m_wrongButton->show();
    }

    loadPicture(1);
    m_timer->start();
    show();
    raise();
}

void OnlyPExercise::loadPicture(int index) {
    const QString path = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("p%1.png").arg(index));
    if (path.isEmpty()) {
        return;
    }
    const QPixmap pixmap(path);
    setAutoSizePixmap(m_picture, pixmap);
    m_picture->move(kPictureLeft, kPictureTop);
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
    const QString rightPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("right.png"));
    const QString wrongPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("notright.png"));
    if (!rightPath.isEmpty()) {
        setWhiteBackedPixmap(m_rightButton, QPixmap(rightPath));
        m_rightButton->move(kRightLeft, kAnswerTop);
        m_rightButton->show();
    }
    if (!wrongPath.isEmpty()) {
        setWhiteBackedPixmap(m_wrongButton, QPixmap(wrongPath));
        m_wrongButton->move(kWrongLeft, kAnswerTop);
        m_wrongButton->show();
    }
    if (m_stopButton) {
        m_stopButton->hide();
    }
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
