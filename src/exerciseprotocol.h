#ifndef EXERCISEPROTOCOL_H
#define EXERCISEPROTOCOL_H

#include <QMap>
#include <QList>
#include <QString>

class ExerciseProtocol {
public:
    struct CheckboxValues {
        QString activity;
        QString help;
    };

    static QString createProtocolHtml(
        const QString &exerciseId,
        const QString &userFio,
        int elapsedSeconds,
        bool partly,
        const QString &existingProtocolHtml,
        const QList<bool> &answers,
        const CheckboxValues &checkboxes);

    static QString protocolViewHtml(
        const QString &exerciseId,
        const QString &protocolBody,
        const QString &patientFio,
        const QString &patientBirthDate);

    static QString wrapEditableProtocolBody(const QString &protocolBody);
    static QString wrapProtocolRecord(const QString &protocolId, const QString &protocolBody);
    static QString extractEditableProtocolBody(const QString &documentHtml);
    static QMap<QString, QString> extractProtocolBodiesById(const QString &documentHtml);

    static CheckboxValues readCheckboxValues(const QString &orHtml);
    static QString applyCheckboxValues(const QString &orHtml, const CheckboxValues &values);
};

#endif
