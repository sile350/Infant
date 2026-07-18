#ifndef PATIENTDISPLAY_H
#define PATIENTDISPLAY_H

#include <QWidget>

class QLabel;
class QTimer;
class OnlyPExercise;

class PatientDisplay final : public QWidget {
    Q_OBJECT
public:
    explicit PatientDisplay(QWidget *parent = nullptr);

    void attachExercise(OnlyPExercise *exercise);
    void attachMirrorWidget(QWidget *source);
    void showOnSecondaryScreen();
    void hideDisplay();
    void switchStep(const QString &stepId);

private:
    void updateMirrorPixmap();
    void onSourcePictureChanged(int index);
    void onSourceBrowseStateChanged(int index);

    OnlyPExercise *m_exercise = nullptr;
    OnlyPExercise *m_mirrorExercise = nullptr;
    QWidget *m_mirrorSource = nullptr;
    QLabel *m_mirrorLabel = nullptr;
    QTimer *m_mirrorTimer = nullptr;
};

#endif
