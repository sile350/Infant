#include "licenseservice.h"
#include "custommessagebox.h"
#include "fieldcrypto.h"
#include "licenseactivationdialog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

namespace {

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
QString wmicPropertyValue(const char *wmicClass, const char *property) {
    QProcess process;
    process.start(
        QStringLiteral("wmic"),
        {QString::fromLatin1(wmicClass), QStringLiteral("get"), QString::fromLatin1(property), QStringLiteral("/value")}
    );
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

QString windowsHardwareFingerprint() {
    const QString processorId = wmicPropertyValue("cpu", "ProcessorId");
    QString boardSerial = wmicPropertyValue("baseboard", "SerialNumber");
    if (boardSerial == QStringLiteral("N/A") || boardSerial == QStringLiteral("NA")) {
        boardSerial = QStringLiteral("0000000000");
    }
    const QString fingerprint = lastFourChars(processorId) + lastFourChars(boardSerial);
    return fingerprint.trimmed().isEmpty() ? QString{} : fingerprint;
}
#else
QString readSysfsTextFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

QString normalizeBoardSerial(QString value) {
    value = value.trimmed();
    if (value.isEmpty()
        || value.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("NA"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("None"), Qt::CaseInsensitive) == 0
        || value.contains(QStringLiteral("To Be Filled"), Qt::CaseInsensitive)) {
        return QStringLiteral("0000000000");
    }
    return value;
}

QString linuxProcessorHardwareId() {
    QProcess process;
    process.start(QStringLiteral("dmidecode"), {QStringLiteral("-t"), QStringLiteral("4")});
    if (process.waitForFinished(5000) && process.exitStatus() == QProcess::NormalExit) {
        const QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString processorId;
        QString processorSerial;
        for (const QString &line : output.split('\n')) {
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith(QStringLiteral("Serial Number:"))) {
                processorSerial = trimmed.mid(14).trimmed();
            } else if (trimmed.startsWith(QStringLiteral("ID:"))) {
                processorId = trimmed.mid(3).trimmed();
                processorId.remove(QLatin1Char(' '));
            }
        }
        if (!processorSerial.isEmpty()
            && !processorSerial.contains(QStringLiteral("Not Specified"), Qt::CaseInsensitive)
            && processorSerial.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) != 0) {
            return processorSerial;
        }
        if (!processorId.isEmpty()) {
            return processorId;
        }
    }

    QFile cpuInfo(QStringLiteral("/proc/cpuinfo"));
    if (cpuInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&cpuInfo);
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            if (line.startsWith(QStringLiteral("Serial"))) {
                const QStringList parts = line.split(':');
                if (parts.size() > 1) {
                    const QString serial = parts.at(1).trimmed();
                    if (!serial.isEmpty()) {
                        return serial;
                    }
                }
            }
        }
    }
    return {};
}

QString linuxHardwareFingerprint() {
    const QString processorId = linuxProcessorHardwareId();
    const QString boardSerial = normalizeBoardSerial(
        readSysfsTextFile(QStringLiteral("/sys/class/dmi/id/board_serial")));

    if (processorId.isEmpty() && boardSerial == QStringLiteral("0000000000")) {
        return {};
    }

    const QString fingerprint = lastFourChars(processorId) + lastFourChars(boardSerial);
    return fingerprint.trimmed().isEmpty() ? QString{} : fingerprint;
}
#endif

QString readKeyFromFile(const QString &path) {
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray raw = file.readAll().trimmed();
    file.close();
    if (raw.isEmpty()) {
        return {};
    }

    const QString asText = QString::fromUtf8(raw);
    if (asText.startsWith(QStringLiteral("~lic~"))) {
        const QString decrypted = FieldCrypto::decryptLicenseBlob(asText);
        const QJsonDocument doc = QJsonDocument::fromJson(decrypted.toUtf8());
        if (doc.isObject()) {
            return doc.object().value(QStringLiteral("key")).toString().trimmed();
        }
        return {};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (doc.isObject()) {
        return doc.object().value(QStringLiteral("key")).toString().trimmed();
    }
    return {};
}

} // namespace

LicenseService::LicenseService(Repository *repository, QObject *parent)
    : QObject(parent), m_repository(repository) {}

bool LicenseService::ensureActivated(QWidget *parent) {
    if (!ApiClient::supportsHttps()) {
        CustomMessageBox::showError(parent, ApiClient::userFacingNetworkError(QStringLiteral("TLS initialization failed")));
        return false;
    }

    m_key = loadSavedKey();
    if (!m_key.isEmpty() && readKeyFromFile(localLicensePath()).isEmpty()) {
        saveKey(m_key);
    }

    const QString fingerprint = hardwareFingerprint();
    if (fingerprint.isEmpty()) {
        CustomMessageBox::showError(parent, "Не удалось получить идентификатор оборудования.");
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
        if (!saveKey(m_key)) {
            CustomMessageBox::showError(parent, "Не удалось сохранить файл лицензии.");
            return false;
        }
    }

    QString errorText;
    if (!m_repository->verifyLicenseKeyForMachine(m_key, fingerprint, &errorText)) {
        CustomMessageBox::showError(parent, errorText);
        return false;
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
    const QString keyDir = QCoreApplication::applicationDirPath() + QStringLiteral("/key");
    QDir dir(keyDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return dir.filePath(QStringLiteral("license.json"));
}

QString LicenseService::loadSavedKey() const {
    QString key = readKeyFromFile(localLicensePath());
    if (!key.isEmpty()) {
        return key;
    }
    return readKeyFromFile(legacyLicensePath());
}

bool LicenseService::saveKey(const QString &key) {
    QJsonObject obj;
    obj.insert(QStringLiteral("key"), key);
    obj.insert(QStringLiteral("updated_at"), QString::number(QDateTime::currentSecsSinceEpoch()));
    const QString encrypted = FieldCrypto::encryptLicenseBlob(
        QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));

    QFile file(localLicensePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(encrypted.toUtf8());
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
