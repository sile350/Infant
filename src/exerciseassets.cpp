#include "exerciseassets.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>

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
    if (dir.isEmpty()) {
        return {};
    }
    const QString path = QDir(dir).filePath(fileName);
    return QFile::exists(path) ? QDir::fromNativeSeparators(path) : QString();
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
        QStringLiteral("<br><a id='procedure' style='font-size:16px; color:#000000' href='#procedure'>"));
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

    const auto applyDivState = [&result](const QString &divId, bool open) {
        const QString style = open ? QStringLiteral("position:relative;height:auto;overflow:visible")
                                   : QStringLiteral("position:relative;height:1px;overflow:hidden");
        const QRegularExpression re(
            QStringLiteral("<div\\s+id=['\"]%1['\"][^>]*>").arg(QRegularExpression::escape(divId)),
            QRegularExpression::CaseInsensitiveOption);
        result.replace(re, QStringLiteral("<div id='%1' style=\"%2\">").arg(divId, style));
    };
    applyDivState(QStringLiteral("div1"), open1);
    applyDivState(QStringLiteral("div2"), open2);
    applyDivState(QStringLiteral("div3"), open3);

    const QString style = QStringLiteral(
        "<style>"
        "body { background-color:#f8f8f8; color:#000000; margin:8px; font-family:'Microsoft Sans Serif',sans-serif; font-size:14px; }"
        "a { color:#000000; text-decoration:none; }"
        "a:hover { text-decoration:underline; }"
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

QString ExerciseAssets::prepareTemplateHtml(const QString &html, const QString &baseDir) {
    QString result = html;
    const QString style = QStringLiteral(
        "<style>body { background-color:#f8f8f8; color:#000000; }</style>");
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
    return result;
}
