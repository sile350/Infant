#include "licenseservice.h"
#include "custommessagebox.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QSysInfo>
#include <QProcess>
#include <QTextStream>

namespace {

QString lastFourChars(const QString &value) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("0000");
    }
    return trimmed.length() >= 4 ? trimmed.right(4) : trimmed.rightJustified(4, QChar('0'));
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
    const QString diskModel = wmicPropertyValue("diskdrive", "Model");
    const QString fingerprint = lastFourChars(processorId) + lastFourChars(boardSerial) + lastFourChars(diskModel);
    return fingerprint.trimmed().isEmpty() ? QString{} : fingerprint;
}
#endif

} // namespace

LicenseService::LicenseService(Repository *repository, QObject *parent)
    : QObject(parent), m_repository(repository) {}

bool LicenseService::ensureActivated(QWidget *parent) {
    if (!ApiClient::supportsHttps()) {
        CustomMessageBox::showError(parent, ApiClient::userFacingNetworkError(QStringLiteral("TLS initialization failed")));
        return false;
    }

    m_key = loadSavedKey();
    const QString fingerprint = hardwareFingerprint();
    if (fingerprint.isEmpty()) {
        CustomMessageBox::showError(parent, "Не удалось получить идентификатор оборудования.");
        return false;
    }

    if (m_key.isEmpty()) {
        bool ok = false;
        const QString enteredKey = QInputDialog::getText(
            parent,
            "Активация лицензии",
            "Введите лицензионный ключ:",
            QLineEdit::Normal,
            "",
            &ok
        ).trimmed();
        if (!ok || enteredKey.isEmpty()) {
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
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(root);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("license.json");
}

QString LicenseService::loadSavedKey() const {
    QFile file(localLicensePath());
    if (!file.exists()) {
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        return {};
    }
    return doc.object().value("key").toString().trimmed();
}

bool LicenseService::saveKey(const QString &key) {
    QFile file(localLicensePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    QJsonObject obj;
    obj.insert("key", key);
    obj.insert("updated_at", QString::number(QDateTime::currentSecsSinceEpoch()));
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    file.close();
    return true;
}

QString LicenseService::hardwareFingerprint() const {
#ifdef Q_OS_WIN
    return windowsHardwareFingerprint();
#else
    QString machineId;
    {
        QFile machineFile("/etc/machine-id");
        if (machineFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            machineId = QString::fromUtf8(machineFile.readAll()).trimmed();
            machineFile.close();
        }
    }

    QString cpuId;
    {
        QFile cpuInfo("/proc/cpuinfo");
        if (cpuInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&cpuInfo);
            while (!stream.atEnd()) {
                const QString line = stream.readLine();
                if (line.startsWith("model name")) {
                    const QStringList parts = line.split(':');
                    if (parts.size() > 1) {
                        cpuId = parts.at(1).trimmed();
                        break;
                    }
                }
            }
            cpuInfo.close();
        }
    }

    const QString source = machineId + "|" + cpuId;
    if (source.trimmed().isEmpty()) {
        return {};
    }
    return QString::fromLatin1(QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha256).toHex().left(12)).toUpper();
#endif
}
