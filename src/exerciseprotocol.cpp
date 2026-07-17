#include "exerciseprotocolcreate.h"

#include "exerciseassets.h"
#include "exerciseconfig.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTextTable>
#include <utility>

namespace {

QString answerText(bool correct) {
    return correct ? QStringLiteral("верно") : QStringLiteral("неверно");
}

QString readExerciseHeaderHtml(const QString &exerciseId) {
    const QString path = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("header.html"));
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QString readHeaderRows(const QString &exerciseId) {
    const QString path = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("header.html"));
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QString html = QString::fromUtf8(file.readAll());
    const int bodyStart = html.indexOf(QStringLiteral("<body"), 0, Qt::CaseInsensitive);
    if (bodyStart < 0) {
        return html;
    }
    const int contentStart = html.indexOf(QLatin1Char('>'), bodyStart);
    const int bodyEnd = html.indexOf(QStringLiteral("</body>"), contentStart, Qt::CaseInsensitive);
    if (contentStart < 0 || bodyEnd < 0) {
        return html;
    }
    html = html.mid(contentStart + 1, bodyEnd - contentStart - 1).trimmed();
    const int tableEnd = html.lastIndexOf(QStringLiteral("</table>"), -1, Qt::CaseInsensitive);
    if (tableEnd > 0) {
        html = html.left(tableEnd);
    }
    return html;
}

QString formatProtocolCellText(const QString &text) {
    if (text.trimmed().isEmpty()) {
        return {};
    }
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n;]+")), Qt::SkipEmptyParts);
    QStringList parts;
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            parts << QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;%1").arg(trimmed.toHtmlEscaped());
        }
    }
    return parts.join(QStringLiteral("<br>"));
}

QString protocolSummaryTableOpenHtml() {
    return QStringLiteral(
        "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
        "<colgroup><col width='165'><col width='506'></colgroup>");
}

QString stripLeadingSummaryTableWrapper(QString chunk) {
    QString trimmed = chunk.trimmed();
    const QRegularExpression wrapRe(
        QStringLiteral("^<table\\b[^>]*>\\s*<colgroup>[\\s\\S]*?</colgroup>\\s*"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    return trimmed.replace(wrapRe, QString());
}

QString summaryRowHtml(const QString &label, const QString &valueHtml) {
    return QStringLiteral("<tr><td width='165' valign='top'>%1</td><td width='506' valign='top'>%2</td></tr>")
        .arg(label, valueHtml);
}

QString resultsTableHeaderHtml() {
    return QStringLiteral(
        "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
        "<tr><td width='229' align='center'>Картинка(описание)</td>"
        "<td width='88' align='center'>Уровень выполнения</td>"
        "<td align='center' width='160'>Характер деятельности ребенка</td>"
        "<td align='center' width='194'>Виды помощи</td></tr>");
}

QString protocolBodyStartMarker() {
    return QStringLiteral(
        "<span id=\"dokit-protocol-body-start\" style=\"font-size:0pt;line-height:0;\">\uFEFF</span>");
}

QString protocolBodyEndMarker() {
    return QStringLiteral(
        "<span id=\"dokit-protocol-body-end\" style=\"font-size:0pt;line-height:0;\">\uFEFF</span>");
}

QString protocolRecordStartMarker(const QString &protocolId) {
    return QStringLiteral("<!--protocol-id:%1-->").arg(protocolId);
}

QString protocolRecordEndMarker(const QString &protocolId) {
    return QStringLiteral("<!--/protocol-id:%1-->").arg(protocolId);
}

QString protocolRecordStartSpan(const QString &protocolId) {
    return QStringLiteral(
               "<span id=\"dokit-pid-%1-start\" style=\"font-size:0pt;line-height:0;\">\uFEFF</span>")
        .arg(protocolId);
}

QString protocolRecordEndSpan(const QString &protocolId) {
    return QStringLiteral(
               "<span id=\"dokit-pid-%1-end\" style=\"font-size:0pt;line-height:0;\">\uFEFF</span>")
        .arg(protocolId);
}

QString extractBetweenMarkers(
    const QString &documentHtml,
    const QString &startPattern,
    const QString &endPattern) {
    const QRegularExpression re(
        startPattern + QStringLiteral("([\\s\\S]*?)") + endPattern,
        QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = re.match(documentHtml);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

int findRowStartBefore(const QString &html, int datePos) {
    if (datePos <= 0) {
        return -1;
    }
    const QString before = html.left(datePos);
    QRegularExpression rowRe(QStringLiteral("<tr\\b"), QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = rowRe.globalMatch(before);
    int last = -1;
    while (it.hasNext()) {
        last = it.next().capturedStart();
    }
    return last;
}

QList<int> findDateSpecialistPositions(const QString &html) {
    QList<int> positions;
    QRegularExpression dateRe(
        QStringLiteral("Дата\\s*(?:/|／)?\\s*специалист"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpressionMatchIterator it = dateRe.globalMatch(html);
    while (it.hasNext()) {
        positions.append(it.next().capturedStart());
    }
    return positions;
}

QString trimProtocolBodyTail(QString body) {
    const int pageBreak = body.indexOf(QStringLiteral("protocol-page-break"), 0, Qt::CaseInsensitive);
    if (pageBreak > 0) {
        body = body.left(pageBreak);
    }
    body = body.trimmed();
    static const QStringList trailers = {
        QStringLiteral("</body></html>"),
        QStringLiteral("</body>"),
        QStringLiteral("</html>"),
    };
    bool trimmed = true;
    while (trimmed) {
        trimmed = false;
        for (const QString &trailer : trailers) {
            if (body.endsWith(trailer, Qt::CaseInsensitive)) {
                body.chop(trailer.size());
                body = body.trimmed();
                trimmed = true;
                break;
            }
        }
    }
    return body;
}

QStringList pictureDescriptions();
int findProtocolChunkEnd(const QString &html, int rowStart, int nextDatePos);
QString normalizeStoredProtocolBody(QString body);

int findProtocol12ResultsEnd(const QString &html, int markerPos) {
    const int tableStart = html.indexOf(QStringLiteral("<table"), markerPos, Qt::CaseInsensitive);
    if (tableStart < 0) {
        return -1;
    }
    const QString lastDesc = pictureDescriptions().at(4);
    const QRegularExpression lastPicRe(
        QStringLiteral("<tr\\b[^>]*>[\\s\\S]*?%1[\\s\\S]*?</tr>")
            .arg(QRegularExpression::escape(lastDesc)),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = lastPicRe.match(html, tableStart);
    if (!match.hasMatch()) {
        return -1;
    }
    const int closeTable = html.indexOf(
        QStringLiteral("</table>"), match.capturedEnd(), Qt::CaseInsensitive);
    if (closeTable < 0) {
        return -1;
    }
    return closeTable + QStringLiteral("</table>").size();
}

bool looksLikeProtocol12Body(const QString &body) {
    return body.contains(QStringLiteral("<!--s-->"))
        || body.contains(QStringLiteral("Процесс выполнения диагностической методики"));
}

QStringList extractProtocol12Sessions(const QString &body) {
    const QList<int> datePositions = findDateSpecialistPositions(body);
    if (datePositions.isEmpty()) {
        return {};
    }

    QList<int> markers;
    int searchFrom = 0;
    while (true) {
        const int markerPos = body.indexOf(QStringLiteral("<!--s-->"), searchFrom);
        if (markerPos < 0) {
            break;
        }
        markers.append(markerPos);
        searchFrom = markerPos + QStringLiteral("<!--s-->").size();
    }

    QStringList sessions;
    for (int i = 0; i < datePositions.size(); ++i) {
        const int rowStart = findRowStartBefore(body, datePositions.at(i));
        if (rowStart < 0) {
            continue;
        }

        int markerPos = -1;
        for (int markerIndex = i; markerIndex < markers.size(); ++markerIndex) {
            if (markers.at(markerIndex) >= rowStart) {
                markerPos = markers.at(markerIndex);
                break;
            }
        }

        int endPos = -1;
        if (markerPos >= rowStart) {
            endPos = findProtocol12ResultsEnd(body, markerPos);
        }
        if (endPos < 0) {
            if (i + 1 < datePositions.size()) {
                const int nextRowStart = findRowStartBefore(body, datePositions.at(i + 1));
                if (nextRowStart > rowStart) {
                    endPos = nextRowStart;
                }
            } else {
                endPos = findProtocolChunkEnd(body, rowStart, -1);
            }
        }
        if (endPos <= rowStart) {
            continue;
        }

        const QString chunk = normalizeStoredProtocolBody(
            trimProtocolBodyTail(body.mid(rowStart, endPos - rowStart)));
        if (!chunk.isEmpty()) {
            sessions.append(chunk);
        }
    }
    return sessions;
}

int findProtocolChunkEnd(const QString &html, int rowStart, int nextDatePos) {
    if (nextDatePos > rowStart) {
        return nextDatePos;
    }
    static const QRegularExpression lastResultRowRe(
        QStringLiteral("5\\.\\s*Ослик без уха[\\s\\S]*?</tr>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch lastRowMatch = lastResultRowRe.match(html, rowStart);
    if (lastRowMatch.hasMatch()) {
        int endPos = lastRowMatch.capturedEnd();
        const int closeTable = html.indexOf(QStringLiteral("</table>"), endPos, Qt::CaseInsensitive);
        if (closeTable >= 0 && closeTable - endPos < 64) {
            endPos = closeTable + QStringLiteral("</table>").size();
        }
        return endPos;
    }
    const int pageBreak = html.indexOf(QStringLiteral("protocol-page-break"), rowStart, Qt::CaseInsensitive);
    if (pageBreak > rowStart) {
        return pageBreak;
    }
    return html.length();
}

QString normalizeStoredProtocolBody(QString body) {
    body.remove(QRegularExpression(
        QStringLiteral("<span[^>]*dokit-[^>]*>.*?</span>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
    return body.trimmed();
}

QString extractProtocolBodyFallback(const QString &documentHtml) {
    const int datePos = documentHtml.indexOf(QStringLiteral("Дата/специалист"));
    if (datePos < 0) {
        return {};
    }
    const int rowStart = documentHtml.lastIndexOf(QStringLiteral("<tr"), datePos);
    if (rowStart < 0) {
        return {};
    }
    const int endMarkerPos = documentHtml.indexOf(QStringLiteral("dokit-protocol-body-end"), datePos);
    if (endMarkerPos > rowStart) {
        const int endSpanStart = documentHtml.lastIndexOf(QStringLiteral("<span"), endMarkerPos);
        if (endSpanStart > rowStart) {
            return documentHtml.mid(rowStart, endSpanStart - rowStart).trimmed();
        }
    }
    const int endBodyPos = documentHtml.indexOf(QStringLiteral("<!--ebody-->"), datePos);
    if (endBodyPos > rowStart) {
        return documentHtml.mid(rowStart, endBodyPos - rowStart).trimmed();
    }
    return {};
}

QStringList pictureDescriptions() {
    return {
        QStringLiteral("1. Бабушка на диване без ножки."),
        QStringLiteral("2. Велосипедист без переднего колеса."),
        QStringLiteral("3. Девочка с расческой без зубчиков."),
        QStringLiteral("4. Пальто без рукава."),
        QStringLiteral("5. Ослик без уха."),
    };
}

QString extractAnswerFromRow(const QString &body, const QString &description) {
    const QRegularExpression answerRe(
        QStringLiteral("<td[^>]*>[\\s\\S]*?%1[\\s\\S]*?</td>\\s*<td[^>]*>\\s*(верно|неверно)")
            .arg(QRegularExpression::escape(description)),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = answerRe.match(body);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString htmlFragmentToPlainText(const QString &html);
QString extractAnswerFromSectionFallback(const QString &section, const QString &description);
bool extractActivityHelpFromSection(const QString &section, QString *activity, QString *help);

struct ParsedProtocolFields {
    bool hasDateSpecialist = false;
    QString dateSpecialist;
    bool hasResult = false;
    QString resultText;
    bool hasNote = false;
    QString noteText;
    QMap<int, QString> answersByIndex;
    bool hasActivityHelp = false;
    QString activity;
    QString help;
};

bool extractActivityHelpFromStoredBody(const QString &body, QString *activity, QString *help) {
    if (!activity && !help) {
        return false;
    }
    const QRegularExpression rowspanRe(
        QStringLiteral(
            "<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?1\\.\\s*Бабушка[\\s\\S]*?</td>\\s*<td[^>]*>[\\s\\S]*?</td>\\s*"
            "<td[^>]*rowspan=['\"]5['\"][^>]*>([\\s\\S]*?)</td>\\s*"
            "<td[^>]*rowspan=['\"]5['\"][^>]*>([\\s\\S]*?)</td>\\s*</tr>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = rowspanRe.match(body);
    if (!match.hasMatch()) {
        const QRegularExpression rowRe(
            QStringLiteral(
                "<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?1\\.\\s*Бабушка[\\s\\S]*?</td>\\s*<td[^>]*>[\\s\\S]*?</td>\\s*"
                "<td[^>]*>([\\s\\S]*?)</td>\\s*<td[^>]*>([\\s\\S]*?)</td>\\s*</tr>"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        match = rowRe.match(body);
        if (!match.hasMatch()) {
            return false;
        }
    }
    if (activity) {
        *activity = htmlFragmentToPlainText(match.captured(1));
    }
    if (help) {
        *help = htmlFragmentToPlainText(match.captured(2));
    }
    return true;
}

QString canonicalPictureRowHtml(
    int index,
    const QString &verno,
    const QString &activity = QString(),
    const QString &help = QString()) {
    const QStringList descriptions = pictureDescriptions();
    const QString desc = descriptions.at(index);
    if (index == 0) {
        return QStringLiteral("<tr><td >%1</td><td valign='top' >%2</td>"
                              "<td valign='top' rowspan='5'><div contenteditable='true'>%3</div></td>"
                              "<td valign='top' rowspan='5'><div contenteditable='true'>%4</div></td></tr>")
            .arg(desc, verno, formatProtocolCellText(activity), formatProtocolCellText(help));
    }
    if (index == 4) {
        return QStringLiteral("<tr><td>%1</td><td>%2</td></tr>").arg(desc, verno);
    }
    return QStringLiteral("<tr><td>%1</td><td valign='top'>%2</td></tr>").arg(desc, verno);
}

QString extractVernoForPictureRow(const QString &body, int index) {
    const QStringList descriptions = pictureDescriptions();
    if (index < 0 || index >= descriptions.size()) {
        return {};
    }
    QString verno = extractAnswerFromRow(body, descriptions.at(index));
    if (verno.isEmpty()) {
        verno = extractAnswerFromSectionFallback(body, descriptions.at(index));
    }
    return verno;
}

bool looksLikePictureAnswersResults(const QString &body) {
    // Только явные маркеры 1.2 — не «Картинка» (слишком широко, ложные срабатывания).
    return body.contains(QStringLiteral("Бабушка на диване"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("Ослик без уха"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("Велосипедист без переднего"), Qt::CaseInsensitive);
}

QString rebuildResultsTableSection(
    QString body,
    const QList<bool> &answers = {},
    const ParsedProtocolFields *parsedOverrides = nullptr) {
    const int markerPos = body.indexOf(QStringLiteral("<!--s-->"));
    if (markerPos < 0) {
        return body;
    }

    const QString prefix = body.left(markerPos);
    QString suffix = body.mid(markerPos + QStringLiteral("<!--s-->").size());

    const int tableStart = suffix.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
    if (tableStart < 0) {
        return body;
    }
    const int tableEnd = suffix.indexOf(QStringLiteral("</table>"), tableStart, Qt::CaseInsensitive);
    if (tableEnd < 0) {
        return body;
    }
    const QString afterResultsTable = suffix.mid(tableEnd + QStringLiteral("</table>").size());

    QString activity;
    QString help;
    if (parsedOverrides && parsedOverrides->hasActivityHelp) {
        activity = parsedOverrides->activity;
        help = parsedOverrides->help;
    } else {
        extractActivityHelpFromSection(suffix, &activity, &help);
    }

    QString newTable = resultsTableHeaderHtml();
    const QStringList descriptions = pictureDescriptions();
    for (int i = 0; i < descriptions.size(); ++i) {
        QString verno;
        if (parsedOverrides && parsedOverrides->answersByIndex.contains(i)) {
            const QString overrideVerno = parsedOverrides->answersByIndex.value(i);
            if (!overrideVerno.isEmpty()) {
                verno = overrideVerno;
            }
        }
        if (verno.isEmpty() && i < answers.size()) {
            verno = answerText(answers.at(i));
        }
        if (verno.isEmpty()) {
            verno = extractVernoForPictureRow(body, i);
        }
        if (verno.isEmpty()) {
            verno = QStringLiteral("неверно");
        }
        newTable += canonicalPictureRowHtml(
            i,
            verno,
            i == 0 ? activity : QString(),
            i == 0 ? help : QString());
    }
    newTable += QStringLiteral("</table>");

    return prefix + QStringLiteral("<!--s-->") + newTable + afterResultsTable;
}

QString replacePictureRow(
    QString body,
    int index,
    const QString &verno,
    const QString &activity = QString(),
    const QString &help = QString()) {
    const int markerPos = body.indexOf(QStringLiteral("<!--s-->"));
    if (markerPos < 0) {
        return body;
    }

    const QString escapedDesc = QRegularExpression::escape(pictureDescriptions().at(index));
    const QRegularExpression rowRe(
        QStringLiteral("<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?%1[\\s\\S]*?</td>[\\s\\S]*?</tr>").arg(escapedDesc),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);

    QString head = body.left(markerPos);
    QString tail = body.mid(markerPos);
    tail.replace(rowRe, canonicalPictureRowHtml(index, verno, activity, help));
    return head + tail;
}

QString repairResultsTableBody(QString body, const QList<bool> &answers) {
    if (!looksLikePictureAnswersResults(body)) {
        return body;
    }
    int markerPos = -1;
    int searchFrom = 0;
    while (true) {
        const int next = body.indexOf(QStringLiteral("<!--s-->"), searchFrom);
        if (next < 0) {
            break;
        }
        markerPos = next;
        searchFrom = next + QStringLiteral("<!--s-->").size();
    }
    if (markerPos < 0) {
        return body;
    }

    const QString prefix = body.left(markerPos);
    QString suffix = body.mid(markerPos + QStringLiteral("<!--s-->").size());

    const int tableStart = suffix.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
    if (tableStart < 0) {
        return body;
    }
    const int tableEnd = suffix.indexOf(QStringLiteral("</table>"), tableStart, Qt::CaseInsensitive);
    if (tableEnd < 0) {
        return body;
    }
    const QString afterResultsTable = suffix.mid(tableEnd + QStringLiteral("</table>").size());

    QString activity;
    QString help;
    extractActivityHelpFromSection(suffix, &activity, &help);

    QString newTable = resultsTableHeaderHtml();
    const QStringList descriptions = pictureDescriptions();
    for (int i = 0; i < descriptions.size(); ++i) {
        QString verno;
        if (i < answers.size()) {
            verno = answerText(answers.at(i));
        }
        if (verno.isEmpty()) {
            verno = extractVernoForPictureRow(body, i);
        }
        if (verno.isEmpty()) {
            verno = QStringLiteral("неверно");
        }
        newTable += canonicalPictureRowHtml(
            i,
            verno,
            i == 0 ? activity : QString(),
            i == 0 ? help : QString());
    }
    newTable += QStringLiteral("</table>");

    return prefix + QStringLiteral("<!--s-->") + newTable + afterResultsTable;
}

QString htmlFragmentToPlainText(const QString &html) {
    if (html.trimmed().isEmpty()) {
        return {};
    }
    QTextDocument document;
    document.setHtml(html);
    QString plain = document.toPlainText();
    plain.replace(QChar(0x2029), QLatin1Char('\n'));
    plain.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    return plain.trimmed();
}

QString readTableCellMultilineText(QTextTable *table, int row, int column) {
    if (!table || row < 0 || column < 0 || row >= table->rows() || column >= table->columns()) {
        return {};
    }
    const QTextTableCell cell = table->cellAt(row, column);
    if (!cell.isValid()) {
        return {};
    }
    QTextCursor cursor = cell.firstCursorPosition();
    cursor.setPosition(cell.lastCursorPosition().position(), QTextCursor::KeepAnchor);
    const QString html = QTextDocumentFragment(cursor).toHtml();
    if (html.contains(QStringLiteral("<br"), Qt::CaseInsensitive)
        || html.contains(QStringLiteral("</p>"), Qt::CaseInsensitive)) {
        const QString fromHtml = htmlFragmentToPlainText(html);
        if (!fromHtml.trimmed().isEmpty()) {
            return fromHtml;
        }
    }
    QString text = cursor.selectedText();
    text.replace(QChar(0x2029), QLatin1Char('\n'));
    text.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    return text.trimmed();
}

QString readTableCellText(QTextTable *table, int row, int column) {
    return readTableCellMultilineText(table, row, column).replace(QLatin1Char('\n'), QLatin1Char(' ')).trimmed();
}

void collectTables(QTextFrame *frame, QList<QTextTable *> &tables) {
    if (!frame) {
        return;
    }
    for (QTextFrame::iterator it = frame->begin(); !it.atEnd(); ++it) {
        QTextFrame *childFrame = it.currentFrame();
        if (!childFrame) {
            continue;
        }
        if (auto *table = qobject_cast<QTextTable *>(childFrame)) {
            tables.append(table);
            continue;
        }
        collectTables(childFrame, tables);
    }
}

QString normalizeVernoText(const QString &text) {
    const QString trimmed = text.trimmed();
    if (trimmed.contains(QStringLiteral("неверно"), Qt::CaseInsensitive)) {
        return QStringLiteral("неверно");
    }
    if (trimmed.contains(QStringLiteral("верно"), Qt::CaseInsensitive)) {
        return QStringLiteral("верно");
    }
    return trimmed;
}

bool cellMatchesPictureDescription(const QString &cellText, const QString &description) {
    const QString normalizedCell = cellText.trimmed();
    const QString normalizedDescription = description.trimmed();
    if (normalizedCell.isEmpty() || normalizedDescription.isEmpty()) {
        return false;
    }
    if (normalizedCell.compare(normalizedDescription, Qt::CaseInsensitive) == 0) {
        return true;
    }
    const QString descriptionTail = normalizedDescription.section(QLatin1Char('.'), 1).trimmed();
    return !descriptionTail.isEmpty() && normalizedCell.contains(descriptionTail, Qt::CaseInsensitive);
}

ParsedProtocolFields parseProtocolFieldsFromDocument(QTextDocument *document, int protocolIndex) {
    ParsedProtocolFields fields;
    if (!document) {
        return fields;
    }

    QList<QTextTable *> tables;
    collectTables(document->rootFrame(), tables);
    const QStringList descriptions = pictureDescriptions();

    int currentSection = -1;
    bool capture = false;

    for (QTextTable *table : tables) {
        const int rows = table->rows();
        const int columns = table->columns();
        for (int row = 0; row < rows; ++row) {
            if (columns < 2) {
                continue;
            }
            const QString firstCell = readTableCellText(table, row, 0);
            const QString secondCell = readTableCellText(table, row, 1);

            if (firstCell.contains(QStringLiteral("Дата"), Qt::CaseInsensitive)
                && firstCell.contains(QStringLiteral("специалист"), Qt::CaseInsensitive)) {
                ++currentSection;
                capture = (currentSection == protocolIndex);
                if (currentSection > protocolIndex) {
                    return fields;
                }
            }
            if (!capture) {
                continue;
            }

            if (firstCell.contains(QStringLiteral("Дата"), Qt::CaseInsensitive)
                && firstCell.contains(QStringLiteral("специалист"), Qt::CaseInsensitive)) {
                fields.hasDateSpecialist = true;
                fields.dateSpecialist = secondCell;
                continue;
            }
            if (firstCell.contains(QStringLiteral("Результат"), Qt::CaseInsensitive)) {
                fields.hasResult = true;
                fields.resultText = secondCell;
                continue;
            }
            if (firstCell.contains(QStringLiteral("Примечание"), Qt::CaseInsensitive)) {
                fields.hasNote = true;
                fields.noteText = secondCell;
                continue;
            }

            for (int i = 0; i < descriptions.size(); ++i) {
                if (!cellMatchesPictureDescription(firstCell, descriptions.at(i))) {
                    continue;
                }
                const QString verno = normalizeVernoText(secondCell);
                if (!verno.isEmpty()) {
                    fields.answersByIndex.insert(i, verno);
                }
                if (i == 0 && columns >= 4) {
                    fields.hasActivityHelp = true;
                    fields.activity = readTableCellMultilineText(table, row, 2);
                    fields.help = readTableCellMultilineText(table, row, 3);
                }
                break;
            }
        }
    }
    return fields;
}

ParsedProtocolFields parseProtocolFieldsFromHtml(const QString &html, int protocolIndex) {
    QTextDocument document;
    document.setHtml(html);
    return parseProtocolFieldsFromDocument(&document, protocolIndex);
}

QList<bool> answersListFromParsedFields(const ParsedProtocolFields &fields) {
    QList<bool> answers;
    for (int i = 0; i < 5; ++i) {
        if (!fields.answersByIndex.contains(i)) {
            return {};
        }
        answers.append(fields.answersByIndex.value(i).compare(QStringLiteral("верно"), Qt::CaseInsensitive) == 0);
    }
    return answers;
}

std::pair<bool, QString> extractSecondCellPlain(const QString &html, const QString &labelPattern) {
    const QRegularExpression rowRe(
        QStringLiteral("<tr[^>]*>\\s*<td[^>]*>[^<]*(?:%1)[^<]*</td>\\s*<td[^>]*>([\\s\\S]*?)</td>\\s*</tr>")
            .arg(labelPattern),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = rowRe.match(html);
    if (!match.hasMatch()) {
        return {false, QString()};
    }
    return {true, htmlFragmentToPlainText(match.captured(1))};
}

QString replaceRowSecondCell(QString body, const QString &rowLabel, const QString &plainText) {
    const QString escapedLabel = QRegularExpression::escape(rowLabel);
    const QRegularExpression rowRe(
        QStringLiteral("(<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)")
            .arg(escapedLabel),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QString inner = body.contains(QStringLiteral("contenteditable"), Qt::CaseInsensitive)
                              ? QStringLiteral("<div contenteditable='true'>%1</div>").arg(plainText.toHtmlEscaped())
                              : plainText.toHtmlEscaped();
    return body.replace(rowRe, QStringLiteral("\\1") + inner + QStringLiteral("\\3"));
}

QString replaceResultRowSecondCell(QString body, const QString &plainText) {
    // У разных методик подписи отличаются: «Результат: вывод…» / «Результат: баллы…» и т.п.
    const QRegularExpression rowRe(
        QStringLiteral(
            "(<tr[^>]*>\\s*<td[^>]*>\\s*Результат[^<]*</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QString inner = body.contains(QStringLiteral("contenteditable"), Qt::CaseInsensitive)
                              ? QStringLiteral("<div contenteditable='true'>%1</div>").arg(plainText.toHtmlEscaped())
                              : plainText.toHtmlEscaped();
    return body.replace(rowRe, QStringLiteral("\\1") + inner + QStringLiteral("\\3"));
}

QString replaceAnswerInBody(QString body, const QString &description, const QString &verno) {
    const QString escapedDesc = QRegularExpression::escape(description);
    const QRegularExpression rowRe(
        QStringLiteral("(<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?%1[\\s\\S]*?</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>)")
            .arg(escapedDesc),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    return body.replace(rowRe, QStringLiteral("\\1") + verno + QStringLiteral("\\3"));
}

QString extractAnswerFromSectionFallback(const QString &section, const QString &description) {
    const QRegularExpression answerRe(
        QStringLiteral("%1[\\s\\S]{0,800}?(верно|неверно)")
            .arg(QRegularExpression::escape(description)),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = answerRe.match(section);
    return match.hasMatch() ? match.captured(1) : QString();
}

bool extractActivityHelpFromSection(const QString &section, QString *activity, QString *help) {
    if (!activity && !help) {
        return false;
    }
    const QRegularExpression rowRe(
        QStringLiteral(
            "1\\.\\s*Бабушка[^<]*</td>\\s*<td[^>]*>\\s*(?:верно|неверно)\\s*</td>\\s*"
            "<td[^>]*>([\\s\\S]*?)</td>\\s*<td[^>]*>([\\s\\S]*?)</td>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = rowRe.match(section);
    if (!match.hasMatch()) {
        return false;
    }
    if (activity) {
        *activity = htmlFragmentToPlainText(match.captured(1));
    }
    if (help) {
        *help = htmlFragmentToPlainText(match.captured(2));
    }
    return true;
}

QString replaceActivityHelpCells(QString body, const QString &activity, const QString &help) {
    const QRegularExpression rowRe(
        QStringLiteral(
            "(<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?1\\.\\s*Бабушка[\\s\\S]*?</td>\\s*<td[^>]*>[\\s\\S]*?</td>\\s*<td[^>]*>)"
            "([\\s\\S]*?)(</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QString activityCell =
        QStringLiteral("<div contenteditable='true'>%1</div>").arg(formatProtocolCellText(activity));
    const QString helpCell =
        QStringLiteral("<div contenteditable='true'>%1</div>").arg(formatProtocolCellText(help));
    return body.replace(
        rowRe,
        QStringLiteral("\\1") + activityCell + QStringLiteral("\\3") + helpCell + QStringLiteral("\\5"));
}

QString applyParsedFieldsToSessionChunk(const QString &sessionChunk, const ParsedProtocolFields &parsed) {
    QString result = sessionChunk;

    if (parsed.hasDateSpecialist) {
        result = replaceRowSecondCell(result, QStringLiteral("Дата/специалист"), parsed.dateSpecialist);
    }
    if (parsed.hasResult) {
        result = replaceResultRowSecondCell(result, parsed.resultText);
    }
    if (parsed.hasNote) {
        result = replaceRowSecondCell(result, QStringLiteral("Примечание"), parsed.noteText);
    }

    // Пересборка таблицы процесса в формате 1.2 допустима только если это уже протокол 1.2.
    // Иначе при saveProtocolEdits у 1.1/других методик «Процесс выполнения» затирается
    // строками «Бабушка на диване… / верно|неверно».
    if (result.contains(QStringLiteral("<!--s-->")) && looksLikePictureAnswersResults(result)) {
        result = rebuildResultsTableSection(result, QList<bool>(), &parsed);
    }

    return result;
}

QString rebuildProtocol12SessionList(const QStringList &sessions) {
    if (sessions.isEmpty()) {
        return {};
    }

    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = stripLeadingSummaryTableWrapper(sessions.at(i));
        const int marker = session.indexOf(QStringLiteral("<!--s-->"));
        QString summaryPart = marker >= 0 ? session.left(marker) : session;
        const QString resultsPart = marker >= 0 ? session.mid(marker) : QString();

        summaryPart.replace(
            QRegularExpression(QStringLiteral("</table>\\s*$"), QRegularExpression::CaseInsensitiveOption),
            QString());
        summaryPart = summaryPart.trimmed();

        if (i > 0) {
            result += protocolSummaryTableOpenHtml();
        }
        if (!summaryPart.isEmpty()) {
            result += summaryPart;
        }
        result += QStringLiteral("</table>");
        result += resultsPart;
    }
    return result;
}

// Гарантирует: summary закрыт перед <!--s-->, таблица процесса закрыта, без вложенных <table>.
QString ensureClosedProtocolSession(QString session) {
    session = stripLeadingSummaryTableWrapper(session.trimmed());
    if (session.isEmpty()) {
        return session;
    }

    const int marker = session.indexOf(QStringLiteral("<!--s-->"));
    if (marker < 0) {
        if (!session.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
            session += QStringLiteral("</table>");
        }
        return session;
    }

    QString summary = session.left(marker);
    summary.replace(
        QRegularExpression(QStringLiteral("</table>\\s*$"), QRegularExpression::CaseInsensitiveOption),
        QString());
    summary = summary.trimmed();

    QString results = session.mid(marker + QStringLiteral("<!--s-->").size()).trimmed();
    const int tableStart = results.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
    if (tableStart < 0) {
        return summary + QStringLiteral("</table><!--s-->");
    }
    results = results.mid(tableStart);

    const int nestedOpen = results.indexOf(QStringLiteral("<table"), 6, Qt::CaseInsensitive);
    const int firstClose = results.indexOf(QStringLiteral("</table>"), 0, Qt::CaseInsensitive);

    QString processTable;
    if (nestedOpen >= 0 && (firstClose < 0 || nestedOpen < firstClose)) {
        int cut = results.lastIndexOf(QStringLiteral("</tr>"), nestedOpen, Qt::CaseInsensitive);
        if (cut > 0) {
            processTable = results.left(cut + QStringLiteral("</tr>").size());
        } else {
            processTable = results.left(nestedOpen);
        }
        processTable += QStringLiteral("</table>");
    } else if (firstClose >= 0) {
        processTable = results.left(firstClose + QStringLiteral("</table>").size());
    } else {
        const int lastTr = results.lastIndexOf(QStringLiteral("</tr>"), -1, Qt::CaseInsensitive);
        if (lastTr >= 0) {
            processTable = results.left(lastTr + QStringLiteral("</tr>").size());
        } else {
            processTable = results;
        }
        processTable += QStringLiteral("</table>");
    }

    return summary + QStringLiteral("</table><!--s-->") + processTable;
}

QString joinClosedProtocolSessions(const QStringList &sessions) {
    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = ensureClosedProtocolSession(sessions.at(i));
        if (session.isEmpty()) {
            continue;
        }
        if (i > 0 && !session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
            session.prepend(protocolSummaryTableOpenHtml());
        }
        result += session;
    }
    return result;
}

QString reassembleProtocolSessions(const QString &originalBody, const QStringList &sessions) {
    if (looksLikePictureAnswersResults(originalBody)
        || (!sessions.isEmpty() && looksLikePictureAnswersResults(sessions.first()))) {
        return rebuildProtocol12SessionList(sessions);
    }
    return joinClosedProtocolSessions(sessions);
}

QString applyParsedFieldsToStoredBody(const QString &storedBody, const ParsedProtocolFields &parsed) {
    const QStringList sessions = ExerciseProtocol::extractProtocolBodiesByDateRows(storedBody);
    if (sessions.isEmpty()) {
        return applyParsedFieldsToSessionChunk(normalizeStoredProtocolBody(storedBody), parsed);
    }
    QStringList updated = sessions;
    const int targetIndex = updated.size() - 1;
    updated[targetIndex] = applyParsedFieldsToSessionChunk(updated.at(targetIndex), parsed);
    return reassembleProtocolSessions(storedBody, updated);
}

QString keepOnlyLastSessionSummaryRows(QString body) {
    const QStringList sessions = ExerciseProtocol::extractProtocolBodiesByDateRows(body);
    if (sessions.size() <= 1) {
        return body;
    }
    QString last = stripLeadingSummaryTableWrapper(sessions.last());
    const int marker = last.indexOf(QStringLiteral("<!--s-->"));
    if (marker >= 0) {
        last = last.left(marker);
    }
    last.replace(
        QRegularExpression(QStringLiteral("</table>\\s*$"), QRegularExpression::CaseInsensitiveOption),
        QString());

    const QList<int> datePositions = findDateSpecialistPositions(body);
    const int firstRowStart = findRowStartBefore(body, datePositions.first());
    const QString prefix = firstRowStart > 0 ? body.left(firstRowStart) : QString();
    return prefix + last;
}

QStringList splitEditorSectionsByTitle(const QString &documentHtml) {
    QStringList sections;
    const QRegularExpression titleRe(
        QStringLiteral("Протокол\\s+фиксации\\s+результатов"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    QList<int> starts;
    QRegularExpressionMatchIterator it = titleRe.globalMatch(documentHtml);
    while (it.hasNext()) {
        starts.append(it.next().capturedStart());
    }
    if (starts.isEmpty()) {
        return sections;
    }
    for (int i = 0; i < starts.size(); ++i) {
        const int end = (i + 1 < starts.size()) ? starts.at(i + 1) : documentHtml.length();
        const QString chunk = documentHtml.mid(starts.at(i), end - starts.at(i)).trimmed();
        if (!chunk.isEmpty()) {
            sections.append(chunk);
        }
    }
    return sections;
}

QString extractProtocolSectionFromEditor(const QString &documentHtml, int protocolIndex) {
    QStringList sections = ExerciseProtocol::extractProtocolBodiesByDateRows(documentHtml);
    if (sections.isEmpty()) {
        sections = splitEditorSectionsByTitle(documentHtml);
    }
    if (protocolIndex >= 0 && protocolIndex < sections.size()) {
        return sections.at(protocolIndex);
    }
    if (protocolIndex == 0) {
        return ExerciseProtocol::extractEditableProtocolBody(documentHtml);
    }
    return {};
}

QString extractCheckedValue(const QString &html, const QString &id) {
    const QRegularExpression checkedRe(
        QStringLiteral("id=['\"]%1['\"][^>]*checked").arg(QRegularExpression::escape(id)),
        QRegularExpression::CaseInsensitiveOption);
    if (!checkedRe.match(html).hasMatch()) {
        return {};
    }
    const QRegularExpression valueRe(
        QStringLiteral("id=['\"]%1['\"][^>]*value=['\"]([^'\"]*)['\"]").arg(QRegularExpression::escape(id)),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = valueRe.match(html);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString stripSpecialistSections(QString body) {
    while (true) {
        const int marker = body.indexOf(QStringLiteral("<!--s-->"));
        if (marker < 0) {
            break;
        }
        const int tableEnd = body.indexOf(QStringLiteral("</table>"), marker, Qt::CaseInsensitive);
        if (tableEnd < 0) {
            body = body.left(marker);
            break;
        }
        body = body.left(marker) + body.mid(tableEnd + QStringLiteral("</table>").size());
    }
    body.replace(QStringLiteral("Процесс выполнения диагностической методики"), QString());
    return body;
}

QString ensureProtocol12SummaryTableOpens(QString body) {
    const QString tableOpen = protocolSummaryTableOpenHtml();
    const QRegularExpression orphanRowRe(
        QStringLiteral("</table>\\s*(<tr[^>]*>\\s*<td[^>]*>\\s*Дата/специалист)"),
        QRegularExpression::CaseInsensitiveOption);
    body.replace(orphanRowRe, QStringLiteral("</table>") + tableOpen + QStringLiteral("\\1"));
    return body;
}

} // namespace

QString replaceDivInnerById(QString html, const QString &divId, const QString &innerHtml);

QString ExerciseProtocol::patientProtocolBody(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    QString body = stripSpecialistSections(protocolBody);
    return ensureProtocol12SummaryTableOpens(body);
}

QString ExerciseProtocol::extractLastSessionStoredBody(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    const QStringList sessions = ExerciseProtocol::extractProtocolBodiesByDateRows(protocolBody);
    if (sessions.size() <= 1) {
        return stripLeadingSummaryTableWrapper(protocolBody);
    }
    // Для отображения на странице упражнения шапка (header.html) уже открывает <table>.
    // Последняя сессия должна продолжаться строками <tr>, без нового <table> внутри.
    return stripLeadingSummaryTableWrapper(sessions.last());
}

QString ExerciseProtocol::formatProtocol12BodyForHeaderView(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    QStringList sessions = extractProtocol12Sessions(protocolBody);
    if (sessions.isEmpty()) {
        sessions = ExerciseProtocol::extractProtocolBodiesByDateRows(protocolBody);
    }
    if (sessions.isEmpty()) {
        return stripLeadingSummaryTableWrapper(protocolBody);
    }
    return rebuildProtocol12SessionList(sessions);
}

QString ExerciseProtocol::buildProtocol12ProtocolsTabRecord(
    const QString &headerFragment,
    const QString &storedBody) {
    if (storedBody.trimmed().isEmpty()) {
        return headerFragment;
    }

    const QString canonicalBody = canonicalizeProtocol12StoredBody(storedBody);
    QStringList sessions = extractProtocol12Sessions(canonicalBody);
    if (sessions.isEmpty()) {
        sessions = ExerciseProtocol::extractProtocolBodiesByDateRows(canonicalBody);
    }
    if (sessions.isEmpty()) {
        QString fallback = headerFragment + stripLeadingSummaryTableWrapper(canonicalBody);
        if (!canonicalBody.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
            fallback += QStringLiteral("</table>");
        }
        return fallback;
    }

    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = sessions.at(i);
        const int marker = session.indexOf(QStringLiteral("<!--s-->"));
        QString summaryRows = marker >= 0 ? session.left(marker) : session;
        QString resultsBlock = marker >= 0 ? session.mid(marker + QStringLiteral("<!--s-->").size()) : QString();

        summaryRows = stripLeadingSummaryTableWrapper(summaryRows);
        summaryRows.replace(
            QRegularExpression(QStringLiteral("</table>\\s*$"), QRegularExpression::CaseInsensitiveOption),
            QString());
        summaryRows = summaryRows.trimmed();
        resultsBlock = resultsBlock.trimmed();

        if (i == 0) {
            if (!headerFragment.trimmed().isEmpty()) {
                result += headerFragment;
            } else {
                result += protocolSummaryTableOpenHtml();
            }
            if (!summaryRows.isEmpty()) {
                result += summaryRows;
            }
            result += QStringLiteral("</table>");
        } else {
            result += protocolSummaryTableOpenHtml();
            if (!summaryRows.isEmpty()) {
                result += summaryRows;
            }
            result += QStringLiteral("</table>");
        }
        if (!resultsBlock.isEmpty()) {
            result += resultsBlock;
        }
    }
    return result;
}

QString ExerciseProtocol::canonicalizeProtocol12StoredBody(const QString &protocolBody) {
    return formatProtocol12BodyForHeaderView(protocolBody);
}

QString ExerciseProtocol::stripProtocolRecordHeader(
    const QString &recordHtml,
    const QString &headerFragment) {
    QString body = recordHtml.trimmed();
    if (body.isEmpty()) {
        return body;
    }
    if (!headerFragment.trimmed().isEmpty() && body.startsWith(headerFragment)) {
        body = body.mid(headerFragment.size());
    }
    return normalizeStoredProtocolBody(body);
}

QString ExerciseProtocol::stripMethodologyFillForDocExport(const QString &protocolHtml) {
    QString html = protocolHtml;
    html.replace(
        QRegularExpression(
            QStringLiteral(" bgcolor=[\"']#a3f3d5[\"']"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    return html;
}

QString ExerciseProtocol::restrictExercisePageEditing(const QString &protocolHtml) {
    QString html = protocolHtml;
    html.replace(
        QRegularExpression(
            QStringLiteral(" contenteditable=['\"]true['\"]"),
            QRegularExpression::CaseInsensitiveOption),
        QString());

    const QRegularExpression resultRe(
        QStringLiteral("(<tr[^>]*>\\s*<td[^>]*>[^<]*Результат[^<]*</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    html.replace(
        resultRe,
        QStringLiteral("\\1<div contenteditable='true'>\\2</div>\\3"));

    const QRegularExpression noteRe(
        QStringLiteral("(<tr[^>]*>\\s*<td[^>]*>[^<]*Примечание[^<]*</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    html.replace(
        noteRe,
        QStringLiteral("\\1<div contenteditable='true'>\\2</div>\\3"));

    const QRegularExpression rowspanRe(
        QStringLiteral("(<td[^>]*rowspan=['\"]5['\"][^>]*>)([\\s\\S]*?)(</td>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    html.replace(
        rowspanRe,
        QStringLiteral("\\1<div contenteditable='true'>\\2</div>\\3"));

    return html;
}

QString ExerciseProtocol::normalizeProtocol12Layout(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    return ensureProtocol12SummaryTableOpens(protocolBody);
}

QString ExerciseProtocol::createProtocolHtml(
    const QString &exerciseId,
    const QString &userFio,
    int elapsedSeconds,
    bool partly,
    const QString &existingProtocolHtml,
    const QList<bool> &answers,
    const CheckboxValues &checkboxes,
    const ProtocolSessionInput &session) {
    const ExerciseDefinition *definition = ExerciseConfig::find(exerciseId);
    if (definition && definition->protocol != ExerciseProtocolKind::PictureAnswers) {
        return createExerciseProtocolBody(
            *definition,
            userFio,
            elapsedSeconds,
            partly,
            existingProtocolHtml,
            answers,
            checkboxes,
            session);
    }
    if (exerciseId != QStringLiteral("1.2")) {
        return existingProtocolHtml;
    }

    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"));
    QString sessionPart = summaryRowHtml(
        QStringLiteral("Дата/специалист"),
        QStringLiteral("%1   %2").arg(now, userFio.toHtmlEscaped()));
    sessionPart += summaryRowHtml(
        QStringLiteral("Результат: вывод об уровне развития"),
        QStringLiteral("<div contenteditable='true'></div>"));
    sessionPart += summaryRowHtml(
        QStringLiteral("Примечание"),
        QStringLiteral("<div contenteditable='true'></div>"));
    sessionPart += QStringLiteral(
        "<tr><td align='center' colspan='2' valign='top'>Процесс выполнения диагностической методики</td></tr>"
        "</table><!--s-->")
               + resultsTableHeaderHtml();

    const QString activityHtml = formatProtocolCellText(checkboxes.activity);
    const QString helpHtml = formatProtocolCellText(checkboxes.help);
    const QStringList descriptions = pictureDescriptions();

    for (int i = 0; i < 5; ++i) {
        const bool correct = i < answers.size() ? answers.at(i) : false;
        const QString verno = answerText(correct);
        if (i == 0) {
            sessionPart += QStringLiteral("<tr><td >%1</td><td valign='top' >%2</td>"
                                "<td valign='top' rowspan='5'><div contenteditable='true'>%3</div></td>"
                                "<td valign='top' rowspan='5'><div contenteditable='true'>%4</div></td></tr>")
                       .arg(descriptions.at(i), verno, activityHtml, helpHtml);
        } else if (i == 4) {
            sessionPart += QStringLiteral("<tr><td>%1</td><td>%2</td></tr>")
                       .arg(descriptions.at(i), verno);
        } else {
            sessionPart += QStringLiteral("<tr><td>%1</td><td valign='top'>%2</td></tr>")
                       .arg(descriptions.at(i), verno);
        }
    }
    sessionPart += QStringLiteral("</table>");

    if (partly && !existingProtocolHtml.trimmed().isEmpty()) {
        const QString base = ExerciseProtocol::canonicalizeProtocol12StoredBody(existingProtocolHtml);
        return base + protocolSummaryTableOpenHtml() + sessionPart;
    }
    return sessionPart;
}

QString ExerciseProtocol::normalizeStoredProtocolBody(const QString &protocolBody) {
    return ::normalizeStoredProtocolBody(protocolBody);
}

QString ExerciseProtocol::repairResultsTableBody(const QString &protocolBody, const QList<bool> &answers) {
    if (!protocolBody.contains(QStringLiteral("<!--s-->"))) {
        return protocolBody;
    }
    return ::repairResultsTableBody(protocolBody, answers);
}

QString ExerciseProtocol::wrapEditableProtocolBody(const QString &protocolBody) {
    return protocolBodyStartMarker() + protocolBody + protocolBodyEndMarker();
}

QString ExerciseProtocol::wrapProtocolRecord(const QString &protocolId, const QString &protocolBody) {
    return protocolRecordStartSpan(protocolId)
        + protocolRecordStartMarker(protocolId)
        + protocolBody
        + protocolRecordEndMarker(protocolId)
        + protocolRecordEndSpan(protocolId);
}

QString ExerciseProtocol::extractEditableProtocolBody(const QString &documentHtml) {
    QString body = extractBetweenMarkers(
        documentHtml,
        QStringLiteral("id=[\"']dokit-protocol-body-start[\"'][^>]*>.*?</span>"),
        QStringLiteral("<span[^>]*id=[\"']dokit-protocol-body-end[\"']"));
    if (!body.isEmpty()) {
        return body;
    }

    const QString startMarker = QStringLiteral("<!--body-->");
    const QString endMarker = QStringLiteral("<!--ebody-->");
    const int start = documentHtml.indexOf(startMarker);
    if (start >= 0) {
        const int contentStart = start + startMarker.size();
        const int end = documentHtml.indexOf(endMarker, contentStart);
        if (end > contentStart) {
            return documentHtml.mid(contentStart, end - contentStart).trimmed();
        }
    }

    const QStringList bodies = extractProtocolBodiesByDateRows(documentHtml);
    if (!bodies.isEmpty()) {
        return bodies.last();
    }

    return extractProtocolBodyFallback(documentHtml);
}

QStringList ExerciseProtocol::extractProtocolBodiesByDateRows(const QString &documentHtml) {
    // Специфичное разбиение 1.2 — только для picture-answers.
    // Иначе тела 1.1/других (тоже с <!--s-->) обрабатываются как 1.2 и портятся при merge/append.
    if (looksLikePictureAnswersResults(documentHtml)) {
        const QStringList sessions = extractProtocol12Sessions(documentHtml);
        if (!sessions.isEmpty()) {
            return sessions;
        }
    }

    QStringList bodies;
    const QList<int> datePositions = findDateSpecialistPositions(documentHtml);
    for (int i = 0; i < datePositions.size(); ++i) {
        const int rowStart = findRowStartBefore(documentHtml, datePositions.at(i));
        if (rowStart < 0) {
            continue;
        }
        int endPos = documentHtml.length();
        if (i + 1 < datePositions.size()) {
            const int nextRowStart = findRowStartBefore(documentHtml, datePositions.at(i + 1));
            if (nextRowStart > rowStart) {
                endPos = nextRowStart;
            }
        } else {
            endPos = findProtocolChunkEnd(documentHtml, rowStart, -1);
            // Для не-1.2 обрезаем по таблице процесса; если внутри уже вложен новый <table> —
            // режем до него (иначе сессии склеиваются «ячейка в ячейке»).
            const int marker = documentHtml.lastIndexOf(QStringLiteral("<!--s-->"), endPos);
            if (marker >= rowStart) {
                const int tableStart =
                    documentHtml.indexOf(QStringLiteral("<table"), marker, Qt::CaseInsensitive);
                if (tableStart >= 0) {
                    const int tableClose = documentHtml.indexOf(
                        QStringLiteral("</table>"), tableStart, Qt::CaseInsensitive);
                    const int nestedTable = documentHtml.indexOf(
                        QStringLiteral("<table"), tableStart + 6, Qt::CaseInsensitive);
                    if (nestedTable >= 0 && (tableClose < 0 || nestedTable < tableClose)) {
                        int cut = documentHtml.lastIndexOf(
                            QStringLiteral("</tr>"), nestedTable, Qt::CaseInsensitive);
                        if (cut > tableStart) {
                            endPos = cut + QStringLiteral("</tr>").size();
                        } else {
                            endPos = nestedTable;
                        }
                    } else if (tableClose >= 0 && tableClose + 8 <= endPos) {
                        endPos = tableClose + QStringLiteral("</table>").size();
                    }
                }
            }
        }
        QString chunk = normalizeStoredProtocolBody(trimProtocolBodyTail(documentHtml.mid(rowStart, endPos - rowStart)));
        if (!chunk.isEmpty()) {
            bodies.append(chunk);
        }
    }
    return bodies;
}

QMap<QString, QString> ExerciseProtocol::extractProtocolBodiesById(const QString &documentHtml) {
    QMap<QString, QString> bodies;
    const QRegularExpression spanRe(
        QStringLiteral("id=[\"']dokit-pid-(\\d+)-start[\"'][^>]*>.*?</span>([\\s\\S]*?)<span[^>]*id=[\"']dokit-pid-\\1-end[\"']"),
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator spanIt = spanRe.globalMatch(documentHtml);
    while (spanIt.hasNext()) {
        const QRegularExpressionMatch match = spanIt.next();
        bodies.insert(match.captured(1), match.captured(2).trimmed());
    }
    if (!bodies.isEmpty()) {
        return bodies;
    }

    const QRegularExpression commentRe(
        QStringLiteral("<!--protocol-id:(\\d+)-->([\\s\\S]*?)<!--/protocol-id:\\1-->"),
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator commentIt = commentRe.globalMatch(documentHtml);
    while (commentIt.hasNext()) {
        const QRegularExpressionMatch match = commentIt.next();
        bodies.insert(match.captured(1), match.captured(2).trimmed());
    }
    return bodies;
}

QString ExerciseProtocol::protocolViewHtml(
    const QString &exerciseId,
    const QString &protocolBody,
    const QString &patientFio,
    const QString &patientBirthDate) {
    QString protocolBlock = readHeaderRows(exerciseId) + protocolBody;
    if (!protocolBody.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
        protocolBlock += QStringLiteral("</table>");
    }
    return QStringLiteral(
               "<div align='center' style='font-size:20px'><br>Протокол фиксации результатов исследования</div>"
               "<br>ФИО: %1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
               "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Дата рождения:%2<br><br>%3")
        .arg(patientFio.toHtmlEscaped(), patientBirthDate.toHtmlEscaped(), protocolBlock);
}

ExerciseProtocol::CheckboxValues ExerciseProtocol::readCheckboxValues(const QString &orHtml) {
    CheckboxValues values;
    static const char *activityIds[] = {"idd1", "idd2", "idd3"};
    for (const char *id : activityIds) {
        const QString value = extractCheckedValue(orHtml, QString::fromUtf8(id));
        if (!value.isEmpty()) {
            values.activity = value;
            break;
        }
    }
    static const char *helpIds[] = {"idp1", "idp2", "idp3", "idp4", "idp5"};
    QStringList helpValues;
    for (const char *id : helpIds) {
        const QString value = extractCheckedValue(orHtml, QString::fromUtf8(id));
        if (!value.isEmpty()) {
            helpValues << value;
        }
    }
    values.help = helpValues.join(QStringLiteral("\n"));
    return values;
}

QString ExerciseProtocol::applyCheckboxValues(const QString &orHtml, const CheckboxValues &values) {
    Q_UNUSED(values);
    return orHtml;
}

QString ExerciseProtocol::mergeEditorDocumentIntoStoredBody(
    const QString &storedBody,
    QTextDocument *editorDocument,
    int protocolIndex) {
    if (storedBody.trimmed().isEmpty() || !editorDocument) {
        return storedBody;
    }

    const QString editorHtml = editorDocument->toHtml();
    QStringList editorSessions = ExerciseProtocol::extractProtocolBodiesByDateRows(editorHtml);
    if (editorSessions.isEmpty()) {
        ParsedProtocolFields parsed = parseProtocolFieldsFromDocument(editorDocument, protocolIndex);
        if (!parsed.hasDateSpecialist && !parsed.hasResult && !parsed.hasNote && parsed.answersByIndex.isEmpty()) {
            parsed = parseProtocolFieldsFromHtml(editorHtml, protocolIndex);
        }
        if (!parsed.hasDateSpecialist && !parsed.hasResult && !parsed.hasNote && parsed.answersByIndex.isEmpty()) {
            return storedBody;
        }
        return applyParsedFieldsToStoredBody(storedBody, parsed);
    }

    const QStringList storedSessions = ExerciseProtocol::extractProtocolBodiesByDateRows(storedBody);
    if (storedSessions.isEmpty()) {
        ParsedProtocolFields parsed = parseProtocolFieldsFromHtml(editorSessions.first(), 0);
        return applyParsedFieldsToStoredBody(storedBody, parsed);
    }

    if (editorSessions.size() == 1 && storedSessions.size() > 1) {
        ParsedProtocolFields parsed = parseProtocolFieldsFromDocument(editorDocument, 0);
        if (!parsed.hasDateSpecialist && !parsed.hasResult && !parsed.hasNote && parsed.answersByIndex.isEmpty()) {
            parsed = parseProtocolFieldsFromHtml(editorSessions.first(), 0);
        }
        QStringList updated = storedSessions;
        const int targetIndex = updated.size() - 1;
        updated[targetIndex] = applyParsedFieldsToSessionChunk(updated.at(targetIndex), parsed);
        return reassembleProtocolSessions(storedBody, updated);
    }

    QStringList updated = storedSessions;
    const int count = qMin(editorSessions.size(), storedSessions.size());
    for (int i = 0; i < count; ++i) {
        ParsedProtocolFields parsed = parseProtocolFieldsFromHtml(editorSessions.at(i), 0);
        if (!parsed.hasDateSpecialist && !parsed.hasResult && !parsed.hasNote && parsed.answersByIndex.isEmpty()) {
            parsed = parseProtocolFieldsFromDocument(editorDocument, i);
        }
        updated[i] = applyParsedFieldsToSessionChunk(storedSessions.at(i), parsed);
    }
    return reassembleProtocolSessions(storedBody, updated);
}

bool looksLikeProtocol126Body(const QString &body) {
    // Таблицы заданий 1/2 с div id — ensureClosed/joinClosed их срезает.
    return body.contains(QStringLiteral("id='idvivod'"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id=\"idvivod\""), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id='col11'"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id=\"col11\""), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id='sum1'"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id=\"sum1\""), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("Задание 1"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("Задание 2"), Qt::CaseInsensitive);
}

// Только по «Дата/специалист», без обрезки вложенных <table> (баллы 1.26).
QStringList extractProtocol126SessionsByDate(const QString &body) {
    const QList<int> datePositions = findDateSpecialistPositions(body);
    if (datePositions.isEmpty()) {
        return {};
    }
    QStringList sessions;
    for (int i = 0; i < datePositions.size(); ++i) {
        const int rowStart = findRowStartBefore(body, datePositions.at(i));
        if (rowStart < 0) {
            continue;
        }
        int endPos = body.length();
        if (i + 1 < datePositions.size()) {
            const int nextRowStart = findRowStartBefore(body, datePositions.at(i + 1));
            if (nextRowStart > rowStart) {
                endPos = nextRowStart;
            }
        }
        const QString chunk = body.mid(rowStart, endPos - rowStart).trimmed();
        if (!chunk.isEmpty()) {
            sessions.append(chunk);
        }
    }
    return sessions;
}

QString joinProtocol126Sessions(const QStringList &sessions) {
    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = sessions.at(i).trimmed();
        if (session.isEmpty()) {
            continue;
        }
        if (i == 0) {
            session = stripLeadingSummaryTableWrapper(session);
        } else if (!session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
            session.prepend(protocolSummaryTableOpenHtml());
        }
        result += session;
    }
    return result;
}

QString ExerciseProtocol::mergeLimitedEditableFieldsIntoStoredBody(
    const QString &storedBody,
    QTextDocument *editorDocument) {
    if (storedBody.trimmed().isEmpty() || !editorDocument) {
        return storedBody;
    }

    ParsedProtocolFields parsed = parseProtocolFieldsFromDocument(editorDocument, 0);
    if (!parsed.hasResult && !parsed.hasNote) {
        return storedBody;
    }

    // 1.26: только Результат/Примечание последней сессии — без joinClosed.
    if (looksLikeProtocol126Body(storedBody)) {
        QStringList sessions = extractProtocol126SessionsByDate(storedBody);
        if (sessions.isEmpty()) {
            QString body = storedBody;
            if (parsed.hasResult) {
                body = replaceResultRowSecondCell(body, parsed.resultText);
            }
            if (parsed.hasNote) {
                body = replaceRowSecondCell(body, QStringLiteral("Примечание"), parsed.noteText);
            }
            return body;
        }
        QString last = sessions.last();
        if (parsed.hasResult) {
            if (last.contains(QStringLiteral("idvivod"), Qt::CaseInsensitive)) {
                last = replaceDivInnerById(last, QStringLiteral("idvivod"), parsed.resultText.toHtmlEscaped());
            } else {
                last = replaceResultRowSecondCell(last, parsed.resultText);
            }
        }
        if (parsed.hasNote) {
            last = replaceRowSecondCell(last, QStringLiteral("Примечание"), parsed.noteText);
        }
        sessions[sessions.size() - 1] = last;
        return joinProtocol126Sessions(sessions);
    }

    QStringList sessions = extractProtocolBodiesByDateRows(storedBody);
    if (sessions.isEmpty()) {
        QString body = storedBody;
        if (parsed.hasResult) {
            body = replaceResultRowSecondCell(body, parsed.resultText);
        }
        if (parsed.hasNote) {
            body = replaceRowSecondCell(body, QStringLiteral("Примечание"), parsed.noteText);
        }
        return ensureClosedProtocolSession(body);
    }

    QString last = sessions.last();
    if (parsed.hasResult) {
        last = replaceResultRowSecondCell(last, parsed.resultText);
    }
    if (parsed.hasNote) {
        last = replaceRowSecondCell(last, QStringLiteral("Примечание"), parsed.noteText);
    }
    sessions[sessions.size() - 1] = last;
    return joinClosedProtocolSessions(sessions);
}

QString replaceDivInnerById(QString html, const QString &divId, const QString &innerHtml) {
    const QRegularExpression re(
        QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"]%1['\"][^>]*>)([\\s\\S]*?)(</div>)")
            .arg(QRegularExpression::escape(divId)),
        QRegularExpression::CaseInsensitiveOption);
    if (!re.match(html).hasMatch()) {
        return html;
    }
    return html.replace(re, QStringLiteral("\\1") + innerHtml + QStringLiteral("\\3"));
}

QString extractDivInnerById(const QString &html, const QString &divId) {
    const QRegularExpression re(
        QStringLiteral("<div\\b[^>]*\\bid\\s*=\\s*['\"]%1['\"][^>]*>([\\s\\S]*?)</div>")
            .arg(QRegularExpression::escape(divId)),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(html);
    if (!match.hasMatch()) {
        return {};
    }
    return htmlFragmentToPlainText(match.captured(1)).trimmed();
}

double sumDivPrefix(const QString &html, const QString &prefix) {
    double sum = 0;
    for (int i = 1; i <= 100; ++i) {
        const QString id = prefix + QString::number(i);
        const QRegularExpression exists(
            QStringLiteral("<div\\b[^>]*\\bid\\s*=\\s*['\"]%1['\"]")
                .arg(QRegularExpression::escape(id)),
            QRegularExpression::CaseInsensitiveOption);
        if (!exists.match(html).hasMatch()) {
            break;
        }
        const QString plain = extractDivInnerById(html, id);
        bool ok = false;
        const double value = plain.toDouble(&ok);
        if (ok) {
            sum += value;
        }
    }
    return sum;
}

QString replaceScoreCellByRowLabel(QString html, const QString &rowLabel, const QString &scorePlain) {
    const QString escaped = QRegularExpression::escape(rowLabel);
    // <tr>...<td>label</td><td>answer</td><td>...score...</td></tr>
    const QRegularExpression re(
        QStringLiteral(
            "(<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>[\\s\\S]*?</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)")
            .arg(escaped),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    if (!re.match(html).hasMatch()) {
        // Итоговая / индекс: colspan на первых ячейках
        const QRegularExpression re2(
            QStringLiteral(
                "(<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?%1[\\s\\S]*?</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)")
                .arg(escaped),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        if (!re2.match(html).hasMatch()) {
            return html;
        }
        const QString inner = QStringLiteral("<div contenteditable='true'>%1</div>").arg(scorePlain.toHtmlEscaped());
        return html.replace(re2, QStringLiteral("\\1") + inner + QStringLiteral("\\3"));
    }
    // Сохраняем id у div, если был.
    QRegularExpressionMatch match = re.match(html);
    QString oldInner = match.captured(2);
    QString newInner;
    const QRegularExpression idRe(
        QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"][^'\"]+['\"][^>]*>)([\\s\\S]*?)(</div>)"),
        QRegularExpression::CaseInsensitiveOption);
    if (idRe.match(oldInner).hasMatch()) {
        newInner = oldInner;
        newInner.replace(idRe, QStringLiteral("\\1") + scorePlain.toHtmlEscaped() + QStringLiteral("\\3"));
    } else {
        newInner = QStringLiteral("<div contenteditable='true'>%1</div>").arg(scorePlain.toHtmlEscaped());
    }
    return html.replace(re, QStringLiteral("\\1") + newInner + QStringLiteral("\\3"));
}

QString replaceNthLabeledScoreCell(
    QString html,
    const QString &rowLabel,
    int occurrence,
    const QString &scorePlain) {
    const QString escaped = QRegularExpression::escape(rowLabel);
    const QRegularExpression re(
        QStringLiteral(
            "(<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?%1[\\s\\S]*?</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)")
            .arg(escaped),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    int found = 0;
    int offset = 0;
    while (true) {
        const QRegularExpressionMatch match = re.match(html, offset);
        if (!match.hasMatch()) {
            break;
        }
        if (found == occurrence) {
            QString oldInner = match.captured(2);
            QString newInner;
            const QRegularExpression idRe(
                QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"][^'\"]+['\"][^>]*>)([\\s\\S]*?)(</div>)"),
                QRegularExpression::CaseInsensitiveOption);
            if (idRe.match(oldInner).hasMatch()) {
                newInner = oldInner;
                newInner.replace(
                    idRe, QStringLiteral("\\1") + scorePlain.toHtmlEscaped() + QStringLiteral("\\3"));
            } else {
                newInner =
                    QStringLiteral("<div contenteditable='true'>%1</div>").arg(scorePlain.toHtmlEscaped());
            }
            html.replace(match.capturedStart(), match.capturedLength(),
                match.captured(1) + newInner + match.captured(3));
            break;
        }
        ++found;
        offset = match.capturedEnd();
    }
    return html;
}

// Эталоны из spravka/1.26.html + порядок портретов в протоколе задания 1.
QStringList expectedEmotionsTask1() {
    return {
        QStringLiteral("радость"),
        QStringLiteral("злость"),
        QStringLiteral("грусть"),
        QStringLiteral("страх"),
        QStringLiteral("удивление"),
        QStringLiteral("спокойствие"),
    };
}

QStringList expectedEmotionsTask2() {
    return {
        QStringLiteral("злость"),
        QStringLiteral("грусть"),
        QStringLiteral("спокойствие"),
        QStringLiteral("злость"),
        QStringLiteral("удивление"),
        QStringLiteral("радость"),
        QStringLiteral("страх"),
        QStringLiteral("спокойствие"),
        QStringLiteral("грусть"),
        QStringLiteral("страх"),
        QStringLiteral("радость"),
        QStringLiteral("удивление"),
    };
}

QStringList emotionSynonyms(const QString &expected) {
    const QString key = expected.toLower().trimmed();
    if (key.startsWith(QStringLiteral("радост"))) {
        return {QStringLiteral("радост"), QStringLiteral("весел"), QStringLiteral("счастли"),
                QStringLiteral("улыб"), QStringLiteral("хорош")};
    }
    if (key.startsWith(QStringLiteral("злост")) || key.startsWith(QStringLiteral("зл"))) {
        return {QStringLiteral("злост"), QStringLiteral("зл"), QStringLiteral("гнев"),
                QStringLiteral("сердит"), QStringLiteral("раздраж")};
    }
    if (key.startsWith(QStringLiteral("груст"))) {
        return {QStringLiteral("груст"), QStringLiteral("печал"), QStringLiteral("плач"),
                QStringLiteral("слез"), QStringLiteral("огорч")};
    }
    if (key.startsWith(QStringLiteral("страх"))) {
        return {QStringLiteral("страх"), QStringLiteral("боит"), QStringLiteral("испуг"),
                QStringLiteral("ужас")};
    }
    if (key.startsWith(QStringLiteral("удивл"))) {
        return {QStringLiteral("удивл"), QStringLiteral("изумл"), QStringLiteral("шок"),
                QStringLiteral("огорош")};
    }
    if (key.startsWith(QStringLiteral("спокой"))) {
        return {QStringLiteral("спокой"), QStringLiteral("нейтрал"), QStringLiteral("обычн"),
                QStringLiteral("норм")};
    }
    return {key};
}

QStringList emotionApproximate(const QString &expected) {
    const QString key = expected.toLower().trimmed();
    if (key.startsWith(QStringLiteral("радост"))) {
        return {QStringLiteral("улыб"), QStringLiteral("сме"), QStringLiteral("радостн")};
    }
    if (key.startsWith(QStringLiteral("груст"))) {
        return {QStringLiteral("плач"), QStringLiteral("слез"), QStringLiteral("грустн")};
    }
    if (key.startsWith(QStringLiteral("злост"))) {
        return {QStringLiteral("сердит"), QStringLiteral("злой"), QStringLiteral("злая")};
    }
    if (key.startsWith(QStringLiteral("страх"))) {
        return {QStringLiteral("боит"), QStringLiteral("тревог")};
    }
    if (key.startsWith(QStringLiteral("удивл"))) {
        return {QStringLiteral("удив"), QStringLiteral("не ожид")};
    }
    if (key.startsWith(QStringLiteral("спокой"))) {
        return {QStringLiteral("спокойн"), QStringLiteral("тих")};
    }
    return {};
}

int scoreEmotionAnswer(const QString &answer, const QString &expected) {
    const QString a = answer.toLower().simplified();
    if (a.isEmpty()) {
        return 0;
    }
    for (const QString &syn : emotionSynonyms(expected)) {
        if (a.contains(syn)) {
            return 2;
        }
    }
    for (const QString &approx : emotionApproximate(expected)) {
        if (a.contains(approx)) {
            return 1;
        }
    }
    return 0;
}

QString extractAnswerFromLabeledRow(const QString &html, const QString &rowLabel) {
    const QString escaped = QRegularExpression::escape(rowLabel);
    // label | answer | score
    const QRegularExpression re(
        QStringLiteral(
            "<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>([\\s\\S]*?)</td>\\s*<td[^>]*>")
            .arg(escaped),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = re.match(html);
    if (!match.hasMatch()) {
        return {};
    }
    return htmlFragmentToPlainText(match.captured(1)).trimmed();
}

QString fillProtocol126RowScores(QString body) {
    const QStringList task1Labels = {
        QStringLiteral("Радость"),
        QStringLiteral("Злость"),
        QStringLiteral("Грусть"),
        QStringLiteral("Страх"),
        QStringLiteral("Удивление"),
        QStringLiteral("Спокойствие"),
    };
    const QStringList expected1 = expectedEmotionsTask1();
    for (int i = 0; i < task1Labels.size(); ++i) {
        const QString answer = extractAnswerFromLabeledRow(body, task1Labels.at(i));
        const int score = scoreEmotionAnswer(answer, expected1.at(i));
        // Только содержимое div по id — как InnerHtml в оригинале, без правки <td>/<tr>.
        body = replaceDivInnerById(body, QStringLiteral("col1%1").arg(i + 1), QString::number(score));
    }

    const QStringList expected2 = expectedEmotionsTask2();
    for (int i = 0; i < expected2.size(); ++i) {
        const QString label = QString::number(i + 1);
        const QString answer = extractAnswerFromLabeledRow(body, label);
        const int score = scoreEmotionAnswer(answer, expected2.at(i));
        body = replaceDivInnerById(body, QStringLiteral("col2%1").arg(i + 1), QString::number(score));
    }
    return body;
}

QString ExerciseProtocol::applyProtocol126SumFromDocument(
    const QString &storedBody,
    QTextDocument *editorDocument,
    bool computeSums) {
    if (storedBody.trimmed().isEmpty()) {
        return storedBody;
    }

    QString body = storedBody;

    // Без «Подвести итог» — только перенос вручную введённых баллов из редактора в HTML.
    if (!computeSums) {
        if (editorDocument) {
            QList<QTextTable *> tables;
            collectTables(editorDocument->rootFrame(), tables);
            for (QTextTable *table : tables) {
                if (!table || table->columns() < 2) {
                    continue;
                }
                int ballsCol = -1;
                int headerRow = -1;
                for (int r = 0; r < table->rows() && ballsCol < 0; ++r) {
                    for (int c = 0; c < table->columns(); ++c) {
                        const QString header = readTableCellText(table, r, c);
                        if (header.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)
                            && header.length() <= 12) {
                            ballsCol = c;
                            headerRow = r;
                            break;
                        }
                    }
                }
                if (ballsCol < 0) {
                    continue;
                }
                for (int r = headerRow + 1; r < table->rows(); ++r) {
                    const QString label = readTableCellText(table, r, 0);
                    const QString score = readTableCellText(table, r, ballsCol);
                    if (label.isEmpty()
                        || label.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)
                        || label.contains(QStringLiteral("Индекс"), Qt::CaseInsensitive)) {
                        continue;
                    }
                    if (score.trimmed().isEmpty()) {
                        continue;
                    }
                    // Предпочитаем id col1N/col2N — не трогаем разметку строки.
                    bool wrote = false;
                    for (int n = 1; n <= 16 && !wrote; ++n) {
                        const QString id1 = QStringLiteral("col1%1").arg(n);
                        const QString id2 = QStringLiteral("col2%1").arg(n);
                        if (body.contains(QStringLiteral("id='%1'").arg(id1))
                            || body.contains(QStringLiteral("id=\"%1\"").arg(id1))) {
                            // Сопоставить строку по подписи задачи 1.
                            static const QStringList kTask1 = {
                                QStringLiteral("Радость"), QStringLiteral("Злость"),
                                QStringLiteral("Грусть"), QStringLiteral("Страх"),
                                QStringLiteral("Удивление"), QStringLiteral("Спокойствие"),
                            };
                            if (n <= kTask1.size() && label.compare(kTask1.at(n - 1), Qt::CaseInsensitive) == 0) {
                                body = replaceDivInnerById(body, id1, score);
                                wrote = true;
                            }
                        }
                        if (!wrote && label == QString::number(n)
                            && (body.contains(QStringLiteral("id='%1'").arg(id2))
                                || body.contains(QStringLiteral("id=\"%1\"").arg(id2)))) {
                            body = replaceDivInnerById(body, id2, score);
                            wrote = true;
                        }
                    }
                }
            }
        }
        return body;
    }

    Q_UNUSED(editorDocument);
    // Подвести итог только для последней сессии (при повторных протоколах в одном теле).
    QStringList sessions = extractProtocol126SessionsByDate(body);
    if (sessions.size() > 1) {
        QString last = sessions.last();
        last = fillProtocol126RowScores(last);
        const double sum1 = sumDivPrefix(last, QStringLiteral("col1"));
        const double sum2 = sumDivPrefix(last, QStringLiteral("col2"));
        const int sum3 = static_cast<int>(sum1 + sum2);
        last = replaceDivInnerById(last, QStringLiteral("sum1"), QString::number(static_cast<int>(sum1)));
        last = replaceDivInnerById(last, QStringLiteral("sum2"), QString::number(static_cast<int>(sum2)));
        last = replaceDivInnerById(last, QStringLiteral("sum3"), QString::number(sum3));
        last = replaceDivInnerById(
            last, QStringLiteral("idvivod"), QString::number(sum3) + QStringLiteral("(36)"));
        sessions[sessions.size() - 1] = last;
        return joinProtocol126Sessions(sessions);
    }

    body = fillProtocol126RowScores(body);

    const double sum1 = sumDivPrefix(body, QStringLiteral("col1"));
    const double sum2 = sumDivPrefix(body, QStringLiteral("col2"));
    const int sum3 = static_cast<int>(sum1 + sum2);
    const QString sum1Text = QString::number(static_cast<int>(sum1));
    const QString sum2Text = QString::number(static_cast<int>(sum2));
    const QString sum3Text = QString::number(sum3);
    const QString vivodText = sum3Text + QStringLiteral("(36)");

    body = replaceDivInnerById(body, QStringLiteral("sum1"), sum1Text);
    body = replaceDivInnerById(body, QStringLiteral("sum2"), sum2Text);
    body = replaceDivInnerById(body, QStringLiteral("sum3"), sum3Text);
    body = replaceDivInnerById(body, QStringLiteral("idvivod"), vivodText);
    return body;
}

QString ExerciseProtocol::mergeEditorHtmlIntoStoredBody(
    const QString &storedBody,
    const QString &editorHtml,
    int protocolIndex) {
    if (storedBody.trimmed().isEmpty()) {
        return storedBody;
    }

    ParsedProtocolFields parsed = parseProtocolFieldsFromHtml(editorHtml, protocolIndex);
    if (!parsed.hasDateSpecialist && !parsed.hasResult && !parsed.hasNote && parsed.answersByIndex.isEmpty()) {
        const QString section = extractProtocolSectionFromEditor(editorHtml, protocolIndex);
        if (!section.trimmed().isEmpty()) {
            parsed = parseProtocolFieldsFromHtml(section, 0);
        }
    }

    return applyParsedFieldsToStoredBody(storedBody, parsed);
}

QString ExerciseProtocol::appendRowsToStoredBody(const QString &existingBody, const QString &rowsHtml) {
    QString addition = rowsHtml.trimmed();
    if (addition.isEmpty()) {
        return existingBody;
    }
    if (!addition.startsWith(QStringLiteral("<tr"), Qt::CaseInsensitive)
        && addition.startsWith(QStringLiteral("<td"), Qt::CaseInsensitive)) {
        addition.prepend(QStringLiteral("<tr>"));
    }
    if (addition.startsWith(QStringLiteral("<tr"), Qt::CaseInsensitive)
        && !addition.contains(QStringLiteral("</tr>"), Qt::CaseInsensitive)) {
        addition.append(QStringLiteral("</tr>"));
    }

    QString base = existingBody;
    if (base.trimmed().isEmpty()) {
        return addition;
    }

    // Обрезаем «хвост» после последнего полного </tr>, если там остались незакрытые теги
    // (типичная причина вложения нового протокола внутрь последней ячейки).
    const int lastTrEnd = base.lastIndexOf(QStringLiteral("</tr>"), -1, Qt::CaseInsensitive);
    if (lastTrEnd >= 0) {
        const int afterPos = lastTrEnd + QStringLiteral("</tr>").size();
        const QString after = base.mid(afterPos);
        const bool hasCompleteTail =
            after.contains(QStringLiteral("</table>"), Qt::CaseInsensitive)
            || after.contains(QStringLiteral("<tr"), Qt::CaseInsensitive)
            || after.contains(QStringLiteral("<!--"), Qt::CaseInsensitive)
            || after.contains(QStringLiteral("<table"), Qt::CaseInsensitive);
        if (!after.trimmed().isEmpty() && !hasCompleteTail) {
            base = base.left(afterPos);
        }
    }

    // Если таблица результатов после <!--s--> уже закрыта — вставляем строки перед </table>.
    const int marker = base.indexOf(QStringLiteral("<!--s-->"));
    if (marker >= 0) {
        const int tableClose = base.lastIndexOf(QStringLiteral("</table>"), -1, Qt::CaseInsensitive);
        if (tableClose > marker) {
            return base.left(tableClose) + addition + base.mid(tableClose);
        }
    }

    return base + addition;
}

QString ExerciseProtocol::appendFullSessionToStoredBody(
    const QString &existingBody,
    const QString &sessionHtml) {
    QString session = sessionHtml.trimmed();
    if (session.isEmpty()) {
        return existingBody;
    }

    // 1.26: не ensureClosed/joinClosed — иначе срезаются таблицы баллов.
    if (looksLikeProtocol126Body(existingBody) || looksLikeProtocol126Body(session)) {
        if (!session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
            session.prepend(protocolSummaryTableOpenHtml());
        }
        if (existingBody.trimmed().isEmpty()) {
            return stripLeadingSummaryTableWrapper(session);
        }
        return existingBody + session;
    }

    session = ensureClosedProtocolSession(session);
    if (session.isEmpty()) {
        return existingBody;
    }
    if (!session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
        session.prepend(protocolSummaryTableOpenHtml());
    }

    if (existingBody.trimmed().isEmpty()) {
        // Первая сессия в БД хранится без обёртки <table> (её даёт header.html).
        return ensureClosedProtocolSession(stripLeadingSummaryTableWrapper(session));
    }

    // Пересобираем все уже сохранённые сессии в плоский список закрытых блоков —
    // иначе 3-й+ протокол вкладывается в незакрытую таблицу после merge/QTextDocument.
    QStringList sessions = extractProtocolBodiesByDateRows(existingBody);
    if (sessions.isEmpty()) {
        sessions.append(existingBody);
    }
    QString base = joinClosedProtocolSessions(sessions);
    return base + session;
}

QString ExerciseProtocol::flattenStoredProtocolBody(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    QStringList sessions = extractProtocolBodiesByDateRows(protocolBody);
    if (sessions.isEmpty()) {
        return ensureClosedProtocolSession(protocolBody);
    }
    // Первая сессия без ведущего <table> — шапка методики уже открывает таблицу.
    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = ensureClosedProtocolSession(sessions.at(i));
        if (session.isEmpty()) {
            continue;
        }
        if (i > 0 && !session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
            session.prepend(protocolSummaryTableOpenHtml());
        } else if (i == 0) {
            session = stripLeadingSummaryTableWrapper(session);
            session = ensureClosedProtocolSession(session);
        }
        result += session;
    }
    return result;
}
