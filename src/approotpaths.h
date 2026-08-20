#ifndef APPROOTPATHS_H
#define APPROOTPATHS_H

#include <QString>

// Портативные пути рядом с исполняемым файлом (Windows и Linux одинаково):
//   {app}/data/base.db, {app}/data/scans/, {app}/key/license.json
namespace AppRootPaths {
QString applicationRoot();
QString dataDir();
QString scansDir();
QString keyDir();
QString licenseFilePath();
QString databaseFilePath();
// Создаёт data/, data/scans/, key/ рядом с exe. Безопасно вызывать при каждом старте.
bool ensureWritableLayout(QString *errorText = nullptr);
} // namespace AppRootPaths

#endif
