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

private:
    void updateMirrorPixmap();

    OnlyPExercise *m_exercise = nullptr;
    OnlyPExercise *m_mirrorExercise = nullptr;
    QWidget *m_mirrorSource = nullptr;
    QLabel *m_mirrorLabel = nullptr;
    QTimer *m_mirrorTimer = nullptr;
};

#endif
