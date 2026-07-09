#include "licenseservice.h"
#include "custommessagebox.h"
#include "fieldcrypto.h"
#include "licenseactivationdialog.h"
#ifndef Q_OS_WIN
#include "smbiosreader.h"
#endif

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

namespace {

QString licenseMismatchMessage() {
    return QStringLiteral("Ключ не активирован для этой машины или срок действия истек.");
}

QString lastFourChars(const QString &value) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("0000");
    }
    return trimmed.length() >= 4 ? trimmed.right(4) : trimmed.rightJustified(4, QChar('0'));
}

QString legacyLicensePath() {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(root).filePath(QStringLiteral("license.json"));
}

#ifdef Q_OS_WIN
QString wmicFirstValue(const char *wmicClass, const char *property) {
    QProcess process;
    process.start(
        QStringLiteral("wmic"),
        {QString::fromLatin1(wmicClass), QStringLiteral("get"), QString::fromLatin1(property), QStringLiteral("/value")});
    if (!process.waitForFinished(10000)) {
        process.kill();
        return {};
    }
    const QStringList lines = QString::fromUtf8(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const int eq = line.indexOf('=');
        if (eq < 0) {
            continue;
        }
        const QString value = line.mid(eq + 1).trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return {};
}

QString powershellFirstDiskModel() {
    QProcess process;
    process.start(
        QStringLiteral("powershell"),
        {QStringLiteral("-NoProfile"),
         QStringLiteral("-Command"),
         QStringLiteral("(Get-CimInstance Win32_DiskDrive | Select-Object -First 1).Model")});
    if (!process.waitForFinished(10000)) {
        process.kill();
        return {};
    }
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

QString windowsHardwareFingerprint() {
    const QString processorId = wmicFirstValue("cpu", "ProcessorId");
    QString boardSerial = wmicFirstValue("baseboard", "SerialNumber");
    if (boardSerial.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0
        || boardSerial.compare(QStringLiteral("NA"), Qt::CaseInsensitive) == 0) {
        boardSerial = QStringLiteral("0000000000");
    }
    QString diskModel = wmicFirstValue("diskdrive", "Model");
    if (diskModel.isEmpty()) {
        diskModel = powershellFirstDiskModel();
    }
    if (processorId.isEmpty() && boardSerial == QStringLiteral("0000000000") && diskModel.isEmpty()) {
        return {};
    }
    return lastFourChars(processorId) + lastFourChars(boardSerial) + lastFourChars(diskModel);
}
#else
QString linuxDiskModel() {
    QFile file(QStringLiteral("/sys/block/sda/device/model"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

QString linuxHardwareFingerprint() {
    SmbiosReader::HardwareIds ids;
    if (!SmbiosReader::readHardwareIds(&ids)) {
        return {};
    }
    const QString diskModel = linuxDiskModel();
    return lastFourChars(ids.processorId) + lastFourChars(ids.boardSerial) + lastFourChars(diskModel);
}
#endif

struct LicenseFileData {
    QString key;
    QString hardware;
};

LicenseFileData readLicenseFile(const QString &path) {
    LicenseFileData data;
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return data;
    }
    const QByteArray raw = file.readAll().trimmed();
    file.close();
    if (raw.isEmpty()) {
        return data;
    }

    const QString asText = QString::fromUtf8(raw);
    QJsonDocument doc;
    if (asText.startsWith(QStringLiteral("~lic~"))) {
        const QString decrypted = FieldCrypto::decryptLicenseBlob(asText);
        doc = QJsonDocument::fromJson(decrypted.toUtf8());
    } else {
        doc = QJsonDocument::fromJson(raw);
    }
    if (!doc.isObject()) {
        return data;
    }
    const QJsonObject obj = doc.object();
    data.key = obj.value(QStringLiteral("key")).toString().trimmed();
    data.hardware = obj.value(QStringLiteral("hardware")).toString().trimmed();
    return data;
}

QString readKeyFromFile(const QString &path) {
    return readLicenseFile(path).key;
}

} // namespace

LicenseService::LicenseService(Repository *repository, QObject *parent)
    : QObject(parent), m_repository(repository) {}

bool LicenseService::ensureActivated(QWidget *parent) {
    if (!ApiClient::supportsHttps()) {
        CustomMessageBox::showError(parent, ApiClient::userFacingNetworkError(QStringLiteral("TLS initialization failed")));
        return false;
    }

    const QString fingerprint = hardwareFingerprint();
    if (fingerprint.isEmpty()) {
        CustomMessageBox::showError(parent, QStringLiteral("Не удалось получить идентификатор оборудования."));
        return false;
    }
    if (isWeakFingerprint(fingerprint)) {
        CustomMessageBox::showError(
            parent,
            QStringLiteral("Не удалось однозначно определить оборудование. Запустите программу от имени администратора."));
        return false;
    }

    const LicenseFileData savedLicense = readLicenseFile(localLicensePath());
    m_key = savedLicense.key;
    if (m_key.isEmpty()) {
        m_key = readKeyFromFile(legacyLicensePath());
    }

    if (!m_key.isEmpty() && savedLicense.key.isEmpty()) {
        QString saveError;
        if (!saveKey(m_key, &saveError, fingerprint)) {
            CustomMessageBox::showError(
                parent,
                saveError.isEmpty()
                    ? QStringLiteral("Не удалось сохранить файл лицензии.")
                    : saveError);
            return false;
        }
    }

    if (!m_key.isEmpty() && !savedLicense.hardware.isEmpty() && savedLicense.hardware != fingerprint) {
        CustomMessageBox::showError(parent, licenseMismatchMessage());
        return false;
    }

    if (m_key.isEmpty()) {
        bool accepted = false;
        const QString enteredKey = LicenseActivationDialog::promptForKey(parent, &accepted);
        if (!accepted || enteredKey.isEmpty()) {
            return false;
        }
        QString errorText;
        if (!m_repository->activateLicenseKey(enteredKey, fingerprint, &errorText)) {
            CustomMessageBox::showError(parent, errorText);
            return false;
        }
        m_key = enteredKey;
        m_freshActivation = true;
        QString saveError;
        if (!saveKey(m_key, &saveError, fingerprint)) {
            CustomMessageBox::showError(
                parent,
                saveError.isEmpty()
                    ? QStringLiteral("Не удалось сохранить файл лицензии.")
                    : saveError);
            return false;
        }
        return true;
    }

    QString errorText;
    if (!m_repository->verifyLicenseKeyForMachine(m_key, fingerprint, &errorText)) {
        CustomMessageBox::showError(parent, errorText);
        return false;
    }

    if (savedLicense.hardware.isEmpty()) {
        QString saveError;
        if (!saveKey(m_key, &saveError, fingerprint)) {
            CustomMessageBox::showError(
                parent,
                saveError.isEmpty()
                    ? QStringLiteral("Не удалось обновить файл лицензии.")
                    : saveError);
            return false;
        }
    }

    return true;
}

QString LicenseService::key() const {
    return m_key;
}

bool LicenseService::freshActivation() const {
    return m_freshActivation;
}

QString LicenseService::localLicensePath() const {
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("key/license.json"));
}

QString LicenseService::loadSavedKey() const {
    const QString key = readLicenseFile(localLicensePath()).key;
    if (!key.isEmpty()) {
        return key;
    }
    return readKeyFromFile(legacyLicensePath());
}

QString LicenseService::loadSavedHardware() const {
    const QString hardware = readLicenseFile(localLicensePath()).hardware;
    if (!hardware.isEmpty()) {
        return hardware;
    }
    return readLicenseFile(legacyLicensePath()).hardware;
}

bool LicenseService::saveKey(const QString &key, QString *errorText, const QString &hardware) {
    const QString path = localLicensePath();
    const QFileInfo info(path);
    const QString dirPath = info.absolutePath();

    if (info.exists() && info.isDir()) {
        if (errorText) {
            *errorText = QStringLiteral("Не удалось сохранить ключ: %1 — это каталог, а не файл.")
                             .arg(path);
        }
        return false;
    }

    if (QFileInfo(dirPath).exists() && !QFileInfo(dirPath).isDir()) {
        if (errorText) {
            *errorText = QStringLiteral("Не удалось сохранить ключ: путь %1 занят файлом.").arg(dirPath);
        }
        return false;
    }

    if (!QDir().mkpath(dirPath)) {
        if (errorText) {
            *errorText = QStringLiteral("Не удалось создать каталог %1.").arg(dirPath);
        }
        return false;
    }

    const QString probePath = QDir(dirPath).filePath(QStringLiteral(".write_test"));
    {
        QFile probe(probePath);
        if (!probe.open(QIODevice::WriteOnly)) {
            if (errorText) {
                *errorText = QStringLiteral(
                    "Нет прав на запись в %1.\n"
                    "Если dist собирался через sudo, выполните:\n"
                    "  sudo chown -R $USER \"%2\"")
                    .arg(dirPath, QCoreApplication::applicationDirPath());
            }
            return false;
        }
    }
    QFile::remove(probePath);

    QJsonObject obj;
    obj.insert(QStringLiteral("key"), key);
    if (!hardware.isEmpty()) {
        obj.insert(QStringLiteral("hardware"), hardware);
    } else {
        const QString existingHardware = loadSavedHardware();
        if (!existingHardware.isEmpty()) {
            obj.insert(QStringLiteral("hardware"), existingHardware);
        }
    }
    obj.insert(QStringLiteral("updated_at"), QString::number(QDateTime::currentSecsSinceEpoch()));
    const QString encrypted = FieldCrypto::encryptLicenseBlob(
        QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorText) {
            *errorText = QStringLiteral("Не удалось сохранить %1: %2")
                             .arg(path, file.errorString());
        }
        return false;
    }
    if (file.write(encrypted.toUtf8()) < 0) {
        if (errorText) {
            *errorText = QStringLiteral("Не удалось записать %1: %2")
                             .arg(path, file.errorString());
        }
        return false;
    }
    file.close();
    return true;
}

QString LicenseService::hardwareFingerprint() const {
#ifdef Q_OS_WIN
    return windowsHardwareFingerprint();
#else
    return linuxHardwareFingerprint();
#endif
}

bool LicenseService::isWeakFingerprint(const QString &fingerprint) {
    if (fingerprint.length() < 8) {
        return true;
    }
    return fingerprint == QStringLiteral("000000000000")
        || fingerprint == QStringLiteral("00000000")
        || fingerprint == QString(12, QChar('0'));
}
