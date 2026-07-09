#include "patientdisplay.h"

#include "onlypexercise.h"

#include <QGuiApplication>
#include <QScreen>

PatientDisplay::PatientDisplay(QWidget *parent) : QWidget(parent, Qt::FramelessWindowHint | Qt::Window) {
    setAttribute(Qt::WA_DeleteOnClose, false);
    setStyleSheet(QStringLiteral("background-color: #ffffff;"));
    m_mirrorExercise = new OnlyPExercise(this);
    m_mirrorExercise->setMirrorMode(true);
    m_mirrorExercise->setGeometry(0, 0, 1920, 1080);
}

void PatientDisplay::attachExercise(OnlyPExercise *exercise) {
    m_exercise = exercise;
    if (!m_exercise || !m_mirrorExercise) {
        return;
    }

    connect(m_exercise, &OnlyPExercise::pictureChanged, m_mirrorExercise, &OnlyPExercise::showPicture, Qt::UniqueConnection);
    connect(m_exercise, &OnlyPExercise::finished, this, &PatientDisplay::hideDisplay, Qt::UniqueConnection);
    connect(m_mirrorExercise, &OnlyPExercise::mirrorAnswerRequested, m_exercise, &OnlyPExercise::submitAnswer, Qt::UniqueConnection);
    connect(m_mirrorExercise, &OnlyPExercise::mirrorStopRequested, m_exercise, &OnlyPExercise::stopExercise, Qt::UniqueConnection);
}

void PatientDisplay::showOnSecondaryScreen() {
    const QList<QScreen *> screens = QGuiApplication::screens();
    QScreen *target = screens.size() > 1 ? screens.at(1) : nullptr;
    if (!target) {
        return;
    }
    setGeometry(target->geometry());
    m_mirrorExercise->setGeometry(rect());
    if (m_exercise) {
        const QString exerciseId = m_exercise->property("exerciseId").toString();
        if (!exerciseId.isEmpty()) {
            m_mirrorExercise->prepareMirrorUi(exerciseId);
            m_mirrorExercise->showPicture(1);
        }
    }
    showFullScreen();
    raise();
}

void PatientDisplay::hideDisplay() {
    if (m_mirrorExercise) {
        m_mirrorExercise->hide();
    }
    hide();
}
