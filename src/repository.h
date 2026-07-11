#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "apiclient.h"
#include "localdatabase.h"
#include "models.h"

#include <QDate>
#include <QList>
#include <QObject>
#include <optional>

struct SessionUser {
    QString id;
    QString fio;
    QString login;
    QString role;
    QString mainId;
};

class Repository : public QObject {
    Q_OBJECT
public:
    explicit Repository(ApiClient *api, QObject *parent = nullptr);

    std::optional<SessionUser> login(const QString &userLogin, const QString &password);

    QList<UserRecord> fetchUsers();
    std::optional<UserRecord> fetchUserById(const QString &id);
    bool createUser(const QString &fio, const QString &login, const QString &password, const QString &role, const QString &mainId, QString *errorText, QString *createdId = nullptr);
    bool updateUser(const QString &id, const QString &fio, const QString &login, const QString &password, const QString &role, QString *errorText);
    bool deleteUser(const QString &id, QString *errorText);

    QList<PatientRecord> fetchPatients(const QString &search, bool useDateFilter, const QDate &from, const QDate &to);
    QString loadPatientAnamnesis(const QString &patientId);
    bool verifyPatientAccess(const QString &patientId, const QString &licenseKey, QString *errorText);
    bool savePatientAnamnesis(QString *patientId, const QString &licenseKey, const QString &plainText, const QString &html, QString *detectedFio, QString *detectedBirthDate, QString *errorText);
    void registerNewPatientAccepted(const QString &patientId, const QString &licenseKey);
    void extractPatientFields(const QString &plainText, QString *fio, QString *birthDate) const;
    bool deletePatient(const QString &patientId, QString *errorText);

    QString defaultAnamnesisTemplate() const;
    QString loadTemplate(const QString &name);
    QStringList loadTemplateNames();
    bool saveTemplate(const QString &name, int fontPointSize, const QString &html, QString *errorText);
    bool deleteTemplate(const QString &name, QString *errorText);
    QString loadPatientProtocols(const QString &patientId);
    QString loadPatientProtocolsForExport(const QString &patientId, const QString &role, const QString &patientDataHeader);

    bool verifyLicenseKeyForMachine(const QString &key, const QString &hardware, QString *errorText);
    bool activateLicenseKey(const QString &key, const QString &hardware, QString *errorText);

    bool saveExerciseProtocol(
        const QString &patientId,
        const QString &exerciseId,
        const QString &protocolHtml,
        bool partly,
        QString *errorText,
        QString *protocolId = nullptr);
    QString loadProtocolViewHtml(
        const QString &exerciseId,
        const QString &protocolId,
        const QString &patientFio,
        const QString &patientBirthDate);
    QString loadLastExerciseProtocolBody(const QString &patientId, const QString &exerciseId);
    QStringList loadPatientProtocolRecordIds(const QString &patientId);
    bool updateProtocolBody(const QString &protocolId, const QString &protocolBody, QString *errorText = nullptr);
    bool updateProtocolsFromEditedHtml(
        const QString &documentHtml,
        const QStringList &recordIdsInOrder,
        QString *errorText = nullptr);

    QString lastError() const;

private:
    QString hashPassword(const QString &password, QString *errorText);
    QString encryptPatientFio(const QString &fio) const;
    QString decryptPatientFio(const QString &storedFio) const;
    bool patientMatchesSearch(const QString &fio, const QString &search) const;
    QString extractValueByPrefix(const QString &text, const QString &prefix) const;
    QString exerciseHeaderFragment(const QString &uprid) const;
    QString assembleProtocolsBody(const QString &patientId, const QString &role);
    qint64 unixTime(const QDate &date, bool endOfDay) const;

    ApiClient *m_api;
    LocalDatabase m_local;
};

#endif
