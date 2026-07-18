#include "patientdisplay.h"

#include "exerciseconfig.h"
#include "onlypexercise.h"

#include <QGuiApplication>
#include <QLabel>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QWindow>

namespace {

QScreen *secondaryScreen() {
    QScreen *primary = QGuiApplication::primaryScreen();
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen != primary) {
            return screen;
        }
    }
    return nullptr;
}

} // namespace

PatientDisplay::PatientDisplay(QWidget *parent) : QWidget(parent, Qt::FramelessWindowHint | Qt::Window) {
    setAttribute(Qt::WA_DeleteOnClose, false);
    setStyleSheet(QStringLiteral("background-color: #ffffff;"));
    m_mirrorExercise = new OnlyPExercise(this);
    m_mirrorExercise->setDisplayRole(OnlyPExercise::DisplayRole::Patient);
    m_mirrorExercise->setGeometry(0, 0, 1920, 1080);

    m_mirrorLabel = new QLabel(this);
    m_mirrorLabel->setAlignment(Qt::AlignCenter);
    m_mirrorLabel->setScaledContents(true);
    m_mirrorLabel->hide();

    m_mirrorTimer = new QTimer(this);
    m_mirrorTimer->setInterval(100);
    connect(m_mirrorTimer, &QTimer::timeout, this, &PatientDisplay::updateMirrorPixmap);
}

void PatientDisplay::attachExercise(OnlyPExercise *exercise) {
    m_exercise = exercise;
    m_mirrorSource = nullptr;
    if (m_mirrorTimer) {
        m_mirrorTimer->stop();
    }
    if (m_mirrorLabel) {
        m_mirrorLabel->hide();
    }
    if (!m_exercise || !m_mirrorExercise) {
        return;
    }

    connect(
        m_exercise,
        &OnlyPExercise::pictureChanged,
        m_mirrorExercise,
        &OnlyPExercise::showPicture,
        Qt::UniqueConnection);
    connect(
        m_exercise,
        &OnlyPExercise::browseStateChanged,
        m_mirrorExercise,
        &OnlyPExercise::applyBrowseIndex,
        Qt::UniqueConnection);
    connect(m_exercise, &OnlyPExercise::finished, this, &PatientDisplay::hideDisplay, Qt::UniqueConnection);
}

void PatientDisplay::attachMirrorWidget(QWidget *source) {
    m_mirrorSource = source;
    m_exercise = nullptr;
    if (m_mirrorExercise) {
        m_mirrorExercise->hide();
    }
    if (m_mirrorLabel) {
        m_mirrorLabel->show();
    }
    m_mirrorTimer->start();
}

void PatientDisplay::updateMirrorPixmap() {
    if (!m_mirrorSource || !m_mirrorLabel) {
        return;
    }
    const QPixmap pixmap = m_mirrorSource->grab();
    if (!pixmap.isNull()) {
        m_mirrorLabel->setPixmap(pixmap);
    }
}

void PatientDisplay::showOnSecondaryScreen() {
    QScreen *target = secondaryScreen();
    if (!target) {
        return;
    }

    if (!isVisible()) {
        show();
    }
    if (QWindow *window = windowHandle()) {
        window->setScreen(target);
    }

    const QRect geometry = target->geometry();
    setGeometry(geometry);
    if (m_mirrorExercise) {
        m_mirrorExercise->setGeometry(0, 0, geometry.width(), geometry.height());
    }
    if (m_mirrorLabel) {
        m_mirrorLabel->setGeometry(0, 0, geometry.width(), geometry.height());
    }
    if (m_exercise && m_mirrorExercise) {
        const QString exerciseId = m_exercise->property("exerciseId").toString();
        if (!exerciseId.isEmpty()) {
            m_mirrorExercise->setDisplayRole(OnlyPExercise::DisplayRole::Patient);
            m_mirrorExercise->prepareMirrorUi(exerciseId);
            OnlyPictureSettings settings;
            if (const ExerciseDefinition *definition = ExerciseConfig::find(exerciseId)) {
                settings = definition->onlyPicture;
            }
            // Текущий шаг с Headless-источника (не всегда «1»).
            const QString stepId = m_exercise->property("stepId").toString();
            m_mirrorExercise->syncMirrorSession(
                exerciseId, settings, stepId.isEmpty() ? QStringLiteral("1") : stepId);
            if (!settings.dualPicture && exerciseId != QStringLiteral("3.2.3")) {
                m_mirrorExercise->showPicture(1);
            }
            m_mirrorExercise->show();
        }
    }
    showFullScreen();
    raise();
}

void PatientDisplay::hideDisplay() {
    if (m_mirrorTimer) {
        m_mirrorTimer->stop();
    }
    if (m_mirrorExercise) {
        m_mirrorExercise->hide();
    }
    if (m_mirrorLabel) {
        m_mirrorLabel->hide();
    }
    m_mirrorSource = nullptr;
    hide();
}
