#ifndef EXERCISEPROTOCOL_H
#define EXERCISEPROTOCOL_H

#include <QMap>
#include <QList>
#include <QString>
#include <QStringList>

class QTextDocument;

struct ProtocolSessionInput {
    QString additional;
    QString doneState;
    QString stepId;
    // Для NumberedDoneTime: все № заданий и время каждого (одна форма → N строк).
    QStringList stepIds;
    QMap<QString, int> stepElapsedSeconds;
    // 5.2.1: данные таблицы по каждому № задания (step → "N;d1;…;d10").
    QMap<QString, QString> additionalByStep;
    int picturesShown = 0;
    QString capturedImagePath;
    QString orHtml;
};

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
        const CheckboxValues &checkboxes,
        const ProtocolSessionInput &session = ProtocolSessionInput());

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
    // 1.26: последняя сессия по «Дата/специалист» без обрезки вложенных таблиц баллов.
    static QString extractLastProtocol126Session(const QString &protocolBody);
    static QString formatProtocol12BodyForHeaderView(const QString &protocolBody);
    static QString buildProtocol12ProtocolsTabRecord(
        const QString &headerFragment,
        const QString &storedBody);
    static QString canonicalizeProtocol12StoredBody(const QString &protocolBody);
    // 1.26: плоская пересборка сессий (summary </table><!--s--><br> + таблицы заданий).
    static QString canonicalizeProtocol126StoredBody(const QString &protocolBody);
    // 4.1.8: summary </table><!--s--> + характер + таблица стимульных слов (без вложения в Примечание).
    static QString canonicalizeProtocol418StoredBody(const QString &protocolBody);
    static QString buildProtocol126ViewRecord(
        const QString &headerFragment,
        const QString &storedBody);
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

    // Только «Результат»/«Примечание» последней сессии — без пересборки таблиц (для 1.1 и др.).
    static QString mergeLimitedEditableFieldsIntoStoredBody(
        const QString &storedBody,
        QTextDocument *editorDocument);

    // 1.26: перенос баллов из редактора → HTML; при computeSums — sum1/sum2/sum3/idvivod (bsum).
    static QString applyProtocol126SumFromDocument(
        const QString &storedBody,
        QTextDocument *editorDocument,
        bool computeSums = true);

    // 3.1.10: сумма idb* → idsum и idvivod = sum(20); 1.272: ids* → (24).
    static QString applyProtocolIdbSum(
        const QString &storedBody,
        const QString &maxSuffix = QStringLiteral("(20)"),
        const QString &idPrefix = QStringLiteral("idb"));

    // 4.1.8: сумма b* → idsum и idvivod = sum(10).
    static QString applyProtocolBPrefixSum(
        const QString &storedBody,
        const QString &maxSuffix = QStringLiteral("(10)"));

    // 3.1.10: перенос «Выбранная картинка» / «Объяснение» / баллов из редактора.
    static QString mergeProtocol3110EditorIntoStoredBody(
        const QString &storedBody,
        QTextDocument *editorDocument);

    // 3.1.10: баллы по «Выбранная картинка» (эталон серии) −0.5 за каждый вид помощи в строке;
    // затем сумма idb → idsum / idvivod(20).
    static QString applyProtocol3110SumFromDocument(
        const QString &storedBody,
        QTextDocument *editorDocument);

    // or_hlp_balls (3.1.18 и др.): OR/HLP/Баллы из редактора без пересборки таблиц.
    static QString mergeOrHlpBallsEditorIntoStoredBody(
        const QString &storedBody,
        QTextDocument *editorDocument);

    // 3.1.18 «Времена года»: idballs → Результат N(10)/уровень (последняя сессия).
    static QString applyProtocol318SumFromDocument(
        const QString &storedBody,
        QTextDocument *editorDocument);

    // 1.272: перенос баллов idsN / OR / HLP из редактора.
    static QString mergeProtocol1272EditorIntoStoredBody(
        const QString &storedBody,
        QTextDocument *editorDocument);

    // Безопасно дописывает строки <tr> к телу протокола (не внутрь последней ячейки).
    static QString appendRowsToStoredBody(const QString &existingBody, const QString &rowsHtml);

    // Дописывает полный повторный протокол, начиная со строки «Дата/специалист».
    static QString appendFullSessionToStoredBody(
        const QString &existingBody,
        const QString &sessionHtml);

    // Плоская пересборка сессий для безопасного отображения (без вложенных таблиц).
    static QString flattenStoredProtocolBody(const QString &protocolBody);

    static CheckboxValues readCheckboxValues(const QString &orHtml);
    static QString applyCheckboxValues(const QString &orHtml, const CheckboxValues &values);
};

#endif
