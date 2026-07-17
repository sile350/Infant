#ifndef ONLYPEXERCISE_H
#define ONLYPEXERCISE_H

#include "exerciseconfig.h"

#include <QList>
#include <QMap>
#include <QPixmap>
#include <QWidget>

class QLabel;
class QTimer;

class OnlyPExercise final : public QWidget {
    Q_OBJECT
public:
    enum class DisplayRole {
        Primary,
        Specialist,
        Patient,
        Headless
    };

    explicit OnlyPExercise(QWidget *parent = nullptr);

    void start(const QString &exerciseId, const OnlyPictureSettings &settings = OnlyPictureSettings(),
               const QString &stepId = QString());
    // Смена задания без raise() оверлея (чтобы не перекрывать селект задания).
    void switchStep(const QString &stepId);
    void showPicture(int index);
    void submitAnswer(bool correct);
    void setMirrorMode(bool enabled);
    void setDisplayRole(DisplayRole role);
    void prepareMirrorUi(const QString &exerciseId);
    void stopExercise();
    QList<bool> answers() const { return m_answers; }
    int elapsedSeconds() const { return m_elapsedSeconds; }
    // Суммарное время по каждому № задания (после смен stepId / финиша).
    QMap<QString, int> stepElapsedSeconds() const { return m_stepElapsedSeconds; }

    int picturesShown() const { return m_picturesShown; }

signals:
    void finished(const QList<bool> &answers, int elapsedSeconds);
    void pictureChanged(int index);
    void answerRecorded(int index, bool correct);
    void mirrorAnswerRequested(bool correct);
    void mirrorStopRequested();

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void loadPicture(int index);
    QString imageFileName(int index) const;
    void recordAnswer(bool correct);
    void finishExercise();
    void commitCurrentStepTime();
    void initAnswerButtons(const QString &exerciseId);
    void updateWidgetLayout();
    void updateOvertimeTimer();
    int overtimeThresholdSeconds() const;

    QString m_exerciseId;
    QString m_stepId;
    OnlyPictureSettings m_settings;
    QList<bool> m_answers;
    int m_index = 0;
    int m_picturesShown = 0;
    int m_elapsedSeconds = 0;
    QMap<QString, int> m_stepElapsedSeconds;
    bool m_mirrorMode = false;
    DisplayRole m_displayRole = DisplayRole::Primary;
    QPixmap m_pictureSource;
    QPixmap m_stopSource;
    QPixmap m_rightSource;
    QPixmap m_wrongSource;

    QLabel *m_picture = nullptr;
    QLabel *m_picture2 = nullptr;
    QLabel *m_stopButton = nullptr;
    QLabel *m_rightButton = nullptr;
    QLabel *m_wrongButton = nullptr;
    QLabel *m_overtimeLabel = nullptr;
    QTimer *m_timer = nullptr;
    QTimer *m_advanceTimer = nullptr;
    QPixmap m_picture2Source;
};

#endif
