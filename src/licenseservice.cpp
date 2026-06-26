#include "licenseservice.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSysInfo>
#include <QStandardPaths>
#include <QTextStream>

LicenseService::LicenseService(Repository *repository, QObject *parent)
    : QObject(parent), m_repository(repository) {}

bool LicenseService::ensureActivated(QWidget *parent) {
#ifdef DEV_LICENSE_BYPASS
    Q_UNUSED(parent);
    m_key = "DEV_LICENSE_BYPASS";
    return true;
#else
    m_key = loadSavedKey();
    const QString fingerprint = hardwareFingerprint();
    if (fingerprint.isEmpty()) {
        QMessageBox::critical(parent, "Лицензия", "Не удалось получить идентификатор оборудования.");
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
            QMessageBox::critical(parent, "Лицензия", errorText);
            return false;
        }
        m_key = enteredKey;
        if (!saveKey(m_key)) {
            QMessageBox::critical(parent, "Лицензия", "Не удалось сохранить файл лицензии.");
            return false;
        }
    }

    QString errorText;
    if (!m_repository->verifyLicenseKeyForMachine(m_key, fingerprint, &errorText)) {
        QMessageBox::critical(parent, "Лицензия", errorText);
        return false;
    }

    return true;
#endif
}

QString LicenseService::key() const {
    return m_key;
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
    QString source = QString::fromLatin1(QSysInfo::machineUniqueId());
    if (source.trimmed().isEmpty()) {
        source = QSysInfo::machineHostName() + "|" + QSysInfo::prettyProductName();
    }
    if (source.trimmed().isEmpty()) {
        return {};
    }
    return QString::fromLatin1(QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha256).toHex().left(12)).toUpper();
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
