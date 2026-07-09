#ifndef LICENSESERVICE_H
#define LICENSESERVICE_H

#include "repository.h"

#include <QObject>
#include <QString>

class LicenseService : public QObject {
    Q_OBJECT
public:
    explicit LicenseService(Repository *repository, QObject *parent = nullptr);

    bool ensureActivated(QWidget *parent);
    QString key() const;
    bool freshActivation() const;

private:
    QString localLicensePath() const;
    QString loadSavedKey() const;
    bool saveKey(const QString &key, QString *errorText = nullptr, const QString &hardware = QString());
    QString loadSavedHardware() const;
    QString hardwareFingerprint() const;
    static bool isWeakFingerprint(const QString &fingerprint);

    Repository *m_repository;
    QString m_key;
    bool m_freshActivation = false;
};

#endif
