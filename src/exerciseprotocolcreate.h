#ifndef EXERCISEPROTOCOLCREATE_H
#define EXERCISEPROTOCOLCREATE_H

#include "exerciseconfig.h"
#include "exerciseprotocol.h"

#include <QList>
#include <QString>

QString createExerciseProtocolBody(
    const ExerciseDefinition &definition,
    const QString &userFio,
    int elapsedSeconds,
    bool partly,
    const QString &existingProtocolHtml,
    const QList<bool> &answers,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session);

QString readDoneStateFromOrHtml(const QString &orHtml);

#endif
