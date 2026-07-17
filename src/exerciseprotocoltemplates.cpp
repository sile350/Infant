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

namespace {

struct ProtocolTemplate {
    QString id;
    QString kind;
    QString scoreKind;
    QString dateRow;
    QString initialBlock;
    QString rowTemplate;
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

    int score = 0;
    if (tmpl.id == QStringLiteral("4.1.2") || tmpl.scoreKind == QStringLiteral("timed11_result")) {
        // timed11_result в шаблонах: для 4.1.2 — шкала 45с; для прочих открытых — та же.
        if (tmpl.id == QStringLiteral("4.1.2")) {
            score = scoreExercise412(elapsedSeconds);
        } else {
            score = scoreExercise11(elapsedSeconds);
        }
    } else if (tmpl.scoreKind == QStringLiteral("timed11")) {
        score = scoreExercise11(elapsedSeconds);
    } else if (tmpl.scoreKind == QStringLiteral("timed18")) {
        score = scoreExercise18(elapsedSeconds);
    } else if (tmpl.scoreKind == QStringLiteral("timed14") || tmpl.id == QStringLiteral("1.4")) {
        score = scoreExercise14(elapsedSeconds, session.picturesShown);
    } else if (tmpl.scoreKind == QStringLiteral("or_checkbox_4")) {
        score = wolfScoreFromOrHtml(session.orHtml);
    }
    vars.insert(QStringLiteral("{{SCORE}}"), QString::number(score));
    vars.insert(QStringLiteral("{{LEVEL}}"), developmentLevel(score).toHtmlEscaped());

    if (!session.capturedImagePath.isEmpty()) {
        vars.insert(QStringLiteral("{{SCAN}}"), scanLinkHtml(session.capturedImagePath));
    } else {
        vars.insert(QStringLiteral("{{SCAN}}"), QString());
    }
    vars.insert(QStringLiteral("{{SCAN_SLOTS}}"), QStringLiteral("скачать1 скачать2 скачать3"));

    if (tmpl.kind == QStringLiteral("wolf_542")) {
        fillWolfVariables(session.additional, &vars);
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

} // namespace

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
    const QString row = buildRow(tmpl, vars, answers, checkboxes, session);

    if (partly) {
        // Повторный протокол: для timed-методик добавляем с новой строки «Дата/специалист».
        if (tmpl.scoreKind.startsWith(QStringLiteral("timed")) || tmpl.scoreKind == QStringLiteral("timed11_result")
            || tmpl.scoreKind == QStringLiteral("timed18_result") || tmpl.id == QStringLiteral("1.1")
            || tmpl.id == QStringLiteral("1.4") || tmpl.id == QStringLiteral("1.8")) {
            QString addition;
            if (!tmpl.dateRow.isEmpty()) {
                addition += substituteAll(tmpl.dateRow, vars);
            }
            addition += row;
            return existingProtocolHtml + addition;
        }
        return existingProtocolHtml + row;
    }

    QString body;
    if (!tmpl.dateRow.isEmpty()) {
        body += substituteAll(tmpl.dateRow, vars);
    }
    if (!tmpl.initialBlock.isEmpty()) {
        body += substituteAll(tmpl.initialBlock, vars);
    }
    body += row;

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
