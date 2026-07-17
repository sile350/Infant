#include "exerciseassets.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>

namespace {

QString extractDivInnerHtml(const QString &html, const QString &divId) {
    const QRegularExpression openRe(
        QStringLiteral("<div\\s+id=['\"]%1['\"][^>]*>").arg(QRegularExpression::escape(divId)),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch openMatch = openRe.match(html);
    if (!openMatch.hasMatch()) {
        return {};
    }
    const int contentStart = openMatch.capturedEnd(0);
    const QRegularExpression closeRe(QStringLiteral("</div>"), QRegularExpression::CaseInsensitiveOption);
    int depth = 1;
    int pos = contentStart;
    while (pos < html.size() && depth > 0) {
        const QRegularExpressionMatch nextOpen = openRe.match(html, pos);
        const QRegularExpressionMatch nextClose = closeRe.match(html, pos);
        if (!nextClose.hasMatch()) {
            break;
        }
        const int openPos = nextOpen.hasMatch() ? nextOpen.capturedStart(0) : -1;
        const int closePos = nextClose.capturedStart(0);
        if (openPos >= 0 && openPos < closePos) {
            ++depth;
            pos = nextOpen.capturedEnd(0);
            continue;
        }
        --depth;
        if (depth == 0) {
            return html.mid(contentStart, closePos - contentStart).trimmed();
        }
        pos = nextClose.capturedEnd(0);
    }
    return {};
}

void replaceDivState(QString &html, const QString &divId, bool open, const QString &sourceHtml) {
    const QRegularExpression divRe(
        QStringLiteral("<div\\s+id=['\"]%1['\"][^>]*>.*?</div>")
            .arg(QRegularExpression::escape(divId)),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    if (!open) {
        html.replace(divRe, QString());
        return;
    }

    QString inner = extractDivInnerHtml(sourceHtml, divId);
    if (divId == QStringLiteral("div2")) {
        inner.replace(QRegularExpression(QStringLiteral("^(\\s|&nbsp;)+")), QString());
    }
    const QString style = QStringLiteral("display:block;margin:0;padding:0;line-height:120%;height:auto;overflow:visible");
    html.replace(
        divRe,
        QStringLiteral("<div id='%1' style=\"%2\">%3</div>").arg(divId, style, inner));
}

void compactOrSectionSpacing(QString &html) {
    html.replace(QRegularExpression(QStringLiteral("</div>\\s+<a")), QStringLiteral("</div><br><br><a"));
    html.replace(
        QRegularExpression(QStringLiteral("</a>\\s+<div")),
        QStringLiteral("</a><div"));
    html.replace(
        QRegularExpression(QStringLiteral("</a>\\s+<a")),
        QStringLiteral("</a><br><br><a"));
    html.replace(QRegularExpression(QStringLiteral("(<br\\s*/?>\\s*){3,}")), QStringLiteral("<br><br>"));
    html.replace(QRegularExpression(QStringLiteral("<body[^>]*>"), QRegularExpression::CaseInsensitiveOption), QStringLiteral("<body>"));
}

QString injectEditableTableCells(QString html) {
    html.replace(
        QRegularExpression(
            QStringLiteral("(<td[^>]*>)\\s*<p[^>]*>\\s*(&nbsp;|&#160;|\\s)*\\s*</p>\\s*</td>"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("\\1<div contenteditable=\"true\"></div></td>"));
    html.replace(
        QRegularExpression(
            QStringLiteral("(<td[^>]*>)\\s*(?:<p[^>]*>\\s*<strong>\\s*(&nbsp;|&#160;|\\s)*\\s*</strong>\\s*</p>\\s*)+</td>"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("\\1<div contenteditable=\"true\"></div></td>"));
    return html;
}

} // namespace

QString ExerciseAssets::exerciseDir(const QString &exerciseId) {
    if (exerciseId.isEmpty()) {
        return {};
    }
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/assets/ex"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../assets/ex"),
        QDir::currentPath() + QStringLiteral("/assets/ex"),
        QDir::currentPath() + QStringLiteral("/../old_project/serv9 2025/WindowsFormsApp1/bin/maindata/ex"),
        QDir::currentPath() + QStringLiteral("/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/ex"),
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(exerciseId);
        if (QDir(candidate).exists()) {
            return QDir::fromNativeSeparators(candidate);
        }
    }
    return {};
}

QString ExerciseAssets::exerciseFile(const QString &exerciseId, const QString &fileName) {
    const QString dir = exerciseDir(exerciseId);
    if (dir.isEmpty() || fileName.trimmed().isEmpty()) {
        return {};
    }
    const QDir folder(dir);
    const QFileInfo want(fileName);
    const QString base = want.completeBaseName();
    const QString ext = want.suffix();

    // Всегда ищем реальное имя в каталоге (3.PNG при запросе 3.png) — так QPixmap
    // получает путь с фактическим регистром, что надёжнее на разных ФС.
    const QStringList entries = folder.entryList(QDir::Files);
    for (const QString &entry : entries) {
        const QFileInfo info(entry);
        if (info.completeBaseName().compare(base, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (!ext.isEmpty() && info.suffix().compare(ext, Qt::CaseInsensitive) != 0) {
            continue;
        }
        return QDir::fromNativeSeparators(folder.filePath(entry));
    }

    const QString direct = folder.filePath(fileName);
    if (QFile::exists(direct)) {
        return QDir::fromNativeSeparators(direct);
    }
    if (!ext.isEmpty()) {
        const QString altExt = (ext == ext.toLower()) ? ext.toUpper() : ext.toLower();
        const QString altPath = folder.filePath(base + QLatin1Char('.') + altExt);
        if (QFile::exists(altPath)) {
            return QDir::fromNativeSeparators(altPath);
        }
    }
    return {};
}

QString ExerciseAssets::sysImage(const QString &fileName) {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/assets/sysImages"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../assets/sysImages"),
        QDir::currentPath() + QStringLiteral("/assets/sysImages"),
        QDir::currentPath() + QStringLiteral("/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/sysImages"),
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(fileName);
        if (QFile::exists(candidate)) {
            return QDir::fromNativeSeparators(candidate);
        }
    }
    return {};
}

QString ExerciseAssets::prepareExerciseHtml(const QString &html, const QString &baseDir) {
    QString result = html;
    result.replace(
        QRegularExpression(QStringLiteral("height:\\s*1px\\s*;\\s*overflow:\\s*hidden"),
                           QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("height:auto;overflow:visible"));
    result.replace(
        QRegularExpression(QStringLiteral("position:\\s*relative\\s*;\\s*height:\\s*1px\\s*;\\s*overflow:\\s*hidden"),
                           QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("position:relative;height:auto;overflow:visible"));
    if (!baseDir.isEmpty()) {
        const QString baseUrl = QUrl::fromLocalFile(baseDir + QLatin1Char('/')).toString();
        if (!result.contains(QStringLiteral("<base"), Qt::CaseInsensitive)) {
            const int headEnd = result.indexOf(QStringLiteral("</head>"), 0, Qt::CaseInsensitive);
            const QString baseTag = QStringLiteral("<base href=\"%1\">").arg(baseUrl);
            if (headEnd >= 0) {
                result.insert(headEnd, baseTag);
            } else {
                result.prepend(baseTag);
            }
        }
    }
    return result;
}

QString ExerciseAssets::prepareOrHtml(
    const QString &html,
    const QString &baseDir,
    bool open1,
    bool open2,
    bool open3) {
    QString result = html;

    result.replace(
        QStringLiteral("id='method' style='font-size:16px;color:#000000' href='###'"),
        QStringLiteral("id='method' style='font-size:16px;color:#000000' href='#method'"));
    result.replace(
        QStringLiteral("<a id='procedure' style='font-size:16px; color:#000000' href='###'>"),
        QStringLiteral("<a id='procedure' style='font-size:16px; color:#000000' href='#procedure'>"));
    result.replace(
        QStringLiteral("id='analis' style='font-size:16px;color:#000000' href='###'"),
        QStringLiteral("id='analis' style='font-size:16px;color:#000000' href='#analis'"));

    const int hrPos = result.indexOf(QStringLiteral("<hr"), 0, Qt::CaseInsensitive);
    if (hrPos > 0) {
        const int bodyEnd = result.indexOf(QStringLiteral("</body>"), hrPos, Qt::CaseInsensitive);
        if (bodyEnd > hrPos) {
            result = result.left(hrPos).trimmed() + QStringLiteral("\n</body></html>");
        }
    }

    const QString sourceHtml = result;
    replaceDivState(result, QStringLiteral("div1"), open1, sourceHtml);
    replaceDivState(result, QStringLiteral("div2"), open2, sourceHtml);
    replaceDivState(result, QStringLiteral("div3"), open3, sourceHtml);
    compactOrSectionSpacing(result);

    result.replace(
        QRegularExpression(
            QStringLiteral("(<strong>Источник\\s+описания:\\s*</strong>)\\s*<br\\s*/?>\\s*"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("\\1 "));

    const int bodyOpen = result.indexOf(QStringLiteral("<body"), 0, Qt::CaseInsensitive);
    const int bodyContentStart = bodyOpen >= 0 ? result.indexOf(QLatin1Char('>'), bodyOpen) + 1 : -1;
    const int bodyClose = result.indexOf(QStringLiteral("</body>"), 0, Qt::CaseInsensitive);
    if (bodyContentStart > 0 && bodyClose > bodyContentStart) {
        QString inner = result.mid(bodyContentStart, bodyClose - bodyContentStart).trimmed();
        inner = QStringLiteral("<br><br>") + inner;
        inner = QStringLiteral("<div class=\"or-strip\">%1</div>").arg(inner);
        result = result.left(bodyContentStart) + inner + result.mid(bodyClose);
    }

    const QString style = QStringLiteral(
        "<style>"
        "body { background-color:#ffffff; color:#000000; margin:0; padding:0; font-family:'Microsoft Sans Serif',sans-serif; font-size:14px; }"
        ".or-strip { background-color:#f8f8f8; margin:0; padding:16px 0 4px 0; }"
        "a { color:#000000; text-decoration:underline; display:block; background-color:#f8f8f8; text-align:left; margin:0 0 12px 0; padding:2px 0; line-height:130%; white-space:nowrap; }"
        "#analis { white-space:normal; }"
        "a:last-of-type { margin-bottom:0; }"
        "a:hover { text-decoration:underline; }"
        "div,ul,li,p,br { margin:0; padding:0; text-align:left; }"
        "</style>");
    const int headEnd = result.indexOf(QStringLiteral("</head>"), 0, Qt::CaseInsensitive);
    if (headEnd >= 0) {
        result.insert(headEnd, style);
    } else {
        result.prepend(style);
    }

    if (!baseDir.isEmpty()) {
        const QString baseUrl = QUrl::fromLocalFile(baseDir + QLatin1Char('/')).toString();
        if (!result.contains(QStringLiteral("<base"), Qt::CaseInsensitive)) {
            const int insertAt = result.indexOf(QStringLiteral("</head>"), 0, Qt::CaseInsensitive);
            const QString baseTag = QStringLiteral("<base href=\"%1\">").arg(baseUrl);
            if (insertAt >= 0) {
                result.insert(insertAt, baseTag);
            }
        }
    }
    return result;
}

QString ExerciseAssets::protocolTableStyleHtml() {
    return QStringLiteral(
        "<style>"
        "body { background-color:#ffffff; color:#000000; margin:0; padding:0; "
        "font-family:'Times New Roman',serif; font-size:11pt; }"
        ".protocol-export-header { font-size:18pt; line-height:1.35; text-align:center; }"
        "table { table-layout:fixed; width:671px; max-width:671px; border-collapse:collapse; "
        "border:1px solid #000000; background-color:#ffffff; box-sizing:border-box; }"
        "td,th { border:1px solid #000000; overflow:hidden; word-wrap:break-word; background-color:#ffffff; "
        "box-sizing:border-box; }"
        "div[contenteditable='true'] { min-height:1.2em; }"
        ".protocol-page-break { page-break-before:always; break-before:page; height:0; margin:24px 0; }"
        "@media print { .protocol-page-break { page-break-before:always; break-before:page; } }"
        "</style>");
}

QString ExerciseAssets::wrapProtocolDocumentHtml(const QString &html) {
    QString result = html;
    result.replace(QStringLiteral("width='186'"), QStringLiteral("width='190'"));
    result.replace(QStringLiteral("width=\"186\""), QStringLiteral("width=\"190\""));
    result.replace(QStringLiteral("width='500'"), QStringLiteral("width='506'"));
    result.replace(QStringLiteral("width=\"500\""), QStringLiteral("width=\"506\""));
    result.replace(QStringLiteral("width='184'"), QStringLiteral("width='194'"));
    result.replace(QStringLiteral("width=\"184\""), QStringLiteral("width=\"194\""));
    result.replace(QStringLiteral("width='230'"), QStringLiteral("width='229'"));
    result.replace(QStringLiteral("width=\"230\""), QStringLiteral("width=\"229\""));
    result.replace(QStringLiteral("width='162'"), QStringLiteral("width='160'"));
    result.replace(QStringLiteral("width=\"162\""), QStringLiteral("width=\"160\""));
    result.replace(QStringLiteral("width='722'"), QStringLiteral("width='671'"));
    result.replace(QStringLiteral("width=\"722\""), QStringLiteral("width=\"671\""));
    if (!result.contains(QStringLiteral("table-layout:fixed"), Qt::CaseInsensitive)) {
        const int headEnd = result.indexOf(QStringLiteral("</head>"), 0, Qt::CaseInsensitive);
        if (headEnd >= 0) {
            result.insert(headEnd, protocolTableStyleHtml());
        }
    }
    return injectEditableTableCells(result);
}

QString ExerciseAssets::buildProtocolDocumentHtml(const QString &bodyFragment) {
    return wrapProtocolDocumentHtml(QStringLiteral(
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\">%1</head>"
        "<body style=\"margin:0;padding:0;background-color:#ffffff;color:#000000;\">%2</body></html>")
        .arg(protocolTableStyleHtml(), bodyFragment));
}

QString ExerciseAssets::prepareTemplateHtml(const QString &html, const QString &baseDir) {
    QString result = html;
    const QString style = protocolTableStyleHtml();
    const int headEnd = result.indexOf(QStringLiteral("</head>"), 0, Qt::CaseInsensitive);
    if (headEnd >= 0) {
        result.insert(headEnd, style);
    } else {
        result.prepend(style);
    }
    if (!baseDir.isEmpty()) {
        const QString baseUrl = QUrl::fromLocalFile(baseDir + QLatin1Char('/')).toString();
        if (!result.contains(QStringLiteral("<base"), Qt::CaseInsensitive)) {
            const int headEnd = result.indexOf(QStringLiteral("</head>"), 0, Qt::CaseInsensitive);
            const QString baseTag = QStringLiteral("<base href=\"%1\">").arg(baseUrl);
            if (headEnd >= 0) {
                result.insert(headEnd, baseTag);
            } else {
                result.prepend(baseTag);
            }
        }
    }
    return injectEditableTableCells(result);
}
