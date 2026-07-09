#include "exerciseprotocol.h"

#include "exerciseassets.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>

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
    if (exerciseId != QStringLiteral("1.2")) {
        return existingProtocolHtml;
    }

    QString add;
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"));
    add += QStringLiteral("<tr><td>Дата/специалист</td><td>%1   %2</td></tr>")
               .arg(now, userFio.toHtmlEscaped());

    if (!partly) {
        add += QStringLiteral(
            "<tr><td>Результат: вывод об уровне развития</td><td><div contenteditable='true'></div></td></tr>"
            "<tr><td>Примечание</td><td><div contenteditable='true'></div></td></tr>"
            "<tr><td align='center' colspan='2'>Процесс выполнения диагностической методики</td></tr>"
            "</table><!--s-->"
            "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
            "<tr><td width='229' align='center'>Картинка(описание)</td>"
            "<td width='88' align='center'>Уровень выполнения</td>"
            "<td align='center' width='160'>Характер деятельности ребенка</td>"
            "<td align='center' width='194'>Виды помощи</td></tr>");
    } else {
        add = existingProtocolHtml;
        const int marker = add.indexOf(QStringLiteral("<!--s-->"));
        if (marker >= 0) {
            add = add.left(marker);
            const int tableStart = add.lastIndexOf(QStringLiteral("<table"));
            if (tableStart >= 0) {
                add = add.left(tableStart);
            }
        }
        add += QStringLiteral(
            "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
            "<tr><td width='229' align='center'>Картинка(описание)</td>"
            "<td width='88' align='center'>Уровень выполнения</td>"
            "<td align='center' width='160'>Характер деятельности ребенка</td>"
            "<td align='center' width='194'>Виды помощи</td></tr>");
    }

    const QString descriptions[] = {
        QStringLiteral("1. Бабушка на диване без ножки."),
        QStringLiteral("2. Велосипедист без переднего колеса."),
        QStringLiteral("3. Девочка с расческой без зубчиков."),
        QStringLiteral("4. Пальто без рукава."),
        QStringLiteral("5. Ослик без уха."),
    };

    for (int i = 0; i < 5; ++i) {
        const bool correct = i < answers.size() ? answers.at(i) : false;
        if (i == 0) {
            add += QStringLiteral("<tr><td>%1</td><td valign='top'>%2</td>"
                                "<td valign='top' rowspan='5'><div contenteditable='true'>%3</div></td>"
                                "<td valign='top' rowspan='5'><div contenteditable='true'>%4</div></td></tr>")
                       .arg(descriptions[i], answerText(correct),
                            checkboxes.activity.toHtmlEscaped(), checkboxes.help.toHtmlEscaped());
        } else {
            add += QStringLiteral("<tr><td>%1</td><td valign='top'>%2</td></tr>")
                       .arg(descriptions[i], answerText(correct));
        }
    }
    return add;
}

QString ExerciseProtocol::protocolViewHtml(
    const QString &exerciseId,
    const QString &protocolBody,
    const QString &patientFio,
    const QString &patientBirthDate) {
    const QString headerHtml = readHeaderRows(exerciseId);
    const QString body = QStringLiteral("<!--body-->") + protocolBody + QStringLiteral("<!--ebody-->");
    return QStringLiteral(
               "<div align='center' style='font-size:20px'><br>Протокол фиксации результатов исследования</div>"
               "<br>ФИО: %1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
               "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Дата рождения:%2<br><br>%3%4</table>")
        .arg(patientFio.toHtmlEscaped(), patientBirthDate.toHtmlEscaped(), headerHtml, body);
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
