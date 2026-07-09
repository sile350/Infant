#ifndef PATIENTDISPLAY_H
#define PATIENTDISPLAY_H

#include <QWidget>

class OnlyPExercise;

class PatientDisplay final : public QWidget {
    Q_OBJECT
public:
    explicit PatientDisplay(QWidget *parent = nullptr);

    void attachExercise(OnlyPExercise *exercise);
    void showOnSecondaryScreen();
    void hideDisplay();

private:
    OnlyPExercise *m_exercise = nullptr;
    OnlyPExercise *m_mirrorExercise = nullptr;
};

#endif
