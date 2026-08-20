#include "appsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtGlobal>

namespace {

QString bundledSettingsFilePath() {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/assets/ex/names"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../assets/ex/names"),
        QDir::currentPath() + QStringLiteral("/assets/ex/names"),
        QDir::currentPath() + QStringLiteral("/../old_project/serv9 2025/WindowsFormsApp1/bin/maindata/ex/names"),
    };
    for (const QString &root : roots) {
        if (QFile::exists(root)) {
            return QDir(root).filePath(QStringLiteral("настройки.txt"));
        }
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("assets/ex/names/настройки.txt"));
}

QString portableSettingsFilePath() {
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("data/настройки.txt"));
}

QString appConfigSettingsFilePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/DokitLabInfant");
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("настройки.txt"));
}

QString userSettingsFilePathForWrite() {
#ifdef Q_OS_WIN
    // Портативная Windows-сборка: настройки рядом с exe в data/.
    QDir().mkpath(QFileInfo(portableSettingsFilePath()).absolutePath());
    return portableSettingsFilePath();
#else
    return appConfigSettingsFilePath();
#endif
}

bool parseDualScreenValue(const QString &content) {
    for (const QString &line : content.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("два_экрана="), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("dual_screen="), Qt::CaseInsensitive)) {
            const QString value = trimmed.section(QLatin1Char('='), 1).trimmed();
            return value == QLatin1String("1") || value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
        }
    }
    return false;
}

bool tryReadDualScreen(const QString &path, bool *ok) {
    *ok = false;
    if (!QFile::exists(path)) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    *ok = true;
    return parseDualScreenValue(QString::fromUtf8(file.readAll()));
}

} // namespace

bool AppSettings::dualScreenEnabled() {
    bool ok = false;
    // Windows: сначала portable data/настройки.txt; Linux — AppConfig; затем fallbacks.
#ifdef Q_OS_WIN
    {
        const bool value = tryReadDualScreen(portableSettingsFilePath(), &ok);
        if (ok) {
            return value;
        }
    }
#endif
    {
        const bool value = tryReadDualScreen(appConfigSettingsFilePath(), &ok);
        if (ok) {
            return value;
        }
    }
#ifndef Q_OS_WIN
    {
        const bool value = tryReadDualScreen(portableSettingsFilePath(), &ok);
        if (ok) {
            return value;
        }
    }
#endif

    const QString bundledPath = bundledSettingsFilePath();
    QFile bundledFile(bundledPath);
    if (!bundledFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    return parseDualScreenValue(QString::fromUtf8(bundledFile.readAll()));
}

void AppSettings::setDualScreenEnabled(bool enabled) {
    const QString path = userSettingsFilePathForWrite();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QStringList lines;
    if (QFile::exists(path)) {
        QFile readFile(path);
        if (readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            lines = QString::fromUtf8(readFile.readAll()).split(QLatin1Char('\n'));
        }
    }

    bool found = false;
    const QString newLine = QStringLiteral("два_экрана=%1").arg(enabled ? 1 : 0);
    for (QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("два_экрана="), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("dual_screen="), Qt::CaseInsensitive)) {
            line = newLine;
            found = true;
            break;
        }
    }
    if (!found) {
        lines << newLine;
    }

    QFile writeFile(path);
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }
    writeFile.write(lines.join(QLatin1Char('\n')).toUtf8());
    if (!lines.isEmpty()) {
        writeFile.write("\n");
    }
}
