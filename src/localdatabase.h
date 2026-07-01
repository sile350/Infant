#ifndef LOCALDATABASE_H
#define LOCALDATABASE_H

#include <QList>
#include <QString>
#include <QStringList>

class LocalDatabase {
public:
    bool open(QString *errorText = nullptr);
    QString lastError() const;

    bool exec(const QString &sql);
    QString queryScalar(const QString &sql);
    QList<QStringList> queryRows(const QString &sql);
    qint64 lastInsertId() const;

    static QString escape(const QString &value);

private:
    bool ensureSchema();
    QString databasePath() const;

    QString m_connectionName;
    QString m_lastError;
};

#endif
