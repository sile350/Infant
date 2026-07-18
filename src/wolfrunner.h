#ifndef WOLFRUNNER_H
#define WOLFRUNNER_H

#include "exerciserunnerwidget.h"

class QComboBox;
class QKeyEvent;
class QLabel;
class QPaintEvent;
class QTableWidget;
class QTimer;

class WolfRunner final : public ExerciseRunnerWidget {
    Q_OBJECT
public:
    explicit WolfRunner(QWidget *parent = nullptr);

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override;
    void stopSession() override;
    void resizeEvent(QResizeEvent *event) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void finishSession();
    void toggleTemplate1();
    void toggleTemplate2();
    void appendHelpText();
    void layoutUi();

    QLabel *m_stop = nullptr;
    QLabel *m_templateBtn1 = nullptr;
    QLabel *m_templateBtn2 = nullptr;
    QLabel *m_taleImage = nullptr;
    QLabel *m_templateImage = nullptr;
    QLabel *m_helpToLabel = nullptr;
    QLabel *m_helpTypeLabel = nullptr;
    QTableWidget *m_table = nullptr;
    QComboBox *m_episodeCombo = nullptr;
    QComboBox *m_helpCombo = nullptr;
    QTimer *m_timer = nullptr;
    int m_elapsed = 0;
    bool m_template1Visible = false;
    bool m_template2Visible = false;
};

#endif
