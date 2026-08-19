#include "exerciseprotocoltemplates.h"

#include "exerciseconfig.h"
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
        return QStringLiteral("&nbsp;");
    }
    QString normalized = text;
    normalized.replace(QChar::LineSeparator, QLatin1Char('\n'));
    normalized.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    const QStringList parts =
        normalized.split(QRegularExpression(QStringLiteral("[\\r\\n;]+")), Qt::SkipEmptyParts);
    QStringList lines;
    for (const QString &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            // Без ведущих &nbsp;: иначе правки в уже заполненных OR/HLP не сохраняются.
            lines << trimmed.toHtmlEscaped();
        }
    }
    return lines.isEmpty() ? QStringLiteral("&nbsp;") : lines.join(QStringLiteral("<br>"));
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

// 4.1.4 «Запомни рисунки» (or.html): диапазоны времени → баллы.
// Число узнанных картинок в ПО не фиксируется — шкала по времени сессии
// (как типичный профиль в правиле: <45→10, 45–55→8–9, …, ≥90→0).
int scoreExercise414(int time) {
    if (time < 45) {
        return 10;
    }
    if (time <= 55) {
        return time <= 50 ? 9 : 8;
    }
    if (time <= 65) {
        return time <= 60 ? 7 : 6;
    }
    if (time <= 75) {
        return time <= 70 ? 5 : 4;
    }
    if (time <= 85) {
        return time <= 80 ? 3 : 2;
    }
    if (time < 90) {
        return 1;
    }
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

// 1.5 «Чем залатать коврик?» (protocols.cs): пороги 20/25/30…/63
int scoreExercise15(int time) {
    if (time < 20) return 10;
    if (time <= 25) return 9;
    if (time <= 30) return 8;
    if (time < 35) return 7;
    if (time < 40) return 6;
    if (time < 45) return 5;
    if (time < 50) return 4;
    if (time < 55) return 3;
    if (time < 60) return 2;
    if (time <= 63) return 1;
    return 0;
}

// 3.1.15 «Кому чего не достает?» (protocols.cs)
int scoreExercise315(int time) {
    if (time < 30) return 10;
    if (time < 41) return 9;
    if (time < 49) return 8;
    if (time < 61) return 7;
    if (time < 69) return 6;
    if (time < 81) return 5;
    if (time < 89) return 4;
    if (time < 101) return 3;
    if (time < 109) return 2;
    if (time >= 110) return 0;
    return 0;
}

// 1.15 база по времени (устаревшая шкала из protocols.cs; не используется по руководству)
int scoreExercise115Base(int time) {
    if (time < 20) return 10;
    if (time < 25) return 9;
    if (time < 30) return 8;
    if (time < 35) return 7;
    if (time < 40) return 6;
    if (time < 45) return 5;
    if (time < 50) return 4;
    if (time < 55) return 3;
    if (time < 60) return 2;
    return 0;
}

int helpPenaltyHalfPoints(const QString &help) {
    const QStringList helpParts =
        help.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    return helpParts.size();
}

// 1.15 «Подбери фигуру к предмету» (руководство):
// IV (зрительное соотнесение) = 3; III (пробы/наложение) = 2.5;
// каждый вид помощи −0.5; макс. 3 за серию, 9 за три серии.
// I/II — балл в руководстве не зафиксирован: специалист заполняет вручную.
double scoreExercise115Series(const QString &activity, const QString &help) {
    double score = -1.0;
    if (activity.contains(QStringLiteral("IV уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("4 уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("безошибочное зрительное"), Qt::CaseInsensitive)) {
        score = 3.0;
    } else if (activity.contains(QStringLiteral("III уровень"), Qt::CaseInsensitive)
               || activity.contains(QStringLiteral("3 уровень"), Qt::CaseInsensitive)
               || activity.contains(QStringLiteral("методом проб"), Qt::CaseInsensitive)
               || activity.contains(QStringLiteral("путем наложения"), Qt::CaseInsensitive)
               || activity.contains(QStringLiteral("путём наложения"), Qt::CaseInsensitive)) {
        score = 2.5;
    } else {
        return -1.0;
    }
    score -= 0.5 * helpPenaltyHalfPoints(help);
    if (score < 0) {
        score = 0;
    }
    if (score > 3.0) {
        score = 3.0;
    }
    return score;
}

// Уровень из текста оценки (I–IV / N балл): IV→3, III→2, иначе 0 (1.12, 2.11).
double activityLevelScoreMax3(const QString &activity) {
    if (activity.contains(QStringLiteral("IV уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("4 уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("3 балла"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("(3 балл"), Qt::CaseInsensitive)) {
        return 3.0;
    }
    if (activity.contains(QStringLiteral("III уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("3 уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("2 балла"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("(2 балл"), Qt::CaseInsensitive)) {
        return 2.0;
    }
    return 0.0;
}

// 2.12: только III уровень → 3; I/II → 0 (как protocols.cs idd3).
double activityLevelScore212(const QString &activity) {
    if (activity.contains(QStringLiteral("III уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("3 уровень"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("3 балла"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("(3 балл"), Qt::CaseInsensitive)
        || activity.contains(QStringLiteral("выполняет задание верно"), Qt::CaseInsensitive)) {
        return 3.0;
    }
    return 0.0;
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

int wolfScoreFromActivityText(const QString &activity) {
    // Порядок: от большего к меньшему (чтобы «4 балла» не перепутать с «балла»).
    if (activity.contains(QStringLiteral("4 балла"), Qt::CaseInsensitive)) {
        return 4;
    }
    if (activity.contains(QStringLiteral("3 балла"), Qt::CaseInsensitive)) {
        return 3;
    }
    if (activity.contains(QStringLiteral("2 балла"), Qt::CaseInsensitive)) {
        return 2;
    }
    if (activity.contains(QStringLiteral("1 балл"), Qt::CaseInsensitive)
        && !activity.contains(QStringLiteral("0 балл"), Qt::CaseInsensitive)) {
        return 1;
    }
    if (activity.contains(QStringLiteral("0 балл"), Qt::CaseInsensitive)) {
        return 0;
    }
    return -1;
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
        const QString help = i < helpParts.size() ? helpParts.at(i).trimmed() : QString();
        const QString answer = i < answerParts.size() ? answerParts.at(i).trimmed() : QString();
        vars->insert(
            QStringLiteral("{{HELP%1}}").arg(i),
            QStringLiteral("<div contenteditable='true'>%1</div>")
                .arg(help.isEmpty() ? QStringLiteral("&nbsp;") : help.toHtmlEscaped()));
        vars->insert(
            QStringLiteral("{{ANSWER%1}}").arg(i),
            QStringLiteral("<div contenteditable='true'>%1</div>")
                .arg(answer.isEmpty() ? QStringLiteral("&nbsp;") : answer.toHtmlEscaped()));
    }
}

// 1.19: подпись разреза в первой колонке (как createP в protocols.cs).
QString protocolStepDisplayLabel(const QString &exerciseId, const QString &stepId) {
    if (exerciseId == QStringLiteral("1.19")) {
        static const QMap<QString, QString> kLabels = {
            {QStringLiteral("Матрешка 2"), QStringLiteral("Матрешка/2 по горизонт.")},
            {QStringLiteral("Мишка 4"), QStringLiteral("Мишка/4 по горизонт. и верт.")},
            {QStringLiteral("Леопард 3"), QStringLiteral("Леопард/3 по верт.")},
            {QStringLiteral("Дом 4"), QStringLiteral("Дом /4 по диагон.")},
        };
        return kLabels.value(stepId.trimmed(), stepId.trimmed());
    }
    return stepId.trimmed();
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
    // OR/HLP: formatProtocolCellText уже ставит &nbsp; в пустые ячейки (якорь для клика в QTextEdit).
    vars.insert(QStringLiteral("{{OR}}"), formatProtocolCellText(checkboxes.activity));
    vars.insert(QStringLiteral("{{HLP}}"), formatProtocolCellText(checkboxes.help));
    vars.insert(QStringLiteral("{{TIME}}"), formatProtocolTime(elapsedSeconds).toHtmlEscaped());

    QString stepId = session.stepId.trimmed().isEmpty() ? QStringLiteral("1") : session.stepId;
    QString doneState = session.doneState.trimmed().isEmpty()
        ? QStringLiteral("не определено")
        : session.doneState;
    bool additionalIsStepDone = false;
    if (!session.additional.isEmpty() && session.additional.contains(QLatin1Char(';'))
        && tmpl.id != QStringLiteral("3.1.21")
        && tmpl.id != QStringLiteral("1.26")
        && tmpl.id != QStringLiteral("5.2.1")) {
        const QStringList parts = session.additional.split(QLatin1Char(';'));
        if (parts.size() >= 2) {
            stepId = parts.at(0);
            doneState = parts.at(1);
            additionalIsStepDone = true;
        }
    }
    vars.insert(QStringLiteral("{{STEP}}"), stepId.toHtmlEscaped());
    vars.insert(QStringLiteral("{{DONE}}"), doneState.toHtmlEscaped());
    // NumberedDoneTime (1.19 и др.): в ADDITIONAL только имя задания, факт — в DONE.
    // Иначе в ячейку попадало «Матрешка 2;выполнено» и повтор не считался новой сессией.
    if (additionalIsStepDone) {
        vars.insert(
            QStringLiteral("{{ADDITIONAL}}"),
            protocolStepDisplayLabel(tmpl.id, stepId).toHtmlEscaped());
    } else {
        vars.insert(QStringLiteral("{{ADDITIONAL}}"), session.additional.toHtmlEscaped());
    }

    double score = 0;
    bool scoreSelected = true;
    bool scoreIsFractional = false;
    const bool manualBalls = tmpl.id == QStringLiteral("4.1.4")
        || tmpl.scoreKind == QStringLiteral("manual_balls")
        || tmpl.scoreKind == QStringLiteral("balls_manual");
    if (manualBalls) {
        // Баллы вручную (время + число найденных картинок) — не автозаполнять.
        scoreSelected = false;
    } else if (tmpl.scoreKind == QStringLiteral("timed414_result")) {
        score = scoreExercise414(elapsedSeconds);
    } else if (tmpl.scoreKind == QStringLiteral("timed15_result") || tmpl.id == QStringLiteral("1.5")) {
        // Стоп / таймаут 70 с → 0; успех по «Готово» → шкала по времени.
        const QString done = session.doneState.trimmed();
        const bool success = done.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0
            || done.compare(QStringLiteral("выполнено"), Qt::CaseInsensitive) == 0;
        score = success ? scoreExercise15(elapsedSeconds) : 0;
    } else if (tmpl.scoreKind == QStringLiteral("timed315_result") || tmpl.id == QStringLiteral("3.1.15")) {
        score = scoreExercise315(elapsedSeconds);
    } else if (tmpl.scoreKind == QStringLiteral("series15_activity")
               || tmpl.scoreKind == QStringLiteral("timed15_help")) {
        // Устаревшая автооценка 1.15 по уровню/помощи — в руководстве и C# ячейка
        // ids оставлялась пустой (только вручную). См. manual_balls для 1.15.
        const double series = scoreExercise115Series(checkboxes.activity, checkboxes.help);
        if (series < 0) {
            scoreSelected = false;
            score = 0;
        } else {
            score = series;
            scoreIsFractional = true;
        }
    } else if (tmpl.id == QStringLiteral("1.15")) {
        // Как protocols.cs: <div id='ids…'> </div> без автоподстановки.
        scoreSelected = false;
        score = 0;
    } else if (tmpl.id == QStringLiteral("2.12")) {
        // Как protocols.cs idd3: III уровень → 3 (не activity_help_3, где III → 2).
        score = activityLevelScore212(checkboxes.activity);
        score -= 0.5 * helpPenaltyHalfPoints(checkboxes.help);
        if (score < 0) {
            score = 0;
        }
        scoreIsFractional = true;
    } else if (tmpl.scoreKind == QStringLiteral("activity_help_3")
               || tmpl.id == QStringLiteral("1.12")
               || tmpl.id == QStringLiteral("2.11")) {
        score = activityLevelScoreMax3(checkboxes.activity);
        score -= 0.5 * helpPenaltyHalfPoints(checkboxes.help);
        if (score < 0) {
            score = 0;
        }
        scoreIsFractional = true;
    } else if (tmpl.id == QStringLiteral("4.1.2") || tmpl.scoreKind == QStringLiteral("timed11_result")) {
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
        // Qt-панель «Оценка результатов» (5.4.2): балл из выбранного пункта.
        const int fromPanel = wolfScoreFromActivityText(checkboxes.activity);
        if (fromPanel >= 0) {
            score = fromPanel;
        } else {
            scoreSelected = false;
            score = 0;
        }
    } else if (tmpl.scoreKind == QStringLiteral("activity_help_2") || tmpl.id == QStringLiteral("3.1.10")) {
        // Как в protocols.cs 3.1.10: idd3 → 2, каждый вид помощи −0.5.
        if (checkboxes.activity.contains(QStringLiteral("Целенаправленное"), Qt::CaseInsensitive)
            || checkboxes.activity.contains(QStringLiteral("III уровень"), Qt::CaseInsensitive)
            || checkboxes.activity.contains(QStringLiteral("2 балла"), Qt::CaseInsensitive)) {
            score = 2.0;
        }
        score -= 0.5 * helpPenaltyHalfPoints(checkboxes.help);
        if (score < 0) {
            score = 0;
        }
        scoreIsFractional = true;
    }
    if (manualBalls || !scoreSelected) {
        vars.insert(QStringLiteral("{{SCORE}}"), QStringLiteral("&nbsp;"));
        vars.insert(QStringLiteral("{{LEVEL}}"), QString());
    } else if (scoreIsFractional
               || tmpl.scoreKind == QStringLiteral("activity_help_2")
               || tmpl.scoreKind == QStringLiteral("activity_help_3")
               || tmpl.scoreKind == QStringLiteral("timed15_help")
               || tmpl.scoreKind == QStringLiteral("series15_activity")
               || tmpl.id == QStringLiteral("3.1.10")
               || tmpl.id == QStringLiteral("1.12")
               || tmpl.id == QStringLiteral("2.11")
               || tmpl.id == QStringLiteral("2.12")) {
        if (qFuzzyIsNull(score - std::floor(score))) {
            vars.insert(QStringLiteral("{{SCORE}}"), QString::number(static_cast<int>(score)));
        } else {
            vars.insert(QStringLiteral("{{SCORE}}"), QString::number(score, 'f', 1));
        }
        vars.insert(QStringLiteral("{{LEVEL}}"), developmentLevel(static_cast<int>(score)).toHtmlEscaped());
    } else {
        vars.insert(QStringLiteral("{{SCORE}}"), QString::number(static_cast<int>(score)));
        vars.insert(QStringLiteral("{{LEVEL}}"), developmentLevel(static_cast<int>(score)).toHtmlEscaped());
    }

    if (!session.capturedImagePath.isEmpty()) {
        vars.insert(QStringLiteral("{{SCAN}}"), QString());
    } else {
        vars.insert(QStringLiteral("{{SCAN}}"), QString());
    }
    vars.insert(QStringLiteral("{{SCAN_SLOTS}}"), QString());

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
        vars.insert(
            QStringLiteral("{{ADDITIONAL}}"),
            protocolStepDisplayLabel(tmpl.id, stepId).toHtmlEscaped());
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
QString orHlpBallsScoreIdPrefix(const QString &exerciseId) {
    if (exerciseId == QStringLiteral("1.14")) {
        return QStringLiteral("idb");
    }
    return QStringLiteral("ids");
}

QString orHlpBallsScoreId(const QString &exerciseId, const QString &stepId) {
    const QString step = stepId.trimmed();
    if (exerciseId == QStringLiteral("1.20")) {
        static const QMap<QString, QString> kMap = {
            {QStringLiteral("Мяч 2"), QStringLiteral("1")},
            {QStringLiteral("Дом 3"), QStringLiteral("2")},
            {QStringLiteral("Мишка 4"), QStringLiteral("3")},
            {QStringLiteral("Машинка 5"), QStringLiteral("4")},
            {QStringLiteral("Чайник 6"), QStringLiteral("5")},
        };
        return kMap.value(step, step);
    }
    if (exerciseId == QStringLiteral("1.21")) {
        static const QMap<QString, QString> kMap = {
            {QStringLiteral("2А"), QStringLiteral("1")},
            {QStringLiteral("2Б"), QStringLiteral("1")},
            {QStringLiteral("3А"), QStringLiteral("2")},
            {QStringLiteral("3Б"), QStringLiteral("3")},
            {QStringLiteral("4А"), QStringLiteral("4")},
            {QStringLiteral("4Б"), QStringLiteral("5")},
            {QStringLiteral("5А"), QStringLiteral("6")},
            {QStringLiteral("5Б"), QStringLiteral("7")},
            {QStringLiteral("6А"), QStringLiteral("8")},
            {QStringLiteral("6Б"), QStringLiteral("9")},
        };
        return kMap.value(step, step);
    }
    return step;
}

QString orHlpBallsStepLabel(const QString &exerciseId, const QString &stepId) {
    const QString step = stepId.trimmed();
    if (exerciseId == QStringLiteral("1.20")) {
        static const QMap<QString, QString> kMap = {
            {QStringLiteral("Мяч 2"), QStringLiteral("Мяч из 2 частей")},
            {QStringLiteral("Дом 3"), QStringLiteral("Домик из 3 частей")},
            {QStringLiteral("Мишка 4"), QStringLiteral("Мишка из 4 частей")},
            {QStringLiteral("Машинка 5"), QStringLiteral("Машинка из 5 частей")},
            {QStringLiteral("Чайник 6"), QStringLiteral("Чайник из 6 частей")},
        };
        return kMap.value(step, step);
    }
    return step;
}

QString stepLabel1272(const QString &stepId) {
    static const QMap<QString, QString> kEmotions = {
        {QStringLiteral("1"), QStringLiteral("Грусть")},
        {QStringLiteral("2"), QStringLiteral("Страх")},
        {QStringLiteral("3"), QStringLiteral("Удивление")},
        {QStringLiteral("4"), QStringLiteral("Злость")},
        {QStringLiteral("5"), QStringLiteral("Радость")},
        {QStringLiteral("6"), QStringLiteral("Спокойствие")},
    };
    const QString emotion = kEmotions.value(stepId.trimmed());
    if (emotion.isEmpty()) {
        return stepId;
    }
    return stepId.trimmed() + QStringLiteral(". ") + emotion;
}

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
        const QString scoreId = orHlpBallsScoreId(tmpl.id, stepId);
        vars.insert(QStringLiteral("{{STEP}}"), stepId.toHtmlEscaped());
        vars.insert(QStringLiteral("{{ADDITIONAL}}"), stepId.toHtmlEscaped());
        vars.insert(QStringLiteral("{{SCORE_ID}}"), scoreId.toHtmlEscaped());
        if (tmpl.id == QStringLiteral("1.272")) {
            vars.insert(QStringLiteral("{{STEP_LABEL}}"), stepLabel1272(stepId).toHtmlEscaped());
        } else {
            vars.insert(
                QStringLiteral("{{STEP_LABEL}}"),
                orHlpBallsStepLabel(tmpl.id, stepId).toHtmlEscaped());
        }
        rows += ensureRowWrapped(substituteAll(rowTpl, vars));
    }
    if (!tmpl.summaryRow.isEmpty()) {
        rows += ensureRowWrapped(substituteAll(tmpl.summaryRow, baseVars));
    }
    return rows;
}

// 5.2.1 «Расскажи по картинке»: на каждое задание — таблица фрагментов речи + OR/HLP/Баллы.
QString buildSpeechTasksOrHlpBlocks(
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

    QString blocks;
    for (const QString &stepId : stepIds) {
        QString additional = session.additionalByStep.value(stepId);
        if (additional.trimmed().isEmpty()) {
            // Один текущий шаг — берём session.additional целиком (уже с № задания).
            if (stepIds.size() == 1
                || stepId == session.stepId
                || session.additional.startsWith(stepId + QLatin1Char(';'))) {
                additional = session.additional;
            } else {
                additional = stepId + QLatin1Char(';');
            }
        }
        if (!additional.startsWith(stepId + QLatin1Char(';'))
            && additional.split(QLatin1Char(';')).value(0).trimmed() != stepId) {
            additional = stepId + QLatin1Char(';') + additional;
        }

        QMap<QString, QString> vars = baseVars;
        vars.insert(QStringLiteral("{{STEP}}"), stepId.toHtmlEscaped());
        vars.insert(QStringLiteral("{{ADDITIONAL}}"), additional.toHtmlEscaped());
        const QStringList tmpParts = additional.split(QLatin1Char(';'));
        for (int i = 0; i < 11; ++i) {
            vars.insert(
                QStringLiteral("{{TMP%1}}").arg(i),
                i < tmpParts.size() ? tmpParts.at(i).toHtmlEscaped() : QString());
        }
        QString block = substituteAll(rowTpl, vars);
        block = substituteTmpIndices(block, additional);
        blocks += block;
    }
    return blocks;
}

bool speechTaskPresentInHtml(const QString &html, const QString &stepId) {
    const QString sid = stepId.trimmed();
    if (sid.isEmpty() || html.isEmpty()) {
        return false;
    }
    // protocols.cs: "Задание №"+tmp[0]
    return html.contains(QStringLiteral("Задание №") + sid, Qt::CaseInsensitive)
        || html.contains(QStringLiteral("Задание № ") + sid, Qt::CaseInsensitive);
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
    } else if (tmpl.kind == QStringLiteral("speech_tasks_or_hlp")) {
        row = buildSpeechTasksOrHlpBlocks(tmpl, vars, session);
    } else if (tmpl.kind == QStringLiteral("or_hlp_balls")
               || tmpl.kind == QStringLiteral("or_hlp_balls_row")) {
        row = buildOrHlpBallsProcessRows(tmpl, vars, session);
    } else {
        row = buildRow(tmpl, vars, answers, checkboxes, session);
    }

    if (partly) {
        // 5.2.1: дописываем таблицу «Задание №N» в конец текущего протокола (как getLastP+таблицы).
        if (tmpl.kind == QStringLiteral("speech_tasks_or_hlp")) {
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
            const QString lastSessionHtml =
                ExerciseProtocol::extractLastProtocol126Session(existingProtocolHtml);
            const QString scopeHtml = lastSessionHtml.trimmed().isEmpty()
                ? existingProtocolHtml
                : lastSessionHtml;
            QStringList newSteps;
            for (const QString &sid : stepIds) {
                if (!speechTaskPresentInHtml(scopeHtml, sid)) {
                    newSteps << sid;
                }
            }
            if (newSteps.isEmpty()) {
                // Все задания уже в последней сессии — повторный протокол с новой датой.
                ProtocolSessionInput repeatSession = session;
                repeatSession.stepIds = stepIds;
                const QString repeatBlocks = buildSpeechTasksOrHlpBlocks(tmpl, vars, repeatSession);
                QString sessionBlock;
                if (!tmpl.dateRow.isEmpty()) {
                    sessionBlock += substituteAll(tmpl.dateRow, vars);
                }
                if (!tmpl.initialBlock.isEmpty()) {
                    sessionBlock += substituteAll(tmpl.initialBlock, vars);
                }
                sessionBlock += repeatBlocks;
                return ExerciseProtocol::appendFullSessionToStoredBody(
                    existingProtocolHtml, sessionBlock);
            }
            ProtocolSessionInput appendSession = session;
            appendSession.stepIds = newSteps;
            const QString appendBlocks = buildSpeechTasksOrHlpBlocks(tmpl, vars, appendSession);
            // Не вставлять внутрь последней </table> — блоки это самостоятельные <table>.
            return existingProtocolHtml + appendBlocks;
        }
        // Numbered multi-step (руководство / упр. 6): дописка строк или новая «Дата/специалист».
        if (tmpl.kind == QStringLiteral("numbered")
            && ExerciseConfig::usesAppendOnlyMultiStepLogic(exerciseId)) {
            QString existing = existingProtocolHtml;
            if (exerciseId == QStringLiteral("1.18")) {
                existing = ExerciseProtocol::canonicalizeProtocol118StoredBody(existing);
            }
            QStringList stepIds = session.stepIds;
            if (stepIds.isEmpty() && !session.stepId.trimmed().isEmpty()) {
                stepIds << session.stepId.trimmed();
            }
            if (stepIds.isEmpty()) {
                stepIds << QStringLiteral("1");
            }
            const QString lastSessionHtml =
                ExerciseProtocol::extractLastProtocol126Session(existing);
            const QString scopeHtml = lastSessionHtml.trimmed().isEmpty()
                ? existing
                : lastSessionHtml;
            QStringList newSteps;
            for (const QString &sid : stepIds) {
                if (!ExerciseProtocol::numberedStepPresentInSessionHtml(scopeHtml, sid)) {
                    newSteps << sid;
                }
            }
            if (newSteps.isEmpty()) {
                ProtocolSessionInput repeatSession = session;
                repeatSession.stepIds = stepIds;
                const QString repeatRows =
                    buildNumberedProcessRows(tmpl, vars, repeatSession, elapsedSeconds);
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
                return ExerciseProtocol::appendFullSessionToStoredBody(existing, sessionBlock);
            }
            ProtocolSessionInput appendSession = session;
            appendSession.stepIds = newSteps;
            const QString appendRows =
                buildNumberedProcessRows(tmpl, vars, appendSession, elapsedSeconds);
            return ExerciseProtocol::appendRowsToStoredBody(
                trimTrailingSummaryRow(existing), appendRows);
        }
        // done_time / done_time_scan multi-step: дописка строк задания как у numbered.
        if ((tmpl.kind == QStringLiteral("done_time")
             || tmpl.kind == QStringLiteral("done_time_scan"))
            && ExerciseConfig::usesAppendOnlyMultiStepLogic(exerciseId)) {
            QStringList stepIds = session.stepIds;
            if (stepIds.isEmpty() && !session.stepId.trimmed().isEmpty()) {
                stepIds << session.stepId.trimmed();
            }
            if (stepIds.isEmpty()) {
                stepIds << QStringLiteral("1");
            }
            const QString lastSessionHtml =
                ExerciseProtocol::extractLastProtocol126Session(existingProtocolHtml);
            const QString scopeHtml = lastSessionHtml.trimmed().isEmpty()
                ? existingProtocolHtml
                : lastSessionHtml;
            QStringList newSteps;
            for (const QString &sid : stepIds) {
                if (!ExerciseProtocol::numberedStepPresentInSessionHtml(scopeHtml, sid)) {
                    newSteps << sid;
                }
            }
            if (newSteps.isEmpty()) {
                ProtocolSessionInput repeatSession = session;
                repeatSession.stepIds = stepIds;
                const QString repeatRows =
                    buildNumberedProcessRows(tmpl, vars, repeatSession, elapsedSeconds);
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
            const QString appendRows =
                buildNumberedProcessRows(tmpl, vars, appendSession, elapsedSeconds);
            return ExerciseProtocol::appendRowsToStoredBody(
                trimTrailingSummaryRow(existingProtocolHtml), appendRows);
        }
        // 3.1.1 / 3.1.11 — одно задание за проход: при partly всегда новая «Дата/специалист»
        // (не дописывать только строку процесса к предыдущей сессии).
        if (tmpl.kind == QStringLiteral("or_hlp_balls")
            && (exerciseId == QStringLiteral("3.1.1")
                || exerciseId == QStringLiteral("3.1.11"))) {
            QStringList stepIds = session.stepIds;
            if (stepIds.isEmpty()) {
                stepIds << QStringLiteral("1");
            }
            ProtocolSessionInput repeatSession = session;
            repeatSession.stepIds = stepIds;
            const QString repeatRows = buildOrHlpBallsProcessRows(tmpl, vars, repeatSession);
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
            const QString prefix = orHlpBallsScoreIdPrefix(exerciseId);
            // Смотрим только последнюю сессию по «Дата/специалист» — иначе ids/idb
            // из предыдущих повторных протоколов дают ложный «уже есть» и плодят
            // лишние блоки с новой датой вместо дописки строк в текущий.
            const QString lastSessionHtml =
                ExerciseProtocol::extractLastProtocol126Session(existingProtocolHtml);
            const QString scopeHtml = lastSessionHtml.trimmed().isEmpty()
                ? existingProtocolHtml
                : lastSessionHtml;
            QStringList newSteps;
            for (const QString &sid : stepIds) {
                const QString idToken = prefix + orHlpBallsScoreId(exerciseId, sid);
                const bool present =
                    scopeHtml.contains(
                        QStringLiteral("id='%1'").arg(idToken), Qt::CaseInsensitive)
                    || scopeHtml.contains(
                        QStringLiteral("id=\"%1\"").arg(idToken), Qt::CaseInsensitive);
                if (!present) {
                    newSteps << sid;
                }
            }
            // Все шаги уже в последней сессии — повторный протокол с новой «Дата/специалист».
            if (newSteps.isEmpty()) {
                ProtocolSessionInput repeatSession = session;
                repeatSession.stepIds = stepIds;
                const QString repeatRows = buildOrHlpBallsProcessRows(tmpl, vars, repeatSession);
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
            const QString appendRows = buildOrHlpBallsProcessRows(tmpl, vars, appendSession);
            return ExerciseProtocol::appendRowsToStoredBody(
                trimTrailingSummaryRow(existingProtocolHtml), appendRows);
        }
        // done_time / additional_time: повторный протокол — всегда новая сессия
        // со строки «Дата/специалист» (ТЗ 14.2). Дописка только process-row без даты
        // ломала повторные визиты (1.6, 1.13, 4.1.1, …). Multi-step continue
        // для numbered идёт через kind=numbered → appendFullSession ниже.
        // Падение в общий блок appendFullSession.
        // 1.26/tmp0_variants уже обработаны выше.
        // Повторный протокол (ТЗ 14.2): всегда новая сессия со строки «Дата/специалист».
        // Для numbered (1.17/1.18/2.10/…) нельзя дописывать только строки процесса —
        // иначе повторная сессия сливается с предыдущей без новой даты.
        QString sessionBlock;
        if (!tmpl.dateRow.isEmpty()) {
            sessionBlock += substituteAll(tmpl.dateRow, vars);
        } else {
            // Защита: шаблон без dateRow — всё равно начинаем с даты.
            const QString now = QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"));
            sessionBlock += QStringLiteral(
                                "<tr><td width='200' valign='top'>Дата/специалист</td>"
                                "<td width='471' valign='top'>%1   %2</td></tr>")
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
    if (tmpl.kind != QStringLiteral("speech_tasks_or_hlp")
        && !body.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
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
    return ExerciseProtocol::normalizeSummaryColumnWidths(body);
}
