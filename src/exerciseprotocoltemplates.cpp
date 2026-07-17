#include "exerciseprotocoltemplates.h"

#include "exerciseprotocolcreate.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QRegularExpression>
#include <QUrl>
#include <QtMath>
#include <cmath>

namespace {

struct ProtocolTemplate {
    QString id;
    QString kind;
    QString scoreKind;
    QString dateRow;
    QString initialBlock;
    QString rowTemplate;
    QString summaryRow;
    QMap<QString, QString> rowVariants;
};

QString protocolTemplatesDir() {
    const QStringList candidates = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/assets/protocol_templates"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../assets/protocol_templates"),
        QDir::currentPath() + QStringLiteral("/assets/protocol_templates"),
    };
    for (const QString &path : candidates) {
        if (QFile::exists(path + QStringLiteral("/index.json"))) {
            return path;
        }
    }
    return candidates.first();
}

bool loadProtocolTemplate(const QString &exerciseId, ProtocolTemplate *out) {
    const QString path = protocolTemplatesDir() + QLatin1Char('/') + exerciseId + QStringLiteral(".json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    if (obj.isEmpty()) {
        return false;
    }
    out->id = obj.value(QStringLiteral("id")).toString(exerciseId);
    out->kind = obj.value(QStringLiteral("kind")).toString();
    out->scoreKind = obj.value(QStringLiteral("scoreKind")).toString();
    out->dateRow = obj.value(QStringLiteral("dateRow")).toString();
    out->initialBlock = obj.value(QStringLiteral("initialBlock")).toString();
    out->rowTemplate = obj.value(QStringLiteral("rowTemplate")).toString();
    out->summaryRow = obj.value(QStringLiteral("summaryRow")).toString();
    out->rowVariants.clear();
    const QJsonObject variants = obj.value(QStringLiteral("rowVariants")).toObject();
    for (auto it = variants.constBegin(); it != variants.constEnd(); ++it) {
        out->rowVariants.insert(it.key(), it.value().toString());
    }
    return true;
}

QString formatProtocolTime(int elapsedSeconds) {
    const int minutes = elapsedSeconds / 60;
    const int seconds = elapsedSeconds - minutes * 60;
    return QStringLiteral("%1:%2 сек").arg(minutes).arg(seconds);
}

QString formatProtocolCellText(const QString &text) {
    if (text.trimmed().isEmpty()) {
        return QString();
    }
    const QStringList parts = text.split(QRegularExpression(QStringLiteral("[\\r\\n;]+")), Qt::SkipEmptyParts);
    QStringList lines;
    for (const QString &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            lines << QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;%1").arg(trimmed.toHtmlEscaped());
        }
    }
    return lines.join(QStringLiteral("<br>"));
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

// 3.1.11 / шкала «Нелепицы» и родственные: пороги 60/75/90…/195
int scoreExerciseTimed60(int time) {
    if (time <= 60) return 10;
    if (time <= 75) return 9;
    if (time <= 90) return 8;
    if (time <= 105) return 7;
    if (time <= 120) return 6;
    if (time <= 135) return 5;
    if (time <= 150) return 4;
    if (time <= 165) return 3;
    if (time <= 180) return 2;
    if (time <= 195) return 1;
    return 0;
}

// 4.1.2 (protocols.cs): пороги 45 / 46-47 / … / ≥86
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

QString scanLinkHtml(const QString &path) {
    if (path.trimmed().isEmpty()) {
        return QString();
    }
    const QString url = QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()).toString();
    return QStringLiteral("<a href='%1'>Показать изображение</a>").arg(url);
}

int wolfScoreFromOrHtml(const QString &orHtml) {
    static const struct {
        const char *id;
        int score;
    } kLevels[] = {
        {"idd1", 4}, {"idd2", 3}, {"idd3", 2}, {"idd4", 1}, {"idd5", 0},
    };
    for (const auto &level : kLevels) {
        const QRegularExpression re(
            QStringLiteral("id=[\"']%1[\"'][^>]*checked(?:=[\"']?checked[\"']?)?")
                .arg(QString::fromUtf8(level.id)),
            QRegularExpression::CaseInsensitiveOption);
        if (re.match(orHtml).hasMatch()) {
            return level.score;
        }
    }
    return 0;
}

QString cleanTemplateArtifacts(QString html) {
    html.replace(QStringLiteral("\" {{"), QStringLiteral("{{"));
    html.replace(QStringLiteral("}} \""), QStringLiteral("}}"));
    html.replace(QStringLiteral("\"{{"), QStringLiteral("{{"));
    html.replace(QStringLiteral("}}\" "), QStringLiteral("}} "));
    html.replace(QStringLiteral("}}\"</div>"), QStringLiteral("}}</div>"));
    html.replace(QStringLiteral("}}\"(4)"), QStringLiteral("}}(4)"));
    // Артефакты экстрактора C#: `" + "` / `"   + "` → пустая подстановка.
    html.replace(QRegularExpression(QStringLiteral("\"\\s*\\+\\s*\"")), QString());
    return html;
}

QString substituteAll(QString html, const QMap<QString, QString> &vars) {
    html = cleanTemplateArtifacts(html);
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        html.replace(it.key(), it.value());
    }
    return html;
}

QString ensureRowWrapped(const QString &rowHtml) {
    const QString trimmed = rowHtml.trimmed();
    if (trimmed.startsWith(QStringLiteral("<tr"), Qt::CaseInsensitive)) {
        return trimmed;
    }
    if (trimmed.startsWith(QStringLiteral("<td"), Qt::CaseInsensitive)) {
        return QStringLiteral("<tr>") + trimmed;
    }
    if (trimmed.contains(QStringLiteral("<tr"), Qt::CaseInsensitive)) {
        return trimmed;
    }
    return trimmed;
}

QString buildPictureAnswersRow(
    const QList<bool> &answers,
    const ExerciseProtocol::CheckboxValues &checkboxes) {
    static const QStringList descriptions = {
        QStringLiteral("1. Бабушка на диване без ножки."),
        QStringLiteral("2. Велосипедист без переднего колеса."),
        QStringLiteral("3. Девочка с расческой без зубчиков."),
        QStringLiteral("4. Пальто без рукава."),
        QStringLiteral("5. Ослик без уха."),
    };
    const QString activityHtml = formatProtocolCellText(checkboxes.activity);
    const QString helpHtml = formatProtocolCellText(checkboxes.help);
    QString row;
    for (int i = 0; i < descriptions.size(); ++i) {
        const bool correct = i < answers.size() ? answers.at(i) : false;
        const QString verno = correct ? QStringLiteral("верно") : QStringLiteral("неверно");
        if (i == 0) {
            row += QStringLiteral("<tr><td >%1</td><td valign='top' >%2</td>"
                                  "<td valign='top' rowspan='5'><div contenteditable='true'>%3</div></td>"
                                  "<td valign='top' rowspan='5'><div contenteditable='true'>%4</div></td></tr>")
                       .arg(descriptions.at(i), verno, activityHtml, helpHtml);
        } else {
            const QString valign = i == 1 || i == 2 ? QStringLiteral(" valign='top'") : QString();
            row += QStringLiteral("<tr><td>%1</td><td%2>%3</td></tr>")
                       .arg(descriptions.at(i), valign, verno);
        }
    }
    return row;
}

void fillWolfVariables(const QString &additional, QMap<QString, QString> *vars) {
    const QStringList parts = additional.split(QLatin1Char('|'));
    QStringList helpParts;
    QStringList answerParts;
    if (parts.size() >= 1) {
        helpParts = parts.at(0).split(QLatin1Char(';'));
    }
    if (parts.size() >= 2) {
        answerParts = parts.at(1).split(QLatin1Char(';'));
    }
    for (int i = 0; i < 7; ++i) {
        vars->insert(
            QStringLiteral("{{HELP%1}}").arg(i),
            i < helpParts.size() ? helpParts.at(i).toHtmlEscaped() : QString());
        vars->insert(
            QStringLiteral("{{ANSWER%1}}").arg(i),
            i < answerParts.size() ? answerParts.at(i).toHtmlEscaped() : QString());
    }
}

QMap<QString, QString> buildVariables(
    const ProtocolTemplate &tmpl,
    const QString &userFio,
    int elapsedSeconds,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session) {
    QMap<QString, QString> vars;
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"));
    vars.insert(QStringLiteral("{{DATE}}"), now);
    vars.insert(QStringLiteral("{{USER}}"), userFio.toHtmlEscaped());
    vars.insert(QStringLiteral("{{OR}}"), formatProtocolCellText(checkboxes.activity));
    vars.insert(QStringLiteral("{{HLP}}"), formatProtocolCellText(checkboxes.help));
    vars.insert(QStringLiteral("{{TIME}}"), formatProtocolTime(elapsedSeconds).toHtmlEscaped());

    QString stepId = session.stepId.trimmed().isEmpty() ? QStringLiteral("1") : session.stepId;
    QString doneState = session.doneState.trimmed().isEmpty()
        ? QStringLiteral("не определено")
        : session.doneState;
    if (!session.additional.isEmpty() && session.additional.contains(QLatin1Char(';'))) {
        const QStringList parts = session.additional.split(QLatin1Char(';'));
        if (parts.size() >= +2) {
            stepId = parts.at(0);
            doneState = parts.at(1);
        }
    }
    vars.insert(QStringLiteral("{{STEP}}"), stepId.toHtmlEscaped());
    vars.insert(QStringLiteral("{{DONE}}"), doneState.toHtmlEscaped());
    vars.insert(QStringLiteral("{{ADDITIONAL}}"), session.additional.toHtmlEscaped());

    double score = 0;
    if (tmpl.id == QStringLiteral("4.1.2") || tmpl.scoreKind == QStringLiteral("timed11_result")) {
        // timed11_result в шаблонах: для 4.1.2 — шкала 45с; для прочих открытых — та же.
        if (tmpl.id == QStringLiteral("4.1.2")) {
            score = scoreExercise412(elapsedSeconds);
        } else if (tmpl.id == QStringLiteral("3.1.11")) {
            score = scoreExerciseTimed60(elapsedSeconds);
        } else {
            score = scoreExercise11(elapsedSeconds);
        }
    } else if (tmpl.scoreKind == QStringLiteral("timed60_result")) {
        score = scoreExerciseTimed60(elapsedSeconds);
    } else if (tmpl.scoreKind == QStringLiteral("timed11")) {
        score = scoreExercise11(elapsedSeconds);
    } else if (tmpl.scoreKind == QStringLiteral("timed18")) {
        score = scoreExercise18(elapsedSeconds);
    } else if (tmpl.scoreKind == QStringLiteral("timed14") || tmpl.id == QStringLiteral("1.4")) {
        score = scoreExercise14(elapsedSeconds, session.picturesShown);
    } else if (tmpl.scoreKind == QStringLiteral("or_checkbox_4")) {
        score = wolfScoreFromOrHtml(session.orHtml);
    } else if (tmpl.scoreKind == QStringLiteral("activity_help_2") || tmpl.id == QStringLiteral("3.1.10")) {
        // Как в protocols.cs 3.1.10: idd3 → 2, каждый вид помощи −0.5.
        if (checkboxes.activity.contains(QStringLiteral("Целенаправленное"), Qt::CaseInsensitive)
            || checkboxes.activity.contains(QStringLiteral("III уровень"), Qt::CaseInsensitive)
            || checkboxes.activity.contains(QStringLiteral("2 балла"), Qt::CaseInsensitive)) {
            score = 2.0;
        }
        const QStringList helpParts =
            checkboxes.help.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
        score = qMax(0.0, score - 0.5 * helpParts.size());
    }
    if (tmpl.scoreKind == QStringLiteral("activity_help_2") || tmpl.id == QStringLiteral("3.1.10")) {
        if (qFuzzyIsNull(score - std::floor(score))) {
            vars.insert(QStringLiteral("{{SCORE}}"), QString::number(static_cast<int>(score)));
        } else {
            vars.insert(QStringLiteral("{{SCORE}}"), QString::number(score, 'f', 1));
        }
    } else {
        vars.insert(QStringLiteral("{{SCORE}}"), QString::number(static_cast<int>(score)));
    }
    vars.insert(QStringLiteral("{{LEVEL}}"), developmentLevel(static_cast<int>(score)).toHtmlEscaped());

    if (!session.capturedImagePath.isEmpty()) {
        vars.insert(QStringLiteral("{{SCAN}}"), scanLinkHtml(session.capturedImagePath));
    } else {
        vars.insert(QStringLiteral("{{SCAN}}"), QString());
    }
    vars.insert(QStringLiteral("{{SCAN_SLOTS}}"), QStringLiteral("скачать1 скачать2 скачать3"));

    if (tmpl.kind == QStringLiteral("wolf_542")) {
        fillWolfVariables(session.additional, &vars);
    }

    // 5.1.1: слова разделены '[' (как в e511 / protocols.cs).
    if (tmpl.id == QStringLiteral("5.1.1")) {
        const QStringList words = session.additional.split(QLatin1Char('['));
        for (int i = 0; i < 8; ++i) {
            vars.insert(
                QStringLiteral("{{W%1}}").arg(i),
                i < words.size() ? words.at(i).toHtmlEscaped() : QString());
        }
    }

    const QStringList tmpParts = session.additional.split(QLatin1Char(';'));
    for (int i = 0; i < tmpParts.size(); ++i) {
        vars.insert(QStringLiteral("{{TMP%1}}").arg(i), tmpParts.at(i).toHtmlEscaped());
    }

    return vars;
}

QString substituteTmpIndices(QString html, const QString &additional) {
    const QStringList parts = additional.split(QLatin1Char(';'));
    for (int i = 0; i < parts.size(); ++i) {
        const QString value = parts.at(i).toHtmlEscaped();
        html.replace(QStringLiteral("{{TMP%1}}").arg(i), value);
        const QRegularExpression re(
            QStringLiteral("\"\\s*\\+\\s*tmp\\[%1\\]\\s*\\+\\s*\"").arg(i));
        html.replace(re, value);
        html.replace(QStringLiteral("\" + tmp[%1] + \"").arg(i), value);
        html.replace(QStringLiteral("\" + tmp[%1]+ \"").arg(i), value);
        html.replace(QStringLiteral("\"+ tmp[%1]+ \"").arg(i), value);
        html.replace(QStringLiteral("\" + tmp[%1]+ \"").arg(i), value);
    }
    return html;
}

QString resolveRowTemplate(const ProtocolTemplate &tmpl, const ProtocolSessionInput &session) {
    if (!tmpl.rowVariants.isEmpty()) {
        const QStringList parts = session.additional.split(QLatin1Char(';'));
        const QString key = parts.isEmpty() ? QString() : parts.at(0).trimmed();
        if (tmpl.rowVariants.contains(key)) {
            return tmpl.rowVariants.value(key);
        }
        if (!tmpl.rowVariants.isEmpty()) {
            return tmpl.rowVariants.constBegin().value();
        }
    }
    return tmpl.rowTemplate;
}

QString buildRow(
    const ProtocolTemplate &tmpl,
    const QMap<QString, QString> &vars,
    const QList<bool> &answers,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session) {
    if (tmpl.kind == QStringLiteral("picture_answers")) {
        return buildPictureAnswersRow(answers, checkboxes);
    }
    const QString rowTpl = resolveRowTemplate(tmpl, session);
    if (rowTpl.isEmpty()) {
        return QString();
    }
    QString row = substituteAll(rowTpl, vars);
    row = substituteTmpIndices(row, session.additional);
    return ensureRowWrapped(row);
}

// Numbered: по строке на каждое задание; TIME — таймер этого задания; DONE/OR/HLP — одинаковые.
QString buildNumberedProcessRows(
    const ProtocolTemplate &tmpl,
    const QMap<QString, QString> &baseVars,
    const ProtocolSessionInput &session,
    int fallbackElapsedSeconds) {
    QStringList stepIds = session.stepIds;
    if (stepIds.isEmpty() && !session.stepId.trimmed().isEmpty()) {
        stepIds << session.stepId.trimmed();
    }
    if (stepIds.isEmpty()) {
        stepIds << QStringLiteral("1");
    }

    const QString rowTpl = resolveRowTemplate(tmpl, session);
    if (rowTpl.isEmpty()) {
        return QString();
    }

    QString rows;
    for (const QString &stepId : stepIds) {
        QMap<QString, QString> vars = baseVars;
        vars.insert(QStringLiteral("{{STEP}}"), stepId.toHtmlEscaped());
        int stepTime = session.stepElapsedSeconds.value(stepId, -1);
        if (stepTime < 0) {
            // Задание не запускали — 0:0; если карта пуста и это единственный/текущий шаг — общий таймер.
            if (session.stepElapsedSeconds.isEmpty()
                && (stepIds.size() == 1 || stepId == session.stepId)) {
                stepTime = fallbackElapsedSeconds;
            } else {
                stepTime = 0;
            }
        }
        vars.insert(QStringLiteral("{{TIME}}"), formatProtocolTime(stepTime).toHtmlEscaped());
        QString row = substituteAll(rowTpl, vars);
        row = substituteTmpIndices(row, session.additional);
        rows += ensureRowWrapped(row);
    }
    return rows;
}

// 1.27 / 1.272: строка на каждое № задания + одна «Итоговая оценка» (как createP + trim1).
QString buildOrHlpBallsProcessRows(
    const ProtocolTemplate &tmpl,
    const QMap<QString, QString> &baseVars,
    const ProtocolSessionInput &session) {
    QStringList stepIds = session.stepIds;
    if (stepIds.isEmpty()) {
        QString step = session.stepId.trimmed();
        if (step.isEmpty() && !session.additional.trimmed().isEmpty()) {
            step = session.additional.split(QLatin1Char(';')).value(0).trimmed();
        }
        if (step.isEmpty()) {
            step = QStringLiteral("1");
        }
        stepIds << step;
    }

    const QString rowTpl = tmpl.rowTemplate;
    if (rowTpl.isEmpty()) {
        return QString();
    }

    QString rows;
    for (const QString &stepId : stepIds) {
        QMap<QString, QString> vars = baseVars;
        vars.insert(QStringLiteral("{{STEP}}"), stepId.toHtmlEscaped());
        vars.insert(QStringLiteral("{{ADDITIONAL}}"), stepId.toHtmlEscaped());
        rows += ensureRowWrapped(substituteAll(rowTpl, vars));
    }
    if (!tmpl.summaryRow.isEmpty()) {
        rows += ensureRowWrapped(substituteAll(tmpl.summaryRow, baseVars));
    }
    return rows;
}

} // namespace

QString trimTrailingSummaryRow(QString body) {
    // Как trim1 в оригинале: срезать последнюю <tr> (строка «Итоговая оценка»).
    int index = body.lastIndexOf(QStringLiteral("<tr"), -1, Qt::CaseInsensitive);
    if (index < 0) {
        return body;
    }
    const QString tail = body.mid(index);
    if (!tail.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)) {
        return body;
    }
    return body.left(index);
}

QString createExerciseProtocolFromTemplate(
    const QString &exerciseId,
    const QString &userFio,
    int elapsedSeconds,
    bool partly,
    const QString &existingProtocolHtml,
    const QList<bool> &answers,
    const ExerciseProtocol::CheckboxValues &checkboxes,
    const ProtocolSessionInput &session) {
    ProtocolTemplate tmpl;
    if (!loadProtocolTemplate(exerciseId, &tmpl)) {
        return QString();
    }

    const QMap<QString, QString> vars = buildVariables(tmpl, userFio, elapsedSeconds, checkboxes, session);
    QString row;
    if (tmpl.kind == QStringLiteral("numbered")) {
        row = buildNumberedProcessRows(tmpl, vars, session, elapsedSeconds);
    } else if (tmpl.kind == QStringLiteral("or_hlp_balls")) {
        row = buildOrHlpBallsProcessRows(tmpl, vars, session);
    } else {
        row = buildRow(tmpl, vars, answers, checkboxes, session);
    }

    if (partly) {
        // 1.26/tmp0_variants, 3.1.10, 1.27/1.272: дописка строк задания в текущий протокол.
        if (tmpl.kind == QStringLiteral("tmp0_variants")
            || tmpl.kind == QStringLiteral("or_hlp_balls_row")
            || tmpl.kind == QStringLiteral("or_hlp_balls")) {
            QStringList stepIds = session.stepIds;
            if (stepIds.isEmpty()) {
                QString stepKey = session.stepId.trimmed();
                if (stepKey.isEmpty()) {
                    stepKey = session.additional.split(QLatin1Char(';')).value(0).trimmed();
                }
                if (stepKey.isEmpty()) {
                    stepKey = QStringLiteral("1");
                }
                stepIds << stepKey;
            }
            const QString idPrefix = (tmpl.kind == QStringLiteral("or_hlp_balls"))
                ? QStringLiteral("ids")
                : QStringLiteral("idb");
            QStringList newSteps;
            for (const QString &sid : stepIds) {
                const QString idToken = idPrefix + sid;
                const bool present =
                    existingProtocolHtml.contains(
                        QStringLiteral("id='%1'").arg(idToken), Qt::CaseInsensitive)
                    || existingProtocolHtml.contains(
                        QStringLiteral("id=\"%1\"").arg(idToken), Qt::CaseInsensitive);
                if (!present) {
                    newSteps << sid;
                }
            }
            // Все шаги уже были — повторный протокол с новой «Дата/специалист».
            if (newSteps.isEmpty()) {
                ProtocolSessionInput repeatSession = session;
                repeatSession.stepIds = stepIds;
                const QString repeatRows = (tmpl.kind == QStringLiteral("or_hlp_balls"))
                    ? buildOrHlpBallsProcessRows(tmpl, vars, repeatSession)
                    : row;
                QString sessionBlock;
                if (!tmpl.dateRow.isEmpty()) {
                    sessionBlock += substituteAll(tmpl.dateRow, vars);
                }
                if (!tmpl.initialBlock.isEmpty()) {
                    sessionBlock += substituteAll(tmpl.initialBlock, vars);
                }
                sessionBlock += repeatRows;
                if (!sessionBlock.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
                    sessionBlock += QStringLiteral("</table>");
                }
                return ExerciseProtocol::appendFullSessionToStoredBody(existingProtocolHtml, sessionBlock);
            }
            ProtocolSessionInput appendSession = session;
            appendSession.stepIds = newSteps;
            const QString appendRows = (tmpl.kind == QStringLiteral("or_hlp_balls"))
                ? buildOrHlpBallsProcessRows(tmpl, vars, appendSession)
                : row;
            return ExerciseProtocol::appendRowsToStoredBody(
                trimTrailingSummaryRow(existingProtocolHtml), appendRows);
        }
        // Повторный протокол (ТЗ 14.2): всегда новая сессия со строки «Дата/специалист».
        // Для numbered (1.17/1.18/2.10/…) нельзя дописывать только строки процесса —
        // иначе повторная сессия сливается с предыдущей без новой даты.
        QString sessionBlock;
        if (!tmpl.dateRow.isEmpty()) {
            sessionBlock += substituteAll(tmpl.dateRow, vars);
        } else {
            // Защита: шаблон без dateRow — всё равно начинаем с даты.
            const QString now = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"));
            sessionBlock += QStringLiteral("<tr><td>Дата/специалист</td><td>%1   %2</td></tr>")
                                .arg(now, userFio.toHtmlEscaped());
        }
        if (!tmpl.initialBlock.isEmpty()) {
            sessionBlock += substituteAll(tmpl.initialBlock, vars);
        }
        sessionBlock += row;
        if (!sessionBlock.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
            sessionBlock += QStringLiteral("</table>");
        }
        if (!session.capturedImagePath.isEmpty()
            && (tmpl.kind == QStringLiteral("done_time_scan") || tmpl.kind == QStringLiteral("scan_slots"))) {
            const QString link = scanLinkHtml(session.capturedImagePath);
            const QStringList parts = session.additional.split(QLatin1Char(';'));
            if (!parts.isEmpty() && !parts.at(0).trimmed().isEmpty()) {
                sessionBlock.replace(QStringLiteral("скачать") + parts.at(0).trimmed(), link);
            } else {
                sessionBlock.replace(QStringLiteral("скачать"), link);
            }
        }
        return ExerciseProtocol::appendFullSessionToStoredBody(existingProtocolHtml, sessionBlock);
    }

    QString body;
    if (!tmpl.dateRow.isEmpty()) {
        body += substituteAll(tmpl.dateRow, vars);
    }
    if (!tmpl.initialBlock.isEmpty()) {
        body += substituteAll(tmpl.initialBlock, vars);
    }
    body += row;
    if (!body.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
        body += QStringLiteral("</table>");
    }

    if (!session.capturedImagePath.isEmpty()
        && (tmpl.kind == QStringLiteral("done_time_scan") || tmpl.kind == QStringLiteral("scan_slots"))) {
        const QString link = scanLinkHtml(session.capturedImagePath);
        const QStringList parts = session.additional.split(QLatin1Char(';'));
        if (!parts.isEmpty() && !parts.at(0).trimmed().isEmpty()) {
            body.replace(QStringLiteral("скачать") + parts.at(0).trimmed(), link);
        } else {
            body.replace(QStringLiteral("скачать"), link);
        }
    }
    return body;
}
