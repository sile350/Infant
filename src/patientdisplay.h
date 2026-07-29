#ifndef PATIENTDISPLAY_H
#define PATIENTDISPLAY_H

#include <QWidget>

class QLabel;
class QResizeEvent;
class QTimer;
class OnlyPExercise;
class E126Canvas;

class PatientDisplay final : public QWidget {
    Q_OBJECT
public:
    explicit PatientDisplay(QWidget *parent = nullptr);

    void attachExercise(OnlyPExercise *exercise);
    void attachEmotionsCanvas(E126Canvas *source);
    void attachMirrorWidget(QWidget *source);
    void attachContentWidget(QWidget *widget);
    void showOnSecondaryScreen();
    void hideDisplay();
    void switchStep(const QString &stepId);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateMirrorPixmap();
    void onSourcePictureChanged(int index);
    void onSourceBrowseStateChanged(int index);
    void onEmotionsContentChanged();

    OnlyPExercise *m_exercise = nullptr;
    OnlyPExercise *m_mirrorExercise = nullptr;
    E126Canvas *m_emotionsSource = nullptr;
    E126Canvas *m_patientEmotions = nullptr;
    QWidget *m_mirrorSource = nullptr;
    QWidget *m_contentWidget = nullptr;
    QLabel *m_mirrorLabel = nullptr;
    QTimer *m_mirrorTimer = nullptr;
};

#endif
