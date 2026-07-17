#ifndef EXERCISEHOST_H
#define EXERCISEHOST_H

#include "exerciseconfig.h"
#include "exerciseprotocol.h"
#include "exercisesession.h"

#include <QList>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

class ImageButton;
class QLabel;
class QTextBrowser;
class QTextEdit;
class QScrollArea;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QPushButton;
class QRadioButton;
class OnlyPExercise;
class ExerciseRunnerWidget;
class PatientDisplay;
class Repository;

struct ExerciseCheckRow {
    QCheckBox *box = nullptr;
    QLabel *label = nullptr;
};

class ExerciseHost final : public QWidget {
    Q_OBJECT
public:
    explicit ExerciseHost(QWidget *parent = nullptr);

    void openExercise(
        const QString &exerciseId,
        const QString &patientId,
        const QString &specialistFio,
        const QString &patientFio,
        const QString &patientBirthDate,
        Repository *repository,
        bool dualScreen);

    void setDualScreenEnabled(bool enabled);
    void saveProtocolEdits();

signals:
    void closed();
    void protocolSaved();
    void exerciseOverlayChanged(bool visible);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void loadExercise();
    void reloadOrBrowser();
    void toggleOrSection(const QString &sectionId);
    void layoutContent();
    void updateContentHeights();
    void updateChromeLayout();
    void updatePreviewLayout();
    void runExerciseSession();
    void runOnlyPExercise();
    void showResultLabels(const QList<bool> &answers, int elapsedSeconds);
    void formProtocol();
    void resetProtocolToInitialTemplate();
    void updateProtocolEditMode();
    void setExerciseChromeVisible(bool visible);
    void showExerciseOverlay();
    void restoreExerciseOverlay();
    void updateExerciseOverlayGeometry();
    void syncPatientDisplay();
    void updateExerciseOptionsPanel();
    void refreshRotateCombos();
    ExerciseSessionOptions buildSessionOptions() const;
    int puzzleFragmentCount() const;
    ExerciseProtocol::CheckboxValues checkboxValues() const;
    QString orHtmlSnapshot() const;
    ProtocolSessionInput buildProtocolSession() const;
    QString currentStepId() const;
    QString selectedDoneState() const;
    bool needsDoneStatePanel() const;
    int nextNumberedProtocolIndex() const;

    QString m_exerciseId;
    QString m_patientId;
    QString m_specialistFio;
    QString m_patientFio;
    QString m_patientBirthDate;
    QString m_rawOrHtml;
    Repository *m_repository = nullptr;
    bool m_dualScreen = false;
    bool m_exerciseDone = false;
    bool m_protocolFormed = true;
    bool m_protocolSavedThisSession = false;
    bool m_partly = false;
    QString m_currentProtocolId;
    bool m_exerciseRunning = false;
    bool m_orOpen1 = false;
    bool m_orOpen2 = false;
    bool m_orOpen3 = false;
    QList<bool> m_answers;
    int m_elapsedSeconds = 0;
    QString m_sessionAdditional;
    int m_picturesShown = 0;
    QString m_capturedImagePath;
    bool m_shardPanelVisible = false;

    QWidget *m_leftBackdrop = nullptr;
    QWidget *m_rightPanel = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_scrollContent = nullptr;
    QTextBrowser *m_orBrowser = nullptr;
    QWidget *m_evaluationPanel = nullptr;
    QWidget *m_checkboxPanel = nullptr;
    QWidget *m_templatePanel = nullptr;
    QTextEdit *m_templateBrowser = nullptr;
    QList<ExerciseCheckRow> m_activityChecks;
    QList<ExerciseCheckRow> m_helpChecks;
    QList<ExerciseCheckRow> m_doneChecks;
    QWidget *m_donePanel = nullptr;
    QLabel *m_previewImage = nullptr;
    QPixmap m_previewSource;
    QLabel *m_rightCountLabel = nullptr;
    QLabel *m_wrongCountLabel = nullptr;
    QLabel *m_timeResultLabel = nullptr;
    ImageButton *m_beginButton = nullptr;
    ImageButton *m_formProtocolButton = nullptr;
    QComboBox *m_stepCombo = nullptr;
    QWidget *m_exerciseOptionsPanel = nullptr;
    QCheckBox *m_showHintCheck = nullptr;
    QCheckBox *m_showTemplateCheck = nullptr;
    QCheckBox *m_rotateEnableCheck = nullptr;
    QComboBox *m_rotateWCombo = nullptr;
    QComboBox *m_rotateCWCombo = nullptr;
    QGroupBox *m_e15ModeGroup = nullptr;
    QRadioButton *m_e15HighlightRadio = nullptr;
    QRadioButton *m_e15SelectRadio = nullptr;
    QPushButton *m_shardButton = nullptr;
    QTimer *m_protocolSaveTimer = nullptr;
    OnlyPExercise *m_onlyP = nullptr;
    ExerciseRunnerWidget *m_sessionRunner = nullptr;
    ExerciseRunnerKind m_sessionRunnerKind = ExerciseRunnerKind::NotImplemented;
    OnlyPExercise *m_specialistExercise = nullptr;
    PatientDisplay *m_patientDisplay = nullptr;
};

#endif
