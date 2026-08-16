#ifndef EXERCISEHOST_H
#define EXERCISEHOST_H

#include "exerciseconfig.h"
#include "exerciseprotocol.h"
#include "exercisesession.h"

#include <QList>
#include <QMap>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

class ImageButton;
class QLabel;
class QLineEdit;
class QTextBrowser;
class QTextEdit;
class QScrollArea;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QPushButton;
class QRadioButton;
class QTableWidget;
class QVBoxLayout;
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
    // Остановить сессию и убрать оверлеи (в т.ч. с корня главного окна).
    void shutdownSessionUi();

    QString exerciseId() const { return m_exerciseId; }
    QString currentProtocolId() const { return m_currentProtocolId; }
    bool protocolPartlyFormed() const { return m_partly; }
    QString selectedStepId() const { return currentStepId(); }
    // Путь к картинке для печати стимульного материала (по текущему № задания).
    QString stimulusPrintImagePath() const;
    static bool supportsScanUpload(const QString &exerciseId);
    static bool supportsStimulusPrint(const QString &exerciseId);
    // Обновить HTML протокола (ссылки «Показать изображение» после загрузки скана).
    void refreshProtocolViewAfterScanUpload();

signals:
    void closed();
    void protocolSaved();
    void exerciseOverlayChanged(bool visible);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void loadExercise();
    void reloadOrBrowser();
    void toggleOrSection(const QString &sectionId);
    void layoutContent();
    void updateContentHeights();
    void updateChromeLayout();
    void layoutStepCombo();
    void updatePreviewLayout();
    void reloadPreviewForCurrentStep();
    void syncActivityChecksFromOrHtml();
    void clearActivityChecks();
    void syncHelpChecksFromOrHtml();
    void clearHelpChecks();
    void ensureWords422Panel();
    void layoutWords422Panel();
    void updateWords422Panel(const QString &additional);
    void syncWords422AdditionalFromPanel();
    void ensureWords511Panel();
    void layoutWords511Panel();
    void setWords511TableEditable(bool editable);
    void ensureFindMark21Panel();
    void updateFindMark21PanelVisibility();
    void updateFindMark21TimesForStep();
    bool buildFindMark21Graph(bool showGraph);
    QString applyFindMark21ScoresToProtocolBody(QString body) const;
    void ensureFindMark22Panel();
    void updateFindMark22PanelVisibility();
    bool calculateFindMark22Score();
    QString applyFindMark22ScoresToProtocolBody(QString body) const;
    void ensureDigitsPreviewRunner();
    void connectSessionRunnerFinished();
    void handleSessionRunnerFinished(const ExerciseSessionResult &result);
    void runExerciseSession();
    void runOnlyPExercise();
    void showResultLabels(const QList<bool> &answers, int elapsedSeconds);
    void formProtocol();
    void sumProtocolScores();
    void sumProtocol126();
    void sumProtocol1272();
    void sumProtocol3110();
    void sumProtocol418();
    void sumProtocol318();
    void sumProtocolOrHlpBalls(const QString &idPrefix, const QString &maxSuffix);
    void syncProtocol317BallsToResult();
    void updateSumButtonVisibility();
    bool usesLastProtocolSessionView() const;
    bool forceNewProtocolSessionOnBegin() const;
    void resetProtocolToInitialTemplate();
    void showLastProtocolInTemplate();
    void updateProtocolEditMode();
    bool isCursorInProtocolBallsColumn() const;
    void onProtocolCursorMoved();
    void setExerciseChromeVisible(bool visible);
    void showExerciseOverlay();
    void restoreExerciseOverlay();
    void resetExerciseOverlays();
    void destroySessionRunner();
    void clearRootExerciseOverlays();
    void updateExerciseOverlayGeometry();
    void presentOverlayWidget(QWidget *overlayWidget);
    void reparentOverlayWidget(QWidget *overlayWidget);
    void syncPatientDisplay();
    void updateExerciseOptionsPanel();
    void layoutE15ModePopup();
    void applyPuzzleOptionsDefaults();
    void refreshRotateCombos();
    void pushLivePuzzleOptionsToRunner();
    ExerciseSessionOptions buildSessionOptions() const;
    int puzzleFragmentCount() const;
    ExerciseProtocol::CheckboxValues checkboxValues() const;
    QString orHtmlSnapshot() const;
    ProtocolSessionInput buildProtocolSession() const;
    QString currentStepId() const;
    QString selectedDoneState() const;
    bool needsDoneStatePanel() const;
    QStringList numberedStepIds() const;

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
    bool m_suppressProtocolAutosave = false;
    bool m_partly = false;
    bool m_forceNewProtocolSession = false;
    bool m_cursorInBallsColumn = false;
    QString m_currentProtocolId;
    bool m_exerciseRunning = false;
    bool m_orOpen1 = false;
    bool m_orOpen2 = false;
    bool m_orOpen3 = false;
    QList<bool> m_answers;
    int m_elapsedSeconds = 0;
    QMap<QString, int> m_stepElapsedSeconds;
    QString m_sessionAdditional;
    // 1.26: ответы по заданиям 1/2 в рамках открытия методики (чтобы не потерять задание 1).
    QMap<QString, QString> m_additionalByStep;
    QString m_sessionStepId;
    int m_picturesShown = 0;
    QString m_capturedImagePath;
    bool m_shardPanelVisible = false;

    QWidget *m_leftBackdrop = nullptr;
    QWidget *m_rightPanel = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_scrollContent = nullptr;
    QTextBrowser *m_orBrowser = nullptr;
    QWidget *m_evaluationPanel = nullptr;
    QWidget *m_activityChecksHost = nullptr;
    QVBoxLayout *m_activityChecksLayout = nullptr;
    QLabel *m_activityTitle = nullptr;
    QWidget *m_checkboxPanel = nullptr;
    QVBoxLayout *m_helpChecksLayout = nullptr;
    QLabel *m_helpPenaltyHintLabel = nullptr;
    QLabel *m_stimHelpLabel = nullptr;
    QLabel *m_directHelpLabel = nullptr;
    QLabel *m_teachHelpLabel = nullptr;
    QWidget *m_templatePanel = nullptr;
    QTextEdit *m_templateBrowser = nullptr;
    QList<ExerciseCheckRow> m_activityChecks;
    QList<ExerciseCheckRow> m_helpChecks;
    QList<ExerciseCheckRow> m_doneChecks;
    QWidget *m_donePanel = nullptr;
    QLabel *m_previewImage = nullptr;
    QWidget *m_previewGenderPanel = nullptr;
    QRadioButton *m_previewGirlRadio = nullptr;
    QRadioButton *m_previewBoyRadio = nullptr;
    QString m_previewGenderPrefix = QStringLiteral("d");
    QPixmap m_previewSource;
    QWidget *m_words422Panel = nullptr;
    QLabel *m_words422Label = nullptr;
    QTableWidget *m_words422Table = nullptr;
    QLabel *m_words422Graph = nullptr;
    QPixmap m_words422GraphBase;
    bool m_words422Editable = false;
    QWidget *m_words511Panel = nullptr;
    QTableWidget *m_words511Table = nullptr;
    QWidget *m_findMark21Panel = nullptr;
    QLabel *m_findMark21TimeLabels[6] = {};
    QLineEdit *m_findMark21NEdits[6] = {};
    QLineEdit *m_findMark21ErrEdits[6] = {};
    QLabel *m_findMark21SLabels[6] = {};
    QLabel *m_findMark21BallsLabel = nullptr;
    ImageButton *m_findMark21BuildButton = nullptr;
    QLabel *m_findMark21Graph = nullptr;
    QPixmap m_findMark21GraphBase;
    int m_findMark21Balls = -1;
    QString m_findMark21Conclusion;
    QWidget *m_findMark22Panel = nullptr;
    QLineEdit *m_findMark22NEdit = nullptr;
    QLineEdit *m_findMark22ErrEdit = nullptr;
    QLabel *m_findMark22SLabel = nullptr;
    QPushButton *m_findMark22CalcButton = nullptr;
    int m_findMark22Balls = -1;
    QString m_findMark22Conclusion;
    QLabel *m_rightCountLabel = nullptr;
    QLabel *m_wrongCountLabel = nullptr;
    QLabel *m_timeResultLabel = nullptr;
    ImageButton *m_beginButton = nullptr;
    ImageButton *m_formProtocolButton = nullptr;
    ImageButton *m_sumButton = nullptr;
    QComboBox *m_stepCombo = nullptr;
    QWidget *m_exerciseOptionsPanel = nullptr;
    QCheckBox *m_showHintCheck = nullptr;
    QCheckBox *m_showTemplateCheck = nullptr;
    QCheckBox *m_rotateEnableCheck = nullptr;
    QComboBox *m_rotateWCombo = nullptr;
    QComboBox *m_rotateCWCombo = nullptr;
    QLabel *m_rotateWLabel = nullptr;
    QLabel *m_rotateCWLabel = nullptr;
    QGroupBox *m_e15ModeGroup = nullptr;
    QGroupBox *m_puzzleOptionsGroup = nullptr;
    QRadioButton *m_e15HighlightRadio = nullptr;
    QRadioButton *m_e15SelectRadio = nullptr;
    QPushButton *m_shardButton = nullptr;
    QPushButton *m_shard316Button = nullptr;
    QPushButton *m_shard15Button = nullptr;
    QGroupBox *m_aparamGroup = nullptr;
    QRadioButton *m_aparamAllRadio = nullptr;
    QRadioButton *m_aparamCurrentRadio = nullptr;
    QWidget *m_remPanel = nullptr;
    QCheckBox *m_remChecks[9] = {};
    int m_attemptCount = 0;
    QTimer *m_protocolSaveTimer = nullptr;
    OnlyPExercise *m_onlyP = nullptr;
    ExerciseRunnerWidget *m_sessionRunner = nullptr;
    ExerciseRunnerKind m_sessionRunnerKind = ExerciseRunnerKind::NotImplemented;
    OnlyPExercise *m_specialistExercise = nullptr;
    PatientDisplay *m_patientDisplay = nullptr;
};

#endif
