#include "repository.h"

#include <QDateTime>
#include <QStringList>

Repository::Repository(ApiClient *api, QObject *parent)
    : QObject(parent), m_api(api) {}

std::optional<SessionUser> Repository::login(const QString &fio, const QString &password) {
    const QString sole = m_api->systemRequest("getsole", password);
    if (sole.isEmpty()) {
        return std::nullopt;
    }
    const QString sql = "SELECT id,fio,pass,role,main FROM users WHERE fio='" + ApiClient::escapeSql(fio) + "' AND pass='" + ApiClient::escapeSql(sole) + "'";
    const QList<QStringList> rows = ApiClient::parseRows(m_api->loadData(sql));
    if (rows.isEmpty() || rows.first().size() < 5) {
        return std::nullopt;
    }
    SessionUser user;
    user.id = rows.first().at(0);
    user.fio = rows.first().at(1);
    user.role = rows.first().at(3);
    user.mainId = rows.first().at(4);
    return user;
}

QList<UserRecord> Repository::fetchUsers() {
    QList<UserRecord> records;
    const QList<QStringList> rows = ApiClient::parseRows(m_api->loadData("SELECT id,fio,role,main FROM users"));
    for (const QStringList &row : rows) {
        if (row.size() < 4) {
            continue;
        }
        records.push_back({row.at(0), row.at(1), row.at(2), row.at(3)});
    }
    return records;
}

bool Repository::createUser(const QString &fio, const QString &password, const QString &role, const QString &mainId, QString *errorText) {
    const QString checkResponse = m_api->systemRequest("checkuser", "fio[" + fio);
    if (checkResponse.trimmed() == QStringLiteral("уже есть")) {
        if (errorText) {
            *errorText = "Такой пользователь уже существует.";
        }
        return false;
    }
    const QString sole = m_api->systemRequest("getsole", password);
    if (sole.isEmpty()) {
        if (errorText) {
            *errorText = "Не удалось хешировать пароль.";
        }
        return false;
    }
    const QString sql = "INSERT INTO users (fio,pass,role,main) VALUES('" + ApiClient::escapeSql(fio) + "','" + ApiClient::escapeSql(sole) + "','" + ApiClient::escapeSql(role) + "','" + ApiClient::escapeSql(mainId) + "')";
    m_api->insert(sql);
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = m_api->lastError();
        }
        return false;
    }
    return true;
}

bool Repository::updateUser(const QString &id, const QString &fio, const QString &password, const QString &role, QString *errorText) {
    QString sql;
    if (password.isEmpty()) {
        sql = "UPDATE users SET fio='" + ApiClient::escapeSql(fio) + "',role='" + ApiClient::escapeSql(role) + "' WHERE id=" + id;
    } else {
        const QString sole = m_api->systemRequest("getsole", password);
        if (sole.isEmpty()) {
            if (errorText) {
                *errorText = "Не удалось хешировать пароль.";
            }
            return false;
        }
        sql = "UPDATE users SET fio='" + ApiClient::escapeSql(fio) + "',pass='" + ApiClient::escapeSql(sole) + "',role='" + ApiClient::escapeSql(role) + "' WHERE id=" + id;
    }
    m_api->update(sql);
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = m_api->lastError();
        }
        return false;
    }
    return true;
}

bool Repository::deleteUser(const QString &id, QString *errorText) {
    m_api->remove("DELETE FROM users WHERE id=" + id);
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = m_api->lastError();
        }
        return false;
    }
    return true;
}

QList<PatientRecord> Repository::fetchPatients(const QString &search, bool useDateFilter, const QDate &from, const QDate &to) {
    QList<PatientRecord> patients;
    QString sql = "SELECT id,fio,dr,dt FROM patients WHERE 1=1";
    if (!search.trimmed().isEmpty()) {
        sql += " AND fio LIKE '%" + ApiClient::escapeSql(search.trimmed()) + "%'";
    }
    if (useDateFilter) {
        sql += " AND dt>=" + QString::number(unixTime(from, false));
        sql += " AND dt<=" + QString::number(unixTime(to, true));
    }
    sql += " ORDER BY fio";
    const QList<QStringList> rows = ApiClient::parseRows(m_api->loadData(sql));
    for (const QStringList &row : rows) {
        if (row.size() < 4) {
            continue;
        }
        PatientRecord patient;
        patient.id = row.at(0);
        patient.fio = row.at(1);
        patient.birthDate = row.at(2);
        patient.createdAt = row.at(3).toLongLong();
        patients.push_back(patient);
    }
    return patients;
}

QString Repository::loadPatientAnamnesis(const QString &patientId) {
    const QString raw = m_api->loadOneData("SELECT an FROM patients WHERE id=" + patientId);
    return raw;
}

bool Repository::savePatientAnamnesis(QString *patientId, const QString &licenseKey, const QString &html, QString *detectedFio, QString *detectedBirthDate, QString *errorText) {
    const QString plain = html;
    const QString fio = extractValueByPrefix(plain, "Ф.И.О. ребенка:");
    const QString dr = extractValueByPrefix(plain, "Дата рождения:");
    if (detectedFio) {
        *detectedFio = fio;
    }
    if (detectedBirthDate) {
        *detectedBirthDate = dr;
    }
    if (fio.trimmed().size() < 3) {
        if (errorText) {
            *errorText = "Заполните Ф.И.О. ребенка в анамнезе.";
        }
        return false;
    }

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const QString escapedHtml = ApiClient::escapeSql(html);
    if (!patientId || patientId->isEmpty()) {
        const QString insertSql = "INSERT INTO patients (fio,dt,dr,an) VALUES('" + ApiClient::escapeSql(fio) + "','" + QString::number(now) + "','" + ApiClient::escapeSql(dr) + "','" + escapedHtml + "')";
        m_api->insert(insertSql);
        if (!m_api->lastError().isEmpty()) {
            if (errorText) {
                *errorText = m_api->lastError();
            }
            return false;
        }
        const QString lastId = m_api->loadOneData("SELECT id FROM patients ORDER BY id DESC LIMIT 1");
        if (patientId) {
            *patientId = lastId.trimmed();
        }
        const QString ownerId = m_api->loadOneData("SELECT id FROM org WHERE ky='" + ApiClient::escapeSql(licenseKey) + "'");
        if (!ownerId.trimmed().isEmpty() && patientId && !patientId->isEmpty()) {
            m_api->insert("INSERT INTO accepted (owner,pid) VALUES(" + ownerId.trimmed() + "," + patientId->trimmed() + ")");
        }
    } else {
        const QString updateSql = "UPDATE patients SET fio='" + ApiClient::escapeSql(fio) + "',dr='" + ApiClient::escapeSql(dr) + "',dt='" + QString::number(now) + "',an='" + escapedHtml + "' WHERE id=" + patientId->trimmed();
        m_api->update(updateSql);
        if (!m_api->lastError().isEmpty()) {
            if (errorText) {
                *errorText = m_api->lastError();
            }
            return false;
        }
    }
    return true;
}

bool Repository::deletePatient(const QString &patientId, QString *errorText) {
    m_api->remove("DELETE FROM patients WHERE id=" + patientId);
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = m_api->lastError();
        }
        return false;
    }
    return true;
}

QString Repository::defaultAnamnesisTemplate() const {
    return "<h2>Анамнез</h2><p>Ф.И.О. ребенка: </p><p>Дата рождения: </p><p>Жалобы: </p><p>Анамнез заболевания: </p><p>Анамнез жизни: </p><p>Объективный статус: </p><p>Заключение: </p><p>Рекомендации: </p>";
}

QString Repository::loadTemplate(const QString &name) {
    if (name == "Стандартный") {
        return defaultAnamnesisTemplate();
    }
    return m_api->loadOneData("SELECT data FROM templates WHERE tname='" + ApiClient::escapeSql(name) + "'");
}

QStringList Repository::loadTemplateNames() {
    QStringList names;
    names << "Стандартный";
    const QList<QStringList> rows = ApiClient::parseRows(m_api->loadData("SELECT tname FROM templates ORDER BY tname"));
    for (const QStringList &row : rows) {
        if (row.isEmpty()) {
            continue;
        }
        const QString name = row.first().trimmed();
        if (!name.isEmpty() && !names.contains(name)) {
            names << name;
        }
    }
    return names;
}

bool Repository::saveTemplate(const QString &name, int fontPointSize, const QString &html, QString *errorText) {
    const QString escapedName = ApiClient::escapeSql(name);
    const QString escapedHtml = ApiClient::escapeSql(html);
    const QString countStr = m_api->loadOneData("SELECT COUNT(*) FROM templates WHERE tname='" + escapedName + "'");
    const bool exists = countStr.trimmed().toInt() > 0;
    QString sql;
    if (exists) {
        sql = "UPDATE templates SET font='" + QString::number(fontPointSize) + "',data='" + escapedHtml + "' WHERE tname='" + escapedName + "'";
        m_api->update(sql);
    } else {
        sql = "INSERT INTO templates (tname,font,data) VALUES('" + escapedName + "','" + QString::number(fontPointSize) + "','" + escapedHtml + "')";
        m_api->insert(sql);
    }
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = m_api->lastError();
        }
        return false;
    }
    return true;
}

bool Repository::verifyLicenseKeyForMachine(const QString &key, const QString &hardware, QString *errorText) {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const QString sql = "SELECT name FROM org WHERE ky='" + ApiClient::escapeSql(key) + "' AND hard='" + ApiClient::escapeSql(hardware) + "' AND dt>=" + QString::number(now);
    const QList<QStringList> rows = ApiClient::parseRows(m_api->loadData(sql));
    if (rows.isEmpty()) {
        if (errorText) {
            *errorText = "Ключ не активирован для этой машины или срок действия истек.";
        }
        return false;
    }
    return true;
}

bool Repository::activateLicenseKey(const QString &key, const QString &hardware, QString *errorText) {
    const QString sql = "SELECT name FROM org WHERE ky='" + ApiClient::escapeSql(key) + "' AND activate='1'";
    const QList<QStringList> rows = ApiClient::parseRows(m_api->loadData(sql));
    if (rows.isEmpty()) {
        if (errorText) {
            *errorText = "Неверный ключ или активация запрещена.";
        }
        return false;
    }
    m_api->update("UPDATE org SET hard='" + ApiClient::escapeSql(hardware) + "',activate='0' WHERE ky='" + ApiClient::escapeSql(key) + "'");
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = m_api->lastError();
        }
        return false;
    }
    return true;
}

QString Repository::lastError() const {
    return m_api->lastError();
}

QString Repository::extractValueByPrefix(const QString &text, const QString &prefix) const {
    const QStringList lines = text.split('\n');
    for (const QString &line : lines) {
        const int index = line.indexOf(prefix, 0, Qt::CaseInsensitive);
        if (index >= 0) {
            return line.mid(index + prefix.size()).trimmed();
        }
    }
    return {};
}

qint64 Repository::unixTime(const QDate &date, bool endOfDay) const {
    if (endOfDay) {
        return QDateTime(date, QTime(23, 59, 59), Qt::UTC).toSecsSinceEpoch();
    }
    return QDateTime(date, QTime(0, 0, 0), Qt::UTC).toSecsSinceEpoch();
}
