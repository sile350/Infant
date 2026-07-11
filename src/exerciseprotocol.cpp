#include "exerciseprotocol.h"

#include "exerciseassets.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QTextDocument>
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
        QStringLiteral("<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>\\s*(верно|неверно)")
            .arg(QRegularExpression::escape(description)),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = answerRe.match(body);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString repairResultsTableBody(QString body, const QList<bool> &answers) {
    body.replace(
        QRegularExpression(QStringLiteral("\\s*rowspan=['\"]\\d+['\"]"), QRegularExpression::CaseInsensitiveOption),
        QString());

    const QStringList descriptions = pictureDescriptions();
    for (int i = 0; i < descriptions.size(); ++i) {
        QString verno;
        if (i < answers.size()) {
            verno = answerText(answers.at(i));
        } else {
            verno = extractAnswerFromRow(body, descriptions.at(i));
            if (verno.isEmpty()) {
                continue;
            }
        }

        const QString escapedDesc = QRegularExpression::escape(descriptions.at(i));
        const QRegularExpression brokenRowRe(
            QStringLiteral("(<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>)\\s*</tr>").arg(escapedDesc),
            QRegularExpression::CaseInsensitiveOption);
        if (brokenRowRe.match(body).hasMatch()) {
            const QString replacement = QStringLiteral("\\1<td valign='top'>%1</td><td>&nbsp;</td><td>&nbsp;</td></tr>")
                                            .arg(verno);
            body.replace(brokenRowRe, replacement);
            continue;
        }

        const QRegularExpression rowRe(
            QStringLiteral("(<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>)")
                .arg(escapedDesc),
            QRegularExpression::CaseInsensitiveOption);
        body.replace(rowRe, QStringLiteral("\\1") + verno + QStringLiteral("\\3"));
    }
    return body;
}

QString htmlFragmentToPlainText(const QString &html) {
    if (html.trimmed().isEmpty()) {
        return {};
    }
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText().trimmed();
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
        QStringLiteral("(<tr[^>]*>\\s*<td[^>]*>\\s*%1\\s*</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>)")
            .arg(escapedDesc),
        QRegularExpression::CaseInsensitiveOption);
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
            "(<tr[^>]*>\\s*<td[^>]*>\\s*1\\.\\s*Бабушка[^<]*</td>\\s*<td[^>]*>[\\s\\S]*?</td>\\s*<td[^>]*>)"
            "([\\s\\S]*?)(</td>\\s*<td[^>]*>)([\\s\\S]*?)(</td>\\s*</tr>)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    const QString activityCell =
        QStringLiteral("<div contenteditable='true'>%1</div>").arg(activity.toHtmlEscaped());
    const QString helpCell = QStringLiteral("<div contenteditable='true'>%1</div>").arg(help.toHtmlEscaped());
    return body.replace(
        rowRe,
        QStringLiteral("\\1") + activityCell + QStringLiteral("\\3") + helpCell + QStringLiteral("\\5"));
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

} // namespace

QString ExerciseProtocol::createProtocolHtml(
    const QString &exerciseId,
    const QString &userFio,
    int elapsedSeconds,
    bool partly,
    const QString &existingProtocolHtml,
    const QList<bool> &answers,
    const CheckboxValues &checkboxes) {
    Q_UNUSED(elapsedSeconds);
    Q_UNUSED(partly);
    Q_UNUSED(existingProtocolHtml);
    if (exerciseId != QStringLiteral("1.2")) {
        return existingProtocolHtml;
    }

    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"));
    QString add = QStringLiteral("<tr><td>Дата/специалист</td><td>%1   %2</td></tr>")
                      .arg(now, userFio.toHtmlEscaped());
    add += QStringLiteral(
        "<tr><td>Результат: вывод об уровне развития</td><td><div contenteditable='true'></div></td></tr>"
        "<tr><td>Примечание</td><td><div contenteditable='true'></div></td></tr>"
        "<tr><td align='center' colspan='2'>Процесс выполнения диагностической методики</td></tr>"
        "</table><!--s-->")
           + resultsTableHeaderHtml();

    const QStringList descriptions = pictureDescriptions();

    for (int i = 0; i < 5; ++i) {
        const bool correct = i < answers.size() ? answers.at(i) : false;
        if (i == 0) {
            add += QStringLiteral("<tr><td>%1</td><td valign='top'>%2</td>"
                                "<td valign='top'><div contenteditable='true'>%3</div></td>"
                                "<td valign='top'><div contenteditable='true'>%4</div></td></tr>")
                       .arg(descriptions.at(i), answerText(correct),
                            checkboxes.activity.toHtmlEscaped(), checkboxes.help.toHtmlEscaped());
        } else {
            add += QStringLiteral("<tr><td>%1</td><td valign='top'>%2</td>"
                                "<td valign='top'>&nbsp;</td><td valign='top'>&nbsp;</td></tr>")
                       .arg(descriptions.at(i), answerText(correct));
        }
    }
    add += QStringLiteral("</table>");
    return repairResultsTableBody(add, answers);
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
    values.help = helpValues.join(QStringLiteral("; "));
    return values;
}

QString ExerciseProtocol::applyCheckboxValues(const QString &orHtml, const CheckboxValues &values) {
    Q_UNUSED(values);
    return orHtml;
}

QString ExerciseProtocol::mergeEditorHtmlIntoStoredBody(
    const QString &storedBody,
    const QString &editorHtml,
    int protocolIndex) {
    if (storedBody.trimmed().isEmpty()) {
        return storedBody;
    }

    const QString section = extractProtocolSectionFromEditor(editorHtml, protocolIndex);
    if (section.trimmed().isEmpty()) {
        return normalizeStoredProtocolBody(storedBody);
    }

    QString result = normalizeStoredProtocolBody(storedBody);

    const auto dateCell = extractSecondCellPlain(section, QStringLiteral("Дата\\s*/\\s*специалист"));
    if (dateCell.first) {
        result = replaceRowSecondCell(result, QStringLiteral("Дата/специалист"), dateCell.second);
    }

    std::pair<bool, QString> resultCell = extractSecondCellPlain(section, QStringLiteral("Результат:\\s*вывод"));
    if (!resultCell.first) {
        resultCell = extractSecondCellPlain(section, QStringLiteral("Результат"));
    }
    if (resultCell.first) {
        result = replaceRowSecondCell(result, QStringLiteral("Результат: вывод об уровне развития"), resultCell.second);
    }

    const auto noteCell = extractSecondCellPlain(section, QStringLiteral("Примечание"));
    if (noteCell.first) {
        result = replaceRowSecondCell(result, QStringLiteral("Примечание"), noteCell.second);
    }

    if (result.contains(QStringLiteral("<!--s-->"))) {
        const QStringList descriptions = pictureDescriptions();
        for (int i = 0; i < descriptions.size(); ++i) {
            QString verno = extractAnswerFromRow(section, descriptions.at(i));
            if (verno.isEmpty()) {
                verno = extractAnswerFromSectionFallback(section, descriptions.at(i));
            }
            if (!verno.isEmpty()) {
                result = replaceAnswerInBody(result, descriptions.at(i), verno);
            }
        }

        QString activity;
        QString help;
        if (extractActivityHelpFromSection(section, &activity, &help)) {
            result = replaceActivityHelpCells(result, activity, help);
        }
        result = repairResultsTableBody(result);
    }

    return result;
}
