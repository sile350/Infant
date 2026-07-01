#include "localdatabase.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QVariant>

bool LocalDatabase::open(QString *errorText) {
    m_connectionName = QUuid::createUuid().toString();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    const QString path = databasePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    db.setDatabaseName(path);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        if (errorText) {
            *errorText = m_lastError;
        }
        return false;
    }
    if (!ensureSchema()) {
        if (errorText) {
            *errorText = m_lastError;
        }
        return false;
    }
    return true;
}

QString LocalDatabase::lastError() const {
    return m_lastError;
}

bool LocalDatabase::exec(const QString &sql) {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return false;
    }
    m_lastError.clear();
    return true;
}

QString LocalDatabase::queryScalar(const QString &sql) {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(sql) || !query.next()) {
        m_lastError = query.lastError().text();
        return {};
    }
    m_lastError.clear();
    return query.value(0).toString();
}

QList<QStringList> LocalDatabase::queryRows(const QString &sql) {
    QList<QStringList> rows;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return rows;
    }
    const int columns = query.record().count();
    while (query.next()) {
        QStringList row;
        for (int i = 0; i < columns; ++i) {
            row << query.value(i).toString();
        }
        rows.push_back(row);
    }
    m_lastError.clear();
    return rows;
}

qint64 LocalDatabase::lastInsertId() const {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (query.exec("SELECT last_insert_rowid()") && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

QString LocalDatabase::escape(const QString &value) {
    QString escaped = value;
    escaped.replace("'", "''");
    return escaped;
}

bool LocalDatabase::ensureSchema() {
    const char *statements[] = {
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fio TEXT NOT NULL,"
        "login TEXT NOT NULL DEFAULT '',"
        "pass TEXT NOT NULL,"
        "role TEXT NOT NULL,"
        "main TEXT NOT NULL"
        ")",
        "CREATE TABLE IF NOT EXISTS patients ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fio TEXT NOT NULL,"
        "an TEXT NOT NULL,"
        "dt INTEGER NOT NULL,"
        "dr TEXT NOT NULL"
        ")",
        "CREATE TABLE IF NOT EXISTS templates ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "tname TEXT NOT NULL,"
        "font TEXT NOT NULL,"
        "data TEXT NOT NULL"
        ")",
        "CREATE TABLE IF NOT EXISTS protocols ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "userid TEXT NOT NULL,"
        "uprid TEXT NOT NULL,"
        "pr TEXT NOT NULL"
        ")"
    };
    for (const char *sql : statements) {
        if (!exec(QString::fromLatin1(sql))) {
            return false;
        }
    }

    const QList<QStringList> userColumns = queryRows("PRAGMA table_info(users)");
    bool hasLoginColumn = false;
    for (const QStringList &column : userColumns) {
        if (column.size() >= 2 && column.at(1) == "login") {
            hasLoginColumn = true;
            break;
        }
    }
    if (!hasLoginColumn) {
        if (!exec("ALTER TABLE users ADD COLUMN login TEXT NOT NULL DEFAULT ''")) {
            return false;
        }
        if (!exec("UPDATE users SET login=fio WHERE login=''")) {
            return false;
        }
    }

    exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_users_login ON users(login)");

    return true;
}

QString LocalDatabase::databasePath() const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/data",
        QCoreApplication::applicationDirPath() + "/../data",
        QDir::currentPath() + "/data"
    };
    for (const QString &root : roots) {
        QDir dir(root);
        if (dir.exists() || root.startsWith(QCoreApplication::applicationDirPath())) {
            return dir.filePath("base.db");
        }
    }
    return QCoreApplication::applicationDirPath() + "/data/base.db";
}
