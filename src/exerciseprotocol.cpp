#include "exerciseprotocolcreate.h"

#include "exerciseassets.h"
#include "exerciseconfig.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFragment>
#include <QTextFrame>
#include <QTextLength>
#include <QTextTable>
#include <QTextTableCellFormat>
#include <QTextTableFormat>
#include <QVector>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <utility>

static int findProtocol126Task2RowStart(const QString &rows) {
    const QRegularExpression re(
        QStringLiteral(
            "<tr\\b[^>]*>\\s*<td\\b[^>]*>\\s*(?:<[^>]*>\\s*)*Задание\\s*2\\b"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m = re.match(rows);
    return m.hasMatch() ? m.capturedStart() : -1;
}

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
        return QStringLiteral("&nbsp;");
    }
    // Line/ParagraphSeparator → \n до split (в QRegularExpression \uXXXX невалиден в Qt/PCRE).
    QString normalized = text;
    normalized.replace(QChar::LineSeparator, QLatin1Char('\n'));
    normalized.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    const QStringList lines =
        normalized.split(QRegularExpression(QStringLiteral("[\\r\\n;]+")), Qt::SkipEmptyParts);
    QStringList parts;
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            // Без ведущих &nbsp;: иначе на «Протоколы» правки с клавиатуры в уже
            // заполненных из чекбоксов ячейках теряются при roundtrip QTextDocument.
            parts << trimmed.toHtmlEscaped();
        }
    }
    return parts.isEmpty() ? QStringLiteral("&nbsp;") : parts.join(QStringLiteral("<br>"));
}

QString protocolSummaryTableOpenHtml() {
    // Как в header.html методик: 200/471 — иначе 2-я сессия (новая <table>)
    // с 165/506 визуально не совпадает с первой (продолжает шапку).
    return QStringLiteral(
        "<table border='1' style='table-layout:fixed;width:671px' cellspacing='0' cellpadding='0' width='671'>"
        "<colgroup>"
        "<col width='200' style='width:200px'>"
        "<col width='471' style='width:471px'>"
        "</colgroup>");
}

QString stripLeadingSummaryTableWrapper(QString chunk) {
    QString trimmed = chunk.trimmed();
    const QRegularExpression wrapRe(
        QStringLiteral("^<table\\b[^>]*>\\s*<colgroup>[\\s\\S]*?</colgroup>\\s*"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    return trimmed.replace(wrapRe, QString());
}

QString summaryRowHtml(const QString &label, const QString &valueHtml) {
    return QStringLiteral("<tr><td width='200' valign='top'>%1</td><td width='471' valign='top'>%2</td></tr>")
        .arg(label, valueHtml);
}

QString normalizeSummaryColumnWidthsHtml(QString body);

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
    // <a id> лучше переживает QTextDocument::setHtml/toHtml, чем пустой span/комментарии.
    return QStringLiteral("<a id=\"dokit-pid-%1-start\" name=\"dokit-pid-%1-start\"></a>")
        .arg(protocolId);
}

QString protocolRecordEndSpan(const QString &protocolId) {
    return QStringLiteral("<a id=\"dokit-pid-%1-end\" name=\"dokit-pid-%1-end\"></a>")
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

// Конец сессии — перед <tr> следующей «Дата/специалист», но без захвата
// открывающего <table> следующей сессии (иначе 3-й+ протокол вкладывается в ячейку).
int sessionEndBeforeNextDateRow(const QString &body, int rowStart, int nextRowStart) {
    int endPos = nextRowStart;
    if (endPos <= rowStart) {
        return endPos;
    }
    const QString beforeNext = body.left(endPos);
    const int lastTableOpen =
        beforeNext.lastIndexOf(QStringLiteral("<table"), -1, Qt::CaseInsensitive);
    const int lastTableClose =
        beforeNext.lastIndexOf(QStringLiteral("</table>"), -1, Qt::CaseInsensitive);
    if (lastTableOpen >= rowStart && lastTableOpen > lastTableClose) {
        endPos = lastTableOpen;
    }
    while (endPos > rowStart) {
        const QChar ch = body.at(endPos - 1);
        if (ch.isSpace()) {
            --endPos;
            continue;
        }
        break;
    }
    return endPos;
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
    // Qt: Enter → ParagraphSeparator, <br> в ячейке → LineSeparator; оба нужны как \n,
    // иначе при сохранении OR/HLP строки склеиваются в одну.
    plain.replace(QChar::LineSeparator, QLatin1Char('\n'));
    plain.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    plain.replace(QChar(0x2028), QLatin1Char('\n'));
    plain.replace(QChar(0x2029), QLatin1Char('\n'));
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
    text.replace(QChar::LineSeparator, QLatin1Char('\n'));
    text.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    text.replace(QChar(0x2028), QLatin1Char('\n'));
    text.replace(QChar(0x2029), QLatin1Char('\n'));
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
                // Qt мог вложить таблицу стимулов 4.1.8 в ячейку «Примечание».
                if (fields.noteText.contains(QStringLiteral("Стимульные"), Qt::CaseInsensitive)
                    || fields.noteText.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)) {
                    fields.noteText.clear();
                    // Не считаем «пустым примечанием» — иначе merge затрёт реальный текст.
                    fields.hasNote = false;
                }
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
    // Допускаем обёртки вроде <p>Примечание</p> внутри td.
    const QRegularExpression rowRe(
        QStringLiteral(
            "(<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?%1[\\s\\S]*?</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)")
            .arg(escapedLabel),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QString inner;
    if (rowLabel.contains(QStringLiteral("Примечание"), Qt::CaseInsensitive)
        && body.contains(QStringLiteral("idnote"), Qt::CaseInsensitive)) {
        inner = QStringLiteral("<div contenteditable='true' id='idnote'>%1</div>")
                    .arg(plainText.toHtmlEscaped());
    } else if (body.contains(QStringLiteral("contenteditable"), Qt::CaseInsensitive)) {
        inner = QStringLiteral("<div contenteditable='true'>%1</div>").arg(plainText.toHtmlEscaped());
    } else {
        inner = plainText.toHtmlEscaped();
    }
    // Последнее совпадение (актуальная сессия).
    QRegularExpressionMatchIterator it = rowRe.globalMatch(body);
    QRegularExpressionMatch match;
    while (it.hasNext()) {
        match = it.next();
    }
    if (!match.hasMatch()) {
        return body;
    }
    return body.left(match.capturedStart())
        + match.captured(1) + inner + match.captured(3)
        + body.mid(match.capturedEnd());
}

QString replaceResultRowSecondCell(QString body, const QString &plainText) {
    // У разных методик подписи отличаются: «Результат: вывод…» / «Результат: баллы…» и т.п.
    // В подписи может быть <br> — нельзя резать по [^<]*.
    const QRegularExpression rowRe(
        QStringLiteral(
            "(<tr[^>]*>\\s*<td[^>]*>\\s*Результат[\\s\\S]*?</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    // Всегда сохраняем id='idvivod' — иначе «Подвести итог» не находит ячейку Результат.
    const QString inner = body.contains(QStringLiteral("contenteditable"), Qt::CaseInsensitive)
                              ? QStringLiteral("<div contenteditable='true' id='idvivod'>%1</div>")
                                    .arg(plainText.toHtmlEscaped())
                              : plainText.toHtmlEscaped();
    // Только первое совпадение в переданном фрагменте (обычно одна сессия).
    const QRegularExpressionMatch match = rowRe.match(body);
    if (!match.hasMatch()) {
        return body;
    }
    return body.left(match.capturedStart())
        + match.captured(1) + inner + match.captured(3)
        + body.mid(match.capturedEnd());
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

// Гарантирует: summary закрыт перед <!--s-->, таблица(ы) процесса закрыты, без вложенных <table>.
// У 4.1.8 после <!--s--> две последовательные таблицы (характер + стимульные слова) — обе сохраняем.
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

    QString afterMarker = session.mid(marker + QStringLiteral("<!--s-->").size()).trimmed();
    // Убрать пустой хвост до таблицы процесса (разрыв между summary и заданием).
    afterMarker.replace(
        QRegularExpression(
            QStringLiteral("^(?:\\s|<br\\s*/?>|<p\\b[^>]*>\\s*(?:&nbsp;|\\s)*\\s*</p\\s*>)+"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    const int tableStart = afterMarker.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
    if (tableStart < 0) {
        return summary + QStringLiteral("</table><!--s-->");
    }
    const QString leading = afterMarker.left(tableStart);
    QString results = afterMarker.mid(tableStart);

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
        // Соседние <table> после первой (не вложенные): характер + стимулы 4.1.8 и т.п.
        QString rest = results.mid(firstClose + QStringLiteral("</table>").size());
        while (true) {
            const int nextOpen = rest.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
            if (nextOpen < 0) {
                break;
            }
            const int nextClose =
                rest.indexOf(QStringLiteral("</table>"), nextOpen, Qt::CaseInsensitive);
            if (nextClose < 0) {
                break;
            }
            processTable += rest.left(nextClose + QStringLiteral("</table>").size());
            rest = rest.mid(nextClose + QStringLiteral("</table>").size());
        }
    } else {
        const int lastTr = results.lastIndexOf(QStringLiteral("</tr>"), -1, Qt::CaseInsensitive);
        if (lastTr >= 0) {
            processTable = results.left(lastTr + QStringLiteral("</tr>").size());
        } else {
            processTable = results;
        }
        processTable += QStringLiteral("</table>");
    }

    return summary + QStringLiteral("</table><!--s-->") + leading + processTable;
}

QString joinClosedProtocolSessions(const QStringList &sessions) {
    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = ensureClosedProtocolSession(sessions.at(i));
        if (session.isEmpty()) {
            continue;
        }
        session.replace(
            QRegularExpression(
                QStringLiteral("^(?:\\s|<br\\s*/?>|<p\\b[^>]*>\\s*(?:&nbsp;|\\s)*\\s*</p\\s*>)+"),
                QRegularExpression::CaseInsensitiveOption),
            QString());
        if (i > 0) {
            result.replace(
                QRegularExpression(
                    QStringLiteral("(?:\\s|<br\\s*/?>|<p\\b[^>]*>\\s*(?:&nbsp;|\\s)*\\s*</p\\s*>)+$"),
                    QRegularExpression::CaseInsensitiveOption),
                QString());
            if (!session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
                session.prepend(protocolSummaryTableOpenHtml());
            }
        }
        result += session;
    }
    return normalizeSummaryColumnWidthsHtml(result);
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

// Задание 1: 3 колонки эмоций (142+469+60); «Процесс»/«Задание 1» — colspan=3;
// «Характер»/«Виды помощи» — ровно 2 ячейки (200+471).
QString applyProtocol126Task1CellWidths(QString html) {
    html.replace(
        QRegularExpression(
            QStringLiteral("<colgroup\\b[^>]*>[\\s\\S]*?</colgroup\\s*>"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption),
        QString());
    html.replace(
        QRegularExpression(QStringLiteral("<col\\b[^>]*/?>"), QRegularExpression::CaseInsensitiveOption),
        QString());

    auto setWidth = [](QString attrs, const QString &w) {
        attrs.remove(QRegularExpression(
            QStringLiteral("\\s*width\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
            QRegularExpression::CaseInsensitiveOption));
        return QStringLiteral(" width='%1'%2").arg(w, attrs);
    };
    auto stripWidth = [](QString attrs) {
        attrs.remove(QRegularExpression(
            QStringLiteral("\\s*width\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
            QRegularExpression::CaseInsensitiveOption));
        return attrs;
    };
    auto setColspan = [](QString attrs, int span) {
        attrs.remove(QRegularExpression(
            QStringLiteral("\\s*colspan\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
            QRegularExpression::CaseInsensitiveOption));
        return QStringLiteral(" colspan='%1'%2").arg(span).arg(attrs);
    };
    auto hasColspan = [](const QString &attrs) {
        return attrs.contains(QRegularExpression(
            QStringLiteral("colspan\\s*="), QRegularExpression::CaseInsensitiveOption));
    };

    const QRegularExpression trRe(
        QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QString rebuilt;
    int trLast = 0;
    QRegularExpressionMatchIterator trIt = trRe.globalMatch(html);
    while (trIt.hasNext()) {
        const QRegularExpressionMatch tr = trIt.next();
        rebuilt += html.mid(trLast, tr.capturedStart() - trLast);
        QString rowInner = tr.captured(2);
        const QRegularExpression tdRe(
            QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QList<QRegularExpressionMatch> tds;
        QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
        while (tdIt.hasNext()) {
            tds.append(tdIt.next());
        }

        const QString rowText = rowInner;
        const bool isProcessOrTaskTitle =
            rowText.contains(QStringLiteral("Процесс выполнения"), Qt::CaseInsensitive)
            || rowText.contains(QRegularExpression(
                   QStringLiteral("Задание\\s*1\\b"), QRegularExpression::CaseInsensitiveOption));
        const bool isCharacterOrHelp =
            rowText.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
            || rowText.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive);

        if (!tds.isEmpty()) {
            QString newRow;
            int cellLast = 0;
            for (int i = 0; i < tds.size(); ++i) {
                const QRegularExpressionMatch &td = tds.at(i);
                newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                QString attrs = td.captured(2);

                if (isProcessOrTaskTitle && tds.size() == 1) {
                    // На всю ширину трёх колонок.
                    attrs = setColspan(stripWidth(attrs), 3);
                } else if (isCharacterOrHelp && tds.size() == 2) {
                    // Две ячейки: подпись 200 + данные на остаток (colspan=2).
                    if (i == 0) {
                        attrs.remove(QRegularExpression(
                            QStringLiteral("\\s*colspan\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
                            QRegularExpression::CaseInsensitiveOption));
                        attrs = setWidth(attrs, QStringLiteral("200"));
                    } else {
                        attrs = setColspan(stripWidth(attrs), 2);
                    }
                } else if (tds.size() == 3 && !hasColspan(tds.at(0).captured(2))
                           && !hasColspan(tds.at(1).captured(2))
                           && !hasColspan(tds.at(2).captured(2))) {
                    // 200+411+60=671 — первая колонка = ширина «Характер»/«Виды помощи».
                    static const char *const kW[] = {"200", "411", "60"};
                    attrs = setWidth(attrs, QString::fromUtf8(kW[i]));
                } else if (tds.size() == 3 && i == 1 && hasColspan(attrs)) {
                    // Старый ответ с colspan=2 → одна ячейка 411.
                    attrs.remove(QRegularExpression(
                        QStringLiteral("\\s*colspan\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
                        QRegularExpression::CaseInsensitiveOption));
                    attrs = setWidth(attrs, QStringLiteral("411"));
                } else if (tds.size() == 2 && hasColspan(tds.at(0).captured(2)) == false
                           && hasColspan(tds.at(1).captured(2))) {
                    // Итог colspan=2 + баллы.
                    if (i == 0) {
                        attrs = setColspan(stripWidth(attrs), 2);
                    } else {
                        attrs = setWidth(stripWidth(attrs), QStringLiteral("60"));
                    }
                } else {
                    attrs = stripWidth(attrs);
                }

                newRow += td.captured(1) + attrs + td.captured(3) + td.captured(4) + td.captured(5);
                cellLast = td.capturedEnd();
            }
            newRow += rowInner.mid(cellLast);
            rowInner = newRow;
        }
        rebuilt += tr.captured(1) + rowInner + tr.captured(3);
        trLast = tr.capturedEnd();
    }
    rebuilt += html.mid(trLast);
    rebuilt.prepend(QStringLiteral(
        "<colgroup><col width='200'><col width='411'><col width='60'></colgroup>"));
    return rebuilt;
}

QString applyProtocol126Task2OrHlpWidths(QString html) {
    // Отдельная 2-колоночная таблица: 200+471=671 (как шапка).
    html.replace(
        QRegularExpression(
            QStringLiteral("<colgroup\\b[^>]*>[\\s\\S]*?</colgroup\\s*>"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption),
        QString());
    html.replace(
        QRegularExpression(QStringLiteral("<col\\b[^>]*/?>"), QRegularExpression::CaseInsensitiveOption),
        QString());

    const QRegularExpression trRe(
        QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QString rebuilt;
    int trLast = 0;
    QRegularExpressionMatchIterator trIt = trRe.globalMatch(html);
    while (trIt.hasNext()) {
        const QRegularExpressionMatch tr = trIt.next();
        rebuilt += html.mid(trLast, tr.capturedStart() - trLast);
        QString rowInner = tr.captured(2);
        const QRegularExpression tdRe(
            QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QList<QRegularExpressionMatch> tds;
        QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
        while (tdIt.hasNext()) {
            tds.append(tdIt.next());
        }
        if (tds.size() == 1) {
            rowInner = QStringLiteral("<td colspan='2' align='center'>%1</td>").arg(tds.at(0).captured(4));
        } else if (tds.size() >= 2) {
            QString data = tds.at(1).captured(4);
            for (int i = 2; i < tds.size(); ++i) {
                data += tds.at(i).captured(4);
            }
            rowInner = QStringLiteral(
                           "<td width='200' valign='top'>%1</td>"
                           "<td width='471' valign='top' align='left'>%2</td>")
                           .arg(tds.at(0).captured(4), data);
        }
        rebuilt += tr.captured(1) + rowInner + tr.captured(3);
        trLast = tr.capturedEnd();
    }
    rebuilt += html.mid(trLast);
    rebuilt.prepend(QStringLiteral(
        "<colgroup><col width='200'><col width='471'></colgroup>"));
    return rebuilt;
}

QString applyProtocol126Task2CellWidths(QString html) {
    // Только сетка рассказов 70+120+421+60. Без width=200 на «Характер».
    html.replace(
        QRegularExpression(
            QStringLiteral("<colgroup\\b[^>]*>[\\s\\S]*?</colgroup\\s*>"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption),
        QString());
    html.replace(
        QRegularExpression(QStringLiteral("<col\\b[^>]*/?>"), QRegularExpression::CaseInsensitiveOption),
        QString());

    auto setWidth = [](QString attrs, const QString &w) {
        attrs.remove(QRegularExpression(
            QStringLiteral("\\s*width\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
            QRegularExpression::CaseInsensitiveOption));
        return QStringLiteral(" width='%1'%2").arg(w, attrs);
    };
    auto stripWidth = [](QString attrs) {
        attrs.remove(QRegularExpression(
            QStringLiteral("\\s*width\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
            QRegularExpression::CaseInsensitiveOption));
        return attrs;
    };
    auto hasColspan = [](const QString &attrs) {
        return attrs.contains(QRegularExpression(
            QStringLiteral("colspan\\s*="), QRegularExpression::CaseInsensitiveOption));
    };

    const QRegularExpression trRe(
        QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QString rebuilt;
    int trLast = 0;
    QRegularExpressionMatchIterator trIt = trRe.globalMatch(html);
    while (trIt.hasNext()) {
        const QRegularExpressionMatch tr = trIt.next();
        rebuilt += html.mid(trLast, tr.capturedStart() - trLast);
        QString rowInner = tr.captured(2);
        const QRegularExpression tdRe(
            QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QList<QRegularExpressionMatch> tds;
        QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
        while (tdIt.hasNext()) {
            tds.append(tdIt.next());
        }
        const bool fourPlain = tds.size() == 4
            && !hasColspan(tds.at(0).captured(2))
            && !hasColspan(tds.at(1).captured(2))
            && !hasColspan(tds.at(2).captured(2))
            && !hasColspan(tds.at(3).captured(2));
        if (!tds.isEmpty()) {
            QString newRow;
            int cellLast = 0;
            for (int i = 0; i < tds.size(); ++i) {
                const QRegularExpressionMatch &td = tds.at(i);
                newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                QString attrs = td.captured(2);
                if (fourPlain) {
                    static const char *const kW[] = {"70", "120", "421", "60"};
                    attrs = setWidth(attrs, QString::fromUtf8(kW[i]));
                } else if (tds.size() == 1) {
                    attrs = stripWidth(attrs);
                    if (!hasColspan(attrs)) {
                        attrs = QStringLiteral(" colspan='4'") + attrs;
                    }
                } else {
                    attrs = stripWidth(attrs);
                }
                newRow += td.captured(1) + attrs + td.captured(3) + td.captured(4) + td.captured(5);
                cellLast = td.capturedEnd();
            }
            newRow += rowInner.mid(cellLast);
            rowInner = newRow;
        }
        rebuilt += tr.captured(1) + rowInner + tr.captured(3);
        trLast = tr.capturedEnd();
    }
    rebuilt += html.mid(trLast);
    rebuilt.prepend(QStringLiteral(
        "<colgroup><col width='70'><col width='120'><col width='421'><col width='60'></colgroup>"));
    return rebuilt;
}

int findProtocol126Task2StoriesStart(const QString &rows) {
    // Маркер шапки рассказов; учитываем &nbsp;/&#160; между «№» и «рассказа».
    const QRegularExpression byNumber(
        QStringLiteral(
            "<tr\\b[^>]*>[\\s\\S]*?№(?:\\s|&nbsp;|&#160;)*рассказа"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m1 = byNumber.match(rows);
    if (m1.hasMatch()) {
        return m1.capturedStart();
    }
    const QRegularExpression byHeader(
        QStringLiteral(
            "<tr\\b[^>]*>[\\s\\S]*?Правильный\\s+ответ[\\s\\S]*?Баллы"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch m2 = byHeader.match(rows);
    return m2.hasMatch() ? m2.capturedStart() : -1;
}

// Режет склейку «Задание 1+2» и «OR/HLP + рассказы» на отдельные куски строк.
QStringList splitProtocol126FlatTaskParts(const QStringList &taskParts) {
    QStringList flat;
    for (const QString &part : taskParts) {
        QStringList chunks;
        const int task2At = findProtocol126Task2RowStart(part);
        if (task2At > 0) {
            const QString left = part.left(task2At).trimmed();
            const QString right = part.mid(task2At).trimmed();
            if (!left.isEmpty()) {
                chunks.append(left);
            }
            if (!right.isEmpty()) {
                chunks.append(right);
            }
        } else if (!part.trimmed().isEmpty()) {
            chunks.append(part.trimmed());
        }
        for (const QString &chunk : chunks) {
            const int storiesAt = findProtocol126Task2StoriesStart(chunk);
            const bool hasOrHlp =
                chunk.contains(QStringLiteral("Задание 2"), Qt::CaseInsensitive)
                || chunk.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
                || chunk.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive);
            if (storiesAt > 0 && hasOrHlp) {
                const QString left = chunk.left(storiesAt).trimmed();
                const QString right = chunk.mid(storiesAt).trimmed();
                if (!left.isEmpty()) {
                    flat.append(left);
                }
                if (!right.isEmpty()) {
                    flat.append(right);
                }
            } else {
                flat.append(chunk);
            }
        }
    }
    return flat;
}

int protocol126ProcessTitleColspan(const QString &rows) {
    if (rows.contains(QStringLiteral("№ рассказа"), Qt::CaseInsensitive)
        || rows.contains(QRegularExpression(
               QStringLiteral("id\\s*=\\s*['\"]col2"), QRegularExpression::CaseInsensitiveOption))) {
        return 4;
    }
    if (rows.contains(QStringLiteral("Задание 2"), Qt::CaseInsensitive)
        || ((rows.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
             || rows.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive))
            && !rows.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive))) {
        return 2;
    }
    return 3;
}

QString applyProtocol126ProcessCellWidths(QString html) {
    const bool task2 = html.contains(QStringLiteral("№ рассказа"), Qt::CaseInsensitive)
        || html.contains(QRegularExpression(
               QStringLiteral("id\\s*=\\s*['\"]col2"), QRegularExpression::CaseInsensitiveOption))
        || (html.contains(QStringLiteral("Задание 2"), Qt::CaseInsensitive)
            && !html.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive));
    if (task2) {
        if (!html.contains(QStringLiteral("№ рассказа"), Qt::CaseInsensitive)) {
            return applyProtocol126Task2OrHlpWidths(html);
        }
        return applyProtocol126Task2CellWidths(html);
    }
    return applyProtocol126Task1CellWidths(html);
}

QString normalizeSummaryColumnWidthsHtml(QString body) {
    // Перед <!--s--> всегда </table>: иначе 3-кол. процесс «Сказки» (и др.) вливается
    // в 2-кол. шапку → пустая третья колонка у «Методика» / «Цель» / «Процесс…».
    {
        const QString marker = QStringLiteral("<!--s-->");
        int pos = 0;
        while ((pos = body.indexOf(marker, pos)) >= 0) {
            int trimEnd = pos;
            while (trimEnd > 0
                   && (body.at(trimEnd - 1).isSpace() || body.at(trimEnd - 1) == QChar(0xA0))) {
                --trimEnd;
            }
            const QString before = body.left(trimEnd);
            if (!before.endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
                body.insert(pos, QStringLiteral("</table>"));
                pos += QStringLiteral("</table>").size() + marker.size();
            } else {
                pos += marker.size();
            }
        }
    }

    // Убрать пустые разрывы между соседними таблицами (повторные сессии на «Протоколы»).
    body.replace(
        QRegularExpression(
            QStringLiteral(
                "(</table\\s*>)\\s*(?:<br\\s*/?>|<p\\b[^>]*>\\s*(?:&nbsp;|\\s|<br\\s*/?>)*\\s*</p\\s*>|"
                "<div\\b[^>]*>\\s*(?:&nbsp;|\\s|<br\\s*/?>)*\\s*</div\\s*>|\\s|&nbsp;)+(?=<table\\b)"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("\\1"));

    // Как header.html: первая сессия наследует 200/471; повторные таблицы — те же.
    body.replace(QStringLiteral("<col width='165'>"), QStringLiteral("<col width='200'>"));
    body.replace(QStringLiteral("<col width=\"165\">"), QStringLiteral("<col width=\"200\">"));
    body.replace(QStringLiteral("<col width='192'>"), QStringLiteral("<col width='200'>"));
    body.replace(QStringLiteral("<col width=\"192\">"), QStringLiteral("<col width=\"200\">"));
    body.replace(QStringLiteral("<col width='506'>"), QStringLiteral("<col width='471'>"));
    body.replace(QStringLiteral("<col width=\"506\">"), QStringLiteral("<col width=\"471\">"));
    body.replace(QStringLiteral("<col width='479'>"), QStringLiteral("<col width='471'>"));
    body.replace(QStringLiteral("<col width=\"479\">"), QStringLiteral("<col width=\"471\">"));

    auto setAttrWidth = [](QString attrs, const QString &w) {
        const QRegularExpression widthRe(
            QStringLiteral("\\s*width\\s*=\\s*['\"][^'\"]*['\"]"),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpression colspanRe(
            QStringLiteral("\\s*colspan\\s*=\\s*['\"][^'\"]*['\"]"),
            QRegularExpression::CaseInsensitiveOption);
        attrs.remove(widthRe);
        attrs.remove(colspanRe);
        return QStringLiteral(" width='%1'%2").arg(w, attrs);
    };

    // Все <table> протокола — ровно 671px.
    {
        const QRegularExpression tableRe(
            QStringLiteral("(<table\\b)([^>]*)(>)"),
            QRegularExpression::CaseInsensitiveOption);
        QString out;
        out.reserve(body.size() + 64);
        int last = 0;
        QRegularExpressionMatchIterator it = tableRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            QString attrs = m.captured(2);
            const QRegularExpression widthRe(
                QStringLiteral("\\s*width\\s*=\\s*['\"][^'\"]*['\"]"),
                QRegularExpression::CaseInsensitiveOption);
            attrs.remove(widthRe);
            if (!attrs.contains(QStringLiteral("table-layout"), Qt::CaseInsensitive)) {
                if (attrs.contains(QStringLiteral("style="), Qt::CaseInsensitive)) {
                    attrs.replace(
                        QRegularExpression(
                            QStringLiteral("style\\s*=\\s*['\"]"),
                            QRegularExpression::CaseInsensitiveOption),
                        QStringLiteral("style='table-layout:fixed; "));
                } else {
                    attrs += QStringLiteral(" style='table-layout:fixed'");
                }
            }
            attrs += QStringLiteral(" width='671'");
            out += m.captured(1) + attrs + m.captured(3);
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    // Шапка «Методика…»: всегда colgroup 200/471 (иначе Qt сжимает по длинной «Цели»).
    {
        const QRegularExpression headerTableRe(
            QStringLiteral(
                "(<table\\b[^>]*>)([\\s\\S]*?Методика[\\s\\S]*?)(</table>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        out.reserve(body.size() + 64);
        int last = 0;
        QRegularExpressionMatchIterator it = headerTableRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            QString open = m.captured(1);
            QString inner = m.captured(2);
            // Не трогать таблицы процесса (там тоже может встретиться слово в тексте).
            if (inner.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive)
                || inner.contains(QStringLiteral("№ рассказа"), Qt::CaseInsensitive)
                || inner.contains(QStringLiteral("Задание 1"), Qt::CaseInsensitive)
                || inner.contains(QStringLiteral("Задание 2"), Qt::CaseInsensitive)
                // 5.4.2: если процесс влился в шапку — не ставить colgroup 200/471 на 3-кол. таблицу.
                || (inner.contains(QStringLiteral("Вопросы"), Qt::CaseInsensitive)
                    && (inner.contains(QStringLiteral("Ответы ребенка"), Qt::CaseInsensitive)
                        || inner.contains(QStringLiteral("Ответы"), Qt::CaseInsensitive))
                    && inner.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive))) {
                out += m.captured(0);
                last = m.capturedEnd();
                continue;
            }
            inner.remove(QRegularExpression(
                QStringLiteral("<colgroup\\b[\\s\\S]*?</colgroup>\\s*"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
            inner.prepend(QStringLiteral(
                "<colgroup><col width='200'><col width='471'></colgroup>"));
            // Явно фиксируем ширину шапки (атрибут + style), иначе Qt сжимает по «Цели».
            {
                QString attrs = open.mid(6); // после "<table"
                if (attrs.endsWith(QLatin1Char('>'))) {
                    attrs.chop(1);
                }
                attrs.remove(QRegularExpression(
                    QStringLiteral("\\s*width\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
                    QRegularExpression::CaseInsensitiveOption));
                if (!attrs.contains(QStringLiteral("table-layout"), Qt::CaseInsensitive)) {
                    if (attrs.contains(QStringLiteral("style="), Qt::CaseInsensitive)) {
                        attrs.replace(
                            QRegularExpression(
                                QStringLiteral("style\\s*=\\s*['\"]"),
                                QRegularExpression::CaseInsensitiveOption),
                            QStringLiteral("style='table-layout:fixed;width:671px; "));
                    } else {
                        attrs += QStringLiteral(" style='table-layout:fixed;width:671px'");
                    }
                } else if (!attrs.contains(QStringLiteral("width:671"), Qt::CaseInsensitive)) {
                    attrs.replace(
                        QRegularExpression(
                            QStringLiteral("style\\s*=\\s*['\"]"),
                            QRegularExpression::CaseInsensitiveOption),
                        QStringLiteral("style='width:671px; "));
                }
                attrs += QStringLiteral(" width='671'");
                open = QStringLiteral("<table") + attrs + QLatin1Char('>');
            }
            const QRegularExpression trRe(
                QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
            QString rebuilt;
            int trLast = 0;
            QRegularExpressionMatchIterator trIt = trRe.globalMatch(inner);
            while (trIt.hasNext()) {
                const QRegularExpressionMatch tr = trIt.next();
                rebuilt += inner.mid(trLast, tr.capturedStart() - trLast);
                QString rowInner = tr.captured(2);
                const QRegularExpression tdRe(
                    QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
                    QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
                QList<QRegularExpressionMatch> tds;
                QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
                while (tdIt.hasNext()) {
                    tds.append(tdIt.next());
                }
                if (tds.size() == 2) {
                    QString newRow;
                    int cellLast = 0;
                    for (int i = 0; i < tds.size(); ++i) {
                        const QRegularExpressionMatch &td = tds.at(i);
                        newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                        newRow += td.captured(1)
                            + setAttrWidth(td.captured(2), i == 0 ? QStringLiteral("200") : QStringLiteral("471"))
                            + td.captured(3) + td.captured(4) + td.captured(5);
                        cellLast = td.capturedEnd();
                    }
                    newRow += rowInner.mid(cellLast);
                    rowInner = newRow;
                } else if (tds.size() == 1) {
                    // «Процесс выполнения…» — на всю ширину шапки; без colspan справа пустая колонка.
                    const QString cellPlain =
                        htmlFragmentToPlainText(tds.at(0).captured(4)).trimmed();
                    if (cellPlain.contains(QStringLiteral("Процесс выполнения"), Qt::CaseInsensitive)) {
                        const QRegularExpressionMatch &td = tds.at(0);
                        QString attrs = td.captured(2);
                        attrs.remove(QRegularExpression(
                            QStringLiteral("\\s*width\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
                            QRegularExpression::CaseInsensitiveOption));
                        attrs.remove(QRegularExpression(
                            QStringLiteral("\\s*colspan\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
                            QRegularExpression::CaseInsensitiveOption));
                        attrs.remove(QRegularExpression(
                            QStringLiteral("\\s*align\\s*=\\s*(?:'[^']*'|\"[^\"]*\")"),
                            QRegularExpression::CaseInsensitiveOption));
                        attrs += QStringLiteral(
                            " colspan='2' align='center' width='671' style='width:671px'");
                        rowInner = td.captured(1) + attrs + td.captured(3) + td.captured(4)
                            + td.captured(5);
                    }
                }
                rebuilt += tr.captured(1) + rowInner + tr.captured(3);
                trLast = tr.capturedEnd();
            }
            rebuilt += inner.mid(trLast);
            // Убрать <p> в шапке — иначе Qt раздувает ширину на вкладке «Протоколы».
            rebuilt.replace(
                QRegularExpression(QStringLiteral("<p\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption),
                QString());
            rebuilt.replace(
                QRegularExpression(QStringLiteral("</p\\s*>"), QRegularExpression::CaseInsensitiveOption),
                QString());
            out += open + rebuilt + m.captured(3);
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    // «Процесс выполнения…» во всех 2-кол. summary (в т.ч. повторная сессия без «Методика»):
    // без colspan='2' справа остаётся пустая колонка.
    {
        const QRegularExpression processBannerRe(
            QStringLiteral(
                "(<tr\\b[^>]*>\\s*<td\\b)([^>]*)(>)("
                "\\s*(?:<(?:p|div|span|b|strong|font)\\b[^>]*>\\s*)*"
                "Процесс\\s+выполнения\\s+диагностическ(?:ого|ой)\\s+(?:задания|методики)"
                "[\\s\\S]*?)(</td>\\s*</tr>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        out.reserve(body.size() + 64);
        int last = 0;
        QRegularExpressionMatchIterator it = processBannerRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            QString attrs = m.captured(2);
            attrs.remove(QRegularExpression(
                QStringLiteral("\\s*width\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
                QRegularExpression::CaseInsensitiveOption));
            attrs.remove(QRegularExpression(
                QStringLiteral("\\s*colspan\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
                QRegularExpression::CaseInsensitiveOption));
            attrs.remove(QRegularExpression(
                QStringLiteral("\\s*align\\s*=\\s*(?:'[^']*'|\"[^\"]*\")"),
                QRegularExpression::CaseInsensitiveOption));
            attrs.remove(QRegularExpression(
                QStringLiteral("\\s*style\\s*=\\s*['\"][^'\"]*['\"]"),
                QRegularExpression::CaseInsensitiveOption));
            attrs += QStringLiteral(
                " colspan='2' align='center' width='671' style='width:671px'");
            out += m.captured(1) + attrs + m.captured(3) + m.captured(4) + m.captured(5);
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    // Дата/Результат/Примечание: 200/471; снимаем лишний colspan у повторных сессий.
    {
        const QRegularExpression summaryRowRe(
            QStringLiteral(
                "(<tr[^>]*>\\s*<td\\b)([^>]*>)"
                "(\\s*(?:<(?:p|div|span|font|b|strong)\\b[^>]*>\\s*)*"
                "(?:Дата\\s*/\\s*специалист|Результат|Примечание)"
                "[\\s\\S]*?</td>\\s*<td\\b)([^>]*>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        out.reserve(body.size() + 64);
        int last = 0;
        QRegularExpressionMatchIterator it = summaryRowRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            auto withStyleWidth = [](QString attrs, const QString &w) {
                attrs.remove(QRegularExpression(
                    QStringLiteral("\\s*width\\s*=\\s*['\"][^'\"]*['\"]"),
                    QRegularExpression::CaseInsensitiveOption));
                attrs.remove(QRegularExpression(
                    QStringLiteral("\\s*colspan\\s*=\\s*['\"][^'\"]*['\"]"),
                    QRegularExpression::CaseInsensitiveOption));
                attrs.remove(QRegularExpression(
                    QStringLiteral("\\s*style\\s*=\\s*['\"][^'\"]*['\"]"),
                    QRegularExpression::CaseInsensitiveOption));
                return QStringLiteral(" width='%1' style='width:%1px'%2").arg(w, attrs);
            };
            out += m.captured(1) + withStyleWidth(m.captured(2), QStringLiteral("200"))
                + m.captured(3) + withStyleWidth(m.captured(4), QStringLiteral("471"));
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    // 1.26: задания 1 и 2 — отдельные таблицы по 671 (общая сетка раздувает ширину).
    {
        const QRegularExpression processTableRe(
            QStringLiteral(
                "(<table\\b[^>]*>)([\\s\\S]*?(?:Портретная|№\\s*рассказа|Задание\\s*2)[\\s\\S]*?)(</table>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        out.reserve(body.size() + 128);
        int last = 0;
        QRegularExpressionMatchIterator it = processTableRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            const QString open = m.captured(1);
            QString inner = m.captured(2);
            const int task2At = findProtocol126Task2RowStart(inner);
            const bool hasPortrait = inner.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive);

            auto emitTable = [&](const QString &rows) {
                out += open + rows + QStringLiteral("</table>");
            };

            if (hasPortrait && task2At > 0) {
                emitTable(applyProtocol126Task1CellWidths(inner.left(task2At)));
                inner = inner.mid(task2At);
            } else if (hasPortrait) {
                out += open + applyProtocol126Task1CellWidths(inner) + m.captured(3);
                last = m.capturedEnd();
                continue;
            }

            // Задание 2: OR/HLP отдельно (200+471), рассказы отдельно (70+120+421+60).
            const int storiesAt = findProtocol126Task2StoriesStart(inner);
            if (storiesAt > 0) {
                emitTable(applyProtocol126Task2OrHlpWidths(inner.left(storiesAt)));
                emitTable(applyProtocol126Task2CellWidths(inner.mid(storiesAt)));
            } else {
                out += open + applyProtocol126ProcessCellWidths(inner) + m.captured(3);
            }
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    // Таблицы процесса «Факт выполнения» (2.8/2.9 — 3 кол.; 2.10 — 4 кол.): сумма = 671.
    {
        const QRegularExpression processTableRe(
            QStringLiteral(
                "(<table\\b[^>]*>)([\\s\\S]*?Факт\\s+выполнения[\\s\\S]*?)(</table>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        out.reserve(body.size() + 128);
        int last = 0;
        QRegularExpressionMatchIterator it = processTableRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            QString open = m.captured(1);
            QString inner = m.captured(2);
            const bool numbered = inner.contains(
                QRegularExpression(
                    QStringLiteral(">\\s*№\\s*<"),
                    QRegularExpression::CaseInsensitiveOption));
            // 3.1.12 и др.: Факт + Характер + Виды помощи — 141+265+265 = 671.
            const bool orHlpFact = !numbered
                && inner.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
                && inner.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive);

            inner.remove(QRegularExpression(
                QStringLiteral("<colgroup\\b[\\s\\S]*?</colgroup>\\s*"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
            if (numbered) {
                inner.prepend(QStringLiteral(
                    "<colgroup><col width='40'><col width='125'><col width='253'><col width='253'></colgroup>"));
            } else if (orHlpFact) {
                inner.prepend(QStringLiteral(
                    "<colgroup><col width='141'><col width='265'><col width='265'></colgroup>"));
            } else {
                inner.prepend(QStringLiteral(
                    "<colgroup><col width='125'><col width='273'><col width='273'></colgroup>"));
            }

            const QRegularExpression trRe(
                QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
            QString rebuilt;
            int trLast = 0;
            QRegularExpressionMatchIterator trIt = trRe.globalMatch(inner);
            while (trIt.hasNext()) {
                const QRegularExpressionMatch tr = trIt.next();
                rebuilt += inner.mid(trLast, tr.capturedStart() - trLast);
                QString rowInner = tr.captured(2);
                const QRegularExpression tdRe(
                    QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
                    QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
                QList<QRegularExpressionMatch> tds;
                QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
                while (tdIt.hasNext()) {
                    tds.append(tdIt.next());
                }
                if ((!numbered && tds.size() == 3) || (numbered && tds.size() == 4)) {
                    QStringList widths;
                    if (numbered) {
                        widths << QStringLiteral("40") << QStringLiteral("125")
                               << QStringLiteral("253") << QStringLiteral("253");
                    } else if (orHlpFact) {
                        widths << QStringLiteral("141") << QStringLiteral("265")
                               << QStringLiteral("265");
                    } else {
                        widths << QStringLiteral("125") << QStringLiteral("273")
                               << QStringLiteral("273");
                    }
                    QString newRow;
                    int cellLast = 0;
                    for (int i = 0; i < tds.size(); ++i) {
                        const QRegularExpressionMatch &td = tds.at(i);
                        newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                        newRow += td.captured(1) + setAttrWidth(td.captured(2), widths.at(i))
                            + td.captured(3) + td.captured(4) + td.captured(5);
                        cellLast = td.capturedEnd();
                    }
                    newRow += rowInner.mid(cellLast);
                    rowInner = newRow;
                }
                rebuilt += tr.captured(1) + rowInner + tr.captured(3);
                trLast = tr.capturedEnd();
            }
            rebuilt += inner.mid(trLast);
            out += open + rebuilt + m.captured(3);
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    // OR/HLP/Баллы (3 колонки): 300+300+71 = 671.
    {
        const QRegularExpression processTableRe(
            QStringLiteral(
                "(<table\\b[^>]*>)([\\s\\S]*?Характер\\s+деятельности[\\s\\S]*?Виды\\s+помощи"
                "[\\s\\S]*?Баллы[\\s\\S]*?)(</table>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        out.reserve(body.size() + 128);
        int last = 0;
        QRegularExpressionMatchIterator it = processTableRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            QString open = m.captured(1);
            QString inner = m.captured(2);
            // 1.26 и др. 4-колоночные с «Портретная»/«№ рассказа» — отдельный блок выше.
            if (inner.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive)
                || inner.contains(QRegularExpression(
                       QStringLiteral("№\\s*рассказа"), QRegularExpression::CaseInsensitiveOption))) {
                out += m.captured(0);
                last = m.capturedEnd();
                continue;
            }
            // Пропускаем 4+ колоночные таблицы (с № / картинкой и т.п.) — кроме известных сеток.
            {
                const QRegularExpression firstTrRe(
                    QStringLiteral("<tr\\b[^>]*>([\\s\\S]*?)</tr>"),
                    QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
                const QRegularExpressionMatch firstTr = firstTrRe.match(inner);
                if (firstTr.hasMatch()) {
                    const int tdCount = firstTr.captured(1).count(
                        QRegularExpression(QStringLiteral("<td\\b"), QRegularExpression::CaseInsensitiveOption));
                    const bool digitsTable = inner.contains(
                        QStringLiteral("Кол-во цифр"), Qt::CaseInsensitive);
                    // 1.272 «№/ответ»: 120+251+250+50 = 671; «Баллы» узкая, № вмещает «6. Спокойствие».
                    const bool answerNoTable =
                        tdCount == 4
                        && (inner.contains(QStringLiteral("№/ответ"), Qt::CaseInsensitive)
                            || inner.contains(QStringLiteral("N/ответ"), Qt::CaseInsensitive)
                            || (inner.contains(QRegularExpression(
                                    QStringLiteral("id\\s*=\\s*['\"]ids\\d"),
                                    QRegularExpression::CaseInsensitiveOption))
                                && inner.contains(QStringLiteral("Характер"), Qt::CaseInsensitive)));
                    // 3.1.10: № / картинка / объяснение / OR / HLP / Баллы — 30+121+143+141+175+61 = 671.
                    const bool pictureChoiceTable =
                        tdCount == 6
                        && (inner.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)
                            || inner.contains(QStringLiteral("Объяснение выбора"), Qt::CaseInsensitive)
                            || inner.contains(
                                   QRegularExpression(
                                       QStringLiteral("id\\s*=\\s*['\"]idb\\d"),
                                       QRegularExpression::CaseInsensitiveOption)));
                    if (tdCount == 4 && (digitsTable || answerNoTable)) {
                        const QStringList widths = digitsTable
                            ? QStringList{QStringLiteral("249"), QStringLiteral("249"),
                                          QStringLiteral("112"), QStringLiteral("61")}
                            : QStringList{QStringLiteral("120"), QStringLiteral("251"),
                                          QStringLiteral("250"), QStringLiteral("50")};
                        const QString colgroup = digitsTable
                            ? QStringLiteral(
                                  "<colgroup><col width='249'><col width='249'>"
                                  "<col width='112'><col width='61'></colgroup>")
                            : QStringLiteral(
                                  "<colgroup><col width='120'><col width='251'>"
                                  "<col width='250'><col width='50'></colgroup>");
                        inner.remove(QRegularExpression(
                            QStringLiteral("<colgroup\\b[\\s\\S]*?</colgroup>\\s*"),
                            QRegularExpression::CaseInsensitiveOption
                                | QRegularExpression::DotMatchesEverythingOption));
                        inner.prepend(colgroup);
                        const QRegularExpression trRe(
                            QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
                            QRegularExpression::CaseInsensitiveOption
                                | QRegularExpression::DotMatchesEverythingOption);
                        QString rebuilt;
                        int trLast = 0;
                        QRegularExpressionMatchIterator trIt = trRe.globalMatch(inner);
                        while (trIt.hasNext()) {
                            const QRegularExpressionMatch tr = trIt.next();
                            rebuilt += inner.mid(trLast, tr.capturedStart() - trLast);
                            QString rowInner = tr.captured(2);
                            const QRegularExpression tdRe(
                                QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
                                QRegularExpression::CaseInsensitiveOption
                                    | QRegularExpression::DotMatchesEverythingOption);
                            QList<QRegularExpressionMatch> tds;
                            QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
                            while (tdIt.hasNext()) {
                                tds.append(tdIt.next());
                            }
                            if (tds.size() == 4
                                || (tds.size() == 2 && tds.at(0).captured(2).contains(
                                                          QStringLiteral("colspan"),
                                                          Qt::CaseInsensitive))) {
                                QString newRow;
                                int cellLast = 0;
                                for (int i = 0; i < tds.size(); ++i) {
                                    const QRegularExpressionMatch &td = tds.at(i);
                                    newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                                    QString attrs = td.captured(2);
                                    QString cellHtml = td.captured(4);
                                    if (tds.size() == 4) {
                                        attrs = setAttrWidth(attrs, widths.at(i));
                                        // 4.2.1: «Кол-во цифр» (i=2) и «Баллы» (i=3) — по центру.
                                        if (digitsTable && (i == 2 || i == 3)) {
                                            if (!attrs.contains(QStringLiteral("align="), Qt::CaseInsensitive)) {
                                                attrs += QStringLiteral(" align='center'");
                                            }
                                            if (!attrs.contains(QStringLiteral("valign="), Qt::CaseInsensitive)) {
                                                attrs += QStringLiteral(" valign='middle'");
                                            }
                                            if (!cellHtml.contains(
                                                    QStringLiteral("text-align:center"),
                                                    Qt::CaseInsensitive)) {
                                                cellHtml.replace(
                                                    QRegularExpression(
                                                        QStringLiteral("(<div\\b)(?![^>]*text-align)"),
                                                        QRegularExpression::CaseInsensitiveOption),
                                                    QStringLiteral("\\1 style='text-align:center'"));
                                            }
                                        } else if (i == 3 || (answerNoTable && i == 0)) {
                                            if (!attrs.contains(QStringLiteral("align="), Qt::CaseInsensitive)) {
                                                attrs += QStringLiteral(" align='center'");
                                            }
                                        }
                                        // Баллы (и idsum): цифры по центру.
                                        if (answerNoTable && i == 3
                                            && !cellHtml.contains(
                                                   QStringLiteral("text-align:center"),
                                                   Qt::CaseInsensitive)) {
                                            cellHtml.replace(
                                                QRegularExpression(
                                                    QStringLiteral("(<div\\b)(?![^>]*text-align)"),
                                                    QRegularExpression::CaseInsensitiveOption),
                                                QStringLiteral("\\1 style='text-align:center'"));
                                        }
                                    } else if (i == tds.size() - 1) {
                                        attrs = setAttrWidth(attrs, widths.last());
                                        if (!attrs.contains(QStringLiteral("align="), Qt::CaseInsensitive)) {
                                            attrs += QStringLiteral(" align='center'");
                                        }
                                    }
                                    newRow += td.captured(1) + attrs + td.captured(3) + cellHtml
                                        + td.captured(5);
                                    cellLast = td.capturedEnd();
                                }
                                newRow += rowInner.mid(cellLast);
                                rowInner = newRow;
                            }
                            rebuilt += tr.captured(1) + rowInner + tr.captured(3);
                            trLast = tr.capturedEnd();
                        }
                        rebuilt += inner.mid(trLast);
                        out += open + rebuilt + m.captured(3);
                        last = m.capturedEnd();
                        continue;
                    }
                    if (pictureChoiceTable) {
                        const QStringList widths = {
                            QStringLiteral("30"), QStringLiteral("121"), QStringLiteral("143"),
                            QStringLiteral("141"), QStringLiteral("175"), QStringLiteral("61")};
                        inner.remove(QRegularExpression(
                            QStringLiteral("<colgroup\\b[\\s\\S]*?</colgroup>\\s*"),
                            QRegularExpression::CaseInsensitiveOption
                                | QRegularExpression::DotMatchesEverythingOption));
                        inner.prepend(QStringLiteral(
                            "<colgroup><col width='30'><col width='121'><col width='143'>"
                            "<col width='141'><col width='175'><col width='61'></colgroup>"));
                        const QRegularExpression trRe(
                            QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
                            QRegularExpression::CaseInsensitiveOption
                                | QRegularExpression::DotMatchesEverythingOption);
                        QString rebuilt;
                        int trLast = 0;
                        QRegularExpressionMatchIterator trIt = trRe.globalMatch(inner);
                        while (trIt.hasNext()) {
                            const QRegularExpressionMatch tr = trIt.next();
                            rebuilt += inner.mid(trLast, tr.capturedStart() - trLast);
                            QString rowInner = tr.captured(2);
                            const QRegularExpression tdRe(
                                QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
                                QRegularExpression::CaseInsensitiveOption
                                    | QRegularExpression::DotMatchesEverythingOption);
                            QList<QRegularExpressionMatch> tds;
                            QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
                            while (tdIt.hasNext()) {
                                tds.append(tdIt.next());
                            }
                            if (tds.size() == 6
                                || (tds.size() == 2
                                    && tds.at(0).captured(2).contains(
                                           QStringLiteral("colspan"), Qt::CaseInsensitive))) {
                                QString newRow;
                                int cellLast = 0;
                                for (int i = 0; i < tds.size(); ++i) {
                                    const QRegularExpressionMatch &td = tds.at(i);
                                    newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                                    QString attrs = td.captured(2);
                                    QString cellHtml = td.captured(4);
                                    if (tds.size() == 6) {
                                        attrs = setAttrWidth(attrs, widths.at(i));
                                        if (i == 0 || i == 5) {
                                            if (!attrs.contains(QStringLiteral("align="), Qt::CaseInsensitive)) {
                                                attrs += QStringLiteral(" align='center'");
                                            }
                                        }
                                        // Колонка «Баллы»: цифры по центру.
                                        if (i == 5
                                            && !cellHtml.contains(
                                                   QStringLiteral("text-align:center"),
                                                   Qt::CaseInsensitive)) {
                                            cellHtml.replace(
                                                QRegularExpression(
                                                    QStringLiteral("(<div\\b)(?![^>]*text-align)"),
                                                    QRegularExpression::CaseInsensitiveOption),
                                                QStringLiteral("\\1 style='text-align:center'"));
                                        }
                                    } else if (i == tds.size() - 1) {
                                        attrs = setAttrWidth(attrs, widths.last());
                                        if (!attrs.contains(QStringLiteral("align="), Qt::CaseInsensitive)) {
                                            attrs += QStringLiteral(" align='center'");
                                        }
                                        if (!cellHtml.contains(
                                                QStringLiteral("text-align:center"),
                                                Qt::CaseInsensitive)) {
                                            cellHtml.replace(
                                                QRegularExpression(
                                                    QStringLiteral("(<div\\b)(?![^>]*text-align)"),
                                                    QRegularExpression::CaseInsensitiveOption),
                                                QStringLiteral("\\1 style='text-align:center'"));
                                        }
                                    }
                                    newRow += td.captured(1) + attrs + td.captured(3) + cellHtml
                                        + td.captured(5);
                                    cellLast = td.capturedEnd();
                                }
                                newRow += rowInner.mid(cellLast);
                                rowInner = newRow;
                            }
                            rebuilt += tr.captured(1) + rowInner + tr.captured(3);
                            trLast = tr.capturedEnd();
                        }
                        rebuilt += inner.mid(trLast);
                        out += open + rebuilt + m.captured(3);
                        last = m.capturedEnd();
                        continue;
                    }
                    if (tdCount != 3) {
                        out += m.captured(0);
                        last = m.capturedEnd();
                        continue;
                    }
                }
            }
            inner.remove(QRegularExpression(
                QStringLiteral("<colgroup\\b[\\s\\S]*?</colgroup>\\s*"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
            inner.prepend(QStringLiteral(
                "<colgroup><col width='300'><col width='300'><col width='71'></colgroup>"));

            const QRegularExpression trRe(
                QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
            QString rebuilt;
            int trLast = 0;
            QRegularExpressionMatchIterator trIt = trRe.globalMatch(inner);
            const QStringList widths = {
                QStringLiteral("300"), QStringLiteral("300"), QStringLiteral("71")};
            while (trIt.hasNext()) {
                const QRegularExpressionMatch tr = trIt.next();
                rebuilt += inner.mid(trLast, tr.capturedStart() - trLast);
                QString rowInner = tr.captured(2);
                const QRegularExpression tdRe(
                    QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
                    QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
                QList<QRegularExpressionMatch> tds;
                QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
                while (tdIt.hasNext()) {
                    tds.append(tdIt.next());
                }
                if (tds.size() == 3) {
                    QString newRow;
                    int cellLast = 0;
                    for (int i = 0; i < tds.size(); ++i) {
                        const QRegularExpressionMatch &td = tds.at(i);
                        newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                        QString attrs = setAttrWidth(td.captured(2), widths.at(i));
                        QString cellHtml = td.captured(4);
                        // Колонка «Баллы»: цифры по центру (td align + div text-align).
                        if (i == 2) {
                            if (!attrs.contains(QStringLiteral("align="), Qt::CaseInsensitive)) {
                                attrs += QStringLiteral(" align='center'");
                            }
                            if (!attrs.contains(QStringLiteral("valign="), Qt::CaseInsensitive)) {
                                attrs += QStringLiteral(" valign='middle'");
                            }
                            if (!cellHtml.contains(
                                    QStringLiteral("text-align:center"), Qt::CaseInsensitive)) {
                                cellHtml.replace(
                                    QRegularExpression(
                                        QStringLiteral("(<div\\b)(?![^>]*text-align)"),
                                        QRegularExpression::CaseInsensitiveOption),
                                    QStringLiteral("\\1 style='text-align:center'"));
                            }
                        }
                        newRow += td.captured(1) + attrs + td.captured(3) + cellHtml
                            + td.captured(5);
                        cellLast = td.capturedEnd();
                    }
                    newRow += rowInner.mid(cellLast);
                    rowInner = newRow;
                }
                rebuilt += tr.captured(1) + rowInner + tr.captured(3);
                trLast = tr.capturedEnd();
            }
            rebuilt += inner.mid(trLast);
            out += open + rebuilt + m.captured(3);
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    // 5.2.1 «Расскажи по картинке»: Фрагменты речи / Частота — 500+171 = 671
    // (wrapProtocolDocumentHtml ошибочно раздувает 500→506 → таблица шире стандарта).
    {
        const QRegularExpression processTableRe(
            QStringLiteral(
                "(<table\\b[^>]*>)([\\s\\S]*?Фрагменты\\s+речи[\\s\\S]*?Частота\\s+употребления"
                "[\\s\\S]*?)(</table>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        out.reserve(body.size() + 128);
        int last = 0;
        QRegularExpressionMatchIterator it = processTableRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            QString open = m.captured(1);
            QString inner = m.captured(2);
            inner.remove(QRegularExpression(
                QStringLiteral("<colgroup\\b[\\s\\S]*?</colgroup>\\s*"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
            inner.prepend(QStringLiteral(
                "<colgroup><col width='500'><col width='171'></colgroup>"));

            const QRegularExpression trRe(
                QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
            QString rebuilt;
            int trLast = 0;
            QRegularExpressionMatchIterator trIt = trRe.globalMatch(inner);
            while (trIt.hasNext()) {
                const QRegularExpressionMatch tr = trIt.next();
                rebuilt += inner.mid(trLast, tr.capturedStart() - trLast);
                QString rowInner = tr.captured(2);
                const QRegularExpression tdRe(
                    QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
                    QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
                QList<QRegularExpressionMatch> tds;
                QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
                while (tdIt.hasNext()) {
                    tds.append(tdIt.next());
                }
                if (tds.size() == 2) {
                    const QStringList widths = {
                        QStringLiteral("500"), QStringLiteral("171")};
                    QString newRow;
                    int cellLast = 0;
                    for (int i = 0; i < tds.size(); ++i) {
                        const QRegularExpressionMatch &td = tds.at(i);
                        newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                        newRow += td.captured(1) + setAttrWidth(td.captured(2), widths.at(i))
                            + td.captured(3) + td.captured(4) + td.captured(5);
                        cellLast = td.capturedEnd();
                    }
                    newRow += rowInner.mid(cellLast);
                    rowInner = newRow;
                } else if (tds.size() == 1) {
                    // «Задание №N» — colspan на всю ширину 671.
                    const QRegularExpressionMatch &td = tds.at(0);
                    QString attrs = setAttrWidth(td.captured(2), QStringLiteral("671"));
                    attrs.replace(
                        QRegularExpression(
                            QStringLiteral("\\s*colspan\\s*=\\s*['\"][^'\"]*['\"]"),
                            QRegularExpression::CaseInsensitiveOption),
                        QString());
                    attrs += QStringLiteral(" colspan='2'");
                    rowInner = td.captured(1) + attrs + td.captured(3) + td.captured(4)
                        + td.captured(5);
                }
                rebuilt += tr.captured(1) + rowInner + tr.captured(3);
                trLast = tr.capturedEnd();
            }
            rebuilt += inner.mid(trLast);
            out += open + rebuilt + m.captured(3);
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    // 5.4.2 «Сказка»: таблица процесса Вопросы/Ответы/Виды помощи — 219+226+226 = 671.
    {
        const QRegularExpression processTableRe(
            QStringLiteral(
                "(<table\\b[^>]*>)([\\s\\S]*?Вопросы[\\s\\S]*?(?:Ответы\\s+ребенка|Ответы)"
                "[\\s\\S]*?(?:Виды\\s+помощи|Виды\\s+и\\s+количество\\s+помощи)[\\s\\S]*?)(</table>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        out.reserve(body.size() + 128);
        int last = 0;
        QRegularExpressionMatchIterator it = processTableRe.globalMatch(body);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += body.mid(last, m.capturedStart() - last);
            QString open = m.captured(1);
            QString inner = m.captured(2);
            inner.remove(QRegularExpression(
                QStringLiteral("<colgroup\\b[\\s\\S]*?</colgroup>\\s*"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
            inner.prepend(QStringLiteral(
                "<colgroup><col width='219'><col width='226'><col width='226'></colgroup>"));

            const QRegularExpression trRe(
                QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
            QString rebuilt;
            int trLast = 0;
            QRegularExpressionMatchIterator trIt = trRe.globalMatch(inner);
            while (trIt.hasNext()) {
                const QRegularExpressionMatch tr = trIt.next();
                rebuilt += inner.mid(trLast, tr.capturedStart() - trLast);
                QString rowInner = tr.captured(2);
                const QRegularExpression tdRe(
                    QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
                    QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
                QList<QRegularExpressionMatch> tds;
                QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
                while (tdIt.hasNext()) {
                    tds.append(tdIt.next());
                }
                if (tds.size() == 3) {
                    const QStringList widths = {
                        QStringLiteral("219"), QStringLiteral("226"), QStringLiteral("226")};
                    QString newRow;
                    int cellLast = 0;
                    for (int i = 0; i < tds.size(); ++i) {
                        const QRegularExpressionMatch &td = tds.at(i);
                        newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                        newRow += td.captured(1) + setAttrWidth(td.captured(2), widths.at(i))
                            + td.captured(3) + td.captured(4) + td.captured(5);
                        cellLast = td.capturedEnd();
                    }
                    newRow += rowInner.mid(cellLast);
                    rowInner = newRow;
                } else if (tds.size() == 2) {
                    // Строка «Баллы» — colspan второй ячейки на 452.
                    const QStringList widths = {
                        QStringLiteral("219"), QStringLiteral("452")};
                    QString newRow;
                    int cellLast = 0;
                    for (int i = 0; i < tds.size(); ++i) {
                        const QRegularExpressionMatch &td = tds.at(i);
                        newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                        QString attrs = setAttrWidth(td.captured(2), widths.at(i));
                        if (i == 1) {
                            attrs.replace(
                                QRegularExpression(
                                    QStringLiteral("\\s*colspan\\s*=\\s*['\"][^'\"]*['\"]"),
                                    QRegularExpression::CaseInsensitiveOption),
                                QString());
                            attrs += QStringLiteral(" colspan='2'");
                        }
                        newRow += td.captured(1) + attrs + td.captured(3) + td.captured(4)
                            + td.captured(5);
                        cellLast = td.capturedEnd();
                    }
                    newRow += rowInner.mid(cellLast);
                    rowInner = newRow;
                }
                rebuilt += tr.captured(1) + rowInner + tr.captured(3);
                trLast = tr.capturedEnd();
            }
            rebuilt += inner.mid(trLast);
            out += open + rebuilt + m.captured(3);
            last = m.capturedEnd();
        }
        out += body.mid(last);
        body = out;
    }

    return body;
}

QString ensureProtocol12SummaryTableOpens(QString body) {
    const QString tableOpen = protocolSummaryTableOpenHtml();
    const QRegularExpression orphanRowRe(
        QStringLiteral("</table>\\s*(<tr[^>]*>\\s*<td[^>]*>\\s*Дата/специалист)"),
        QRegularExpression::CaseInsensitiveOption);
    body.replace(orphanRowRe, QStringLiteral("</table>") + tableOpen + QStringLiteral("\\1"));
    return normalizeSummaryColumnWidthsHtml(body);
}

} // namespace

QString cleanProtocol126SummaryRows(QString summary);
QString canonicalizeProtocol126Session(QString session);
QStringList extractProtocol126SessionsByDate(const QString &body);
QString joinProtocol126Sessions(const QStringList &sessions);
QString extractTableInnerRows(const QString &tableHtml);
QStringList extractProtocol126TaskTables(QString *htmlInOut);
QString normalizeProtocol126TaskRows(QString taskRows);

// Таблица процесса 1.26: ширины колонок задаёт applyProtocol126ProcessCellWidths / normalize.
constexpr const char kProtocol126ProcessTableOpen[] =
    "<table border='1' style='table-layout:fixed' cellspacing='0' "
    "width='671' cellpadding='0'>";

QString replaceDivInnerById(QString html, const QString &divId, const QString &innerHtml);
QString extractDivInnerById(const QString &html, const QString &divId);

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
    // Если разбор по таблицам склеил сессии — режем только по «Дата/специалист».
    if (sessions.size() <= 1) {
        return extractLastProtocol126Session(protocolBody);
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
                result += ExerciseProtocol::canonicalizeProtocolHeaderFragment(headerFragment);
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
    return normalizeProtocol12Layout(formatProtocol12BodyForHeaderView(protocolBody));
}

QString ExerciseProtocol::canonicalizeProtocol126StoredBody(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    QStringList sessions = extractProtocol126SessionsByDate(protocolBody);
    if (sessions.isEmpty()) {
        return canonicalizeProtocol126Session(protocolBody);
    }
    QStringList flat;
    flat.reserve(sessions.size());
    for (const QString &session : sessions) {
        const QString cleaned = canonicalizeProtocol126Session(session);
        if (!cleaned.isEmpty()) {
            flat.append(cleaned);
        }
    }
    return joinProtocol126Sessions(flat);
}

namespace {

QString plainNoteWithoutNestedTables(const QString &html) {
    // Если Qt вложил таблицу стимулов в «Примечание» — не сохраняем её как текст заметки.
    QString note = html;
    const int tablePos = note.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
    if (tablePos >= 0) {
        note = note.left(tablePos);
    }
    note.remove(QRegularExpression(QStringLiteral("<[^>]+>")));
    note.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    note.replace(QChar(0x00A0), QLatin1Char(' '));
    note = note.trimmed();
    if (note.contains(QStringLiteral("Стимульные"), Qt::CaseInsensitive)
        || note.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)) {
        return {};
    }
    return note;
}

QString canonicalizeProtocol418Session(QString session) {
    session = stripLeadingSummaryTableWrapper(session.trimmed());
    if (session.isEmpty()) {
        return session;
    }
    // Уже плоская разметка с маркером и стимульной таблицей снаружи.
    if (session.contains(QStringLiteral("<!--s-->"))
        && (session.contains(QStringLiteral("id='sel1'"), Qt::CaseInsensitive)
            || session.contains(QStringLiteral("id=\"sel1\""), Qt::CaseInsensitive))
        && !session.contains(QRegularExpression(
               QStringLiteral("Примечание[\\s\\S]{0,400}<table"),
               QRegularExpression::CaseInsensitiveOption))) {
        // 32.1: «Процесс выполнения» вне таблицы → без боковых границ; завернуть в table.
        const QRegularExpression processParaRe(
            QStringLiteral(
                "<p\\b[^>]*>\\s*(?:<b>)?\\s*Процесс выполнения диагностической методики\\s*(?:</b>)?\\s*</p>"),
            QRegularExpression::CaseInsensitiveOption);
        if (processParaRe.match(session).hasMatch()) {
            session.replace(
                processParaRe,
                QStringLiteral(
                    "<table style='table-layout:fixed' border='1' cellspacing='0' cellpadding='0' width='671'>"
                    "<tr><td colspan='2' align='center' valign='top' width='671'>"
                    "Процесс выполнения диагностической методики</td></tr></table>"));
        }
        // Убедимся, что стимульная таблица закрыта.
        if (!session.contains(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
            session += QStringLiteral("</table>");
        }
        return session;
    }

    static const char *kWords[] = {"Школа", "Обед", "Утро", "Красота", "Прогулка"};
    static const char *kPrefixes[] = {"sel", "ex", "re", "hlp", "rea", "b"};

    const QString idspc = extractDivInnerById(session, QStringLiteral("idspc"));
    const QString idvivod = extractDivInnerById(session, QStringLiteral("idvivod"));
    const QString cidd = extractDivInnerById(session, QStringLiteral("cidd"));
    const QString idsum = extractDivInnerById(session, QStringLiteral("idsum"));

    QString noteText;
    {
        const QString idnote = extractDivInnerById(session, QStringLiteral("idnote"));
        if (!idnote.trimmed().isEmpty()) {
            noteText = htmlFragmentToPlainText(idnote);
        } else {
            const auto noteCell = extractSecondCellPlain(session, QStringLiteral("Примечание"));
            if (noteCell.first) {
                noteText = plainNoteWithoutNestedTables(noteCell.second);
            }
        }
    }

    QString body;
    body += QStringLiteral(
        "<tr><td width='200' valign='top'><p>Дата / специалист</p></td>"
        "<td width='471' valign='top'><div contenteditable='true' id='idspc'>%1</div></td></tr>")
                .arg(idspc);
    body += QStringLiteral(
        "<tr><td width='200' valign='top'><p>Результат: баллы (макс.) /<br /> вывод об уровне развития </p></td>"
        "<td width='471' valign='top'><div contenteditable='true' id='idvivod'>%1</div></td></tr>")
                .arg(idvivod);
    body += QStringLiteral(
        "<tr><td width='200' valign='top'><p>Примечание</p></td>"
        "<td width='471' valign='top'><div contenteditable='true' id='idnote'>%1</div></td></tr>")
                .arg(noteText.toHtmlEscaped());
    body += QStringLiteral("</table><!--s-->");
    body += QStringLiteral(
        "<table style='table-layout:fixed' border='1' cellspacing='0' cellpadding='0' width='671'>"
        "<tr><td colspan='2' align='center' valign='top' width='671'>"
        "Процесс выполнения диагностической методики</td></tr>"
        "<tr><td width='200' valign='top'><p>Характер деятельности ребенка</p></td>"
        "<td width='471' valign='top'><div contenteditable='true' id='cidd'>%1</div></td></tr>"
        "</table>")
                .arg(cidd);
    body += QStringLiteral(
        "<table style='table-layout:fixed' border='1' cellspacing='0' cellpadding='0' width='671'>"
        "<tr>"
        "<td width='85' valign='top'><p align='center'>Стимульные<br>слова</p></td>"
        "<td width='70' valign='top'><p align='center'>Выбранная<br>картинка</p></td>"
        "<td width='110' valign='top'><p align='center'>Объяснение выбора</p></td>"
        "<td width='100' valign='top'><p align='center'>Воспроизведенное слово<br>до предъявления помощи</p></td>"
        "<td width='140' valign='top'><p align='center'>Виды помощи</p></td>"
        "<td width='110' valign='top'><p align='center'>Воспроизведенное слово<br>после предъявления помощи</p></td>"
        "<td width='56' valign='top'><p align='center'>Баллы</p></td>"
        "</tr>");
    for (int r = 0; r < 5; ++r) {
        body += QStringLiteral("<tr><td width='85' valign='top'><div id='word%1'>%2</div></td>")
                    .arg(r + 1)
                    .arg(QString::fromUtf8(kWords[r]));
        for (int c = 0; c < 6; ++c) {
            const QString id = QString::fromUtf8(kPrefixes[c]) + QString::number(r + 1);
            const QString val = extractDivInnerById(session, id);
            const QString align = (c == 5)
                ? QStringLiteral(" align='center' valign='middle'")
                : QStringLiteral(" valign='top'");
            const QString style = (c == 5) ? QStringLiteral(" style='text-align:center'") : QString();
            body += QStringLiteral(
                        "<td%1><div id='%2' contenteditable='true'%3>%4</div></td>")
                        .arg(align, id, style, val);
        }
        body += QStringLiteral("</tr>");
    }
    body += QStringLiteral(
        "<tr><td colspan='6' valign='top'><p>Итоговая оценка</p></td>"
        "<td align='center' valign='middle' width='56'>"
        "<div id='idsum' contenteditable='false' style='text-align:center'>%1</div></td></tr>"
        "</table>")
                .arg(idsum);
    return body;
}

} // namespace

QString ExerciseProtocol::canonicalizeProtocol418StoredBody(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    QStringList sessions = extractProtocol126SessionsByDate(protocolBody);
    if (sessions.isEmpty()) {
        return canonicalizeProtocol418Session(protocolBody);
    }
    QStringList flat;
    flat.reserve(sessions.size());
    for (const QString &session : sessions) {
        const QString cleaned = canonicalizeProtocol418Session(session);
        if (!cleaned.isEmpty()) {
            flat.append(cleaned);
        }
    }
    return joinProtocol126Sessions(flat);
}

QString ExerciseProtocol::canonicalizeProtocol118StoredBody(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }

    // Хранение как у 1.17: summary (Дата/Результат/Примечание) </table><!--s--> + таблица процесса.
    // Если процесс/повторные сессии вложены в ячейку шапки (баг QTextDocument) — вытащить наружу.
    auto unpackNestedTables = [](QString html) {
        // Вырезать целиком вложенные <table>…</table> из ячеек и дописать в конец сессии.
        QString extracted;
        const QRegularExpression nestedRe(
            QStringLiteral("(<table\\b[^>]*>[\\s\\S]*?</table\\s*>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        // Только таблицы внутри ячеек: после <td…> до </td>, не корневые.
        const QRegularExpression cellNestedRe(
            QStringLiteral(
                "(<td\\b[^>]*>)([\\s\\S]*?)(</td>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QString out;
        int last = 0;
        QRegularExpressionMatchIterator cellIt = cellNestedRe.globalMatch(html);
        while (cellIt.hasNext()) {
            const QRegularExpressionMatch cell = cellIt.next();
            out += html.mid(last, cell.capturedStart() - last);
            QString inner = cell.captured(2);
            QRegularExpressionMatchIterator nestIt = nestedRe.globalMatch(inner);
            QString cleaned = inner;
            // Собрать вложенные таблицы с конца, чтобы индексы не плыли.
            QList<QRegularExpressionMatch> nests;
            while (nestIt.hasNext()) {
                nests.append(nestIt.next());
            }
            for (int i = nests.size() - 1; i >= 0; --i) {
                const QRegularExpressionMatch &n = nests.at(i);
                const QString tableHtml = n.captured(1);
                // Не трогать мелкую разметку без процесса/даты.
                if (tableHtml.contains(QStringLiteral("Процесс выполнения"), Qt::CaseInsensitive)
                    || tableHtml.contains(QStringLiteral("Дата/специалист"), Qt::CaseInsensitive)
                    || tableHtml.contains(QStringLiteral("Факт выполнения"), Qt::CaseInsensitive)
                    || tableHtml.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                    extracted.prepend(tableHtml);
                    cleaned.replace(n.capturedStart(), n.capturedLength(), QString());
                }
            }
            // Пустая ячейка после вырезания — якорь для клика.
            if (htmlFragmentToPlainText(cleaned).trimmed().isEmpty()
                && cleaned.contains(QStringLiteral("contenteditable"), Qt::CaseInsensitive)) {
                cleaned = QStringLiteral("<div contenteditable='true'>&nbsp;</div>");
            }
            out += cell.captured(1) + cleaned + cell.captured(3);
            last = cell.capturedEnd();
        }
        out += html.mid(last);
        if (!extracted.isEmpty()) {
            // Если маркера нет — закрыть summary и начать процесс.
            if (!out.contains(QStringLiteral("<!--s-->"))) {
                out.replace(
                    QRegularExpression(QStringLiteral("</table>\\s*$"), QRegularExpression::CaseInsensitiveOption),
                    QString());
                out += QStringLiteral("</table><!--s-->");
            }
            out += extracted;
        }
        return out;
    };

    auto fixSession = [&](QString session) {
        session = unpackNestedTables(stripLeadingSummaryTableWrapper(session.trimmed()));
        if (session.isEmpty()) {
            return session;
        }
        const int marker = session.indexOf(QStringLiteral("<!--s-->"));
        if (marker < 0) {
            return ensureClosedProtocolSession(session);
        }

        QString summary = session.left(marker);
        QString process = session.mid(marker + QStringLiteral("<!--s-->").size());

        summary.replace(
            QRegularExpression(QStringLiteral("</table>\\s*$"), QRegularExpression::CaseInsensitiveOption),
            QString());
        summary = summary.trimmed();

        const QRegularExpression processTitleRow(
            QStringLiteral(
                "<tr\\b[^>]*>\\s*<td\\b[^>]*>\\s*(?:<[^>]*>\\s*)*Процесс\\s+выполнения\\s+диагностической\\s+методики"
                "\\s*(?:</[^>]+>\\s*)*</td>\\s*</tr>\\s*"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);

        summary.replace(processTitleRow, QString());
        summary = summary.trimmed();
        // Убрать случайные теги <table> из summary (строки <tr> оставить).
        summary.remove(QRegularExpression(
            QStringLiteral("</?table\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption));

        if (!process.trimmed().startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
            // process может быть несколькими таблицами подряд после unpack.
            if (!process.contains(QStringLiteral("<table"), Qt::CaseInsensitive)) {
                process.prepend(
                    QStringLiteral(
                        "<table border='1' style='table-layout:fixed;width:671px' "
                        "cellspacing='0' cellpadding='0' width='671'>"));
            }
        }

        if (!processTitleRow.match(process).hasMatch()) {
            const QString titleRow = QStringLiteral(
                "<tr><td colspan='4' align='center'>"
                "Процесс выполнения диагностической методики</td></tr>");
            const QRegularExpression tableOpenRe(
                QStringLiteral("(<table\\b[^>]*>)"),
                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch openMatch = tableOpenRe.match(process);
            if (openMatch.hasMatch()) {
                process.insert(openMatch.capturedEnd(), titleRow);
            } else {
                process.prepend(
                    QStringLiteral(
                        "<table border='1' style='table-layout:fixed;width:671px' "
                        "cellspacing='0' cellpadding='0' width='671'>")
                    + titleRow);
            }
        }

        return ensureClosedProtocolSession(
            summary + QStringLiteral("</table><!--s-->") + process);
    };

    QStringList sessions = extractProtocol126SessionsByDate(protocolBody);
    if (sessions.isEmpty()) {
        return normalizeSummaryColumnWidthsHtml(fixSession(protocolBody));
    }
    QStringList flat;
    flat.reserve(sessions.size());
    for (const QString &session : sessions) {
        const QString cleaned = fixSession(session);
        if (!cleaned.isEmpty()) {
            flat.append(cleaned);
        }
    }
    return joinProtocol126Sessions(flat);
}

QString ExerciseProtocol::buildProtocol118ViewRecord(
    const QString &headerFragment,
    const QString &storedBody) {
    if (storedBody.trimmed().isEmpty()) {
        return headerFragment;
    }

    // Как buildProtocol12ProtocolsTabRecord / 1.26: закрыть summary </table>, затем
    // отдельный блок процесса — без склейки header+body (Qt иначе вкладывает <table>).
    const QString canonical = canonicalizeProtocol118StoredBody(storedBody);
    QStringList sessions = extractProtocol126SessionsByDate(canonical);
    if (sessions.isEmpty()) {
        sessions = QStringList{stripLeadingSummaryTableWrapper(canonical)};
    }

    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = ensureClosedProtocolSession(sessions.at(i));
        const int marker = session.indexOf(QStringLiteral("<!--s-->"));
        QString summaryRows = marker >= 0 ? session.left(marker) : session;
        QString resultsBlock =
            marker >= 0 ? session.mid(marker + QStringLiteral("<!--s-->").size()) : QString();

        summaryRows = stripLeadingSummaryTableWrapper(summaryRows);
        summaryRows.replace(
            QRegularExpression(QStringLiteral("</table>\\s*$"), QRegularExpression::CaseInsensitiveOption),
            QString());
        summaryRows.replace(
            QRegularExpression(
                QStringLiteral(
                    "<tr\\b[^>]*>\\s*<td\\b[^>]*>\\s*(?:<[^>]*>\\s*)*Процесс\\s+выполнения\\s+"
                    "диагностической\\s+методики\\s*(?:</[^>]+>\\s*)*</td>\\s*</tr>\\s*"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption),
            QString());
        summaryRows = summaryRows.trimmed();
        resultsBlock = resultsBlock.trimmed();

        if (i == 0) {
            if (!headerFragment.trimmed().isEmpty()) {
                result += ExerciseProtocol::canonicalizeProtocolHeaderFragment(headerFragment);
            } else {
                result += protocolSummaryTableOpenHtml();
            }
        } else {
            result += protocolSummaryTableOpenHtml();
        }
        if (!summaryRows.isEmpty()) {
            result += summaryRows;
        }
        result += QStringLiteral("</table>");
        if (!resultsBlock.isEmpty()) {
            if (!resultsBlock.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
                resultsBlock.prepend(
                    QStringLiteral(
                        "<table border='1' style='table-layout:fixed;width:671px' "
                        "cellspacing='0' cellpadding='0' width='671'>"));
            }
            result += resultsBlock;
            if (!resultsBlock.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
                result += QStringLiteral("</table>");
            }
        }
    }
    return ExerciseProtocol::normalizeSummaryColumnWidths(result);
}

QString ExerciseProtocol::buildProtocol542ViewRecord(
    const QString &headerFragment,
    const QString &storedBody) {
    if (storedBody.trimmed().isEmpty()) {
        return headerFragment;
    }

    // Как 1.18: закрыть 2-кол. summary </table>, затем 3-кол. таблицу Вопросы/Ответы/Помощь.
    // Иначе после 2-й сессии Qt вливает процесс в шапку → пустая 3-я колонка у «Методика».
    const QString flat = flattenStoredProtocolBody(storedBody);
    QStringList sessions = extractProtocol126SessionsByDate(flat);
    if (sessions.isEmpty()) {
        sessions = ExerciseProtocol::extractProtocolBodiesByDateRows(flat);
    }
    if (sessions.isEmpty()) {
        sessions = QStringList{stripLeadingSummaryTableWrapper(flat)};
    }

    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = ensureClosedProtocolSession(sessions.at(i));
        const int marker = session.indexOf(QStringLiteral("<!--s-->"));
        QString summaryRows = marker >= 0 ? session.left(marker) : session;
        QString resultsBlock =
            marker >= 0 ? session.mid(marker + QStringLiteral("<!--s-->").size()) : QString();

        summaryRows = stripLeadingSummaryTableWrapper(summaryRows);
        summaryRows.replace(
            QRegularExpression(QStringLiteral("</table>\\s*$"), QRegularExpression::CaseInsensitiveOption),
            QString());
        summaryRows = summaryRows.trimmed();
        resultsBlock = resultsBlock.trimmed();

        if (i == 0) {
            if (!headerFragment.trimmed().isEmpty()) {
                result += ExerciseProtocol::canonicalizeProtocolHeaderFragment(headerFragment);
            } else {
                result += protocolSummaryTableOpenHtml();
            }
        } else {
            result += protocolSummaryTableOpenHtml();
        }
        if (!summaryRows.isEmpty()) {
            result += summaryRows;
        }
        result += QStringLiteral("</table>");
        if (!resultsBlock.isEmpty()) {
            if (!resultsBlock.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
                resultsBlock.prepend(
                    QStringLiteral(
                        "<table border='1' style='table-layout:fixed;width:671px' "
                        "cellspacing='0' cellpadding='0' width='671'>"));
            }
            result += resultsBlock;
            if (!resultsBlock.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
                result += QStringLiteral("</table>");
            }
        }
    }
    return ExerciseProtocol::normalizeSummaryColumnWidths(result);
}

QString ExerciseProtocol::buildProtocol126ViewRecord(
    const QString &headerFragment,
    const QString &storedBody) {
    if (storedBody.trimmed().isEmpty()) {
        return headerFragment;
    }

    const QString canonical = canonicalizeProtocol126StoredBody(storedBody);
    QStringList sessions = extractProtocol126SessionsByDate(canonical);
    if (sessions.isEmpty()) {
        sessions = QStringList{canonicalizeProtocol126Session(canonical)};
    }

    const QString tableOpen = QString::fromUtf8(kProtocol126ProcessTableOpen);

    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = sessions.at(i);
        const int marker = session.indexOf(QStringLiteral("<!--s-->"));
        QString summaryRows = marker >= 0 ? session.left(marker) : session;
        QString resultsBlock =
            marker >= 0 ? session.mid(marker + QStringLiteral("<!--s-->").size()) : QString();

        summaryRows = cleanProtocol126SummaryRows(summaryRows);
        resultsBlock = resultsBlock.trimmed();
        // Убрать старый <p>Процесс…</p> — заголовок теперь строка таблицы с границами.
        resultsBlock.replace(
            QRegularExpression(
                QStringLiteral("<p\\b[^>]*>\\s*(?:<b>)?\\s*Процесс\\s+выполнения\\s+диагностического\\s+задания\\s*(?:</b>)?\\s*</p>\\s*"),
                QRegularExpression::CaseInsensitiveOption),
            QString());

        QStringList taskParts;
        QStringList taskTables = extractProtocol126TaskTables(&resultsBlock);
        for (const QString &table : taskTables) {
            const QString rows = extractTableInnerRows(table).trimmed();
            if (!rows.isEmpty()) {
                taskParts.append(rows);
            }
        }
        // Хвост resultsBlock может содержать голые <tr> без обёртки.
        resultsBlock = resultsBlock.trimmed();
        if (resultsBlock.contains(QStringLiteral("<tr"), Qt::CaseInsensitive)) {
            taskParts.append(resultsBlock);
        }

        const QStringList flatParts = splitProtocol126FlatTaskParts(taskParts);

        if (i == 0) {
            if (!headerFragment.trimmed().isEmpty()) {
                result += ExerciseProtocol::canonicalizeProtocolHeaderFragment(headerFragment);
            } else {
                result += protocolSummaryTableOpenHtml();
            }
        } else {
            // Без пустого <p>&nbsp;</p> — сессии стыкуются без разрыва.
            result += protocolSummaryTableOpenHtml();
        }
        if (!summaryRows.isEmpty()) {
            result += summaryRows;
        }
        result += QStringLiteral("</table>");
        bool needProcess = true;
        for (QString rows : flatParts) {
            if (needProcess && !rows.contains(QStringLiteral("Процесс выполнения"), Qt::CaseInsensitive)) {
                const int span = protocol126ProcessTitleColspan(rows);
                rows.prepend(QStringLiteral(
                    "<tr><td colspan='%1' align='center'>"
                    "<b>Процесс выполнения диагностического задания</b></td></tr>")
                                 .arg(span));
            }
            needProcess = false;
            result += tableOpen + rows + QStringLiteral("</table>");
        }
    }
    return ExerciseProtocol::normalizeSummaryColumnWidths(result);
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

QString ExerciseProtocol::normalizeSummaryColumnWidths(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    return normalizeSummaryColumnWidthsHtml(protocolBody);
}

QString ExerciseProtocol::canonicalizeProtocolHeaderFragment(const QString &headerFragment) {
    QString rows = headerFragment.trimmed();
    if (rows.isEmpty()) {
        return {};
    }
    // Убрать мусор до первой <table> (<strong>, &nbsp;, пустые теги) — иначе
    // ^<table не снимается, открывается вторая <table> и Qt вкладывает процесс в ячейку.
    {
        const int tableAt = rows.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
        if (tableAt > 0) {
            rows = rows.mid(tableAt);
        } else if (tableAt < 0) {
            // Только строки <tr> без обёртки.
            rows.replace(
                QRegularExpression(
                    QStringLiteral("^(?:\\s|<(?:strong|b|span|div|p|br)\\b[^>]*>|</(?:strong|b|span|div|p)>|&nbsp;)+"),
                    QRegularExpression::CaseInsensitiveOption),
                QString());
        }
    }
    // Убрать обёртку <table> / colgroup — откроем стандартную 671/200/471.
    rows.replace(
        QRegularExpression(
            QStringLiteral("^<table\\b[^>]*>\\s*"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    rows.replace(
        QRegularExpression(
            QStringLiteral("^<colgroup\\b[^>]*>[\\s\\S]*?</colgroup\\s*>\\s*"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption),
        QString());
    rows.replace(
        QRegularExpression(
            QStringLiteral("</table\\s*>\\s*$"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    // На случай вложенной второй <table> в хвосте шапки — отрезать.
    {
        const int nested = rows.indexOf(QStringLiteral("<table"), 0, Qt::CaseInsensitive);
        if (nested >= 0) {
            rows = rows.left(nested);
        }
        const int closeAt = rows.lastIndexOf(QStringLiteral("</table>"), -1, Qt::CaseInsensitive);
        if (closeAt >= 0) {
            rows = rows.left(closeAt);
        }
    }
    // <p> в ячейках шапки раздувают ширину в QTextDocument (вкладка «Протоколы»).
    rows.replace(
        QRegularExpression(QStringLiteral("<p\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption),
        QString());
    rows.replace(
        QRegularExpression(QStringLiteral("</p\\s*>"), QRegularExpression::CaseInsensitiveOption),
        QString());

    // Жёстко 200/471 на двухколоночных строках шапки.
    const QRegularExpression trRe(
        QStringLiteral("(<tr\\b[^>]*>)([\\s\\S]*?)(</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QString rebuilt;
    int trLast = 0;
    QRegularExpressionMatchIterator trIt = trRe.globalMatch(rows);
    while (trIt.hasNext()) {
        const QRegularExpressionMatch tr = trIt.next();
        rebuilt += rows.mid(trLast, tr.capturedStart() - trLast);
        QString rowInner = tr.captured(2);
        const QRegularExpression tdRe(
            QStringLiteral("(<td\\b)([^>]*)(>)([\\s\\S]*?)(</td>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QList<QRegularExpressionMatch> tds;
        QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(rowInner);
        while (tdIt.hasNext()) {
            tds.append(tdIt.next());
        }
        if (tds.size() == 2) {
            QString newRow;
            int cellLast = 0;
            for (int i = 0; i < tds.size(); ++i) {
                const QRegularExpressionMatch &td = tds.at(i);
                newRow += rowInner.mid(cellLast, td.capturedStart() - cellLast);
                QString attrs = td.captured(2);
                attrs.remove(QRegularExpression(
                    QStringLiteral("\\s*width\\s*=\\s*(?:'[^']*'|\"[^\"]*\"|\\d+)"),
                    QRegularExpression::CaseInsensitiveOption));
                attrs.remove(QRegularExpression(
                    QStringLiteral("\\s*style\\s*=\\s*['\"][^'\"]*['\"]"),
                    QRegularExpression::CaseInsensitiveOption));
                const QString w = i == 0 ? QStringLiteral("200") : QStringLiteral("471");
                attrs = QStringLiteral(" width='%1' style='width:%1px'").arg(w) + attrs;
                newRow += td.captured(1) + attrs + td.captured(3) + td.captured(4) + td.captured(5);
                cellLast = td.capturedEnd();
            }
            newRow += rowInner.mid(cellLast);
            rowInner = newRow;
        }
        rebuilt += tr.captured(1) + rowInner + tr.captured(3);
        trLast = tr.capturedEnd();
    }
    rebuilt += rows.mid(trLast);
    return protocolSummaryTableOpenHtml() + rebuilt;
}

void ExerciseProtocol::forceProtocolDocumentTableWidths(QTextDocument *document, int widthPx) {
    if (!document || widthPx <= 0) {
        return;
    }
    QList<QTextTable *> tables;
    collectTables(document->rootFrame(), tables);
    for (QTextTable *table : tables) {
        if (!table || table->columns() <= 0) {
            continue;
        }
        QTextTableFormat fmt = table->format();
        fmt.setWidth(QTextLength(QTextLength::FixedLength, widthPx));
        fmt.setBorder(1);
        fmt.setCellPadding(0);
        fmt.setCellSpacing(0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        fmt.setBorderCollapse(true);
#endif
        const int cols = table->columns();
        if (cols == 2) {
            // Шапка / Дата / OR-HLP задания 2: всегда 200 + (671-200).
            fmt.setColumnWidthConstraints({
                QTextLength(QTextLength::FixedLength, 200),
                QTextLength(QTextLength::FixedLength, widthPx - 200),
            });
        } else {
            QVector<QTextLength> constraints = fmt.columnWidthConstraints();
            bool applied = false;
            if (constraints.size() == cols) {
                qreal sum = 0;
                bool allFixed = true;
                for (const QTextLength &c : constraints) {
                    if (c.type() != QTextLength::FixedLength) {
                        allFixed = false;
                        break;
                    }
                    sum += c.rawValue();
                }
                if (allFixed && sum > 1.0 && qAbs(sum - widthPx) > 0.5) {
                    QVector<QTextLength> scaled;
                    scaled.reserve(cols);
                    qreal used = 0;
                    for (int i = 0; i < cols; ++i) {
                        if (i == cols - 1) {
                            scaled.append(QTextLength(QTextLength::FixedLength, widthPx - used));
                        } else {
                            const qreal w = qRound(constraints.at(i).rawValue() * widthPx / sum);
                            scaled.append(QTextLength(QTextLength::FixedLength, w));
                            used += w;
                        }
                    }
                    fmt.setColumnWidthConstraints(scaled);
                    applied = true;
                } else if (allFixed && qAbs(sum - widthPx) <= 0.5) {
                    applied = true; // уже корректная сетка
                }
            }
            if (!applied) {
                // Определяем тип по заголовку — нельзя всем 4-кол. давать сетку 1.26.
                QString header0;
                QString headerJoin;
                if (table->rows() > 0) {
                    for (int c = 0; c < cols; ++c) {
                        const QString h = readTableCellText(table, 0, c);
                        if (c == 0) {
                            header0 = h;
                        }
                        headerJoin += h + QLatin1Char(' ');
                    }
                }
                if (cols == 4
                    && (header0.contains(QStringLiteral("№/ответ"), Qt::CaseInsensitive)
                        || headerJoin.contains(QStringLiteral("№/ответ"), Qt::CaseInsensitive))) {
                    // 1.272: 120+251+250+50
                    fmt.setColumnWidthConstraints({
                        QTextLength(QTextLength::FixedLength, 120),
                        QTextLength(QTextLength::FixedLength, 251),
                        QTextLength(QTextLength::FixedLength, 250),
                        QTextLength(QTextLength::FixedLength, 50),
                    });
                } else if (cols == 4
                           && (headerJoin.contains(QStringLiteral("№ рассказа"), Qt::CaseInsensitive)
                               || headerJoin.contains(QStringLiteral("Правильный ответ"), Qt::CaseInsensitive))) {
                    fmt.setColumnWidthConstraints({
                        QTextLength(QTextLength::FixedLength, 70),
                        QTextLength(QTextLength::FixedLength, 120),
                        QTextLength(QTextLength::FixedLength, widthPx - 250),
                        QTextLength(QTextLength::FixedLength, 60),
                    });
                } else if (cols == 3
                           && headerJoin.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive)) {
                    fmt.setColumnWidthConstraints({
                        QTextLength(QTextLength::FixedLength, 200),
                        QTextLength(QTextLength::FixedLength, widthPx - 260),
                        QTextLength(QTextLength::FixedLength, 60),
                    });
                } else if (cols == 3
                           && headerJoin.contains(QStringLiteral("Характер"), Qt::CaseInsensitive)
                           && headerJoin.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)) {
                    fmt.setColumnWidthConstraints({
                        QTextLength(QTextLength::FixedLength, 300),
                        QTextLength(QTextLength::FixedLength, 300),
                        QTextLength(QTextLength::FixedLength, 71),
                    });
                } else if (cols == 3
                           && headerJoin.contains(QStringLiteral("Факт"), Qt::CaseInsensitive)
                           && headerJoin.contains(QStringLiteral("Характер"), Qt::CaseInsensitive)) {
                    // 3.1.12: 141+265+265 = 671
                    fmt.setColumnWidthConstraints({
                        QTextLength(QTextLength::FixedLength, 141),
                        QTextLength(QTextLength::FixedLength, 265),
                        QTextLength(QTextLength::FixedLength, 265),
                    });
                } else if (cols == 6
                           && (headerJoin.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)
                               || headerJoin.contains(QStringLiteral("Объяснение выбора"), Qt::CaseInsensitive))) {
                    // 3.1.10: 30+121+143+141+175+61 = 671
                    fmt.setColumnWidthConstraints({
                        QTextLength(QTextLength::FixedLength, 30),
                        QTextLength(QTextLength::FixedLength, 121),
                        QTextLength(QTextLength::FixedLength, 143),
                        QTextLength(QTextLength::FixedLength, 141),
                        QTextLength(QTextLength::FixedLength, 175),
                        QTextLength(QTextLength::FixedLength, 61),
                    });
                }
            }
        }
        table->setFormat(fmt);

        // Колонка «Баллы» / «Кол-во цифр» (4.2.1): цифры по центру в QTextEdit.
        if (table->rows() >= 1 && cols >= 3) {
            QVector<int> centerCols;
            for (int c = 0; c < cols; ++c) {
                const QString h = readTableCellText(table, 0, c).trimmed();
                if (h.compare(QStringLiteral("Баллы"), Qt::CaseInsensitive) == 0
                    || (h.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive) && h.length() <= 12)
                    || h.contains(QStringLiteral("Кол-во цифр"), Qt::CaseInsensitive)
                    || h.contains(QStringLiteral("нарастание"), Qt::CaseInsensitive)) {
                    centerCols.append(c);
                }
            }
            for (int ballsCol : centerCols) {
                for (int r = 1; r < table->rows(); ++r) {
                    QTextTableCell cell = table->cellAt(r, ballsCol);
                    if (!cell.isValid()) {
                        continue;
                    }
                    QTextCursor cur = cell.firstCursorPosition();
                    const int endPos = cell.lastCursorPosition().position();
                    while (cur.position() <= endPos && !cur.atEnd()) {
                        QTextBlockFormat bf = cur.blockFormat();
                        if (bf.alignment() != Qt::AlignHCenter) {
                            bf.setAlignment(Qt::AlignHCenter);
                            cur.mergeBlockFormat(bf);
                        }
                        const QTextBlock next = cur.block().next();
                        if (!next.isValid() || next.position() > endPos) {
                            break;
                        }
                        cur.setPosition(next.position());
                    }
                }
            }
        }

        // 3.2.2 / 3.2.3 и др.: при 1 короткой строке процесса (только «Выполнение»)
        // QTextEdit с overflow обрезает нижнюю линию у колонки «Факт выполнения/время».
        // Не трогаем border (иначе линии толстеют) — только нижний padding последней строки.
        if (table->rows() >= 1 && cols >= 3) {
            QString headerJoin;
            for (int c = 0; c < cols; ++c) {
                headerJoin += readTableCellText(table, 0, c) + QLatin1Char(' ');
            }
            if (headerJoin.contains(QStringLiteral("Факт"), Qt::CaseInsensitive)
                && headerJoin.contains(QStringLiteral("Характер"), Qt::CaseInsensitive)) {
                const int lastRow = table->rows() - 1;
                for (int c = 0; c < cols; ++c) {
                    QTextTableCell cell = table->cellAt(lastRow, c);
                    if (!cell.isValid()) {
                        continue;
                    }
                    QTextTableCellFormat cellFmt = cell.format().toTableCellFormat();
                    if (cellFmt.bottomPadding() < 2.0) {
                        cellFmt.setBottomPadding(2.0);
                        cell.setFormat(cellFmt);
                    }
                }
            }
        }
    }
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
                endPos = sessionEndBeforeNextDateRow(documentHtml, rowStart, nextRowStart);
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

    auto collect = [&](const QRegularExpression &re) {
        QRegularExpressionMatchIterator it = re.globalMatch(documentHtml);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            bodies.insert(match.captured(1), match.captured(2).trimmed());
        }
    };

    // Якоря <a id> (новый формат) и старые <span id>.
    collect(QRegularExpression(
        QStringLiteral(
            "id=[\"']dokit-pid-(\\d+)-start[\"'][^>]*>\\s*(?:</a>|</span>)?"
            "([\\s\\S]*?)"
            "<(?:a|span)\\b[^>]*id=[\"']dokit-pid-\\1-end[\"']"),
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption));
    if (!bodies.isEmpty()) {
        return bodies;
    }

    collect(QRegularExpression(
        QStringLiteral("<!--protocol-id:(\\d+)-->([\\s\\S]*?)<!--/protocol-id:\\1-->"),
        QRegularExpression::DotMatchesEverythingOption));
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
    const QStringList storedSessions = ExerciseProtocol::extractProtocolBodiesByDateRows(storedBody);

    // Вкладка «Протоколы»: в документе все методики. Нельзя сопоставлять editorSessions[0]
    // с телом другой методики — иначе в «Результат» попадают баллы/текст чужого протокола.
    if (!editorSessions.isEmpty()
        && editorSessions.size() > qMax(1, storedSessions.size())
        && protocolIndex >= 0
        && protocolIndex < editorSessions.size()) {
        ParsedProtocolFields parsed = parseProtocolFieldsFromHtml(editorSessions.at(protocolIndex), 0);
        if (!parsed.hasDateSpecialist && !parsed.hasResult && !parsed.hasNote
            && parsed.answersByIndex.isEmpty()) {
            parsed = parseProtocolFieldsFromDocument(editorDocument, protocolIndex);
        }
        if (!parsed.hasDateSpecialist && !parsed.hasResult && !parsed.hasNote
            && parsed.answersByIndex.isEmpty()) {
            return storedBody;
        }
        return applyParsedFieldsToStoredBody(storedBody, parsed);
    }

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
    // Строго 1.26 (эмоции): не путать с другими методиками, где тоже есть idvivod.
    return body.contains(QStringLiteral("id='col11'"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id=\"col11\""), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id='sum1'"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id=\"sum1\""), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id='col21'"), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("id=\"col21\""), Qt::CaseInsensitive)
        || body.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive);
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
                endPos = sessionEndBeforeNextDateRow(body, rowStart, nextRowStart);
            }
        }
        const QString chunk = body.mid(rowStart, endPos - rowStart).trimmed();
        if (!chunk.isEmpty()) {
            sessions.append(chunk);
        }
    }
    return sessions;
}

QString ExerciseProtocol::extractLastProtocol126Session(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    const QStringList sessions = extractProtocol126SessionsByDate(protocolBody);
    if (sessions.isEmpty() || sessions.size() == 1) {
        return stripLeadingSummaryTableWrapper(protocolBody);
    }
    return stripLeadingSummaryTableWrapper(sessions.last());
}

bool ExerciseProtocol::numberedStepPresentInSessionHtml(
    const QString &sessionHtml,
    const QString &stepId) {
    const QString sid = stepId.trimmed();
    if (sid.isEmpty() || sessionHtml.trimmed().isEmpty()) {
        return false;
    }
    if (sessionHtml.contains(
            QStringLiteral("<!--step") + sid + QStringLiteral("-->"), Qt::CaseInsensitive)) {
        return true;
    }
    const QRegularExpression rowRe(
        QStringLiteral(
            "<tr[^>]*>\\s*<td[^>]*align\\s*=\\s*['\"]center['\"][^>]*>\\s*%1\\s*</td>")
            .arg(QRegularExpression::escape(sid)),
        QRegularExpression::CaseInsensitiveOption);
    if (rowRe.match(sessionHtml).hasMatch()) {
        return true;
    }
    // После QTextDocument align иногда пропадает — первая ячейка строки = №.
    const QRegularExpression plainNumRe(
        QStringLiteral(
            "<tr[^>]*>\\s*<td[^>]*>\\s*(?:<[^>]+>\\s*)*%1\\s*(?:</[^>]+>\\s*)*</td>")
            .arg(QRegularExpression::escape(sid)),
        QRegularExpression::CaseInsensitiveOption);
    return plainNumRe.match(sessionHtml).hasMatch();
}

QString closeDanglingTables(QString html) {
    // По глубине вложенности, не по «opens-closes»: иначе хвост `<table>` следующей
    // сессии в конце куска не закрывается и 3-й протокол вкладывается в ячейку.
    int depth = 0;
    int pos = 0;
    static const QRegularExpression openRe(
        QStringLiteral("<table\\b"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression closeRe(
        QStringLiteral("</table\\s*>"), QRegularExpression::CaseInsensitiveOption);
    while (pos < html.size()) {
        const QRegularExpressionMatch openMatch = openRe.match(html, pos);
        const QRegularExpressionMatch closeMatch = closeRe.match(html, pos);
        const int openPos = openMatch.hasMatch() ? openMatch.capturedStart() : -1;
        const int closePos = closeMatch.hasMatch() ? closeMatch.capturedStart() : -1;
        if (openPos < 0 && closePos < 0) {
            break;
        }
        if (openPos >= 0 && (closePos < 0 || openPos < closePos)) {
            ++depth;
            pos = openMatch.capturedEnd();
            continue;
        }
        if (depth > 0) {
            --depth;
        }
        pos = closeMatch.capturedEnd();
    }
    while (depth-- > 0) {
        html += QStringLiteral("</table>");
    }
    return html;
}

QString joinProtocol126Sessions(const QStringList &sessions) {
    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        // Каждую сессию привести к summary</table><!--s-->+process</table>.
        QString session = ensureClosedProtocolSession(sessions.at(i));
        if (session.isEmpty()) {
            continue;
        }
        session.replace(
            QRegularExpression(
                QStringLiteral("^(?:\\s|<br\\s*/?>|<p\\b[^>]*>\\s*(?:&nbsp;|\\s)*\\s*</p\\s*>)+"),
                QRegularExpression::CaseInsensitiveOption),
            QString());
        // Срезать висячий незакрытый <table> в конце куска (артефакт extract).
        session.replace(
            QRegularExpression(
                QStringLiteral("<table\\b[^>]*>\\s*$"),
                QRegularExpression::CaseInsensitiveOption),
            QString());
        if (i == 0) {
            session = stripLeadingSummaryTableWrapper(session);
            session = ensureClosedProtocolSession(session);
        } else {
            result = closeDanglingTables(result);
            result.replace(
                QRegularExpression(
                    QStringLiteral("(?:\\s|<br\\s*/?>|<p\\b[^>]*>\\s*(?:&nbsp;|\\s)*\\s*</p\\s*>)+$"),
                    QRegularExpression::CaseInsensitiveOption),
                QString());
            result.replace(
                QRegularExpression(
                    QStringLiteral("<table\\b[^>]*>\\s*$"),
                    QRegularExpression::CaseInsensitiveOption),
                QString());
            if (!session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
                session.prepend(protocolSummaryTableOpenHtml());
            }
        }
        result += session;
    }
    return normalizeSummaryColumnWidthsHtml(closeDanglingTables(result));
}

int findMatchingTableEnd(const QString &html, int tableStart) {
    if (tableStart < 0 || tableStart >= html.size()) {
        return -1;
    }
    static const QRegularExpression openRe(
        QStringLiteral("<table\\b"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression closeRe(
        QStringLiteral("</table\\s*>"), QRegularExpression::CaseInsensitiveOption);
    int depth = 0;
    int pos = tableStart;
    while (pos < html.size()) {
        const QRegularExpressionMatch openMatch = openRe.match(html, pos);
        const QRegularExpressionMatch closeMatch = closeRe.match(html, pos);
        const int openPos = openMatch.hasMatch() ? openMatch.capturedStart() : -1;
        const int closePos = closeMatch.hasMatch() ? closeMatch.capturedStart() : -1;
        if (closePos < 0) {
            return -1;
        }
        if (openPos >= 0 && openPos < closePos) {
            ++depth;
            pos = openMatch.capturedEnd();
            continue;
        }
        --depth;
        pos = closeMatch.capturedEnd();
        if (depth == 0) {
            return pos;
        }
    }
    return -1;
}

bool isProtocol126TaskTable(const QString &tableHtml) {
    return tableHtml.contains(QStringLiteral("Задание 1"), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("Задание 2"), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("№ рассказа"), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("id='col11'"), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("id=\"col11\""), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("id='col21'"), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("id=\"col21\""), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("id='sum1'"), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("id=\"sum1\""), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("id='sum2'"), Qt::CaseInsensitive)
        || tableHtml.contains(QStringLiteral("id=\"sum2\""), Qt::CaseInsensitive);
}

QStringList extractProtocol126TaskTables(QString *htmlInOut) {
    QStringList tables;
    if (!htmlInOut) {
        return tables;
    }
    QString &work = *htmlInOut;
    bool extracted = true;
    while (extracted) {
        extracted = false;
        int searchFrom = 0;
        while (searchFrom < work.size()) {
            const int start = work.indexOf(QStringLiteral("<table"), searchFrom, Qt::CaseInsensitive);
            if (start < 0) {
                break;
            }
            const int end = findMatchingTableEnd(work, start);
            if (end < 0) {
                break;
            }
            const QString table = work.mid(start, end - start);
            if (isProtocol126TaskTable(table)) {
                tables.append(table);
                work = work.left(start) + work.mid(end);
                extracted = true;
                break;
            }
            searchFrom = start + 6;
        }
    }
    return tables;
}

QString cleanProtocol126SummaryRows(QString summary) {
    summary = stripLeadingSummaryTableWrapper(summary.trimmed());
    summary.replace(QStringLiteral("<!--s-->"), QString());
    summary.replace(
        QRegularExpression(QStringLiteral("</table\\s*>"), QRegularExpression::CaseInsensitiveOption),
        QString());
    // Старая разметка: строка «Процесс…» в таблице — убираем (заголовок теперь <p>).
    summary.replace(
        QRegularExpression(
            QStringLiteral(
                "<tr\\b[^>]*>\\s*<td\\b[^>]*>\\s*Процесс\\s+выполнения\\s+диагностического\\s+задания"
                "[\\s\\S]*?</tr>"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption),
        QString());
    summary.replace(
        QRegularExpression(
            QStringLiteral(
                "<tr\\b[^>]*>\\s*<td\\b[^>]*>\\s*Процесс\\s+выполнения\\s+диагностической\\s+методики"
                "[\\s\\S]*?</tr>"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption),
        QString());
    summary.replace(
        QRegularExpression(
            QStringLiteral(
                "<p\\b[^>]*>\\s*(?:<b>)?\\s*Процесс\\s+выполнения\\s+диагностического\\s+задания\\s*(?:</b>)?\\s*</p>"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    summary.replace(
        QRegularExpression(
            QStringLiteral(
                "<p\\b[^>]*>\\s*(?:<b>)?\\s*Процесс\\s+выполнения\\s+диагностической\\s+методики\\s*(?:</b>)?\\s*</p>"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    summary.replace(
        QRegularExpression(QStringLiteral("(<br\\s*/?>\\s*)+$"), QRegularExpression::CaseInsensitiveOption),
        QString());
    return summary.trimmed();
}

QString extractTableInnerRows(const QString &tableHtml) {
    const int openEnd = tableHtml.indexOf(QLatin1Char('>'));
    if (openEnd < 0) {
        return {};
    }
    int close = tableHtml.lastIndexOf(QStringLiteral("</table>"), -1, Qt::CaseInsensitive);
    if (close < 0) {
        close = tableHtml.size();
    }
    return tableHtml.mid(openEnd + 1, close - (openEnd + 1)).trimmed();
}

QString normalizeProtocol126TaskRows(QString taskRows) {
    // Через общий нормализатор: width на ячейках + colgroup (70+120+421+60).
    const QString wrapped = QStringLiteral("<table width='671'>") + taskRows + QStringLiteral("</table>");
    return extractTableInnerRows(ExerciseProtocol::normalizeSummaryColumnWidths(wrapped));
}

QString canonicalizeProtocol126Session(QString session) {
    session = stripLeadingSummaryTableWrapper(session.trimmed());
    if (session.isEmpty()) {
        return session;
    }

    QString work = session;
    const QStringList taskTables = extractProtocol126TaskTables(&work);

    // work без task-<table>: summary + возможно <p>Процесс + голые <tr> задания 2.
    QString afterMarker;
    const int marker = work.indexOf(QStringLiteral("<!--s-->"));
    QString beforeMarker = marker >= 0 ? work.left(marker) : work;
    if (marker >= 0) {
        afterMarker = work.mid(marker + QStringLiteral("<!--s-->").size());
    }
    afterMarker.replace(
        QRegularExpression(
            QStringLiteral(
                "<p\\b[^>]*>\\s*(?:<b>)?\\s*Процесс\\s+выполнения\\s+диагностического\\s+задания\\s*(?:</b>)?\\s*</p>\\s*"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    afterMarker.replace(
        QRegularExpression(QStringLiteral("^(?:<br\\s*/?>\\s*)+"), QRegularExpression::CaseInsensitiveOption),
        QString());

    QString summary = cleanProtocol126SummaryRows(beforeMarker);
    QStringList taskParts;
    for (const QString &table : taskTables) {
        const QString rows = extractTableInnerRows(table).trimmed();
        if (!rows.isEmpty()) {
            taskParts.append(rows);
        }
    }
    const QString bareRows = afterMarker.trimmed();
    if (bareRows.contains(QStringLiteral("<tr"), Qt::CaseInsensitive)) {
        taskParts.append(bareRows);
    }

    // Разделить склейки «Задание 1+2» и «OR/HLP + рассказы» (иначе width > 671).
    const QStringList flatParts = splitProtocol126FlatTaskParts(taskParts);

    QString result = summary + QStringLiteral("</table><!--s-->");
    bool needProcess = true;
    for (QString rows : flatParts) {
        if (needProcess && !rows.contains(QStringLiteral("Процесс выполнения"), Qt::CaseInsensitive)) {
            const int span = protocol126ProcessTitleColspan(rows);
            rows.prepend(QStringLiteral(
                "<tr><td colspan='%1' align='center'>"
                "<b>Процесс выполнения диагностического задания</b></td></tr>")
                             .arg(span));
        }
        needProcess = false;
        result += QString::fromUtf8(kProtocol126ProcessTableOpen) + rows + QStringLiteral("</table>");
    }
    return ExerciseProtocol::normalizeSummaryColumnWidths(result);
}

QString ExerciseProtocol::mergeLimitedEditableFieldsIntoStoredBody(
    const QString &storedBody,
    QTextDocument *editorDocument) {
    if (storedBody.trimmed().isEmpty() || !editorDocument) {
        return storedBody;
    }

    // Берём поля из последней сессии в редакторе (повторный протокол), не из первой.
    int editorSectionIndex = 0;
    {
        const QStringList editorSessions =
            ExerciseProtocol::extractProtocolBodiesByDateRows(editorDocument->toHtml());
        if (editorSessions.size() > 1) {
            editorSectionIndex = editorSessions.size() - 1;
        }
    }
    ParsedProtocolFields parsed = parseProtocolFieldsFromDocument(editorDocument, editorSectionIndex);

    // Примечание из таблицы / idnote (на «Протоколы» id часто теряется).
    QString noteFromTable;
    bool haveNoteFromTable = false;
    {
        QList<QTextTable *> tables;
        collectTables(editorDocument->rootFrame(), tables);
        for (QTextTable *table : tables) {
            if (!table || table->columns() < 2) {
                continue;
            }
            for (int r = 0; r < table->rows(); ++r) {
                const QString first = readTableCellText(table, r, 0);
                if (!first.contains(QStringLiteral("Примечание"), Qt::CaseInsensitive)) {
                    continue;
                }
                QString second = readTableCellMultilineText(table, r, 1);
                if (second.contains(QStringLiteral("Стимульные"), Qt::CaseInsensitive)
                    || second.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)
                    || second.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                    continue;
                }
                noteFromTable = second.trimmed();
                haveNoteFromTable = true;
            }
        }
    }
    if (haveNoteFromTable) {
        parsed.hasNote = true;
        parsed.noteText = noteFromTable;
    } else {
        const QString editorHtml = editorDocument->toHtml();
        if (editorHtml.contains(QStringLiteral("idnote"), Qt::CaseInsensitive)) {
            parsed.hasNote = true;
            parsed.noteText =
                htmlFragmentToPlainText(extractDivInnerById(editorHtml, QStringLiteral("idnote")));
        }
    }

    if (!parsed.hasResult && !parsed.hasNote) {
        return storedBody;
    }

    auto applyNoteSafely = [](QString chunk, const ParsedProtocolFields &fields) {
        if (!fields.hasNote) {
            return chunk;
        }
        // Не затирать сохранённое примечание пустым чтением из редактора.
        const QString existing =
            htmlFragmentToPlainText(extractDivInnerById(chunk, QStringLiteral("idnote"))).trimmed();
        if (fields.noteText.trimmed().isEmpty() && !existing.isEmpty()) {
            return chunk;
        }
        if (chunk.contains(QStringLiteral("idnote"), Qt::CaseInsensitive)) {
            return replaceDivInnerById(
                chunk, QStringLiteral("idnote"), fields.noteText.toHtmlEscaped());
        }
        return replaceRowSecondCell(chunk, QStringLiteral("Примечание"), fields.noteText);
    };

    // 1.26: только Результат/Примечание последней сессии — без joinClosed.
    if (looksLikeProtocol126Body(storedBody)) {
        QStringList sessions = extractProtocol126SessionsByDate(storedBody);
        if (sessions.isEmpty()) {
            QString body = storedBody;
            if (parsed.hasResult) {
                body = replaceResultRowSecondCell(body, parsed.resultText);
            }
            body = applyNoteSafely(body, parsed);
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
        last = applyNoteSafely(last, parsed);
        sessions[sessions.size() - 1] = last;
        return joinProtocol126Sessions(sessions);
    }

    QStringList sessions = extractProtocolBodiesByDateRows(storedBody);
    if (sessions.isEmpty()) {
        QString body = storedBody;
        if (parsed.hasResult) {
            body = replaceResultRowSecondCell(body, parsed.resultText);
        }
        body = applyNoteSafely(body, parsed);
        return ensureClosedProtocolSession(body);
    }

    QString last = sessions.last();
    if (parsed.hasResult) {
        if (last.contains(QStringLiteral("idvivod"), Qt::CaseInsensitive)) {
            last = replaceDivInnerById(last, QStringLiteral("idvivod"), parsed.resultText.toHtmlEscaped());
        } else {
            last = replaceResultRowSecondCell(last, parsed.resultText);
        }
    }
    last = applyNoteSafely(last, parsed);
    sessions[sessions.size() - 1] = last;
    return joinClosedProtocolSessions(sessions);
}

namespace {

QString replace418DivFromEditor(
    QString body,
    const QString &editorHtml,
    const QString &id,
    bool skipIfEditorEmpty) {
    const QRegularExpression re(
        QStringLiteral("<div\\b[^>]*\\bid\\s*=\\s*['\"]%1['\"][^>]*>([\\s\\S]*?)</div>")
            .arg(QRegularExpression::escape(id)),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(editorHtml);
    if (!match.hasMatch()) {
        return body;
    }
    const QString inner = match.captured(1);
    if (skipIfEditorEmpty && htmlFragmentToPlainText(inner).trimmed().isEmpty()) {
        return body;
    }
    const QRegularExpression targetRe(
        QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"]%1['\"][^>]*>)([\\s\\S]*?)(</div>)")
            .arg(QRegularExpression::escape(id)),
        QRegularExpression::CaseInsensitiveOption);
    // Последняя сессия: ищем последнее вхождение id (при повторных протоколах).
    QRegularExpressionMatchIterator it = targetRe.globalMatch(body);
    QRegularExpressionMatch target;
    while (it.hasNext()) {
        target = it.next();
    }
    if (!target.hasMatch()) {
        return body;
    }
    body.replace(
        target.capturedStart(0),
        target.capturedLength(0),
        target.captured(1) + inner + target.captured(3));
    return body;
}

} // namespace

QString ExerciseProtocol::mergeProtocol418EditorIntoStoredBody(
    const QString &storedBody,
    QTextDocument *editorDocument) {
    if (storedBody.trimmed().isEmpty() || !editorDocument) {
        return storedBody;
    }

    // Нельзя mergeLimited → joinClosedProtocolSessions: у 4.1.8 после <!--s--> две
    // последовательные <table> (характер + стимулы), и ensureClosed обрезает вторую
    // (со словами/баллами) — протокол «очищается».
    QString body = storedBody;
    const QString editorHtml = editorDocument->toHtml();

    // Примечание: надёжнее читать из QTextTable (на «Протоколы» idnote в toHtml часто теряется).
    QString noteFromTable;
    bool haveNoteFromTable = false;
    {
        QList<QTextTable *> noteTables;
        collectTables(editorDocument->rootFrame(), noteTables);
        for (QTextTable *table : noteTables) {
            if (!table || table->columns() < 2) {
                continue;
            }
            for (int r = 0; r < table->rows(); ++r) {
                const QString first = readTableCellText(table, r, 0);
                if (!first.contains(QStringLiteral("Примечание"), Qt::CaseInsensitive)) {
                    continue;
                }
                QString second = readTableCellMultilineText(table, r, 1);
                if (second.contains(QStringLiteral("Стимульные"), Qt::CaseInsensitive)
                    || second.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)
                    || second.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                    continue;
                }
                noteFromTable = second.trimmed();
                haveNoteFromTable = true;
            }
        }
    }

    {
        int editorSectionIndex = 0;
        const QStringList editorSessions =
            ExerciseProtocol::extractProtocolBodiesByDateRows(editorHtml);
        if (editorSessions.size() > 1) {
            editorSectionIndex = editorSessions.size() - 1;
        }
        ParsedProtocolFields parsed =
            parseProtocolFieldsFromDocument(editorDocument, editorSectionIndex);

        QStringList sessions = extractProtocol126SessionsByDate(body);
        const bool multi = sessions.size() > 1;
        QString chunk = multi ? sessions.last() : body;

        if (parsed.hasResult) {
            if (chunk.contains(QStringLiteral("idvivod"), Qt::CaseInsensitive)) {
                chunk = replaceDivInnerById(
                    chunk, QStringLiteral("idvivod"), parsed.resultText.toHtmlEscaped());
            } else {
                chunk = replaceResultRowSecondCell(chunk, parsed.resultText);
            }
        }

        QString notePlain;
        bool haveReliableNote = false;
        if (haveNoteFromTable) {
            notePlain = noteFromTable;
            haveReliableNote = true;
        } else {
            const bool editorHasIdnote =
                editorHtml.contains(QStringLiteral("id='idnote'"), Qt::CaseInsensitive)
                || editorHtml.contains(QStringLiteral("id=\"idnote\""), Qt::CaseInsensitive)
                || editorHtml.contains(QStringLiteral("id=idnote"), Qt::CaseInsensitive);
            if (editorHasIdnote) {
                notePlain = htmlFragmentToPlainText(
                    extractDivInnerById(editorHtml, QStringLiteral("idnote")));
                haveReliableNote = true;
            } else if (parsed.hasNote) {
                notePlain = parsed.noteText;
                haveReliableNote = true;
            }
        }
        if (haveReliableNote) {
            // Не затирать уже сохранённое примечание пустым чтением из редактора
            // (типично при «Подвести итог» / autosave, пока Qt не отдал текст ячейки).
            const QString existingNote =
                htmlFragmentToPlainText(extractDivInnerById(chunk, QStringLiteral("idnote")))
                    .trimmed();
            if (notePlain.trimmed().isEmpty() && !existingNote.isEmpty()) {
                haveReliableNote = false;
            }
        }
        if (haveReliableNote) {
            if (chunk.contains(QStringLiteral("idnote"), Qt::CaseInsensitive)) {
                chunk = replaceDivInnerById(
                    chunk, QStringLiteral("idnote"), notePlain.toHtmlEscaped());
            } else {
                // Допускаем <p>Примечание</p> внутри td (как после Qt / template.html).
                const QRegularExpression noteRowRe(
                    QStringLiteral(
                        "(<tr[^>]*>\\s*<td[^>]*>[\\s\\S]*?Примечание[\\s\\S]*?</td>\\s*<td[^>]*>)"
                        "([\\s\\S]*?)(</td>\\s*</tr>)"),
                    QRegularExpression::CaseInsensitiveOption
                        | QRegularExpression::DotMatchesEverythingOption);
                QRegularExpressionMatchIterator it = noteRowRe.globalMatch(chunk);
                QRegularExpressionMatch match;
                while (it.hasNext()) {
                    match = it.next();
                }
                if (match.hasMatch()) {
                    chunk.replace(
                        match.capturedStart(0),
                        match.capturedLength(0),
                        match.captured(1)
                            + QStringLiteral("<div contenteditable='true' id='idnote'>%1</div>")
                                  .arg(notePlain.toHtmlEscaped())
                            + match.captured(3));
                }
            }
        }

        if (multi) {
            sessions[sessions.size() - 1] = chunk;
            body = joinProtocol126Sessions(sessions);
        } else {
            body = chunk;
        }
    }

    // Характер / дата / результат — по id. Примечание — только непустым значением.
    body = replace418DivFromEditor(body, editorHtml, QStringLiteral("cidd"), true);
    body = replace418DivFromEditor(body, editorHtml, QStringLiteral("idspc"), true);
    body = replace418DivFromEditor(body, editorHtml, QStringLiteral("idvivod"), true);
    if (haveNoteFromTable && !noteFromTable.trimmed().isEmpty()) {
        // Последняя сессия: заменить последний idnote.
        const QRegularExpression idnoteRe(
            QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"]idnote['\"][^>]*>)([\\s\\S]*?)(</div>)"),
            QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = idnoteRe.globalMatch(body);
        QRegularExpressionMatch last;
        while (it.hasNext()) {
            last = it.next();
        }
        if (last.hasMatch()) {
            body.replace(
                last.capturedStart(0),
                last.capturedLength(0),
                last.captured(1) + noteFromTable.toHtmlEscaped() + last.captured(3));
        }
    } else if (!haveNoteFromTable) {
        body = replace418DivFromEditor(body, editorHtml, QStringLiteral("idnote"), true);
    }

    // Таблица стимулов: читаем ячейки из QTextTable (id часто теряются в toHtml).
    static const char *kWords[] = {"Школа", "Обед", "Утро", "Красота", "Прогулка"};
    static const char *kPrefixes[] = {"sel", "ex", "re", "hlp", "rea", "b"};

    QList<QTextTable *> tables;
    collectTables(editorDocument->rootFrame(), tables);
    for (QTextTable *table : tables) {
        if (!table || table->columns() < 7) {
            continue;
        }
        int headerRow = -1;
        int wordCol = -1;
        int cols[6] = {-1, -1, -1, -1, -1, -1};
        for (int r = 0; r < qMin(3, table->rows()); ++r) {
            for (int c = 0; c < table->columns(); ++c) {
                const QString h = readTableCellText(table, r, c);
                if (h.contains(QStringLiteral("Стимульные"), Qt::CaseInsensitive)) {
                    wordCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Выбранная"), Qt::CaseInsensitive)) {
                    cols[0] = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Объяснение"), Qt::CaseInsensitive)) {
                    cols[1] = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("до предъявления"), Qt::CaseInsensitive)
                    || (h.contains(QStringLiteral("Воспроиз"), Qt::CaseInsensitive)
                        && h.contains(QStringLiteral("до"), Qt::CaseInsensitive))) {
                    cols[2] = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                    && !h.contains(QStringLiteral("возможной"), Qt::CaseInsensitive)) {
                    cols[3] = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("после предъявления"), Qt::CaseInsensitive)
                    || (h.contains(QStringLiteral("Воспроиз"), Qt::CaseInsensitive)
                        && h.contains(QStringLiteral("после"), Qt::CaseInsensitive))) {
                    cols[4] = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive) && h.length() <= 16) {
                    cols[5] = c;
                    headerRow = r;
                }
            }
            if (headerRow >= 0 && wordCol >= 0 && cols[0] >= 0) {
                break;
            }
        }
        if (headerRow < 0 || wordCol < 0) {
            continue;
        }

        for (int r = headerRow + 1; r < table->rows(); ++r) {
            const QString word = readTableCellText(table, r, wordCol).trimmed();
            int wordIndex = -1;
            for (int i = 0; i < 5; ++i) {
                if (word.contains(QString::fromUtf8(kWords[i]), Qt::CaseInsensitive)
                    || QString::fromUtf8(kWords[i]).compare(word, Qt::CaseInsensitive) == 0) {
                    wordIndex = i;
                    break;
                }
            }
            if (wordIndex < 0) {
                // Строка «Итоговая оценка» — сохранить idsum из редактора.
                if (word.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive) && cols[5] >= 0) {
                    const QString sumText = readTableCellText(table, r, cols[5]);
                    if (!sumText.trimmed().isEmpty()) {
                        const QRegularExpression idsumRe(
                            QStringLiteral(
                                "(<div\\b[^>]*\\bid\\s*=\\s*['\"]idsum['\"][^>]*>)([\\s\\S]*?)(</div>)"),
                            QRegularExpression::CaseInsensitiveOption);
                        QRegularExpressionMatchIterator it = idsumRe.globalMatch(body);
                        QRegularExpressionMatch target;
                        while (it.hasNext()) {
                            target = it.next();
                        }
                        if (target.hasMatch()) {
                            body.replace(
                                target.capturedStart(0),
                                target.capturedLength(0),
                                target.captured(1) + sumText.toHtmlEscaped() + target.captured(3));
                        }
                    }
                }
                continue;
            }
            for (int c = 0; c < 6; ++c) {
                if (cols[c] < 0) {
                    continue;
                }
                const QString val = readTableCellMultilineText(table, r, cols[c]);
                // Пустое чтение из QTextDocument не должно затирать сохранённые ячейки.
                if (val.trimmed().isEmpty()) {
                    continue;
                }
                const QString id =
                    QString::fromUtf8(kPrefixes[c]) + QString::number(wordIndex + 1);
                const QRegularExpression targetRe(
                    QStringLiteral(
                        "(<div\\b[^>]*\\bid\\s*=\\s*['\"]%1['\"][^>]*>)([\\s\\S]*?)(</div>)")
                        .arg(QRegularExpression::escape(id)),
                    QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatchIterator it = targetRe.globalMatch(body);
                QRegularExpressionMatch target;
                while (it.hasNext()) {
                    target = it.next();
                }
                if (!target.hasMatch()) {
                    continue;
                }
                body.replace(
                    target.capturedStart(0),
                    target.capturedLength(0),
                    target.captured(1) + formatProtocolCellText(val) + target.captured(3));
            }
        }
        break;
    }

    // Fallback: id из editorHtml, если таблица не разобралась.
    static const char *kIds[] = {
        "b1", "b2", "b3", "b4", "b5", "sel1", "sel2", "sel3", "sel4", "sel5",
        "ex1", "ex2", "ex3", "ex4", "ex5", "re1", "re2", "re3", "re4", "re5",
        "hlp1", "hlp2", "hlp3", "hlp4", "hlp5", "rea1", "rea2", "rea3", "rea4", "rea5",
        "idsum"};
    for (const char *idRaw : kIds) {
        body = replace418DivFromEditor(
            body, editorHtml, QString::fromUtf8(idRaw), true);
    }

    return canonicalizeProtocol418StoredBody(body);
}

QString replaceDivInnerById(QString html, const QString &divId, const QString &innerHtml) {
    const QRegularExpression re(
        QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"]%1['\"][^>]*>)([\\s\\S]*?)(</div>)")
            .arg(QRegularExpression::escape(divId)),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(html);
    if (!match.hasMatch()) {
        return html;
    }
    // Только первое вхождение: при повторных сессиях id дублируются (col11, sum1, …).
    return html.left(match.capturedStart())
        + match.captured(1) + innerHtml + match.captured(3)
        + html.mid(match.capturedEnd());
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

// Сумма всех idbN / idsN без требования непрерывной нумерации.
double sumAllDivIdsWithPrefix(const QString &html, const QString &prefix) {
    double sum = 0;
    const QRegularExpression re(
        QStringLiteral("<div\\b[^>]*\\bid\\s*=\\s*['\"]%1(\\d+)['\"][^>]*>([\\s\\S]*?)</div>")
            .arg(QRegularExpression::escape(prefix)),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = re.globalMatch(html);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        QString plain = htmlFragmentToPlainText(match.captured(2)).trimmed();
        plain.replace(QLatin1Char(','), QLatin1Char('.'));
        plain.remove(QRegularExpression(QStringLiteral("[^0-9.\\-]")));
        bool ok = false;
        const double value = plain.toDouble(&ok);
        if (ok) {
            sum += value;
        }
    }
    return sum;
}

QString formatBallsNumber(double value) {
    if (qFuzzyCompare(value + 1.0, std::floor(value) + 1.0)) {
        return QString::number(static_cast<int>(value));
    }
    return QString::number(value, 'f', 1);
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
    const QStringList expected1 = expectedEmotionsTask1();
    for (int i = 0; i < expected1.size(); ++i) {
        const QString id = QStringLiteral("col1%1").arg(i + 1);
        QString answer = htmlFragmentToPlainText(
            extractDivInnerById(body, QStringLiteral("ans1%1").arg(i + 1))).trimmed();
        if (answer.isEmpty()) {
            static const QStringList kTask1Labels = {
                QStringLiteral("Радость"), QStringLiteral("Злость"), QStringLiteral("Грусть"),
                QStringLiteral("Страх"), QStringLiteral("Удивление"), QStringLiteral("Спокойствие"),
            };
            answer = extractAnswerFromLabeledRow(body, kTask1Labels.at(i));
        }
        if (answer.isEmpty()) {
            // Не обнулять вручную введённые баллы, если ответа нет.
            continue;
        }
        const int score = scoreEmotionAnswer(answer, expected1.at(i));
        body = replaceDivInnerById(body, id, QString::number(score));
    }

    const QStringList expected2 = expectedEmotionsTask2();
    const QStringList expectedPlain = {
        QStringLiteral("Злость"), QStringLiteral("Грусть"), QStringLiteral("Спокойное"),
        QStringLiteral("Злость"), QStringLiteral("Удивление"), QStringLiteral("Радость"),
        QStringLiteral("Страх"), QStringLiteral("Спокойное"), QStringLiteral("Грусть"),
        QStringLiteral("Страх"), QStringLiteral("Радость"), QStringLiteral("Удивление"),
    };
    const bool hasCorrectColumn =
        body.contains(QStringLiteral("Правильный ответ"), Qt::CaseInsensitive);
    for (int i = 0; i < expected2.size(); ++i) {
        const QString id = QStringLiteral("col2%1").arg(i + 1);
        // Только ответ ребёнка (ans2*), не колонка «Правильный ответ».
        QString answer = htmlFragmentToPlainText(
            extractDivInnerById(body, QStringLiteral("ans2%1").arg(i + 1))).trimmed();
        const bool fromAnsId = !answer.isEmpty();
        if (answer.isEmpty() && !hasCorrectColumn) {
            // Старый формат без колонки эталона: № | ответ | баллы.
            answer = extractAnswerFromLabeledRow(body, QString::number(i + 1));
        } else if (answer.isEmpty() && hasCorrectColumn) {
            // № | эталон | ответ ребёнка | баллы — берём 3-ю колонку.
            const QString escaped = QRegularExpression::escape(QString::number(i + 1));
            const QRegularExpression re4(
                QStringLiteral(
                    "<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>[\\s\\S]*?</td>\\s*"
                    "<td[^>]*>([\\s\\S]*?)</td>\\s*<td[^>]*>")
                    .arg(escaped),
                QRegularExpression::CaseInsensitiveOption
                    | QRegularExpression::DotMatchesEverythingOption);
            const QRegularExpressionMatch m4 = re4.match(body);
            if (m4.hasMatch()) {
                answer = htmlFragmentToPlainText(m4.captured(1)).trimmed();
            }
        }
        // Не считать эталон ответом ребёнка, только если ans2* пуст и 3-я колонка пуста.
        if (!fromAnsId && i < expectedPlain.size()
            && answer.compare(expectedPlain.at(i), Qt::CaseInsensitive) == 0
            && hasCorrectColumn) {
            const QString escaped = QRegularExpression::escape(QString::number(i + 1));
            const QRegularExpression re4(
                QStringLiteral(
                    "<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>[\\s\\S]*?</td>\\s*"
                    "<td[^>]*>([\\s\\S]*?)</td>\\s*<td[^>]*>")
                    .arg(escaped),
                QRegularExpression::CaseInsensitiveOption
                    | QRegularExpression::DotMatchesEverythingOption);
            const QRegularExpressionMatch m4 = re4.match(body);
            const QString third = m4.hasMatch()
                ? htmlFragmentToPlainText(m4.captured(1)).trimmed()
                : QString();
            if (third.isEmpty()) {
                answer.clear();
            }
        }
        if (answer.isEmpty()) {
            // Не обнулять вручную введённые баллы.
            continue;
        }
        const int score = scoreEmotionAnswer(answer, expected2.at(i));
        body = replaceDivInnerById(body, id, QString::number(score));
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

    auto applyManualScoresFromEditor = [&](QString chunk) -> QString {
        if (!editorDocument) {
            return chunk;
        }
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
                bool wrote = false;
                for (int n = 1; n <= 16 && !wrote; ++n) {
                    const QString id1 = QStringLiteral("col1%1").arg(n);
                    const QString id2 = QStringLiteral("col2%1").arg(n);
                    if (chunk.contains(QStringLiteral("id='%1'").arg(id1))
                        || chunk.contains(QStringLiteral("id=\"%1\"").arg(id1))) {
                        static const QStringList kTask1 = {
                            QStringLiteral("Радость"), QStringLiteral("Злость"),
                            QStringLiteral("Грусть"), QStringLiteral("Страх"),
                            QStringLiteral("Удивление"), QStringLiteral("Спокойствие"),
                        };
                        if (n <= kTask1.size()
                            && label.compare(kTask1.at(n - 1), Qt::CaseInsensitive) == 0) {
                            chunk = replaceDivInnerById(chunk, id1, score);
                            wrote = true;
                        }
                    }
                    if (!wrote && label == QString::number(n)
                        && (chunk.contains(QStringLiteral("id='%1'").arg(id2))
                            || chunk.contains(QStringLiteral("id=\"%1\"").arg(id2)))) {
                        chunk = replaceDivInnerById(chunk, id2, score);
                        wrote = true;
                    }
                }
            }
        }
        return chunk;
    };

    auto applyAnswersFromEditor = [&](QString chunk) -> QString {
        if (!editorDocument) {
            return chunk;
        }
        static const QStringList kTask1 = {
            QStringLiteral("Радость"), QStringLiteral("Злость"),
            QStringLiteral("Грусть"), QStringLiteral("Страх"),
            QStringLiteral("Удивление"), QStringLiteral("Спокойствие"),
        };
        static const QStringList kTask2Correct = {
            QStringLiteral("Злость"), QStringLiteral("Грусть"), QStringLiteral("Спокойное"),
            QStringLiteral("Злость"), QStringLiteral("Удивление"), QStringLiteral("Радость"),
            QStringLiteral("Страх"), QStringLiteral("Спокойное"), QStringLiteral("Грусть"),
            QStringLiteral("Страх"), QStringLiteral("Радость"), QStringLiteral("Удивление"),
        };
        QList<QTextTable *> tables;
        collectTables(editorDocument->rootFrame(), tables);
        for (QTextTable *table : tables) {
            if (!table || table->columns() < 2) {
                continue;
            }
            for (int r = 0; r < table->rows(); ++r) {
                const QString label = readTableCellText(table, r, 0);
                if (label.isEmpty()
                    || label.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Индекс"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Характер"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Задание"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Ответ"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Правильный"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Портрет"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("рассказ"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)) {
                    continue;
                }

                // Колонка «Ответ ребенка» — из ближайшего заголовка ВЫШЕ строки
                // (у задания 1 и 2 индексы колонок разные).
                int answerCol = -1;
                for (int hr = r - 1; hr >= 0 && answerCol < 0; --hr) {
                    for (int c = 0; c < table->columns(); ++c) {
                        const QString header = readTableCellText(table, hr, c);
                        if (header.contains(QStringLiteral("Ответ ребенка"), Qt::CaseInsensitive)) {
                            answerCol = c;
                            break;
                        }
                    }
                }
                if (answerCol < 0 || answerCol >= table->columns()) {
                    continue;
                }

                const QString answer = readTableCellText(table, r, answerCol);
                for (int i = 0; i < kTask1.size(); ++i) {
                    if (label.compare(kTask1.at(i), Qt::CaseInsensitive) == 0) {
                        chunk = replaceDivInnerById(
                            chunk,
                            QStringLiteral("ans1%1").arg(i + 1),
                            answer.toHtmlEscaped());
                        break;
                    }
                }
                bool okNum = false;
                const int storyNo = label.toInt(&okNum);
                if (okNum && storyNo >= 1 && storyNo <= 12) {
                    // Не перезаписывать ответ ребёнка эталоном из соседней колонки.
                    if (storyNo <= kTask2Correct.size()
                        && answer.compare(kTask2Correct.at(storyNo - 1), Qt::CaseInsensitive) == 0) {
                        int correctCol = -1;
                        for (int hr = r - 1; hr >= 0 && correctCol < 0; --hr) {
                            for (int c = 0; c < table->columns(); ++c) {
                                const QString header = readTableCellText(table, hr, c);
                                if (header.contains(
                                        QStringLiteral("Правильный ответ"), Qt::CaseInsensitive)) {
                                    correctCol = c;
                                    break;
                                }
                            }
                        }
                        if (correctCol >= 0 && correctCol == answerCol) {
                            continue;
                        }
                        // Если текст совпал с эталоном, но колонка — «Ответ ребенка»,
                        // оставляем как есть (ребёнок мог ответить верно).
                    }
                    chunk = replaceDivInnerById(
                        chunk,
                        QStringLiteral("ans2%1").arg(storyNo),
                        answer.toHtmlEscaped());
                }
            }
        }
        return chunk;
    };

    // Редактор показывает только последнюю сессию — писать баллы/суммы только в неё.
    QStringList sessions = extractProtocol126SessionsByDate(body);
    const bool multiSession = sessions.size() > 1;

    // Без «Подвести итог» — только перенос вручную введённых баллов из редактора в HTML.
    if (!computeSums) {
        if (multiSession) {
            sessions[sessions.size() - 1] = applyManualScoresFromEditor(sessions.last());
            return joinProtocol126Sessions(sessions);
        }
        return applyManualScoresFromEditor(body);
    }

    // «Подвести итог»: только сумма уже введённых баллов (без автоподстановки по ответам).
    auto finalizeSums = [&](QString chunk) {
        chunk = applyManualScoresFromEditor(chunk);
        chunk = applyAnswersFromEditor(chunk);
        const double sum1 = sumDivPrefix(chunk, QStringLiteral("col1"));
        const double sum2 = sumDivPrefix(chunk, QStringLiteral("col2"));
        const int sum3 = static_cast<int>(sum1 + sum2);
        chunk = replaceDivInnerById(chunk, QStringLiteral("sum1"), QString::number(static_cast<int>(sum1)));
        chunk = replaceDivInnerById(chunk, QStringLiteral("sum2"), QString::number(static_cast<int>(sum2)));
        chunk = replaceDivInnerById(chunk, QStringLiteral("sum3"), QString::number(sum3));
        chunk = replaceDivInnerById(
            chunk, QStringLiteral("idvivod"), QString::number(sum3) + QStringLiteral("(36)"));
        return chunk;
    };

    if (multiSession) {
        sessions[sessions.size() - 1] = finalizeSums(sessions.last());
        return joinProtocol126Sessions(sessions);
    }
    return finalizeSums(body);
}

QString replaceLabeledValueCellNth(
    QString html,
    const QString &labelKey,
    int occurrence,
    const QString &plainText) {
    if (occurrence < 0 || html.isEmpty()) {
        return html;
    }
    const QRegularExpression re(
        QStringLiteral(
            "(<tr[^>]*>\\s*<td[^>]*>\\s*(?:<[^>]+>\\s*)*%1[^<]*</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>)")
            .arg(QRegularExpression::escape(labelKey)),
        QRegularExpression::CaseInsensitiveOption);
    int found = 0;
    int offset = 0;
    while (true) {
        const QRegularExpressionMatch match = re.match(html, offset);
        if (!match.hasMatch()) {
            break;
        }
        if (found == occurrence) {
            const QString inner = QStringLiteral("<div contenteditable='true'>%1</div>")
                                      .arg(formatProtocolCellText(plainText));
            return html.left(match.capturedStart())
                + match.captured(1) + inner + match.captured(3)
                + html.mid(match.capturedEnd());
        }
        ++found;
        offset = match.capturedEnd();
    }
    return html;
}

QString ExerciseProtocol::mergeProtocol126EditorIntoStoredBody(
    const QString &storedBody,
    QTextDocument *editorDocument) {
    if (storedBody.trimmed().isEmpty() || !editorDocument) {
        return storedBody;
    }

    QString body = mergeLimitedEditableFieldsIntoStoredBody(storedBody, editorDocument);
    const QString editorHtml = editorDocument->toHtml();

    auto copyDivById = [&](const QString &id) {
        const QRegularExpression idProbe(
            QStringLiteral("\\bid\\s*=\\s*['\"]?%1['\"]?").arg(QRegularExpression::escape(id)),
            QRegularExpression::CaseInsensitiveOption);
        if (!idProbe.match(editorHtml).hasMatch()) {
            return;
        }
        // extractDivInnerById уже возвращает plain text.
        const QString plain = extractDivInnerById(editorHtml, id).trimmed();
        // Не затирать введённые баллы/ответы пустым чтением из редактора:
        // без «Подвести итог» Qt часто отдаёт пустой div, а цифра ещё в ячейке
        // или только в stored HTML — иначе при формировании задания 2 баллы №1 пропадают.
        if (plain.isEmpty()) {
            return;
        }
        body = replaceDivInnerById(body, id, plain.toHtmlEscaped());
    };

    for (int i = 1; i <= 6; ++i) {
        copyDivById(QStringLiteral("ans1%1").arg(i));
        copyDivById(QStringLiteral("col1%1").arg(i));
    }
    for (int i = 1; i <= 12; ++i) {
        copyDivById(QStringLiteral("ans2%1").arg(i));
        copyDivById(QStringLiteral("col2%1").arg(i));
    }
    copyDivById(QStringLiteral("sum1"));
    copyDivById(QStringLiteral("sum2"));
    copyDivById(QStringLiteral("sum3"));
    copyDivById(QStringLiteral("idvivod"));

    // Ответы/баллы по ячейкам таблицы (если Qt потерял id после задания 2).
    {
        static const QStringList kTask1 = {
            QStringLiteral("Радость"), QStringLiteral("Злость"),
            QStringLiteral("Грусть"), QStringLiteral("Страх"),
            QStringLiteral("Удивление"), QStringLiteral("Спокойствие"),
        };
        QList<QTextTable *> answerTables;
        collectTables(editorDocument->rootFrame(), answerTables);
        for (QTextTable *table : answerTables) {
            if (!table || table->columns() < 2) {
                continue;
            }
            for (int r = 0; r < table->rows(); ++r) {
                const QString label = readTableCellText(table, r, 0);
                if (label.isEmpty()
                    || label.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Индекс"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Характер"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Задание"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Ответ"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Правильный"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Портрет"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("рассказ"), Qt::CaseInsensitive)
                    || label.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)) {
                    continue;
                }
                int answerCol = -1;
                int ballsCol = -1;
                for (int hr = r - 1; hr >= 0; --hr) {
                    for (int c = 0; c < table->columns(); ++c) {
                        const QString header = readTableCellText(table, hr, c);
                        if (answerCol < 0
                            && header.contains(QStringLiteral("Ответ ребенка"), Qt::CaseInsensitive)) {
                            answerCol = c;
                        }
                        if (ballsCol < 0
                            && header.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)
                            && header.length() <= 12) {
                            ballsCol = c;
                        }
                    }
                    if (answerCol >= 0 && ballsCol >= 0) {
                        break;
                    }
                }
                if (answerCol >= 0 && answerCol < table->columns()) {
                    const QString answer = readTableCellText(table, r, answerCol);
                    for (int i = 0; i < kTask1.size(); ++i) {
                        if (label.compare(kTask1.at(i), Qt::CaseInsensitive) == 0) {
                            body = replaceDivInnerById(
                                body, QStringLiteral("ans1%1").arg(i + 1), answer.toHtmlEscaped());
                            break;
                        }
                    }
                    bool okNum = false;
                    const int storyNo = label.toInt(&okNum);
                    if (okNum && storyNo >= 1 && storyNo <= 12) {
                        int correctCol = -1;
                        for (int hr = r - 1; hr >= 0 && correctCol < 0; --hr) {
                            for (int c = 0; c < table->columns(); ++c) {
                                if (readTableCellText(table, hr, c).contains(
                                        QStringLiteral("Правильный ответ"), Qt::CaseInsensitive)) {
                                    correctCol = c;
                                    break;
                                }
                            }
                        }
                        if (correctCol < 0 || correctCol != answerCol) {
                            body = replaceDivInnerById(
                                body, QStringLiteral("ans2%1").arg(storyNo), answer.toHtmlEscaped());
                        }
                    }
                }
                if (ballsCol >= 0 && ballsCol < table->columns()) {
                    const QString score = readTableCellText(table, r, ballsCol).trimmed();
                    if (!score.isEmpty()) {
                        for (int i = 0; i < kTask1.size(); ++i) {
                            if (label.compare(kTask1.at(i), Qt::CaseInsensitive) == 0) {
                                body = replaceDivInnerById(
                                    body, QStringLiteral("col1%1").arg(i + 1), score.toHtmlEscaped());
                                break;
                            }
                        }
                        bool okNum = false;
                        const int storyNo = label.toInt(&okNum);
                        if (okNum && storyNo >= 1 && storyNo <= 12) {
                            body = replaceDivInnerById(
                                body, QStringLiteral("col2%1").arg(storyNo), score.toHtmlEscaped());
                        }
                    }
                }
            }
        }
    }

    QStringList activityValues;
    QStringList helpValues;
    QList<QTextTable *> tables;
    collectTables(editorDocument->rootFrame(), tables);
    for (QTextTable *table : tables) {
        if (!table) {
            continue;
        }
        for (int r = 0; r < table->rows(); ++r) {
            const QString label = readTableCellText(table, r, 0);
            if (table->columns() < 2) {
                continue;
            }
            const QString value = readTableCellMultilineText(table, r, 1);
            if (label.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                activityValues << value;
            } else if (label.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                       && !label.contains(QStringLiteral("возможной"), Qt::CaseInsensitive)) {
                helpValues << value;
            }
        }
    }

    QStringList sessions = extractProtocol126SessionsByDate(body);
    auto applyOrHlp = [&](QString chunk) {
        for (int i = 0; i < activityValues.size(); ++i) {
            chunk = replaceLabeledValueCellNth(
                chunk, QStringLiteral("Характер деятельности"), i, activityValues.at(i));
        }
        for (int i = 0; i < helpValues.size(); ++i) {
            chunk = replaceLabeledValueCellNth(
                chunk, QStringLiteral("Виды помощи"), i, helpValues.at(i));
        }
        return chunk;
    };
    if (sessions.size() > 1) {
        sessions[sessions.size() - 1] = applyOrHlp(sessions.last());
        return joinProtocol126Sessions(sessions);
    }
    return applyOrHlp(body);
}

QString ExerciseProtocol::applyProtocolIdbSum(
    const QString &storedBody,
    const QString &maxSuffix,
    const QString &idPrefix) {
    if (storedBody.trimmed().isEmpty()) {
        return storedBody;
    }
    // Как bsum для 3.1.10 / 1.272: только последняя сессия с idb*/ids*.
    QStringList sessions = extractProtocol126SessionsByDate(storedBody);
    auto applyToChunk = [&](QString chunk) {
        const double sum = sumAllDivIdsWithPrefix(chunk, idPrefix);
        const QString sumText = formatBallsNumber(sum);
        const QString resultText = sumText + maxSuffix;
        chunk = replaceDivInnerById(chunk, QStringLiteral("idsum"), sumText);
        // Результат = Итоговая оценка + (макс.); если idvivod потерян — пишем в строку Результат.
        const QString before = chunk;
        chunk = replaceDivInnerById(chunk, QStringLiteral("idvivod"), resultText);
        if (chunk == before) {
            chunk = replaceResultRowSecondCell(chunk, resultText);
        }
        return chunk;
    };
    if (sessions.size() > 1) {
        sessions[sessions.size() - 1] = applyToChunk(sessions.last());
        return joinProtocol126Sessions(sessions);
    }
    return applyToChunk(storedBody);
}

QString ExerciseProtocol::applyProtocolBPrefixSum(const QString &storedBody, const QString &maxSuffix) {
    if (storedBody.trimmed().isEmpty()) {
        return storedBody;
    }
    // Как bsum для 4.1.8: сумма b1..b5 → idsum / idvivod=sum(10).
    QStringList sessions = extractProtocol126SessionsByDate(storedBody);
    auto replaceLastDiv = [](QString chunk, const QString &id, const QString &inner) {
        const QRegularExpression re(
            QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"]%1['\"][^>]*>)([\\s\\S]*?)(</div>)")
                .arg(QRegularExpression::escape(id)),
            QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = re.globalMatch(chunk);
        QRegularExpressionMatch match;
        while (it.hasNext()) {
            match = it.next();
        }
        if (!match.hasMatch()) {
            return chunk;
        }
        chunk.replace(
            match.capturedStart(0),
            match.capturedLength(0),
            match.captured(1) + inner + match.captured(3));
        return chunk;
    };
    auto applyToChunk = [&](QString chunk) {
        double sum = 0;
        for (int i = 1; i <= 5; ++i) {
            QString plain = extractDivInnerById(chunk, QStringLiteral("b") + QString::number(i));
            plain = htmlFragmentToPlainText(plain).trimmed();
            plain.replace(QLatin1Char(','), QLatin1Char('.'));
            plain.remove(QRegularExpression(QStringLiteral("[^0-9.\\-]")));
            bool ok = false;
            const double value = plain.toDouble(&ok);
            if (ok) {
                sum += value;
            }
        }
        const QString sumText = formatBallsNumber(sum);
        chunk = replaceLastDiv(chunk, QStringLiteral("idsum"), sumText);
        chunk = replaceLastDiv(chunk, QStringLiteral("idvivod"), sumText + maxSuffix);
        return chunk;
    };
    if (sessions.size() > 1) {
        sessions[sessions.size() - 1] = applyToChunk(sessions.last());
        return joinProtocol126Sessions(sessions);
    }
    return applyToChunk(storedBody);
}

QString editable3110Cell(const QString &plain) {
    return QStringLiteral("<div contenteditable='true'>%1</div>").arg(formatProtocolCellText(plain));
}

QString replace3110ProcessRowCells(
    QString body,
    const QString &stepNo,
    const QString &picture,
    const QString &explanation,
    const QString &activity,
    const QString &help,
    const QString &score) {
    const QString escaped = QRegularExpression::escape(stepNo);
    const QRegularExpression rowRe(
        QStringLiteral(
            "(<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*<td[^>]*>)"
            "([\\s\\S]*?)(</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*<td[^>]*>)([\\s\\S]*?)"
            "(</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)")
            .arg(escaped),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = rowRe.match(body);
    if (!match.hasMatch()) {
        if (!score.trimmed().isEmpty()) {
            body = replaceDivInnerById(body, QStringLiteral("idb") + stepNo, score.toHtmlEscaped());
        }
        return body;
    }
    const QString picInner = editable3110Cell(picture);
    const QString explInner = editable3110Cell(explanation);
    const QString orInner = editable3110Cell(activity);
    const QString hlpInner = editable3110Cell(help);
    QString scoreInner = match.captured(10);
    if (!score.trimmed().isEmpty()) {
        const QString id = QStringLiteral("idb") + stepNo;
        scoreInner = QStringLiteral("<div id='%1' contenteditable='true'>%2</div>")
                         .arg(id, score.toHtmlEscaped());
    } else if (!scoreInner.contains(QStringLiteral("idb") + stepNo, Qt::CaseInsensitive)) {
        // Сохранить idb, если Qt потерял атрибут id.
        const QString plain = htmlFragmentToPlainText(scoreInner).trimmed();
        scoreInner = QStringLiteral("<div id='idb%1' contenteditable='true'>%2</div>")
                         .arg(stepNo, plain.isEmpty() ? QStringLiteral("&nbsp;") : plain.toHtmlEscaped());
    }
    return body.left(match.capturedStart())
        + match.captured(1) + picInner + match.captured(3) + explInner + match.captured(5) + orInner
        + match.captured(7) + hlpInner + match.captured(9) + scoreInner + match.captured(11)
        + body.mid(match.capturedEnd());
}

QString ExerciseProtocol::mergeProtocol3110EditorIntoStoredBody(
    const QString &storedBody,
    QTextDocument *editorDocument) {
    if (storedBody.trimmed().isEmpty() || !editorDocument) {
        return storedBody;
    }
    QString body = mergeLimitedEditableFieldsIntoStoredBody(storedBody, editorDocument);

    QList<QTextTable *> tables;
    collectTables(editorDocument->rootFrame(), tables);
    for (QTextTable *table : tables) {
        if (!table || table->columns() < 6) {
            continue;
        }
        int headerRow = -1;
        int picCol = -1;
        int explCol = -1;
        int activityCol = -1;
        int helpCol = -1;
        int ballsCol = -1;
        for (int r = 0; r < table->rows() && headerRow < 0; ++r) {
            for (int c = 0; c < table->columns(); ++c) {
                const QString h = readTableCellText(table, r, c);
                if (h.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)) {
                    picCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Объяснение выбора"), Qt::CaseInsensitive)) {
                    explCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                    activityCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                    && !h.contains(QStringLiteral("возможной"), Qt::CaseInsensitive)) {
                    helpCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive) && h.length() <= 12) {
                    ballsCol = c;
                    headerRow = r;
                }
            }
        }
        if (headerRow < 0 || picCol < 0) {
            continue;
        }
        for (int r = headerRow + 1; r < table->rows(); ++r) {
            const QString stepNo = readTableCellText(table, r, 0).trimmed();
            bool isNumber = false;
            stepNo.toInt(&isNumber);
            if (!isNumber) {
                continue;
            }
            const QString picture = readTableCellText(table, r, picCol);
            const QString explanation = explCol >= 0 ? readTableCellText(table, r, explCol) : QString();
            const QString activity =
                activityCol >= 0 ? readTableCellMultilineText(table, r, activityCol) : QString();
            const QString help =
                helpCol >= 0 ? readTableCellMultilineText(table, r, helpCol) : QString();
            const QString score = ballsCol >= 0 ? readTableCellText(table, r, ballsCol) : QString();
            body = replace3110ProcessRowCells(
                body, stepNo, picture, explanation, activity, help, score);
        }
    }
    return body;
}

int countHelpEntries3110(const QString &helpCell) {
    QString normalized = helpCell;
    normalized.replace(QChar::LineSeparator, QLatin1Char('\n'));
    normalized.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    const QStringList parts =
        normalized.split(QRegularExpression(QStringLiteral("[\\r\\n;]+")), Qt::SkipEmptyParts);
    int count = 0;
    for (const QString &part : parts) {
        if (!part.trimmed().isEmpty()) {
            ++count;
        }
    }
    return count;
}

double score3110FromActivityAndHelp(const QString &activity, const QString &help) {
    // Как protocols.cs / шаблон activity_help_2: III уровень → 2, иначе 0; −0.5 за каждый вид помощи.
    double score = 0.0;
    if (activity.contains(QStringLiteral("III уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("2 балла"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("Целенаправленное"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("содержательное обобщение"), Qt::CaseInsensitive)) {
        score = 2.0;
    }
    return qMax(0.0, score - 0.5 * countHelpEntries3110(help));
}

QString ExerciseProtocol::applyProtocol3110SumFromDocument(
    const QString &storedBody,
    QTextDocument *editorDocument) {
    if (storedBody.trimmed().isEmpty()) {
        return storedBody;
    }

    // Правки и пересчёт — только в последней сессии (иначе №1.. попадут в старые блоки).
    QStringList sessions = extractProtocol126SessionsByDate(storedBody);
    const bool multi = sessions.size() > 1;
    QString chunk = multi ? sessions.last() : storedBody;
    if (editorDocument) {
        chunk = mergeProtocol3110EditorIntoStoredBody(chunk, editorDocument);
    }

    const QRegularExpression rowRe(
        QStringLiteral(
            "<tr[^>]*>\\s*<td[^>]*>\\s*(\\d+)\\s*</td>\\s*<td[^>]*>([\\s\\S]*?)</td>\\s*<td[^>]*>"
            "([\\s\\S]*?)</td>\\s*<td[^>]*>([\\s\\S]*?)</td>\\s*<td[^>]*>([\\s\\S]*?)</td>\\s*<td[^>]*>"
            "([\\s\\S]*?)</td>\\s*</tr>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = rowRe.globalMatch(chunk);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString stepNo = match.captured(1).trimmed();
        const QString activity = htmlFragmentToPlainText(match.captured(4)).trimmed();
        const QString help = htmlFragmentToPlainText(match.captured(5)).trimmed();
        // Без характера деятельности не трогаем уже выставленные баллы строки.
        if (activity.isEmpty()) {
            continue;
        }
        const double score = score3110FromActivityAndHelp(activity, help);
        chunk = replaceDivInnerById(
            chunk, QStringLiteral("idb") + stepNo, formatBallsNumber(score));
    }

    chunk = applyProtocolIdbSum(chunk, QStringLiteral("(20)"), QStringLiteral("idb"));
    if (!multi) {
        return chunk;
    }
    sessions[sessions.size() - 1] = chunk;
    return joinProtocol126Sessions(sessions);
}

QList<QTextTable *> ExerciseProtocol::collectOrHlpProcessTables(QTextDocument *document) {
    QList<QTextTable *> result;
    if (!document) {
        return result;
    }
    QList<QTextTable *> tables;
    collectTables(document->rootFrame(), tables);
    for (QTextTable *table : tables) {
        if (!table || table->columns() < 2) {
            continue;
        }
        bool hasActivity = false;
        bool hasAnswers = false;
        bool hasHelp = false;
        bool isSelectedPic = false;
        for (int r = 0; r < qMin(3, table->rows()) && !isSelectedPic; ++r) {
            for (int c = 0; c < table->columns(); ++c) {
                const QString h = readTableCellText(table, r, c);
                if (h.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)) {
                    isSelectedPic = true;
                    break;
                }
                if (h.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                    hasActivity = true;
                }
                if (h.contains(QStringLiteral("Ответы ребенка"), Qt::CaseInsensitive)
                    || h.contains(QStringLiteral("Ответ ребенка"), Qt::CaseInsensitive)) {
                    hasAnswers = true;
                }
                if (h.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                    && !h.contains(QStringLiteral("возможной"), Qt::CaseInsensitive)) {
                    hasHelp = true;
                }
            }
        }
        // OR/HLP или 5.4.2 «Сказка» (Вопросы / Ответы ребенка / Виды помощи).
        if (!isSelectedPic && hasHelp && (hasActivity || hasAnswers)) {
            result.append(table);
        }
    }
    return result;
}

QTextTable *ExerciseProtocol::findOrHlpProcessTableForProtocol(
    QTextDocument *document,
    const QString &protocolId) {
    if (!document || protocolId.trimmed().isEmpty()) {
        return nullptr;
    }
    const QString startName = QStringLiteral("dokit-pid-%1-start").arg(protocolId);
    const QString endName = QStringLiteral("dokit-pid-%1-end").arg(protocolId);
    int startPos = -1;
    int endPos = -1;
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid()) {
                continue;
            }
            const QTextCharFormat fmt = frag.charFormat();
            if (!fmt.isAnchor()) {
                continue;
            }
            const QStringList names = fmt.anchorNames();
            if (startPos < 0 && names.contains(startName)) {
                startPos = frag.position();
            }
            if (endPos < 0 && names.contains(endName)) {
                endPos = frag.position();
            }
        }
    }
    if (startPos < 0) {
        return nullptr;
    }
    if (endPos < 0) {
        endPos = document->characterCount();
    }

    const QList<QTextTable *> tables = collectOrHlpProcessTables(document);
    QTextTable *inRange = nullptr;
    for (QTextTable *table : tables) {
        if (!table || table->rows() <= 0 || table->columns() <= 0) {
            continue;
        }
        const QTextTableCell cell = table->cellAt(0, 0);
        if (!cell.isValid()) {
            continue;
        }
        const int pos = cell.firstCursorPosition().position();
        if (pos >= startPos && pos < endPos) {
            // При нескольких сессиях в одном протоколе — последняя таблица процесса.
            inRange = table;
        }
    }
    return inRange;
}

QString ExerciseProtocol::mergeOrHlpBallsEditorIntoStoredBody(
    const QString &storedBody,
    QTextDocument *editorDocument,
    QTextTable *processTable) {
    if (storedBody.trimmed().isEmpty() || !editorDocument) {
        return storedBody;
    }
    // Сначала OR/HLP по исходному HTML процесса, потом Результат/Примечание:
    // иначе joinClosed/ensureClosed после mergeLimited может сдвинуть разметку строк
    // (особенно после дописки задания 2) и правки по №1 с уже заполненными ячейками теряются.
    QString body = storedBody;

    // 5.2.1: на каждое «Задание №N» — своя таблица OR/HLP. Нужны все таблицы документа,
    // иначе после break сохранялась только первая (или при одной live-таблице — не та строка).
    QList<QTextTable *> tables;
    if (processTable) {
        QTextDocument *liveDoc = processTable->document();
        const QList<QTextTable *> allLive =
            liveDoc ? collectOrHlpProcessTables(liveDoc) : QList<QTextTable *>();
        if (allLive.size() > 1) {
            tables = allLive;
        } else {
            tables.append(processTable);
        }
    } else {
        tables = collectOrHlpProcessTables(editorDocument);
    }

    auto normalizeStepKey = [](QString key) {
        key.replace(QChar(0xA0), QLatin1Char(' '));
        key = key.trimmed().simplified();
        static const QRegularExpression numRe(QStringLiteral("^(\\d+)"));
        const QRegularExpressionMatch m = numRe.match(key);
        return m.hasMatch() ? m.captured(1) : key;
    };

    struct EditorRow {
        QString stepKey;
        QString activity;
        QString help;
        QString score;
        int activityCol = 0;
        int helpCol = 1;
        int ballsCol = -1;
        bool activityInFirstCol = false;
    };
    QList<EditorRow> editorDataRows;
    bool anyActivityInFirstCol = false;

    for (QTextTable *table : tables) {
        if (!table || table->columns() < 2) {
            continue;
        }
        int headerRow = -1;
        int activityCol = -1;
        int helpCol = -1;
        int ballsCol = -1;
        for (int r = 0; r < table->rows() && headerRow < 0; ++r) {
            for (int c = 0; c < table->columns(); ++c) {
                const QString h = readTableCellText(table, r, c);
                if (h.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)) {
                    headerRow = -2;
                    break;
                }
                if (h.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                    activityCol = c;
                    headerRow = r;
                }
                // 5.4.2: колонка «Ответы ребенка» — как редактируемое поле (вместо OR).
                if (activityCol < 0
                    && (h.contains(QStringLiteral("Ответы ребенка"), Qt::CaseInsensitive)
                        || h.contains(QStringLiteral("Ответ ребенка"), Qt::CaseInsensitive))) {
                    activityCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                    && !h.contains(QStringLiteral("возможной"), Qt::CaseInsensitive)) {
                    helpCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive) && h.length() <= 12
                    && c > 0) {
                    ballsCol = c;
                    headerRow = r;
                }
            }
            if (headerRow == -2) {
                headerRow = -1;
                break;
            }
        }
        if (headerRow < 0 || activityCol < 0 || helpCol < 0) {
            continue;
        }
        const bool activityInFirstCol = (activityCol == 0 && helpCol == 1);
        if (activityInFirstCol) {
            anyActivityInFirstCol = true;
        }

        for (int r = headerRow + 1; r < table->rows(); ++r) {
            const QString label = readTableCellText(table, r, 0);
            if (label.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)
                || label.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
                || label.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                || label.compare(QStringLiteral("Вопросы"), Qt::CaseInsensitive) == 0
                || (label.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)
                    && label.length() < 20)) {
                continue;
            }
            EditorRow er;
            er.stepKey = normalizeStepKey(label);
            er.activity = readTableCellMultilineText(table, r, activityCol);
            er.help = readTableCellMultilineText(table, r, helpCol);
            er.score = ballsCol >= 0 ? readTableCellText(table, r, ballsCol) : QString();
            er.activityCol = activityCol;
            er.helpCol = helpCol;
            er.ballsCol = ballsCol;
            er.activityInFirstCol = activityInFirstCol;
            editorDataRows.append(er);
        }
    }
    if (editorDataRows.isEmpty()) {
        body = mergeLimitedEditableFieldsIntoStoredBody(body, editorDocument);
        return body;
    }

    const int marker = body.lastIndexOf(QStringLiteral("<!--s-->"));
    if (marker < 0) {
        body = mergeLimitedEditableFieldsIntoStoredBody(body, editorDocument);
        return body;
    }
    const QString head = body.left(marker + QStringLiteral("<!--s-->").size());
    QString tail = body.mid(marker + QStringLiteral("<!--s-->").size());

    const QRegularExpression trRe(
        QStringLiteral("(<tr[^>]*>)([\\s\\S]*?)(</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpression stepMarkerRe(
        QStringLiteral("<!--\\s*step\\s*([^\\s>]+)\\s*-->"),
        QRegularExpression::CaseInsensitiveOption);
    struct RowPos {
        int start = 0;
        int len = 0;
        QString open;
        QString inner;
        QString close;
        QString stepKey;
    };
    QList<RowPos> dataRows;
    QRegularExpressionMatchIterator it = trRe.globalMatch(tail);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString rowInner = m.captured(2);
        const QRegularExpressionMatch stepMark = stepMarkerRe.match(rowInner);
        const QRegularExpression firstTdRe(
            QStringLiteral("<td\\b[^>]*>([\\s\\S]*?)</td>"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        const QRegularExpressionMatch firstTd = firstTdRe.match(rowInner);
        const QString firstPlain = firstTd.hasMatch()
            ? htmlFragmentToPlainText(firstTd.captured(1)).trimmed()
            : QString();
        QString stepKey = stepMark.hasMatch()
            ? normalizeStepKey(stepMark.captured(1))
            : normalizeStepKey(firstPlain);
        // Заголовок — только по первой ячейке / маркеру, не по тексту OR/HLP.
        // 3-кол. без «№» (3.1.17/18, 5.2.1): в col0 — текст характера; фильтр «Задание»
        // иначе отбрасывал строку с чекбоксом «…выполнить задание».
        if (firstPlain.compare(QStringLiteral("№"), Qt::CaseInsensitive) == 0
            || firstPlain.compare(QStringLiteral("N"), Qt::CaseInsensitive) == 0
            || firstPlain.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
            || firstPlain.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
            || firstPlain.compare(QStringLiteral("Вопросы"), Qt::CaseInsensitive) == 0
            || firstPlain.contains(QStringLiteral("Ответы ребенка"), Qt::CaseInsensitive)
            || (firstPlain.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)
                && firstPlain.length() < 20)
            || firstPlain.contains(QStringLiteral("Процесс выполнения"), Qt::CaseInsensitive)
            || (!anyActivityInFirstCol
                && (firstPlain.contains(QStringLiteral("Кол-во цифр"), Qt::CaseInsensitive)
                    || firstPlain.contains(QStringLiteral("Факт выполнения"), Qt::CaseInsensitive)
                    || firstPlain.contains(QStringLiteral("Картинка"), Qt::CaseInsensitive)
                    || firstPlain.contains(QStringLiteral("Задание"), Qt::CaseInsensitive)
                    || firstPlain.contains(QStringLiteral("Фрагменты речи"), Qt::CaseInsensitive)
                    || firstPlain.contains(
                        QStringLiteral("Частота употребления"), Qt::CaseInsensitive)))) {
            continue;
        }
        RowPos row;
        row.start = m.capturedStart();
        row.len = m.capturedLength();
        row.open = m.captured(1);
        row.inner = rowInner;
        row.close = m.captured(3);
        row.stepKey = stepKey;
        {
            const QRegularExpression tdCountRe(
                QStringLiteral("<td\\b"),
                QRegularExpression::CaseInsensitiveOption);
            int tdCount = 0;
            QRegularExpressionMatchIterator tdCountIt = tdCountRe.globalMatch(row.inner);
            while (tdCountIt.hasNext()) {
                tdCountIt.next();
                ++tdCount;
            }
            if (tdCount != 3 && tdCount != 4) {
                continue;
            }
        }
        dataRows.append(row);
    }
    if (dataRows.isEmpty()) {
        body = mergeLimitedEditableFieldsIntoStoredBody(body, editorDocument);
        return body;
    }

    auto makeEditable = [](const QString &text) {
        return QStringLiteral("<div contenteditable='true' style='text-align:left'>%1</div>")
            .arg(formatProtocolCellText(text));
    };

    auto findStoredRowIndex = [&](const EditorRow &er, const QSet<int> &used) -> int {
        // 3-кол. OR/HLP без «№» (3.1.11 / 5.2.1 и др.): stepKey = текст характера —
        // после правки с клавиатуры ключ меняется, сопоставление только по порядку.
        if (er.activityInFirstCol) {
            return -1;
        }
        if (!er.stepKey.isEmpty()) {
            for (int i = 0; i < dataRows.size(); ++i) {
                if (used.contains(i)) {
                    continue;
                }
                if (dataRows.at(i).stepKey.compare(er.stepKey, Qt::CaseInsensitive) == 0) {
                    return i;
                }
            }
        }
        return -1;
    };

    QVector<int> storedForEditor(editorDataRows.size(), -1);
    QSet<int> usedStored;
    for (int ei = 0; ei < editorDataRows.size(); ++ei) {
        const int si = findStoredRowIndex(editorDataRows.at(ei), usedStored);
        if (si >= 0) {
            storedForEditor[ei] = si;
            usedStored.insert(si);
        }
    }
    QList<int> freeEditors;
    QList<int> freeStored;
    for (int ei = 0; ei < editorDataRows.size(); ++ei) {
        if (storedForEditor.at(ei) < 0) {
            freeEditors.append(ei);
        }
    }
    for (int si = 0; si < dataRows.size(); ++si) {
        if (!usedStored.contains(si)) {
            freeStored.append(si);
        }
    }
    const int pairFree = qMin(freeEditors.size(), freeStored.size());
    // Свободные пары — по порядку (задание1→1, задание2→2), не с хвоста.
    for (int i = 0; i < pairFree; ++i) {
        const int ei = freeEditors.at(i);
        const int si = freeStored.at(i);
        storedForEditor[ei] = si;
        usedStored.insert(si);
    }

    QList<QPair<int, int>> applyOrder; // storedIndex, editorIndex
    for (int ei = 0; ei < editorDataRows.size(); ++ei) {
        if (storedForEditor.at(ei) >= 0) {
            applyOrder.append(qMakePair(storedForEditor.at(ei), ei));
        }
    }
    std::sort(applyOrder.begin(), applyOrder.end(),
              [](const QPair<int, int> &a, const QPair<int, int> &b) {
                  return a.first > b.first;
              });

    for (const auto &pair : applyOrder) {
        const EditorRow &er = editorDataRows.at(pair.second);
        RowPos &htmlRow = dataRows[pair.first];

        const QRegularExpression tdRe(
            QStringLiteral("(<td[^>]*>)([\\s\\S]*?)(</td>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QList<QRegularExpressionMatch> tds;
        QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(htmlRow.inner);
        while (tdIt.hasNext()) {
            tds.append(tdIt.next());
        }
        if (tds.size() <= qMax(er.activityCol, er.helpCol)) {
            continue;
        }

        QList<QPair<int, QString>> replacements;
        replacements.append(qMakePair(er.activityCol, makeEditable(er.activity)));
        replacements.append(qMakePair(er.helpCol, makeEditable(er.help)));
        if (er.ballsCol >= 0 && er.ballsCol < tds.size()) {
            QString scoreId = QStringLiteral("idballs");
            const QRegularExpression existingIdRe(
                QStringLiteral("\\bid\\s*=\\s*['\"]([^'\"]+)['\"]"),
                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch idMatch = existingIdRe.match(tds.at(er.ballsCol).captured(2));
            if (idMatch.hasMatch()) {
                scoreId = idMatch.captured(1);
            }
            replacements.append(qMakePair(
                er.ballsCol,
                QStringLiteral(
                    "<div id='%1' contenteditable='true' style='text-align:center'>%2</div>")
                    .arg(scoreId, er.score.toHtmlEscaped())));
        }
        std::sort(replacements.begin(), replacements.end(),
                  [](const QPair<int, QString> &a, const QPair<int, QString> &b) {
                      return a.first > b.first;
                  });

        QString inner = htmlRow.inner;
        for (const auto &rep : replacements) {
            tds.clear();
            tdIt = tdRe.globalMatch(inner);
            while (tdIt.hasNext()) {
                tds.append(tdIt.next());
            }
            if (rep.first < 0 || rep.first >= tds.size()) {
                continue;
            }
            const QRegularExpressionMatch &td = tds.at(rep.first);
            inner.replace(
                td.capturedStart(),
                td.capturedLength(),
                td.captured(1) + rep.second + td.captured(3));
        }
        const QString newRow = htmlRow.open + inner + htmlRow.close;
        tail.replace(htmlRow.start, htmlRow.len, newRow);
    }
    body = head + tail;

    body = mergeLimitedEditableFieldsIntoStoredBody(body, editorDocument);
    return body;
}

QString developmentLevelFromBalls(int score) {
    if (score >= 10) {
        return QStringLiteral("очень высокий");
    }
    if (score >= 8) {
        return QStringLiteral("высокий");
    }
    if (score >= 4) {
        return QStringLiteral("средний");
    }
    if (score >= 2) {
        return QStringLiteral("низкий");
    }
    return QStringLiteral("очень низкий");
}

QString ExerciseProtocol::applyProtocol318SumFromDocument(
    const QString &storedBody,
    QTextDocument *editorDocument,
    QTextTable *processTable) {
    if (storedBody.trimmed().isEmpty()) {
        return storedBody;
    }
    // 33.3 / 4.2.1: перенос баллов → Результат не должен очищать «Примечание».
    const QRegularExpression idnoteRe(
        QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"]idnote['\"][^>]*>)([\\s\\S]*?)(</div>)"),
        QRegularExpression::CaseInsensitiveOption);
    QString preservedNoteInner;
    {
        QRegularExpressionMatchIterator it = idnoteRe.globalMatch(storedBody);
        QRegularExpressionMatch last;
        while (it.hasNext()) {
            last = it.next();
        }
        if (last.hasMatch()) {
            preservedNoteInner = last.captured(2);
        }
    }

    QString body = mergeOrHlpBallsEditorIntoStoredBody(storedBody, editorDocument, processTable);

    QStringList sessions = extractProtocol126SessionsByDate(body);
    const bool multi = sessions.size() > 1;
    QString chunk = multi ? sessions.last() : body;

    // Сначала балл из живой таблицы редактора (ввод с клавиатуры), потом idballs в HTML.
    QString ballsPlain;
    if (editorDocument) {
        QList<QTextTable *> tables;
        if (processTable) {
            tables.append(processTable);
        } else {
            collectTables(editorDocument->rootFrame(), tables);
        }
        for (QTextTable *table : tables) {
            if (!table || table->columns() < 3) {
                continue;
            }
            int ballsCol = -1;
            int headerRow = -1;
            for (int r = 0; r < table->rows() && ballsCol < 0; ++r) {
                for (int c = 0; c < table->columns(); ++c) {
                    const QString h = readTableCellText(table, r, c);
                    if (h.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive) && h.length() <= 24) {
                        ballsCol = c;
                        headerRow = r;
                        break;
                    }
                }
            }
            if (ballsCol < 0) {
                continue;
            }
            for (int r = table->rows() - 1; r > headerRow; --r) {
                const QString score = readTableCellText(table, r, ballsCol).trimmed();
                if (!score.isEmpty()
                    && !score.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)) {
                    ballsPlain = score;
                    break;
                }
            }
            if (!ballsPlain.isEmpty()) {
                break;
            }
        }
    }
    if (ballsPlain.trimmed().isEmpty()) {
        ballsPlain = extractDivInnerById(chunk, QStringLiteral("idballs"));
    }

    // «8», «8 баллов», «8,5» → число.
    bool ok = false;
    double value = 0;
    {
        const QString cleaned = ballsPlain.trimmed().replace(QLatin1Char(','), QLatin1Char('.'));
        value = cleaned.toDouble(&ok);
        if (!ok) {
            const QRegularExpression numRe(QStringLiteral("(-?\\d+(?:\\.\\d+)?)"));
            const QRegularExpressionMatch m = numRe.match(cleaned);
            if (m.hasMatch()) {
                value = m.captured(1).toDouble(&ok);
            }
        }
    }
    if (!ok) {
        if (multi) {
            sessions[sessions.size() - 1] = chunk;
            body = joinProtocol126Sessions(sessions);
        } else {
            body = chunk;
        }
    } else {
        const int scoreInt = qBound(0, qRound(value), 10);
        const QString resultText = QStringLiteral("%1(10)/%2")
                                       .arg(formatBallsNumber(scoreInt), developmentLevelFromBalls(scoreInt));
        if (chunk.contains(QStringLiteral("idballs"), Qt::CaseInsensitive)) {
            chunk = replaceDivInnerById(chunk, QStringLiteral("idballs"), formatBallsNumber(scoreInt));
        }
        if (chunk.contains(QStringLiteral("idvivod"), Qt::CaseInsensitive)) {
            chunk = replaceDivInnerById(chunk, QStringLiteral("idvivod"), resultText.toHtmlEscaped());
        } else {
            chunk = replaceResultRowSecondCell(chunk, resultText);
        }

        if (multi) {
            sessions[sessions.size() - 1] = chunk;
            body = joinProtocol126Sessions(sessions);
        } else {
            body = chunk;
        }
    }

    // Восстановить «Примечание», если merge его стёр.
    if (!preservedNoteInner.isEmpty()) {
        QRegularExpressionMatchIterator it = idnoteRe.globalMatch(body);
        QRegularExpressionMatch last;
        while (it.hasNext()) {
            last = it.next();
        }
        if (last.hasMatch()) {
            const QString after = htmlFragmentToPlainText(last.captured(2)).trimmed();
            const QString before = htmlFragmentToPlainText(preservedNoteInner).trimmed();
            if (after.isEmpty() && !before.isEmpty()) {
                body.replace(
                    last.capturedStart(0),
                    last.capturedLength(0),
                    last.captured(1) + preservedNoteInner + last.captured(3));
            }
        }
    }
    return body;
}

QString ExerciseProtocol::mergeProtocol1272EditorIntoStoredBody(
    const QString &storedBody,
    QTextDocument *editorDocument) {
    if (storedBody.trimmed().isEmpty() || !editorDocument) {
        return storedBody;
    }
    QString body = mergeLimitedEditableFieldsIntoStoredBody(storedBody, editorDocument);

    QStringList sessions = extractProtocol126SessionsByDate(body);
    const bool multiSession = sessions.size() > 1;
    QString target = multiSession ? sessions.last() : body;

    auto makeEditable = [](const QString &text) {
        return QStringLiteral("<div contenteditable='true'>%1</div>")
            .arg(formatProtocolCellText(text));
    };

    auto replaceNthTdInner = [](QString trInner, int tdIndex, const QString &newInner) {
        const QRegularExpression tdRe(
            QStringLiteral("(<td[^>]*>)([\\s\\S]*?)(</td>)"),
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QList<QRegularExpressionMatch> tds;
        QRegularExpressionMatchIterator tdIt = tdRe.globalMatch(trInner);
        while (tdIt.hasNext()) {
            tds.append(tdIt.next());
        }
        if (tdIndex < 0 || tdIndex >= tds.size()) {
            return trInner;
        }
        const QRegularExpressionMatch &td = tds.at(tdIndex);
        return trInner.left(td.capturedStart())
            + td.captured(1) + newInner + td.captured(3)
            + trInner.mid(td.capturedEnd());
    };

    QList<QTextTable *> tables;
    collectTables(editorDocument->rootFrame(), tables);
    for (QTextTable *table : tables) {
        if (!table || table->columns() < 4) {
            continue;
        }
        int headerRow = -1;
        int activityCol = -1;
        int helpCol = -1;
        int ballsCol = -1;
        for (int r = 0; r < table->rows() && headerRow < 0; ++r) {
            for (int c = 0; c < table->columns(); ++c) {
                const QString h = readTableCellText(table, r, c);
                if (h.compare(QStringLiteral("№"), Qt::CaseInsensitive) == 0
                    || h.compare(QStringLiteral("N"), Qt::CaseInsensitive) == 0
                    || h.contains(QStringLiteral("№/ответ"), Qt::CaseInsensitive)
                    || h.contains(QStringLiteral("N/ответ"), Qt::CaseInsensitive)) {
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                    activityCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                    && !h.contains(QStringLiteral("возможной"), Qt::CaseInsensitive)) {
                    helpCol = c;
                    headerRow = r;
                }
                if (h.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive) && h.length() <= 12) {
                    ballsCol = c;
                    headerRow = r;
                }
            }
        }
        if (headerRow < 0) {
            continue;
        }

        static const QRegularExpression leadingNumRe(QStringLiteral("^\\s*(\\d+)"));

        for (int r = headerRow + 1; r < table->rows(); ++r) {
            const QString label = readTableCellText(table, r, 0).trimmed();
            if (label.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)) {
                if (ballsCol >= 0) {
                    const QString sum = readTableCellText(table, r, ballsCol);
                    if (!sum.trimmed().isEmpty()) {
                        target = replaceDivInnerById(target, QStringLiteral("idsum"), sum);
                    }
                }
                continue;
            }
            const QRegularExpressionMatch numMatch = leadingNumRe.match(label);
            if (!numMatch.hasMatch()) {
                continue;
            }
            const QString stepNo = numMatch.captured(1);
            const QString scoreId = QStringLiteral("ids") + stepNo;

            if (ballsCol >= 0) {
                const QString score = readTableCellText(table, r, ballsCol);
                if (!score.trimmed().isEmpty()) {
                    target = replaceDivInnerById(target, scoreId, score.toHtmlEscaped());
                }
            }

            if (activityCol < 0 || helpCol < 0) {
                continue;
            }
            const QString activity = readTableCellMultilineText(table, r, activityCol);
            const QString help = readTableCellMultilineText(table, r, helpCol);

            // Строка процесса с idsN — обновляем ячейки характера и помощи.
            const QRegularExpression idProbe(
                QStringLiteral("\\bid\\s*=\\s*['\"]%1['\"]").arg(QRegularExpression::escape(scoreId)),
                QRegularExpression::CaseInsensitiveOption);
            if (!idProbe.match(target).hasMatch()) {
                continue;
            }
            const QRegularExpression trRe(
                QStringLiteral("(<tr[^>]*>)([\\s\\S]*?)(</tr>)"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
            QRegularExpressionMatchIterator it = trRe.globalMatch(target);
            while (it.hasNext()) {
                const QRegularExpressionMatch m = it.next();
                if (!idProbe.match(m.captured(2)).hasMatch()) {
                    continue;
                }
                QString inner = m.captured(2);
                inner = replaceNthTdInner(inner, activityCol, makeEditable(activity));
                inner = replaceNthTdInner(inner, helpCol, makeEditable(help));
                target.replace(m.capturedStart(), m.capturedLength(), m.captured(1) + inner + m.captured(3));
                break;
            }
        }
        break;
    }

    // Ручная правка «Результат» (idvivod) — поверх mergeLimited, если Qt сохранил id.
    {
        const QString editorHtml = editorDocument->toHtml();
        const QRegularExpression idProbe(
            QStringLiteral("\\bid\\s*=\\s*['\"]?idvivod['\"]?"),
            QRegularExpression::CaseInsensitiveOption);
        if (idProbe.match(editorHtml).hasMatch()) {
            const QString plain = extractDivInnerById(editorHtml, QStringLiteral("idvivod"));
            target = replaceDivInnerById(target, QStringLiteral("idvivod"), plain.toHtmlEscaped());
        }
    }

    if (multiSession) {
        sessions[sessions.size() - 1] = target;
        return joinProtocol126Sessions(sessions);
    }
    return target;
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

QString appendRowsIntoSingleProtocolBody(const QString &existingBody, const QString &rowsHtml) {
    QString addition = rowsHtml.trimmed();
    if (addition.isEmpty()) {
        return existingBody;
    }

    QString base = existingBody;
    if (base.trimmed().isEmpty()) {
        return addition;
    }

    // 1.26: задание 2 — отдельная таблица после таблицы задания 1
    // (общая сетка колонок в одной таблице раздувает width > 671).
    if (looksLikeProtocol126Body(base) || looksLikeProtocol126Body(addition)
        || addition.contains(QStringLiteral("Задание 2"), Qt::CaseInsensitive)
        || addition.contains(QStringLiteral("id='col2"), Qt::CaseInsensitive)
        || addition.contains(QStringLiteral("id=\"col2"), Qt::CaseInsensitive)) {
        QString rowsToInsert = addition;
        while (rowsToInsert.startsWith(QStringLiteral("<br"), Qt::CaseInsensitive)) {
            const int gt = rowsToInsert.indexOf(QLatin1Char('>'));
            if (gt < 0) {
                break;
            }
            rowsToInsert = rowsToInsert.mid(gt + 1).trimmed();
        }
        if (rowsToInsert.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
            rowsToInsert = extractTableInnerRows(rowsToInsert);
        }
        if (!rowsToInsert.startsWith(QStringLiteral("<tr"), Qt::CaseInsensitive)
            && rowsToInsert.startsWith(QStringLiteral("<td"), Qt::CaseInsensitive)) {
            rowsToInsert.prepend(QStringLiteral("<tr>"));
        }

        const QString task2Table = QString::fromUtf8(kProtocol126ProcessTableOpen)
            + rowsToInsert + QStringLiteral("</table>");

        const int marker = base.lastIndexOf(QStringLiteral("<!--s-->"));
        int tableClose = -1;
        if (marker >= 0) {
            tableClose = base.lastIndexOf(QStringLiteral("</table>"), -1, Qt::CaseInsensitive);
            if (tableClose < marker) {
                tableClose = -1;
            }
        } else {
            tableClose = base.lastIndexOf(QStringLiteral("</table>"), -1, Qt::CaseInsensitive);
        }
        if (tableClose >= 0) {
            const int afterClose = tableClose + QStringLiteral("</table>").size();
            return base.left(afterClose) + task2Table + base.mid(afterClose);
        }
        // Нет закрытой таблицы процесса — дописать целиком.
        if (!base.contains(QStringLiteral("Процесс выполнения диагностического задания"), Qt::CaseInsensitive)) {
            base += QStringLiteral("</table><!--s-->")
                + QString::fromUtf8(kProtocol126ProcessTableOpen)
                + QStringLiteral(
                    "<tr><td colspan='4' align='center'><b>Процесс выполнения диагностического задания</b></td></tr>")
                + QStringLiteral("</table>");
        }
        return base + task2Table;
    }

    if (!addition.startsWith(QStringLiteral("<tr"), Qt::CaseInsensitive)
        && addition.startsWith(QStringLiteral("<td"), Qt::CaseInsensitive)) {
        addition.prepend(QStringLiteral("<tr>"));
    }
    if (addition.startsWith(QStringLiteral("<tr"), Qt::CaseInsensitive)
        && !addition.contains(QStringLiteral("</tr>"), Qt::CaseInsensitive)) {
        addition.append(QStringLiteral("</tr>"));
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
    // Берём последний <!--s--> — при нескольких сессиях дописываем в текущую, не в первую.
    const int marker = base.lastIndexOf(QStringLiteral("<!--s-->"));
    if (marker >= 0) {
        const int tableClose = base.lastIndexOf(QStringLiteral("</table>"), -1, Qt::CaseInsensitive);
        if (tableClose > marker) {
            return base.left(tableClose) + addition + base.mid(tableClose);
        }
    }

    return base + addition;
}

QString ExerciseProtocol::appendRowsToStoredBody(const QString &existingBody, const QString &rowsHtml) {
    if (rowsHtml.trimmed().isEmpty()) {
        return existingBody;
    }

    // 1.26: при нескольких «Дата/специалист» дописываем только в последнюю сессию,
    // иначе первый <!--s--> + последний </table> ломают границы.
    if (looksLikeProtocol126Body(existingBody) || looksLikeProtocol126Body(rowsHtml)) {
        QStringList sessions = extractProtocol126SessionsByDate(existingBody);
        if (sessions.size() > 1) {
            sessions[sessions.size() - 1] =
                appendRowsIntoSingleProtocolBody(sessions.last(), rowsHtml);
            return ExerciseProtocol::canonicalizeProtocol126StoredBody(
                joinProtocol126Sessions(sessions));
        }
        return ExerciseProtocol::canonicalizeProtocol126StoredBody(
            appendRowsIntoSingleProtocolBody(existingBody, rowsHtml));
    }

    return appendRowsIntoSingleProtocolBody(existingBody, rowsHtml);
}

QString ExerciseProtocol::appendFullSessionToStoredBody(
    const QString &existingBody,
    const QString &sessionHtml) {
    QString session = sessionHtml.trimmed();
    if (session.isEmpty()) {
        if (looksLikeProtocol126Body(existingBody)) {
            return ExerciseProtocol::canonicalizeProtocol126StoredBody(existingBody);
        }
        return existingBody;
    }

    // Новая сессия — всегда законченный блок summary + process (закрытые </table>).
    session = ensureClosedProtocolSession(stripLeadingSummaryTableWrapper(session));
    if (!session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
        session.prepend(protocolSummaryTableOpenHtml());
    }
    session = closeDanglingTables(session);

    QString existing = existingBody.trimmed();
    if (existing.isEmpty()) {
        return normalizeSummaryColumnWidthsHtml(session);
    }
    // Закрыть висячие таблицы и не тащить хвост `<table>` следующей сессии.
    existing = closeDanglingTables(existing);
    existing.replace(
        QRegularExpression(
            QStringLiteral("<table\\b[^>]*>\\s*$"),
            QRegularExpression::CaseInsensitiveOption),
        QString());

    // Плоская склейка соседних сессий (без потери границ из‑за extract).
    // Если в existing уже есть даты — пересоберём через extract+join (с исправленным концом куска).
    QStringList sessions = extractProtocol126SessionsByDate(existing);
    if (sessions.isEmpty()) {
        sessions.append(stripLeadingSummaryTableWrapper(existing));
    }
    sessions.append(stripLeadingSummaryTableWrapper(session));
    const QString joined = joinProtocol126Sessions(sessions);
    if (looksLikeProtocol126Body(joined) || looksLikeProtocol126Body(session)) {
        return ExerciseProtocol::canonicalizeProtocol126StoredBody(joined);
    }
    if (joined.contains(QStringLiteral("sel1"), Qt::CaseInsensitive)
        && joined.contains(QStringLiteral("Стимульные"), Qt::CaseInsensitive)) {
        return ExerciseProtocol::canonicalizeProtocol418StoredBody(joined);
    }
    return normalizeSummaryColumnWidthsHtml(joined);
}

QString ExerciseProtocol::flattenStoredProtocolBody(const QString &protocolBody) {
    if (protocolBody.trimmed().isEmpty()) {
        return {};
    }
    // 4.1.8: две таблицы после <!--s--> — flatten через ensureClosed раньше отрезал стимулы.
    if (protocolBody.contains(QStringLiteral("id='sel1'"), Qt::CaseInsensitive)
        || protocolBody.contains(QStringLiteral("id=\"sel1\""), Qt::CaseInsensitive)
        || protocolBody.contains(QStringLiteral("Стимульные"), Qt::CaseInsensitive)) {
        return normalizeSummaryColumnWidthsHtml(
            ExerciseProtocol::canonicalizeProtocol418StoredBody(protocolBody));
    }
    // 5.2.1: после <!--s--> несколько таблиц (Задание №N + OR/HLP) — не отрезать.
    if (protocolBody.contains(QStringLiteral("Частота употребления"), Qt::CaseInsensitive)
        || protocolBody.contains(QStringLiteral("Задание №"), Qt::CaseInsensitive)) {
        const QStringList sessions = extractProtocolBodiesByDateRows(protocolBody);
        if (sessions.isEmpty()) {
            return normalizeSummaryColumnWidthsHtml(ensureClosedProtocolSession(protocolBody));
        }
        return joinClosedProtocolSessions(sessions);
    }
    // 5.4.2 «Сказка»: summary </table><!--s--> + 3-кол. Вопросы/Ответы/Помощь.
    if (protocolBody.contains(QStringLiteral("Вопросы"), Qt::CaseInsensitive)
        && protocolBody.contains(QStringLiteral("Ответы ребенка"), Qt::CaseInsensitive)
        && protocolBody.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
        && protocolBody.contains(QStringLiteral("О чем эта сказка"), Qt::CaseInsensitive)) {
        QStringList sessions = extractProtocol126SessionsByDate(protocolBody);
        if (sessions.isEmpty()) {
            sessions = extractProtocolBodiesByDateRows(protocolBody);
        }
        if (sessions.isEmpty()) {
            return normalizeSummaryColumnWidthsHtml(ensureClosedProtocolSession(protocolBody));
        }
        return joinClosedProtocolSessions(sessions);
    }
    QStringList sessions = extractProtocolBodiesByDateRows(protocolBody);
    if (sessions.isEmpty()) {
        return normalizeSummaryColumnWidthsHtml(ensureClosedProtocolSession(protocolBody));
    }
    // Первая сессия без ведущего <table> — шапка методики уже открывает таблицу.
    QString result;
    for (int i = 0; i < sessions.size(); ++i) {
        QString session = ensureClosedProtocolSession(sessions.at(i));
        if (session.isEmpty()) {
            continue;
        }
        session.replace(
            QRegularExpression(
                QStringLiteral("^(?:\\s|<br\\s*/?>|<p\\b[^>]*>\\s*(?:&nbsp;|\\s)*\\s*</p\\s*>)+"),
                QRegularExpression::CaseInsensitiveOption),
            QString());
        if (i > 0) {
            result.replace(
                QRegularExpression(
                    QStringLiteral("(?:\\s|<br\\s*/?>|<p\\b[^>]*>\\s*(?:&nbsp;|\\s)*\\s*</p\\s*>)+$"),
                    QRegularExpression::CaseInsensitiveOption),
                QString());
            if (!session.startsWith(QStringLiteral("<table"), Qt::CaseInsensitive)) {
                session.prepend(protocolSummaryTableOpenHtml());
            }
        } else {
            session = stripLeadingSummaryTableWrapper(session);
            session = ensureClosedProtocolSession(session);
        }
        result += session;
    }
    return normalizeSummaryColumnWidthsHtml(result);
}
