#ifndef EXERCISEPROTOCOL_H
#define EXERCISEPROTOCOL_H

#include <QMap>
#include <QList>
#include <QString>
#include <QStringList>

class QTextDocument;

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
    static QString normalizeStoredProtocolBody(const QString &protocolBody);
    static QString normalizeProtocol12Layout(const QString &protocolBody);
    static QString patientProtocolBody(const QString &protocolBody);
    static QString extractLastSessionStoredBody(const QString &protocolBody);
    static QString formatProtocol12BodyForHeaderView(const QString &protocolBody);
    static QString buildProtocol12ProtocolsTabRecord(
        const QString &headerFragment,
        const QString &storedBody);
    static QString canonicalizeProtocol12StoredBody(const QString &protocolBody);
    static QString restrictExercisePageEditing(const QString &protocolHtml);
    static QString stripMethodologyFillForDocExport(const QString &protocolHtml);
    static QString stripProtocolRecordHeader(const QString &recordHtml, const QString &headerFragment);
    static QString repairResultsTableBody(const QString &protocolBody, const QList<bool> &answers = {});
    static QString extractEditableProtocolBody(const QString &documentHtml);
    static QStringList extractProtocolBodiesByDateRows(const QString &documentHtml);
    static QMap<QString, QString> extractProtocolBodiesById(const QString &documentHtml);
    static QString mergeEditorHtmlIntoStoredBody(
        const QString &storedBody,
        const QString &editorHtml,
        int protocolIndex = 0);
    static QString mergeEditorDocumentIntoStoredBody(
        const QString &storedBody,
        QTextDocument *editorDocument,
        int protocolIndex = 0);

    static CheckboxValues readCheckboxValues(const QString &orHtml);
    static QString applyCheckboxValues(const QString &orHtml, const CheckboxValues &values);
};

#endif
