#include "patientdisplay.h"

#include "e126canvas.h"
#include "exerciseconfig.h"
#include "onlypexercise.h"

#include <QGuiApplication>
#include <QLabel>
#include <QPainter>
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
    m_mirrorExercise->setMirrorMode(true);
    m_mirrorExercise->setGeometry(0, 0, 1920, 1080);

    m_patientEmotions = new E126Canvas(this);
    m_patientEmotions->setDisplayRole(E126Canvas::DisplayRole::Patient);
    m_patientEmotions->setGeometry(0, 0, 1920, 1080);
    m_patientEmotions->hide();

    m_mirrorLabel = new QLabel(this);
    m_mirrorLabel->setAlignment(Qt::AlignCenter);
    m_mirrorLabel->setScaledContents(true);
    m_mirrorLabel->hide();

    m_mirrorTimer = new QTimer(this);
    m_mirrorTimer->setInterval(200);
    connect(m_mirrorTimer, &QTimer::timeout, this, &PatientDisplay::updateMirrorPixmap);
}

void PatientDisplay::attachExercise(OnlyPExercise *exercise) {
    m_exercise = exercise;
    m_emotionsSource = nullptr;
    m_mirrorSource = nullptr;
    if (m_mirrorTimer) {
        m_mirrorTimer->stop();
    }
    if (m_mirrorLabel) {
        m_mirrorLabel->hide();
    }
    if (m_patientEmotions) {
        m_patientEmotions->hide();
    }
    if (!m_exercise || !m_mirrorExercise) {
        return;
    }

    connect(
        m_exercise,
        &OnlyPExercise::pictureChanged,
        this,
        &PatientDisplay::onSourcePictureChanged,
        Qt::UniqueConnection);
    connect(
        m_exercise,
        &OnlyPExercise::browseStateChanged,
        this,
        &PatientDisplay::onSourceBrowseStateChanged,
        Qt::UniqueConnection);
    connect(m_exercise, &OnlyPExercise::finished, this, &PatientDisplay::hideDisplay, Qt::UniqueConnection);
}

void PatientDisplay::attachEmotionsCanvas(E126Canvas *source) {
    m_emotionsSource = source;
    m_exercise = nullptr;
    m_mirrorSource = nullptr;
    if (m_mirrorTimer) {
        m_mirrorTimer->stop();
    }
    if (m_mirrorLabel) {
        m_mirrorLabel->hide();
    }
    if (m_mirrorExercise) {
        m_mirrorExercise->hide();
    }
    if (!m_emotionsSource || !m_patientEmotions) {
        return;
    }
    connect(
        m_emotionsSource,
        &E126Canvas::patientContentChanged,
        this,
        &PatientDisplay::onEmotionsContentChanged,
        Qt::UniqueConnection);
    onEmotionsContentChanged();
    m_patientEmotions->show();
    m_patientEmotions->raise();
}

void PatientDisplay::onEmotionsContentChanged() {
    if (!m_emotionsSource || !m_patientEmotions) {
        return;
    }
    m_patientEmotions->syncPatientFrom(m_emotionsSource);
    m_patientEmotions->show();
    m_patientEmotions->raise();
}

void PatientDisplay::onSourcePictureChanged(int index) {
    if (!m_mirrorExercise || !m_exercise) {
        return;
    }
    // pictureChanged при смене задания идёт с новым stepId у источника —
    // зеркало должно сначала переключить шаг, затем показать нужный кадр.
    const QString stepId = m_exercise->property("stepId").toString();
    const QString mirrorStep = m_mirrorExercise->property("stepId").toString();
    if (!stepId.isEmpty() && stepId != mirrorStep) {
        m_mirrorExercise->switchStep(stepId);
    }
    m_mirrorExercise->showPicture(index);
}

void PatientDisplay::onSourceBrowseStateChanged(int index) {
    if (!m_mirrorExercise || !m_exercise) {
        return;
    }
    const QString stepId = m_exercise->property("stepId").toString();
    const QString mirrorStep = m_mirrorExercise->property("stepId").toString();
    if (!stepId.isEmpty() && stepId != mirrorStep) {
        m_mirrorExercise->switchStep(stepId);
    }
    m_mirrorExercise->applyBrowseIndex(index);
}

void PatientDisplay::switchStep(const QString &stepId) {
    if (m_patientEmotions && m_emotionsSource) {
        m_patientEmotions->switchStep(stepId);
        return;
    }
    if (!m_mirrorExercise || stepId.trimmed().isEmpty()) {
        return;
    }
    m_mirrorExercise->switchStep(stepId);
}

void PatientDisplay::attachMirrorWidget(QWidget *source) {
    m_mirrorSource = source;
    m_exercise = nullptr;
    m_emotionsSource = nullptr;
    if (m_mirrorExercise) {
        m_mirrorExercise->hide();
    }
    if (m_patientEmotions) {
        m_patientEmotions->hide();
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
    if (!m_mirrorSource->isVisible() || m_mirrorSource->size().isEmpty()) {
        return;
    }

    // Не трогаем UI специалиста (hide/setUpdatesEnabled давали зависания в 2.8 и др.).
    // Берём полный кадр и закрашиваем области управляющих элементов белым.
    QPixmap pixmap = m_mirrorSource->grab();
    if (pixmap.isNull()) {
        return;
    }

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    const QList<QWidget *> widgets =
        m_mirrorSource->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively);
    for (QWidget *widget : widgets) {
        if (!widget || !widget->property("dokitPatientControl").toBool()) {
            continue;
        }
        if (!widget->isVisible()) {
            continue;
        }
        const QPoint topLeft = widget->mapTo(m_mirrorSource, QPoint(0, 0));
        painter.fillRect(QRect(topLeft, widget->size()), Qt::white);
    }
    painter.end();

    m_mirrorLabel->setPixmap(pixmap);
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
    if (m_patientEmotions) {
        m_patientEmotions->setGeometry(0, 0, geometry.width(), geometry.height());
    }
    if (m_mirrorLabel) {
        m_mirrorLabel->setGeometry(0, 0, geometry.width(), geometry.height());
    }
    if (m_emotionsSource && m_patientEmotions) {
        onEmotionsContentChanged();
        m_patientEmotions->show();
        m_patientEmotions->raise();
    } else if (m_exercise && m_mirrorExercise) {
        const QString exerciseId = m_exercise->property("exerciseId").toString();
        if (!exerciseId.isEmpty()) {
            m_mirrorExercise->setDisplayRole(OnlyPExercise::DisplayRole::Patient);
            m_mirrorExercise->setMirrorMode(true);
            m_mirrorExercise->prepareMirrorUi(exerciseId);
            OnlyPictureSettings settings;
            if (const ExerciseDefinition *definition = ExerciseConfig::find(exerciseId)) {
                settings = definition->onlyPicture;
            }
            // Текущий шаг с Headless-источника (пустой — у autoAdvance вроде 1.4).
            const QString stepId = m_exercise->property("stepId").toString();
            m_mirrorExercise->syncMirrorSession(exerciseId, settings, stepId);
            if (!settings.dualPicture && exerciseId != QStringLiteral("3.2.3")) {
                const int picIndex = m_exercise->picturesShown() > 0
                    ? m_exercise->picturesShown()
                    : 1;
                m_mirrorExercise->showPicture(picIndex);
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
    if (m_patientEmotions) {
        m_patientEmotions->hide();
    }
    if (m_mirrorLabel) {
        m_mirrorLabel->hide();
    }
    m_mirrorSource = nullptr;
    m_emotionsSource = nullptr;
    hide();
}
