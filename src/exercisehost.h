#ifndef EXERCISEHOST_H
#define EXERCISEHOST_H

#include "exerciseprotocol.h"

#include <QList>
#include <QWidget>

class QLabel;
class QPushButton;
class QTextBrowser;
class QScrollArea;
class QCheckBox;
class OnlyPExercise;
class PatientDisplay;
class Repository;

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

signals:
    void closed();
    void protocolSaved();

private:
    void loadStaticPictureExercise();
    void layoutContent();
    void updateContentHeights();
    void runOnlyPExercise();
    void showResultLabels(const QList<bool> &answers, int elapsedSeconds);
    void formProtocol();
    ExerciseProtocol::CheckboxValues checkboxValues() const;
    QString orHtmlSnapshot() const;

    QString m_exerciseId;
    QString m_patientId;
    QString m_specialistFio;
    QString m_patientFio;
    QString m_patientBirthDate;
    Repository *m_repository = nullptr;
    bool m_dualScreen = false;
    bool m_exerciseDone = false;
    bool m_protocolFormed = true;
    bool m_partly = false;
    QList<bool> m_answers;
    int m_elapsedSeconds = 0;

    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_scrollContent = nullptr;
    QTextBrowser *m_orBrowser = nullptr;
    QWidget *m_evaluationPanel = nullptr;
    QWidget *m_checkboxPanel = nullptr;
    QWidget *m_templatePanel = nullptr;
    QTextBrowser *m_templateBrowser = nullptr;
    QList<QCheckBox *> m_activityChecks;
    QList<QCheckBox *> m_helpChecks;
    QLabel *m_previewImage = nullptr;
    QLabel *m_rightCountLabel = nullptr;
    QLabel *m_wrongCountLabel = nullptr;
    QPushButton *m_beginButton = nullptr;
    QPushButton *m_formProtocolButton = nullptr;
    OnlyPExercise *m_onlyP = nullptr;
    PatientDisplay *m_patientDisplay = nullptr;
};

#endif
