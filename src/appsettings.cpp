#include "appsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace {

QString settingsFilePath() {
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

} // namespace

bool AppSettings::dualScreenEnabled() {
    const QString path = settingsFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    const QString content = QString::fromUtf8(file.readAll());
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

void AppSettings::setDualScreenEnabled(bool enabled) {
    const QString path = settingsFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }
    file.write(QStringLiteral("два_экрана=%1\n").arg(enabled ? 1 : 0).toUtf8());
}
