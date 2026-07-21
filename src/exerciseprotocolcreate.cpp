#include "exerciseprotocolcreate.h"

#include "exerciseassets.h"
#include "exerciseprotocoltemplates.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

namespace {

QString formatProtocolTime(int elapsedSeconds) {
    const int minutes = elapsedSeconds / 60;
    const int seconds = elapsedSeconds - minutes * 60;
    return QStringLiteral("%1:%2 сек").arg(minutes).arg(seconds);
}

QString editableCell(const QString &text = QString()) {
    if (text.trimmed().isEmpty()) {
        return QStringLiteral("<div contenteditable='true'>&nbsp;</div>");
    }
    // formatProtocolCellText уже возвращает HTML (&nbsp;/br) — не экранировать повторно.
    if (text.contains(QStringLiteral("&nbsp;")) || text.contains(QLatin1Char('<'))) {
        return QStringLiteral("<div contenteditable='true'>%1</div>").arg(text);
    }
    return QStringLiteral("<div contenteditable='true'>%1</div>").arg(text.toHtmlEscaped());
}

QString developmentLevel(int score) {
    if (score == 10) {
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

int scoreExercise11(int time) {
    if (time <= 25) return 10;
    if (time <= 28) return 9;
    if (time <= 30) return 8;
    if (time <= 32) return 7;
    if (time <= 35) return 6;
    if (time <= 37) return 5;
    if (time <= 39) return 4;
    if (time <= 42) return 3;
    if (time <= 44) return 2;
    if (time <= 50) return 1;
    return 0;
}

int scoreExercise412(int time) {
    if (time <= 45) return 10;
    if (time <= 47) return 9;
    if (time <= 50) return 8;
    if (time <= 55) return 7;
    if (time <= 60) return 6;
    if (time <= 65) return 5;
    if (time <= 70) return 4;
    if (time <= 75) return 3;
    if (time <= 80) return 2;
    if (time <= 85) return 1;
    return 0;
}

int scoreExercise18(int time) {
    if (time <= 20) return 10;
    if (time <= 25) return 9;
    if (time <= 30) return 8;
    if (time <= 35) return 7;
    if (time <= 40) return 6;
    if (time <= 45) return 5;
    if (time <= 50) return 4;
    if (time <= 55) return 3;
    if (time <= 60) return 2;
    if (time <= 65) return 1;
    return 0;
}

int scoreExercise14(int time, int picturesShown) {
    const QString count = QString::number(picturesShown);
    if (time <= 10 && count == QStringLiteral("0")) return 10;
    if (time <= 13 && count == QStringLiteral("1")) return 9;
    if (time <= 16 && count == QStringLiteral("1")) return 8;
    if (time <= 20 && count == QStringLiteral("1")) return 7;
    if (time <= 23 && count == QStringLiteral("2")) return 6;
    if (time <= 27 && count == QStringLiteral("2")) return 5;
    if (time <= 30 && count == QStringLiteral("2")) return 4;
    if (time <= 35 && count == QStringLiteral("3")) return 3;
    if (time <= 40 && count == QStringLiteral("3")) return 2;
    if (time >= 50 && count == QStringLiteral("3")) return 0;
    return 0;
}

QString formatProtocolCellText(const QString &text) {
    if (text.trimmed().isEmpty()) {
        return QStringLiteral("&nbsp;");
    }
    const QStringList parts = text.split(QRegularExpression(QStringLiteral("[\\r\\n;]+")), Qt::SkipEmptyParts);
    QStringList lines;
    for (const QString &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            lines << QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;%1").arg(trimmed.toHtmlEscaped());
        }
    }
    return lines.isEmpty() ? QStringLiteral("&nbsp;") : lines.join(QStringLiteral("<br>"));
}

QString dateSpecialistRow(const QString &userFio, int colspan = 2) {
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"));
    if (colspan > 1) {
        return QStringLiteral("<tr><td>Дата/специалист</td><td colspan='%1'>%2   %3</td></tr>")
            .arg(colspan)
            .arg(now, userFio.toHtmlEscaped());
    }
    return QStringLiteral("<tr><td width='200'>Дата/специалист</td><td width='471'>%1   %2</td></tr>")
        .arg(now, userFio.toHtmlEscaped());
}

QString buildTimedBallsInitial(
    const QString &userFio,
    int score,
    bool maxTen,
    const QString &resultsHeader,
    const ExerciseProtocol::CheckboxValues &checkboxes) {
    QString body = dateSpecialistRow(userFio, 2);
    const QString level = developmentLevel(score);
    const QString resultText = maxTen
        ? QStringLiteral("%1(10)/%2").arg(score).arg(level)
        : QStringLiteral("%1/%2").arg(score).arg(level);
    body += QStringLiteral("<tr><td>Результат: баллы (макс.)/ вывод об уровне развития</td><td colspan='2'>")
        + editableCell(resultText) + QStringLiteral("</td></tr>");
    body += QStringLiteral("<tr><td>Примечание</td><td colspan='3'>") + editableCell() + QStringLiteral("</td></tr>");
    body += QStringLiteral(
        "<tr><td align='center' colspan='4'>Процесс выполнения диагностической методики</td></tr>"
        "</table><!--s-->");
    body += resultsHeader;
    body += QStringLiteral("<tr><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.activity))
        + QStringLiteral("</td><td>") + editableCell(formatProtocolCellText(checkboxes.help))
        + QStringLiteral("</td><td align='center'>") + editableCell(QString::number(score))
        + QStringLiteral("</td></tr></table>");
    return body;
}

QString appendTimedBallsRow(
    const QString &existing,
    int score,
    const ExerciseProtocol::CheckboxValues &checkboxes) {
    const QString row = QStringLiteral("<tr><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.activity))
        + QStringLiteral("</td><td>") + editableCell(formatProtocolCellText(checkboxes.help))
        + QStringLiteral("</td><td align='center'>") + editableCell(QString::number(score))
        + QStringLiteral("</td></tr>");
    return ExerciseProtocol::appendRowsToStoredBody(existing, row);
}

QString buildDoneTimeInitial(
    const QString &userFio,
    int colspan,
    const QString &resultsHeader,
    const QString &doneState,
    int elapsedSeconds,
    const ExerciseProtocol::CheckboxValues &checkboxes) {
    QString body = dateSpecialistRow(userFio, colspan);
    body += QStringLiteral("<tr><td>Результат: вывод об уровне развития</td><td colspan='")
        + QString::number(colspan) + QStringLiteral("'>") + editableCell() + QStringLiteral("</td></tr>");
    body += QStringLiteral("<tr><td>Примечание</td><td colspan='")
        + QString::number(colspan + 1) + QStringLiteral("'>") + editableCell() + QStringLiteral("</td></tr>");
    body += QStringLiteral(
        "<tr><td align='center' colspan='4'>Процесс выполнения диагностической методики</td></tr>"
        "</table><!--s-->");
    body += resultsHeader;
    body += QStringLiteral("<tr><td>") + doneState.toHtmlEscaped() + QStringLiteral("/")
        + formatProtocolTime(elapsedSeconds).toHtmlEscaped()
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.activity))
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.help))
        + QStringLiteral("</td></tr></table>");
    return body;
}

QString appendDoneTimeRow(
    const QString &existing,
    const QString &doneState,
    int elapsedSeconds,
    const ExerciseProtocol::CheckboxValues &checkboxes) {
    const QString row = QStringLiteral("<tr><td>") + doneState.toHtmlEscaped() + QStringLiteral("/")
        + formatProtocolTime(elapsedSeconds).toHtmlEscaped()
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.activity))
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.help))
        + QStringLiteral("</td></tr>");
    return ExerciseProtocol::appendRowsToStoredBody(existing, row);
}

QString buildNumberedInitial(
    const QString &userFio,
    const QString &resultsHeader,
    const QString &stepId,
    const QString &doneState,
    int elapsedSeconds,
    const ExerciseProtocol::CheckboxValues &checkboxes) {
    QString body = dateSpecialistRow(userFio, 2);
    body += QStringLiteral("<tr><td>Результат: вывод об уровне развития</td><td>") + editableCell()
        + QStringLiteral("</td></tr>");
    body += QStringLiteral("<tr><td>Примечание</td><td>") + editableCell() + QStringLiteral("</td></tr>");
    body += QStringLiteral(
        "<tr><td align='center' colspan='2'>Процесс выполнения диагностической методики</td></tr>"
        "</table><!--s-->");
    body += resultsHeader;
    body += QStringLiteral("<tr><td align='center'>") + stepId.toHtmlEscaped()
        + QStringLiteral("</td><td>") + doneState.toHtmlEscaped() + QStringLiteral("/")
        + formatProtocolTime(elapsedSeconds).toHtmlEscaped()
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.activity))
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.help))
        + QStringLiteral("</td></tr></table>");
    return body;
}

QString appendNumberedRow(
    const QString &existing,
    const QString &stepId,
    const QString &doneState,
    int elapsedSeconds,
    const ExerciseProtocol::CheckboxValues &checkboxes) {
    const QString row = QStringLiteral("<tr><td align='center'>") + stepId.toHtmlEscaped()
        + QStringLiteral("</td><td>") + doneState.toHtmlEscaped() + QStringLiteral("/")
        + formatProtocolTime(elapsedSeconds).toHtmlEscaped()
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.activity))
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.help))
        + QStringLiteral("</td></tr>");
    return ExerciseProtocol::appendRowsToStoredBody(existing, row);
}

QString buildOrHlpInitial(
    const QString &userFio,
    const QString &resultsHeader,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    bool withBallsColumn) {
    QString body = dateSpecialistRow(userFio, 2);
    body += QStringLiteral(
        "<tr><td>Результат: баллы (макс.)/ вывод об уровне развития</td><td>")
        + editableCell(QStringLiteral("(10)")) + QStringLiteral("</td></tr>");
    body += QStringLiteral("<tr><td>Примечание</td><td>") + editableCell() + QStringLiteral("</td></tr>");
    body += QStringLiteral(
        "<tr><td align='center' colspan='2'>Процесс выполнения диагностической методики</td></tr>"
        "</table><!--s-->");
    body += resultsHeader;
    body += QStringLiteral("<tr><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.activity))
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.help));
    if (withBallsColumn) {
        body += QStringLiteral("</td><td align='center'>") + editableCell();
    }
    body += QStringLiteral("</td></tr></table>");
    return body;
}

QString appendOrHlpRow(
    const QString &existing,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    bool withBallsColumn) {
    QString row = QStringLiteral("<tr><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.activity))
        + QStringLiteral("</td><td valign='top'>") + editableCell(formatProtocolCellText(checkboxes.help));
    if (withBallsColumn) {
        row += QStringLiteral("</td><td align='center'>") + editableCell();
    }
    row += QStringLiteral("</td></tr>");
    return ExerciseProtocol::appendRowsToStoredBody(existing, row);
}

QString createExerciseProtocolBodyFallback(
    const ExerciseDefinition &definition,
    const QString &userFio,
    int elapsedSeconds,
    bool partly,
    const QString &existingProtocolHtml,
    const QList<bool> &answers,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session) {
    Q_UNUSED(answers);
    const QString doneState = session.doneState.trimmed().isEmpty()
        ? QStringLiteral("не определено")
        : session.doneState;
    const QString stepId = session.stepId.trimmed().isEmpty() ? QStringLiteral("1") : session.stepId;
    const QString base = partly ? existingProtocolHtml : QString();

    switch (definition.protocol) {
    case ExerciseProtocolKind::TimedBalls: {
        const int score = definition.id == QStringLiteral("1.8")
            ? scoreExercise18(elapsedSeconds)
            : scoreExercise11(elapsedSeconds);
        const QString header = QStringLiteral(
            "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
            "<tr><td width='308' align='center'>Характер деятельности ребенка</td>"
            "<td width='298' align='center'>Виды помощи</td>"
            "<td width='63' align='center'>Баллы</td></tr>");
        if (partly) {
            return ExerciseProtocol::appendFullSessionToStoredBody(
                base, buildTimedBallsInitial(userFio, score, definition.id != QStringLiteral("1.8"), header, checkboxes));
        }
        const bool maxTen = definition.id != QStringLiteral("1.8");
        return buildTimedBallsInitial(userFio, score, maxTen, header, checkboxes);
    }
    case ExerciseProtocolKind::TimedBallsWithPictureCount: {
        const int score = scoreExercise14(elapsedSeconds, session.picturesShown);
        const QString header = QStringLiteral(
            "<table border='1' cellspacing='0' cellpadding='0' style='table-layout:fixed' width='671'>"
            "<tr><td width='300' align='center'>Характер деятельности ребенка</td>"
            "<td width='309' align='center'>Виды помощи</td>"
            "<td width='60' align='center'>Баллы</td></tr>");
        if (partly) {
            return ExerciseProtocol::appendFullSessionToStoredBody(
                base, buildTimedBallsInitial(userFio, score, true, header, checkboxes));
        }
        return buildTimedBallsInitial(userFio, score, true, header, checkboxes);
    }
    case ExerciseProtocolKind::DoneTimeOrHlp: {
        const QString header = QStringLiteral(
            "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
            "<tr><td width='134' align='center'>Факт выполнения<br>/ время</td>"
            "<td width='268' align='center'>Характер деятельности ребенка</td>"
            "<td width='267' align='center'>Виды помощи</td></tr>");
        if (partly) {
            return ExerciseProtocol::appendFullSessionToStoredBody(
                base, buildDoneTimeInitial(userFio, 3, header, doneState, elapsedSeconds, checkboxes));
        }
        return buildDoneTimeInitial(userFio, 3, header, doneState, elapsedSeconds, checkboxes);
    }
    case ExerciseProtocolKind::NumberedDoneTime: {
        const QString header = QStringLiteral(
            "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
            "<tr><td width='33' align='center'>№</td>"
            "<td width='148' align='center'>Факт выполнения / время</td>"
            "<td width='243' align='center'>Характер деятельности ребенка</td>"
            "<td width='243' align='center'>Виды помощи</td></tr>");
        QString rowDone = doneState;
        QStringList stepIds = session.stepIds;
        if (stepIds.isEmpty()) {
            QString rowStep = stepId;
            if (!session.additional.isEmpty() && session.additional.contains(QLatin1Char(';'))) {
                const QStringList parts = session.additional.split(QLatin1Char(';'));
                if (parts.size() >= 2) {
                    rowStep = parts.at(0);
                    rowDone = parts.at(1);
                }
            }
            stepIds << (rowStep.isEmpty() ? QStringLiteral("1") : rowStep);
        } else if (!session.additional.isEmpty() && session.additional.contains(QLatin1Char(';'))) {
            const QStringList parts = session.additional.split(QLatin1Char(';'));
            if (parts.size() >= 2) {
                rowDone = parts.at(1);
            }
        }

        auto stepTimeOf = [&](const QString &sid) {
            int stepTime = session.stepElapsedSeconds.value(sid, -1);
            if (stepTime < 0) {
                if (session.stepElapsedSeconds.isEmpty()
                    && (stepIds.size() == 1 || sid == session.stepId)) {
                    return elapsedSeconds;
                }
                return 0;
            }
            return stepTime;
        };
        // Повторный протокол — всегда полный блок с «Дата/специалист», не только <tr> процесса.
        QString sessionBody = buildNumberedInitial(
            userFio, header, stepIds.first(), rowDone, stepTimeOf(stepIds.first()), checkboxes);
        for (int i = 1; i < stepIds.size(); ++i) {
            const QString &sid = stepIds.at(i);
            sessionBody = appendNumberedRow(sessionBody, sid, rowDone, stepTimeOf(sid), checkboxes);
        }
        if (partly) {
            return ExerciseProtocol::appendFullSessionToStoredBody(base, sessionBody);
        }
        return sessionBody;
    }
    case ExerciseProtocolKind::OrHlpRow: {
        const QString header = QStringLiteral(
            "<table style='table-layout:fixed' border='1' cellspacing='0' cellpadding='0' width='671'>"
            "<tr><td width='300' align='center'>Характер деятельности ребенка</td>"
            "<td width='300' align='center'>Виды помощи</td>"
            "<td width='69' align='center'>Баллы</td></tr>");
        if (partly) {
            return ExerciseProtocol::appendFullSessionToStoredBody(
                base, buildOrHlpInitial(userFio, header, checkboxes, false));
        }
        return buildOrHlpInitial(userFio, header, checkboxes, false);
    }
    case ExerciseProtocolKind::OrHlpBallsRow: {
        const QString header = QStringLiteral(
            "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
            "<tr><td width='300' align='center'>Характер деятельности ребенка</td>"
            "<td width='300' align='center'>Виды помощи</td>"
            "<td width='69' align='center'>Баллы</td></tr>");
        if (definition.id == QStringLiteral("4.1.2")) {
            const int score = scoreExercise412(elapsedSeconds);
            QString body = dateSpecialistRow(userFio, 2);
            body += QStringLiteral(
                "<tr><td>Результат:баллы (макс.) /<br>вывод об уровне развития</td><td>")
                + editableCell(QStringLiteral("%1(10)/%2").arg(score).arg(developmentLevel(score)))
                + QStringLiteral("</td></tr>");
            body += QStringLiteral("<tr><td>Примечание</td><td>") + editableCell()
                + QStringLiteral("</td></tr>");
            body += QStringLiteral(
                "<tr><td align='center' colspan='2'>Процесс выполнения диагностической методики</td></tr>"
                "</table><!--s-->");
            body += header;
            body += QStringLiteral("<tr><td valign='top'>")
                + editableCell(formatProtocolCellText(checkboxes.activity))
                + QStringLiteral("</td><td valign='top'>")
                + editableCell(formatProtocolCellText(checkboxes.help))
                + QStringLiteral("</td><td align='center'>") + editableCell(QString::number(score))
                + QStringLiteral("</td></tr></table>");
            if (partly) {
                return ExerciseProtocol::appendFullSessionToStoredBody(base, body);
            }
            return body;
        }
        if (partly) {
            return ExerciseProtocol::appendFullSessionToStoredBody(
                base, buildOrHlpInitial(userFio, header, checkboxes, true));
        }
        return buildOrHlpInitial(userFio, header, checkboxes, true);
    }
    case ExerciseProtocolKind::PictureAnswers:
        break;
    }
    return existingProtocolHtml;
}

QString tmpAt(const QStringList &parts, int index) {
    if (index < 0 || index >= parts.size()) {
        return {};
    }
    return parts.at(index).toHtmlEscaped();
}

// Порт protocols.cs createP("1.26"): additional = №задания;answers[0..12]
//
// QTextDocument вкладывает следующую <table> в последнюю ячейку предыдущей.
// Поэтому:
//  - «Процесс…» — обычный <p> после закрытой шапки (не ячейка таблицы);
//  - задания 1 и 2 — строки ОДНОЙ таблицы процесса (не соседние <table>).
QString buildProtocol126(
    const QString &userFio,
    bool partly,
    const QString &existingProtocolHtml,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session) {
    QStringList tmp = session.additional.split(QLatin1Char(';'));
    while (tmp.size() < 14) {
        tmp.append(QString());
    }
    const QString step = tmp.at(0).trimmed().isEmpty() ? QStringLiteral("1") : tmp.at(0).trimmed();
    const QString orText = formatProtocolCellText(checkboxes.activity);
    const QString hlpText = formatProtocolCellText(checkboxes.help);

    const QString tableOpen = QStringLiteral(
        "<table border='1' style='table-layout:fixed' cellspacing='0' width='674' cellpadding='0'>");
    const QString processHeading = QStringLiteral(
        "<p align='center'><b>Процесс выполнения диагностического задания</b></p>");

    QString rows;
    if (step == QStringLiteral("1")) {
        rows += QStringLiteral(
            "<tr><td colspan='3' align='center'> Задание 1 </td></tr>");
        rows += QStringLiteral(
            "<tr><td width='24%'>Характер деятельности ребенка</td>"
            "<td valign='top' colspan='2' align='left'><div contenteditable='true'>")
            + orText + QStringLiteral("</div></td></tr>");
        rows += QStringLiteral(
            "<tr><td width='24%'>Виды помощи</td>"
            "<td valign='top' colspan='2' align='left'><div contenteditable='true'>")
            + hlpText + QStringLiteral("</div></td></tr>");
        rows += QStringLiteral(
            "<tr><td width='24%' align='center'>Портретная картинка</td>"
            "<td align='center'>Ответ ребенка</td>"
            "<td align='center' width='10%'>Баллы</td></tr>");
        rows += QStringLiteral("<tr><td>Радость</td><td align='left'>") + tmpAt(tmp, 2)
            + QStringLiteral("</td><td align='center'><div id='col11' contenteditable='true'></div></td></tr>");
        rows += QStringLiteral("<tr><td>Злость</td><td align='left'>") + tmpAt(tmp, 3)
            + QStringLiteral("</td><td align='center'><div id='col12' contenteditable='true'></div></td></tr>");
        rows += QStringLiteral("<tr><td>Грусть</td><td align='left'>") + tmpAt(tmp, 4)
            + QStringLiteral("</td><td align='center'><div id='col13' contenteditable='true'></div></td></tr>");
        rows += QStringLiteral("<tr><td>Страх</td><td align='left'>") + tmpAt(tmp, 5)
            + QStringLiteral("</td><td align='center'><div id='col14' contenteditable='true'></div></td></tr>");
        rows += QStringLiteral("<tr><td>Удивление</td><td align='left'>") + tmpAt(tmp, 6)
            + QStringLiteral("</td><td align='center'><div id='col15' contenteditable='true'></div></td></tr>");
        rows += QStringLiteral("<tr><td>Спокойствие</td><td align='left'>") + tmpAt(tmp, 7)
            + QStringLiteral("</td><td align='center'><div id='col16' contenteditable='true'></div></td></tr>");
        rows += QStringLiteral(
            "<tr><td align='left' colspan='2'>Итоговая оценка</td>"
            "<td align='center'><div id='sum1' contenteditable='true'></div></td></tr>");
    } else if (step == QStringLiteral("2")) {
        rows += QStringLiteral("<tr><td colspan='3' align='center'><b>Задание 2</b></td></tr>");
        rows += QStringLiteral(
            "<tr><td>Характер деятельности ребенка</td>"
            "<td valign='top' colspan='2' align='left'><div contenteditable='true'>")
            + orText + QStringLiteral("</div></td></tr>");
        rows += QStringLiteral(
            "<tr><td>Виды помощи</td>"
            "<td valign='top' colspan='2' align='left'><div contenteditable='true'>")
            + hlpText + QStringLiteral("</div></td></tr>");
        rows += QStringLiteral(
            "<tr><td align='center'><b>№ рассказа</b></td>"
            "<td align='center'><b>Ответ ребенка</b></td>"
            "<td align='center'><b>Баллы</b></td></tr>");
        for (int i = 1; i <= 12; ++i) {
            rows += QStringLiteral("<tr><td align='center'>") + QString::number(i)
                + QStringLiteral("</td><td align='left'>") + tmpAt(tmp, i + 1)
                + QStringLiteral("</td><td align='center'><div id='col2") + QString::number(i)
                + QStringLiteral("' contenteditable='true'></div></td></tr>");
        }
        rows += QStringLiteral(
            "<tr><td colspan='2' align='left'>Итоговая оценка</td>"
            "<td align='center'><div id='sum2' contenteditable='true'></div></td></tr>");
        rows += QStringLiteral(
            "<tr><td colspan='2' align='left'>Индекс успешности по двум сериям</td>"
            "<td align='center'><div id='sum3' contenteditable='true'></div></td></tr>");
    } else {
        return QString();
    }

    if (partly) {
        // Дописка задания 2 строками в ту же таблицу процесса (не новой <table>).
        return ExerciseProtocol::canonicalizeProtocol126StoredBody(
            ExerciseProtocol::appendRowsToStoredBody(existingProtocolHtml, rows));
    }

    QString add = dateSpecialistRow(userFio, 1);
    add += QStringLiteral(
        "<tr><td>Результат: : баллы (макс.)/вывод об уровне развития</td>"
        "<td><div id='idvivod' contenteditable='true'>(36)</div></td></tr>");
    add += QStringLiteral(
        "<tr><td>Примечание</td><td><div contenteditable='true'></div></td></tr>");
    add += QStringLiteral("</table><!--s-->");
    add += processHeading;
    add += tableOpen + rows + QStringLiteral("</table>");
    return add;
}

// Порт exbegin.cs 4.1.8: заполнение полей sel/ex/re/hlp/rea/b + cidd + idspc.
// Разметка плоская: summary </table><!--s--> + характер + таблица слов
// (иначе Qt вкладывает таблицу стимулов в ячейку «Примечание»).
QString buildProtocol418(
    const QString &userFio,
    bool partly,
    const QString &existingProtocolHtml,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session) {
    static const char *kWords[] = {"Школа", "Обед", "Утро", "Красота", "Прогулка"};
    static const char *kPrefixes[] = {"sel", "ex", "re", "hlp", "rea", "b"};

    const QStringList rows = session.additional.split(QLatin1Char('|'));
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy hh:mm"));

    QString body;
    body += QStringLiteral(
        "<tr><td width='200' valign='top'><p>Дата / специалист</p></td>"
        "<td width='471' valign='top'><div contenteditable='true' id='idspc'>%1   %2</div></td></tr>")
                .arg(now, userFio.toHtmlEscaped());
    body += QStringLiteral(
        "<tr><td width='200' valign='top'><p>Результат: баллы (макс.) /<br /> вывод об уровне развития </p></td>"
        "<td width='471' valign='top'><div contenteditable='true' id='idvivod'></div></td></tr>");
    body += QStringLiteral(
        "<tr><td width='200' valign='top'><p>Примечание</p></td>"
        "<td width='471' valign='top'><div contenteditable='true'></div></td></tr>");
    body += QStringLiteral("</table><!--s-->");
    body += QStringLiteral(
        "<p align='center'><b>Процесс выполнения диагностической методики</b></p>");
    body += QStringLiteral(
        "<table style='table-layout:fixed' border='1' cellspacing='0' cellpadding='0' width='671'>"
        "<tr><td width='200' valign='top'><p>Характер деятельности ребенка</p></td>"
        "<td width='471' valign='top'><div contenteditable='true' id='cidd'>%1</div></td></tr>"
        "</table>")
                .arg(formatProtocolCellText(checkboxes.activity));

    body += QStringLiteral(
        "<table style='table-layout:fixed' border='1' cellspacing='0' cellpadding='0' width='671'>"
        "<tr>"
        "<td width='85' valign='top'><p align='center'>Стимульные слова</p></td>"
        "<td width='70' valign='top'><p align='center'>Выбранная картинка</p></td>"
        "<td width='110' valign='top'><p align='center'>Объяснение выбора</p></td>"
        "<td width='100' valign='top'><p align='center'>Воспроизв. слово до предъявления помощи</p></td>"
        "<td width='140' valign='top'><p align='center'>Виды помощи</p></td>"
        "<td width='110' valign='top'><p align='center'>Воспроизв. слово после предъявления помощи</p></td>"
        "<td width='56' valign='top'><p align='center'>Баллы</p></td>"
        "</tr>");

    for (int r = 0; r < 5; ++r) {
        QStringList cells;
        if (r < rows.size()) {
            cells = rows.at(r).split(QLatin1Char(';'));
        }
        while (cells.size() < 6) {
            cells.append(QString());
        }
        body += QStringLiteral("<tr><td width='85' valign='top'><div id='word%1'>%2</div></td>")
                    .arg(r + 1)
                    .arg(QString::fromUtf8(kWords[r]));
        for (int c = 0; c < 6; ++c) {
            const QString id = QString::fromUtf8(kPrefixes[c]) + QString::number(r + 1);
            const QString align = (c == 5) ? QStringLiteral(" align='center'") : QString();
            body += QStringLiteral(
                        "<td%1 valign='top'><div id='%2' contenteditable='true'>%3</div></td>")
                        .arg(align, id, cells.at(c).toHtmlEscaped());
        }
        body += QStringLiteral("</tr>");
    }
    body += QStringLiteral(
        "<tr><td colspan='6' valign='top'><p>Итоговая оценка</p></td>"
        "<td align='center' width='56' valign='top'><div id='idsum' contenteditable='true'></div></td></tr>"
        "</table>");

    if (partly && !existingProtocolHtml.trimmed().isEmpty()) {
        return ExerciseProtocol::appendFullSessionToStoredBody(existingProtocolHtml, body);
    }
    return body;
}

} // namespace

QString readDoneStateFromOrHtml(const QString &orHtml) {
    static const struct {
        const char *id;
        const char *label;
    } kStates[] = {
        {"idv1", "Выполнено"},
        {"idv2", "Выполнено частично"},
        {"idv3", "Не выполнено"},
    };
    for (const auto &state : kStates) {
        const QRegularExpression re(
            QStringLiteral("id=[\"']%1[\"'][^>]*checked(?:=[\"']?checked[\"']?)?")
                .arg(QString::fromUtf8(state.id)),
            QRegularExpression::CaseInsensitiveOption);
        if (re.match(orHtml).hasMatch()) {
            return QString::fromUtf8(state.label);
        }
        const QRegularExpression valueRe(
            QStringLiteral("id=[\"']%1[\"'][^>]*value=[\"']([^\"']+)[\"'][^>]*checked")
                .arg(QString::fromUtf8(state.id)),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = valueRe.match(orHtml);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    return QStringLiteral("не определено");
}

QString createExerciseProtocolBody(
    const ExerciseDefinition &definition,
    const QString &userFio,
    int elapsedSeconds,
    bool partly,
    const QString &existingProtocolHtml,
    const QList<bool> &answers,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session) {
    // 1.26 — явная сборка как в оригинале (ответы tmp[] + таблицы заданий).
    if (definition.id == QStringLiteral("1.26")) {
        const QString body126 = buildProtocol126(userFio, partly, existingProtocolHtml, checkboxes, session);
        if (!body126.isEmpty()) {
            return body126;
        }
    }
    // 4.1.8 — заполнение template.html (sel/ex/re/hlp/rea/b + cidd + idspc), как в exbegin.
    if (definition.id == QStringLiteral("4.1.8")) {
        const QString body418 = buildProtocol418(userFio, partly, existingProtocolHtml, checkboxes, session);
        if (!body418.isEmpty()) {
            return body418;
        }
    }
    const QString fromTemplate = createExerciseProtocolFromTemplate(
        definition.id,
        userFio,
        elapsedSeconds,
        partly,
        existingProtocolHtml,
        answers,
        checkboxes,
        session);
    if (!fromTemplate.isEmpty()) {
        return fromTemplate;
    }
    return createExerciseProtocolBodyFallback(
        definition,
        userFio,
        elapsedSeconds,
        partly,
        existingProtocolHtml,
        answers,
        checkboxes,
        session);
}
