#include "onlypexercise.h"

#include "exerciseassets.h"

#include <QMouseEvent>
#include <QLabel>
#include <QPixmap>
#include <QShowEvent>
#include <QTimer>
#include <functional>

namespace {

class ClickableLabel final : public QLabel {
public:
    using QLabel::QLabel;
    std::function<void()> onClick;
protected:
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

} // namespace

OnlyPExercise::OnlyPExercise(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("background-color: #f8f8f8;"));

    m_picture = new QLabel(this);
    m_picture->setGeometry(700, 240, 800, 600);
    m_picture->setAlignment(Qt::AlignCenter);

    m_stopButton = new ClickableLabel(this);
    m_stopButton->setGeometry(971, 72, 134, 29);
    m_stopButton->setScaledContents(true);

    m_rightButton = new ClickableLabel(this);
    m_rightButton->setGeometry(1225, 72, 134, 29);
    m_rightButton->setScaledContents(true);

    m_wrongButton = new ClickableLabel(this);
    m_wrongButton->setGeometry(1355, 72, 134, 29);
    m_wrongButton->setScaledContents(true);

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

void OnlyPExercise::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
    if (!stopPath.isEmpty()) {
        asClickable(m_stopButton)->setPixmap(QPixmap(stopPath));
    }
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
        asClickable(m_rightButton)->setPixmap(QPixmap(rightPath));
        m_rightButton->show();
    }
    if (!wrongPath.isEmpty()) {
        asClickable(m_wrongButton)->setPixmap(QPixmap(wrongPath));
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
    m_picture->setPixmap(QPixmap(path));
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
        asClickable(m_rightButton)->setPixmap(QPixmap(rightPath));
        m_rightButton->show();
    }
    if (!wrongPath.isEmpty()) {
        asClickable(m_wrongButton)->setPixmap(QPixmap(wrongPath));
        m_wrongButton->show();
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
