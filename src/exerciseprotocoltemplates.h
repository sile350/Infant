#ifndef EXERCISEPROTOCOLTEMPLATES_H
#define EXERCISEPROTOCOLTEMPLATES_H

#include "exerciseprotocol.h"

#include <QList>
#include <QString>

QString createExerciseProtocolFromTemplate(
    const QString &exerciseId,
    const QString &userFio,
    int elapsedSeconds,
    bool partly,
    const QString &existingProtocolHtml,
    const QList<bool> &answers,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session);

#endif
