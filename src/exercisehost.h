#ifndef EXERCISEHOST_H
#define EXERCISEHOST_H

#include "exerciseprotocol.h"

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
class OnlyPExercise;
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
    void loadStaticPictureExercise();
    void reloadOrBrowser();
    void toggleOrSection(const QString &sectionId);
    void layoutContent();
    void updateContentHeights();
    void updateChromeLayout();
    void updatePreviewLayout();
    void runOnlyPExercise();
    void showResultLabels(const QList<bool> &answers, int elapsedSeconds);
    void formProtocol();
    void setExerciseChromeVisible(bool visible);
    void showExerciseOverlay();
    void restoreExerciseOverlay();
    void updateExerciseOverlayGeometry();
    void syncPatientDisplay();
    ExerciseProtocol::CheckboxValues checkboxValues() const;
    QString orHtmlSnapshot() const;

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
    QLabel *m_previewImage = nullptr;
    QPixmap m_previewSource;
    QLabel *m_rightCountLabel = nullptr;
    QLabel *m_wrongCountLabel = nullptr;
    ImageButton *m_beginButton = nullptr;
    ImageButton *m_formProtocolButton = nullptr;
    QTimer *m_protocolSaveTimer = nullptr;
    OnlyPExercise *m_onlyP = nullptr;
    OnlyPExercise *m_specialistExercise = nullptr;
    PatientDisplay *m_patientDisplay = nullptr;
};

#endif
