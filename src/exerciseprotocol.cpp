#include "exerciseprotocol.h"

#include "exerciseassets.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>
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
            parts << QStringLiteral("&nbsp;&nbsp;%1").arg(trimmed.toHtmlEscaped());
        }
    }
    return parts.join(QStringLiteral("<br>"));
}

QString protocolSummaryTableOpenHtml() {
    return QStringLiteral(
        "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
        "<colgroup><col width='165'><col width='506'></colgroup>");
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
    return QStringLiteral("<span id=\"dokit-pid-%1-start\" style=\"font-size:0pt;line-height:0;\">\uFEFF</span>")
        .arg(protocolId);
}

QString protocolRecordEndMarker(const QString &protocolId) {
    return QStringLiteral("<span id=\"dokit-pid-%1-end\" style=\"font-size:0pt;line-height:0;\">\uFEFF</span>")
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
    return rebuildResultsTableSection(body, answers, nullptr);
}

QString htmlFragmentToPlainText(const QString &html) {
    if (html.trimmed().isEmpty()) {
        return {};
    }
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText().trimmed();
}

QString readTableCellText(QTextTable *table, int row, int column) {
    if (!table || row < 0 || column < 0 || row >= table->rows() || column >= table->columns()) {
        return {};
    }
    const QTextTableCell cell = table->cellAt(row, column);
    if (!cell.isValid()) {
        return {};
    }
    QTextCursor cursor = cell.firstCursorPosition();
    cursor.setPosition(cell.lastCursorPosition().position(), QTextCursor::KeepAnchor);
    QString text = cursor.selectedText();
    text.replace(QChar(0x2029), QLatin1Char(' '));
    text.replace(QChar::ParagraphSeparator, QLatin1Char(' '));
    return text.trimmed();
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
                    fields.activity = readTableCellText(table, row, 2);
                    fields.help = readTableCellText(table, row, 3);
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

QString applyParsedFieldsToStoredBody(const QString &storedBody, const ParsedProtocolFields &parsed) {
    QString result = normalizeStoredProtocolBody(storedBody);

    if (parsed.hasDateSpecialist) {
        result = replaceRowSecondCell(result, QStringLiteral("Дата/специалист"), parsed.dateSpecialist);
    }
    if (parsed.hasResult) {
        result = replaceRowSecondCell(result, QStringLiteral("Результат: вывод об уровне развития"), parsed.resultText);
    }
    if (parsed.hasNote) {
        result = replaceRowSecondCell(result, QStringLiteral("Примечание"), parsed.noteText);
    }

    if (result.contains(QStringLiteral("<!--s-->"))) {
        result = rebuildResultsTableSection(result, QList<bool>(), &parsed);
    }

    return result;
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

QString ExerciseProtocol::patientProtocolBody(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    return ensureProtocol12SummaryTableOpens(stripSpecialistSections(protocolBody));
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
    const CheckboxValues &checkboxes) {
    Q_UNUSED(elapsedSeconds);
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
        return ensureProtocol12SummaryTableOpens(existingProtocolHtml) + protocolSummaryTableOpenHtml() + sessionPart;
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
    return protocolRecordStartMarker(protocolId) + protocolBody + protocolRecordEndMarker(protocolId);
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

    ParsedProtocolFields parsed = parseProtocolFieldsFromDocument(editorDocument, protocolIndex);
    if (!parsed.hasDateSpecialist && !parsed.hasResult && !parsed.hasNote && parsed.answersByIndex.isEmpty()) {
        parsed = parseProtocolFieldsFromHtml(editorDocument->toHtml(), protocolIndex);
    }
    return applyParsedFieldsToStoredBody(storedBody, parsed);
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
