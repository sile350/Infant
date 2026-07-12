#include "repository.h"

#include "exerciseassets.h"

#include "exerciseprotocol.h"
#include "fieldcrypto.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QStringList>
#include <QTextDocument>
#include <QTimer>
#include <algorithm>
#include <functional>

namespace {

QString protocolPageBreakHtml() {
    return QStringLiteral(
        "<div class='protocol-page-break' style='page-break-before:always; break-before:page; height:0;'></div>");
}

void appendProtocolRecord(
    QString &body,
    const QString &uprid,
    const QString &protocolBody,
    bool continuation,
    const std::function<QString(const QString &)> &headerForExercise) {
    if (continuation) {
        body += protocolPageBreakHtml();
    }
    QString record;
    if (continuation) {
        record = QStringLiteral(
                      "<table border='1' style='table-layout:fixed' cellspacing='0' cellpadding='0' width='671'>"
                      "<colgroup><col width='165'><col width='506'></colgroup>")
                  + protocolBody;
    } else {
        record = headerForExercise(uprid) + protocolBody;
    }
    if (!record.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
        record += QStringLiteral("</table>");
    }
    body += record;
}

} // namespace

Repository::Repository(ApiClient *api, QObject *parent)
    : QObject(parent), m_api(api) {
    QString openError;
    m_local.open(&openError);
}

QString Repository::hashPassword(const QString &password, QString *errorText) {
    const QString sole = m_api->systemRequest("getsole", password);
    if (sole.isEmpty()) {
        if (errorText) {
            *errorText = "Не удалось хешировать пароль.";
        }
    }
    return sole;
}

std::optional<SessionUser> Repository::login(const QString &userLogin, const QString &password) {
    QString err;
    const QString sole = hashPassword(password, &err);
    if (sole.isEmpty()) {
        return std::nullopt;
    }
    const QString sql = "SELECT id,fio,login,pass,role,main FROM users WHERE login='"
        + LocalDatabase::escape(userLogin) + "' AND pass='" + LocalDatabase::escape(sole) + "'";
    const QList<QStringList> rows = m_local.queryRows(sql);
    if (rows.isEmpty() || rows.first().size() < 6) {
        return std::nullopt;
    }
    SessionUser user;
    user.id = rows.first().at(0);
    user.fio = rows.first().at(1);
    user.login = rows.first().at(2);
    user.role = rows.first().at(4);
    user.mainId = rows.first().at(5);
    return user;
}

QList<UserRecord> Repository::fetchUsers() {
    QList<UserRecord> records;
    const QList<QStringList> rows = m_local.queryRows("SELECT id,fio,login,role,main FROM users");
    for (const QStringList &row : rows) {
        if (row.size() < 5) {
            continue;
        }
        records.push_back({row.at(0), row.at(1), row.at(2), row.at(3), row.at(4)});
    }
    return records;
}

std::optional<UserRecord> Repository::fetchUserById(const QString &id) {
    const QList<QStringList> rows = m_local.queryRows(
        "SELECT id,fio,login,role,main FROM users WHERE id=" + id
    );
    if (rows.isEmpty() || rows.first().size() < 5) {
        return std::nullopt;
    }
    const QStringList &row = rows.first();
    return UserRecord{row.at(0), row.at(1), row.at(2), row.at(3), row.at(4)};
}

bool Repository::createUser(const QString &fio, const QString &login, const QString &password, const QString &role, const QString &mainId, QString *errorText, QString *createdId) {
    const QString localExists = m_local.queryScalar(
        "SELECT COUNT(*) FROM users WHERE login='" + LocalDatabase::escape(login) + "'");
    if (localExists.trimmed().toInt() > 0) {
        if (errorText) {
            *errorText = "Такой пользователь уже есть!";
        }
        return false;
    }
    const QString checkResponse = m_api->systemRequest("checkuser", "fio[" + login);
    if (checkResponse.trimmed() == QStringLiteral("уже есть")) {
        if (errorText) {
            *errorText = "Такой пользователь уже есть!";
        }
        return false;
    }
    const QString sole = hashPassword(password, errorText);
    if (sole.isEmpty()) {
        return false;
    }
    const QString duplicateAfterHash = m_local.queryScalar(
        "SELECT COUNT(*) FROM users WHERE login='" + LocalDatabase::escape(login) + "'");
    if (duplicateAfterHash.trimmed().toInt() > 0) {
        if (errorText) {
            *errorText = "Такой пользователь уже есть!";
        }
        return false;
    }
    const QString sql = "INSERT INTO users (fio,login,pass,role,main) VALUES('"
        + LocalDatabase::escape(fio) + "','"
        + LocalDatabase::escape(login) + "','"
        + LocalDatabase::escape(sole) + "','"
        + LocalDatabase::escape(role) + "','"
        + LocalDatabase::escape(mainId) + "')";
    if (!m_local.exec(sql)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    if (createdId) {
        *createdId = QString::number(m_local.lastInsertId());
    }
    return true;
}

bool Repository::updateUser(const QString &id, const QString &fio, const QString &login, const QString &password, const QString &role, QString *errorText) {
    const QString duplicateLogin = m_local.queryScalar(
        "SELECT COUNT(*) FROM users WHERE login='" + LocalDatabase::escape(login) + "' AND id<>" + id
    );
    if (duplicateLogin.trimmed().toInt() > 0) {
        if (errorText) {
            *errorText = "Такой пользователь уже есть!";
        }
        return false;
    }

    QString sql;
    if (password.isEmpty()) {
        sql = "UPDATE users SET fio='" + LocalDatabase::escape(fio) + "',login='"
            + LocalDatabase::escape(login) + "',role='" + LocalDatabase::escape(role) + "' WHERE id=" + id;
    } else {
        const QString sole = hashPassword(password, errorText);
        if (sole.isEmpty()) {
            return false;
        }
        sql = "UPDATE users SET fio='" + LocalDatabase::escape(fio) + "',login='"
            + LocalDatabase::escape(login) + "',pass='" + LocalDatabase::escape(sole) + "',role='"
            + LocalDatabase::escape(role) + "' WHERE id=" + id;
    }
    if (!m_local.exec(sql)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}

bool Repository::deleteUser(const QString &id, QString *errorText) {
    if (!m_local.exec("DELETE FROM users WHERE id=" + id)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}

QList<PatientRecord> Repository::fetchPatients(const QString &search, bool useDateFilter, const QDate &from, const QDate &to) {
    QList<PatientRecord> patients;
    QString sql = "SELECT id,fio,dr,dt FROM patients WHERE 1=1";
    if (useDateFilter) {
        sql += " AND dt>=" + QString::number(unixTime(from, false));
        sql += " AND dt<=" + QString::number(unixTime(to, true));
    }
    sql += " ORDER BY id";
    const QList<QStringList> rows = m_local.queryRows(sql);
    for (const QStringList &row : rows) {
        if (row.size() < 4) {
            continue;
        }
        PatientRecord patient;
        patient.id = row.at(0);
        patient.fio = decryptPatientFio(row.at(1));
        patient.birthDate = row.at(2);
        patient.createdAt = row.at(3).toLongLong();
        if (!patientMatchesSearch(patient.fio, search)) {
            continue;
        }
        patients.push_back(patient);
    }
    std::sort(patients.begin(), patients.end(), [](const PatientRecord &a, const PatientRecord &b) {
        return a.fio.localeAwareCompare(b.fio) < 0;
    });
    return patients;
}

QString Repository::loadPatientAnamnesis(const QString &patientId) {
    return m_local.queryScalar("SELECT an FROM patients WHERE id=" + patientId);
}

bool Repository::verifyPatientAccess(const QString &patientId, const QString &licenseKey, QString *errorText) {
    const QString ownerId = m_api->loadOneData("SELECT id FROM org WHERE ky='" + ApiClient::escapeSql(licenseKey) + "'");
    if (ownerId.trimmed().isEmpty()) {
        return true;
    }
    const QString acceptedId = m_api->loadOneData(
        "SELECT id FROM accepted WHERE pid='" + patientId + "' AND owner='" + ownerId.trimmed() + "'");
    if (acceptedId.trimmed() == QStringLiteral("123213")) {
        if (errorText) {
            *errorText = "Потеряна целостность данных или нелицензионная копия.";
        }
        return false;
    }
    return true;
}

bool Repository::savePatientAnamnesis(QString *patientId, const QString &licenseKey, const QString &plainText, const QString &html, QString *detectedFio, QString *detectedBirthDate, QString *errorText) {
    const QString fio = extractValueByPrefix(plainText, QStringLiteral("Ф.И.О. ребенка:"));
    const QString dr = extractValueByPrefix(plainText, QStringLiteral("Дата рождения:"));
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
    const QString escapedHtml = LocalDatabase::escape(html);
    const QString storedFio = encryptPatientFio(fio);
    if (!patientId || patientId->isEmpty()) {
        const QString insertSql = "INSERT INTO patients (fio,dt,dr,an) VALUES('"
            + LocalDatabase::escape(storedFio) + "','"
            + QString::number(now) + "','"
            + LocalDatabase::escape(dr) + "','"
            + escapedHtml + "')";
        if (!m_local.exec(insertSql)) {
            if (errorText) {
                *errorText = m_local.lastError();
            }
            return false;
        }
        const QString lastId = QString::number(m_local.lastInsertId());
        if (patientId) {
            *patientId = lastId;
        }
        const QString capturedKey = licenseKey;
        QTimer::singleShot(0, this, [this, lastId, capturedKey]() {
            registerNewPatientAccepted(lastId, capturedKey);
        });
    } else {
        const QString updateSql = "UPDATE patients SET fio='" + LocalDatabase::escape(storedFio) + "',dr='"
            + LocalDatabase::escape(dr) + "',dt='" + QString::number(now) + "',an='" + escapedHtml + "' WHERE id="
            + patientId->trimmed();
        if (!m_local.exec(updateSql)) {
            if (errorText) {
                *errorText = m_local.lastError();
            }
            return false;
        }
    }
    return true;
}

void Repository::registerNewPatientAccepted(const QString &patientId, const QString &licenseKey) {
    if (patientId.trimmed().isEmpty()) {
        return;
    }
    const QString ownerId = m_api->loadOneData(
        "SELECT id FROM org WHERE ky='" + ApiClient::escapeSql(licenseKey) + "'");
    if (!ownerId.trimmed().isEmpty()) {
        m_api->insert("INSERT INTO accepted (owner,pid) VALUES(" + ownerId.trimmed() + "," + patientId + ")");
    }
}

bool Repository::deletePatient(const QString &patientId, QString *errorText) {
    if (!m_local.exec("DELETE FROM patients WHERE id=" + patientId)) {
        if (errorText) {
            *errorText = m_local.lastError();
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
    return m_local.queryScalar("SELECT data FROM templates WHERE tname='" + LocalDatabase::escape(name) + "'");
}

QStringList Repository::loadTemplateNames() {
    QStringList names;
    names << "Стандартный";
    const QList<QStringList> rows = m_local.queryRows("SELECT tname FROM templates ORDER BY tname");
    for (const QStringList &row : rows) {
        if (row.isEmpty()) {
            continue;
        }
        const QString templateName = row.first().trimmed();
        if (!templateName.isEmpty() && !names.contains(templateName)) {
            names << templateName;
        }
    }
    return names;
}

bool Repository::saveTemplate(const QString &name, int fontPointSize, const QString &html, QString *errorText) {
    const QString escapedName = LocalDatabase::escape(name);
    const QString escapedHtml = LocalDatabase::escape(html);
    const QString countStr = m_local.queryScalar("SELECT COUNT(*) FROM templates WHERE tname='" + escapedName + "'");
    const bool exists = countStr.trimmed().toInt() > 0;
    QString sql;
    if (exists) {
        sql = "UPDATE templates SET font='" + QString::number(fontPointSize) + "',data='" + escapedHtml + "' WHERE tname='" + escapedName + "'";
    } else {
        sql = "INSERT INTO templates (tname,font,data) VALUES('" + escapedName + "','" + QString::number(fontPointSize) + "','" + escapedHtml + "')";
    }
    if (!m_local.exec(sql)) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}

bool Repository::deleteTemplate(const QString &name, QString *errorText) {
    if (name == QStringLiteral("Стандартный")) {
        if (errorText) {
            *errorText = "Стандартный шаблон нельзя удалить.";
        }
        return false;
    }
    if (!m_local.exec("DELETE FROM templates WHERE tname='" + LocalDatabase::escape(name) + "'")) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}

QString Repository::assembleProtocolsBody(const QString &patientId, const QString &role) {
    const QList<QStringList> rows = m_local.queryRows(
        "SELECT id, uprid, pr FROM protocols WHERE userid='" + LocalDatabase::escape(patientId)
        + "' ORDER BY uprid, id ASC");
    if (rows.isEmpty()) {
        return {};
    }

    QString body;
    QString lastUprid;
    for (const QStringList &row : rows) {
        if (row.size() < 3) {
            continue;
        }
        const QString protocolId = row.at(0);
        const QString uprid = row.at(1);
        QString pr = row.at(2);
        const bool continuation = !lastUprid.isEmpty() && uprid == lastUprid;
        if (uprid != lastUprid) {
            lastUprid = uprid;
        }
        pr.replace(QStringLiteral("скачать"), QString());
        if (uprid == QStringLiteral("1.2")) {
            pr = ExerciseProtocol::normalizeProtocol12Layout(pr);
            if (role == QLatin1String("s")) {
                pr = ExerciseProtocol::repairResultsTableBody(pr);
            } else {
                pr = ExerciseProtocol::patientProtocolBody(pr);
            }
        } else if (role != QLatin1String("s")) {
            const int marker = pr.indexOf(QStringLiteral("<!--s-->"));
            if (marker >= 0) {
                pr = pr.left(marker);
                pr.replace(QStringLiteral("Процесс выполнения диагностической методики"), QString());
            }
        }
        const QString markedPr = ExerciseProtocol::wrapProtocolRecord(protocolId, pr);
        appendProtocolRecord(
            body,
            uprid,
            markedPr,
            continuation,
            [this](const QString &exerciseId) { return exerciseHeaderFragment(exerciseId); });
    }
    return body;
}

QString Repository::loadPatientProtocols(const QString &patientId) {
    const QString body = assembleProtocolsBody(patientId, QStringLiteral("s"));
    if (body.isEmpty()) {
        return {};
    }

    const QString zag = QStringLiteral(
        "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' "
        "'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>"
        "<html xmlns='http://www.w3.org/1999/xhtml'>"
        "<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"
        "<title>Выгрузка</title>")
        + ExerciseAssets::protocolTableStyleHtml()
        + QStringLiteral("</head><body>");
    return ExerciseAssets::wrapProtocolDocumentHtml(
        zag + body + QStringLiteral("</body></html>"));
}

QString Repository::exerciseHeaderFragment(const QString &uprid) const {
    QString normalized = uprid.trimmed();
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/../assets/ex"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../assets/ex"),
        QDir::currentPath() + QStringLiteral("/assets/ex"),
        QDir::currentPath() + QStringLiteral("/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/ex"),
        QDir::currentPath() + QStringLiteral("/../old_project/serv9 2025/WindowsFormsApp1/bin/maindata/ex")
    };
    for (const QString &root : roots) {
        const QString path = QDir(root).filePath(normalized + QStringLiteral("/header.html"));
        if (!QFile::exists(path)) {
            continue;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        const QString content = QString::fromUtf8(file.readAll());
        const int bodyPos = content.indexOf(QStringLiteral("<body"), 0, Qt::CaseInsensitive);
        if (bodyPos >= 0) {
            const int tagEnd = content.indexOf(QLatin1Char('>'), bodyPos);
            if (tagEnd >= 0 && tagEnd + 1 < content.size()) {
                return content.mid(tagEnd + 1).trimmed();
            }
        }
        return content;
    }
    return QStringLiteral("<table border='1' cellspacing='0' style='table-layout:fixed' cellpadding='0' width='671'>");
}

QString Repository::loadPatientProtocolsForExport(const QString &patientId, const QString &role, const QString &patientDataHeader) {
    const QString body = assembleProtocolsBody(patientId, role);
    if (body.isEmpty()) {
        return {};
    }

    const QString zag = QStringLiteral(
        "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Transitional//EN' "
        "'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>"
        "<html xmlns='http://www.w3.org/1999/xhtml'>"
        "<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"
        "<title>Выгрузка</title>")
        + ExerciseAssets::protocolTableStyleHtml()
        + QStringLiteral("</head><body>");
    const QString piddata = QStringLiteral(
        "<div align='center' style='font-size:28px'>"
        "Индивидуальная карта психологического развития ребенка<br><br>"
        "<span style='font-size:24px'>%1</span></div><br>")
                            .arg(patientDataHeader);
    return ExerciseAssets::wrapProtocolDocumentHtml(
        zag + piddata + body + QStringLiteral("</body></html>"));
}

bool Repository::verifyLicenseKeyForMachine(const QString &key, const QString &hardware, QString *errorText) {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    QString networkError;
    const auto queryHardware = [&](const QString &hardwareId) -> QList<QStringList> {
        const QString sql = "SELECT name FROM org WHERE ky='" + ApiClient::escapeSql(key) + "' AND hard='"
            + ApiClient::escapeSql(hardwareId) + "' AND dt>=" + QString::number(now);
        const QString raw = m_api->loadData(sql);
        if (!m_api->lastError().isEmpty()) {
            networkError = ApiClient::userFacingNetworkError(m_api->lastError());
            return {};
        }
        return ApiClient::parseRows(raw);
    };

    QList<QStringList> rows = queryHardware(hardware);
    if (rows.isEmpty() && networkError.isEmpty() && hardware.length() > 8) {
        rows = queryHardware(hardware.left(8));
    }
    if (!networkError.isEmpty()) {
        if (errorText) {
            *errorText = networkError;
        }
        return false;
    }
    if (rows.isEmpty()) {
        if (errorText) {
            *errorText = QStringLiteral("Ключ не активирован для этой машины или срок действия истек.");
        }
        return false;
    }
    return true;
}

bool Repository::activateLicenseKey(const QString &key, const QString &hardware, QString *errorText) {
    const QString sql = "SELECT name FROM org WHERE ky='" + ApiClient::escapeSql(key) + "' AND activate='1'";
    const QString raw = m_api->loadData(sql);
    if (!m_api->lastError().isEmpty()) {
        if (errorText) {
            *errorText = ApiClient::userFacingNetworkError(m_api->lastError());
        }
        return false;
    }
    const QList<QStringList> rows = ApiClient::parseRows(raw);
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
    if (!m_local.lastError().isEmpty()) {
        return m_local.lastError();
    }
    return m_api->lastError();
}

QString Repository::extractValueByPrefix(const QString &text, const QString &prefix) const {
    const QStringList lines = text.split(QLatin1Char('\n'));
    const int limit = qMin(14, lines.size());
    for (int i = 0; i < limit; ++i) {
        const QString line = lines.at(i);
        const int index = line.indexOf(prefix, 0, Qt::CaseInsensitive);
        if (index >= 0) {
            return line.mid(index + prefix.size()).trimmed();
        }
    }
    return {};
}

void Repository::extractPatientFields(const QString &plainText, QString *fio, QString *birthDate) const {
    if (fio) {
        *fio = extractValueByPrefix(plainText, QStringLiteral("Ф.И.О. ребенка:"));
    }
    if (birthDate) {
        *birthDate = extractValueByPrefix(plainText, QStringLiteral("Дата рождения:"));
    }
}

qint64 Repository::unixTime(const QDate &date, bool endOfDay) const {
    if (endOfDay) {
        return QDateTime(date, QTime(23, 59, 59), Qt::UTC).toSecsSinceEpoch();
    }
    return QDateTime(date, QTime(0, 0, 0), Qt::UTC).toSecsSinceEpoch();
}

QString Repository::encryptPatientFio(const QString &fio) const {
    return FieldCrypto::encryptPatientFio(fio);
}

QString Repository::decryptPatientFio(const QString &storedFio) const {
    return FieldCrypto::decryptPatientFio(storedFio);
}

bool Repository::patientMatchesSearch(const QString &fio, const QString &search) const {
    const QString query = search.trimmed();
    if (query.isEmpty()) {
        return true;
    }
    const QStringList parts = fio.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        if (part.startsWith(query, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool Repository::saveExerciseProtocol(
    const QString &patientId,
    const QString &exerciseId,
    const QString &protocolHtml,
    bool partly,
    QString *errorText,
    QString *protocolId) {
    if (patientId.trimmed().isEmpty()) {
        if (errorText) {
            *errorText = QStringLiteral("Не выбран пациент");
        }
        return false;
    }
    const QString escapedHtml = LocalDatabase::escape(protocolHtml);
    if (partly) {
        const QString lastId = m_local.queryScalar(
            "SELECT id FROM protocols WHERE userid='" + LocalDatabase::escape(patientId) + "' AND uprid='"
            + LocalDatabase::escape(exerciseId) + "' ORDER BY id DESC LIMIT 1");
        if (lastId.isEmpty()) {
            if (errorText) {
                *errorText = QStringLiteral("Не найден протокол для обновления");
            }
            return false;
        }
        if (!m_local.exec(
                "UPDATE protocols SET pr='" + escapedHtml + "' WHERE id='" + LocalDatabase::escape(lastId) + "'")) {
            if (errorText) {
                *errorText = m_local.lastError();
            }
            return false;
        }
        if (protocolId) {
            *protocolId = lastId;
        }
        return true;
    }

    if (!m_local.exec(
            "INSERT INTO protocols (userid, uprid, pr) VALUES('"
            + LocalDatabase::escape(patientId) + "','" + LocalDatabase::escape(exerciseId) + "','" + escapedHtml
            + "')")) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    const QString newId = m_local.queryScalar("SELECT last_insert_rowid()");
    if (protocolId) {
        *protocolId = newId;
    }
    return true;
}

QString Repository::loadLastExerciseProtocolBody(const QString &patientId, const QString &exerciseId) {
    if (patientId.trimmed().isEmpty() || exerciseId.trimmed().isEmpty()) {
        return {};
    }
    return m_local.queryScalar(
        "SELECT pr FROM protocols WHERE userid='" + LocalDatabase::escape(patientId) + "' AND uprid='"
        + LocalDatabase::escape(exerciseId) + "' ORDER BY id DESC LIMIT 1");
}

QString Repository::loadProtocolViewHtml(
    const QString &exerciseId,
    const QString &protocolId,
    const QString &patientFio,
    const QString &patientBirthDate) {
    const QString protocolBody = m_local.queryScalar(
        "SELECT pr FROM protocols WHERE id='" + LocalDatabase::escape(protocolId) + "'");
    if (protocolBody.isEmpty()) {
        return {};
    }

    QString body = protocolBody;
    if (exerciseId == QStringLiteral("1.2")) {
        body = ExerciseProtocol::extractLastSessionStoredBody(body);
        body = ExerciseProtocol::normalizeProtocol12Layout(body);
        body = ExerciseProtocol::repairResultsTableBody(body);
        body = ExerciseProtocol::restrictExercisePageEditing(body);
    }

    QString protocolBlock = exerciseHeaderFragment(exerciseId) + body;
    if (!body.trimmed().endsWith(QStringLiteral("</table>"), Qt::CaseInsensitive)) {
        protocolBlock += QStringLiteral("</table>");
    }
    return QStringLiteral(
               "<div align='center' style='font-size:20px'><br>Протокол фиксации результатов исследования</div>"
               "<br>ФИО: %1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
               "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Дата рождения:%2<br><br>%3")
        .arg(patientFio.toHtmlEscaped(), patientBirthDate.toHtmlEscaped(), protocolBlock);
}

bool Repository::updateProtocolsFromEditedDocument(
    QTextDocument *document,
    const QStringList &recordIdsInOrder,
    QString *errorText) {
    if (recordIdsInOrder.isEmpty()) {
        return true;
    }
    if (!document) {
        if (errorText) {
            *errorText = QStringLiteral("Пустой документ протокола");
        }
        return false;
    }

    const QMap<QString, QString> bodiesById =
        ExerciseProtocol::extractProtocolBodiesById(document->toHtml());

    for (int i = 0; i < recordIdsInOrder.size(); ++i) {
        const QString protocolId = recordIdsInOrder.at(i);
        const QString storedBody = loadProtocolBodyById(protocolId);
        if (storedBody.trimmed().isEmpty()) {
            continue;
        }

        QString mergedBody;
        if (bodiesById.contains(protocolId)) {
            QTextDocument sectionDocument;
            sectionDocument.setHtml(bodiesById.value(protocolId));
            mergedBody = ExerciseProtocol::mergeEditorDocumentIntoStoredBody(
                storedBody, &sectionDocument, 0);
        } else {
            mergedBody = ExerciseProtocol::mergeEditorDocumentIntoStoredBody(
                storedBody, document, i);
        }
        if (!updateProtocolBody(protocolId, mergedBody, errorText)) {
            return false;
        }
    }
    return true;
}

bool Repository::updateProtocolBody(const QString &protocolId, const QString &protocolBody, QString *errorText) {
    if (protocolId.trimmed().isEmpty()) {
        if (errorText) {
            *errorText = QStringLiteral("Не указан протокол для сохранения");
        }
        return false;
    }
    if (!m_local.exec(
            "UPDATE protocols SET pr='"
            + LocalDatabase::escape(ExerciseProtocol::normalizeStoredProtocolBody(protocolBody))
            + "' WHERE id='"
            + LocalDatabase::escape(protocolId) + "'")) {
        if (errorText) {
            *errorText = m_local.lastError();
        }
        return false;
    }
    return true;
}

bool Repository::updateProtocolsFromEditedHtml(
    const QString &documentHtml,
    const QStringList &recordIdsInOrder,
    QString *errorText) {
    if (recordIdsInOrder.isEmpty()) {
        return true;
    }

    QTextDocument document;
    document.setHtml(documentHtml);
    return updateProtocolsFromEditedDocument(&document, recordIdsInOrder, errorText);
}

QString Repository::loadProtocolBodyById(const QString &protocolId) {
    if (protocolId.trimmed().isEmpty()) {
        return {};
    }
    return m_local.queryScalar(
        "SELECT pr FROM protocols WHERE id='" + LocalDatabase::escape(protocolId) + "'");
}

QStringList Repository::loadPatientProtocolRecordIds(const QString &patientId) {
    if (patientId.trimmed().isEmpty()) {
        return {};
    }
    QStringList ids;
    const QList<QStringList> rows = m_local.queryRows(
        "SELECT id FROM protocols WHERE userid='" + LocalDatabase::escape(patientId)
        + "' ORDER BY uprid, id ASC");
    for (const QStringList &row : rows) {
        if (!row.isEmpty()) {
            ids << row.first();
        }
    }
    return ids;
}
