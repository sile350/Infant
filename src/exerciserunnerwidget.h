#ifndef EXERCISERUNNERWIDGET_H
#define EXERCISERUNNERWIDGET_H

#include "exerciseconfig.h"
#include "exercisesession.h"

#include <QWidget>

class ExerciseRunnerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ExerciseRunnerWidget(QWidget *parent = nullptr);

    virtual void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) = 0;
    virtual void stopSession() = 0;
    virtual void switchStep(const QString &stepId) { Q_UNUSED(stepId); }

    void setSessionOptions(const ExerciseSessionOptions &options) { m_sessionOptions = options; }
    const ExerciseSessionOptions &sessionOptions() const { return m_sessionOptions; }

signals:
    void sessionFinished(const ExerciseSessionResult &result);

protected:
    void emitFinished(const ExerciseSessionResult &result);
    QString m_exerciseId;
    QString m_stepId;
    ExerciseSessionOptions m_sessionOptions;
};

ExerciseRunnerWidget *createExerciseRunner(ExerciseRunnerKind kind, QWidget *parent);

#endif
