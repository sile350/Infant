#include "approotpaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtGlobal>

namespace AppRootPaths {

QString applicationRoot() {
    return QCoreApplication::applicationDirPath();
}

QString dataDir() {
    return QDir(applicationRoot()).filePath(QStringLiteral("data"));
}

QString scansDir() {
    return QDir(dataDir()).filePath(QStringLiteral("scans"));
}

QString keyDir() {
    return QDir(applicationRoot()).filePath(QStringLiteral("key"));
}

QString licenseFilePath() {
    return QDir(keyDir()).filePath(QStringLiteral("license.json"));
}

QString databaseFilePath() {
    return QDir(dataDir()).filePath(QStringLiteral("base.db"));
}

bool ensureWritableLayout(QString *errorText) {
    const QStringList dirs = {dataDir(), scansDir(), keyDir()};
    for (const QString &dirPath : dirs) {
        if (!QDir().mkpath(dirPath)) {
            if (errorText) {
                *errorText = QStringLiteral("Не удалось создать каталог %1.").arg(dirPath);
            }
            return false;
        }
        const QString probePath = QDir(dirPath).filePath(QStringLiteral(".write_test"));
        QFile probe(probePath);
        if (!probe.open(QIODevice::WriteOnly)) {
            if (errorText) {
#ifdef Q_OS_WIN
                *errorText = QStringLiteral(
                    "Нет прав на запись в %1.\n"
                    "Запустите программу из папки с правами записи "
                    "(не из Program Files без прав администратора) "
                    "или дайте пользователю доступ на запись к каталогу:\n%2")
                    .arg(dirPath, applicationRoot());
#else
                *errorText = QStringLiteral(
                    "Нет прав на запись в %1.\n"
                    "Если dist собирался через sudo, выполните:\n"
                    "  sudo chown -R $USER \"%2\"")
                    .arg(dirPath, applicationRoot());
#endif
            }
            return false;
        }
        probe.close();
        QFile::remove(probePath);
    }
    return true;
}

} // namespace AppRootPaths
