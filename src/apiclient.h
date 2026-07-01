#ifndef APICLIENT_H
#define APICLIENT_H

#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);

    QString systemRequest(const QString &word, const QString &data);
    QString loadData(const QString &sql);
    QString loadOneData(const QString &sql);
    QString insert(const QString &sql);
    QString update(const QString &sql);
    QString remove(const QString &sql);

    static QList<QStringList> parseRows(const QString &raw);
    static QString escapeSql(const QString &value);

    QString lastError() const;
    QString baseUrl() const;

    static bool supportsHttps();
    static QString userFacingNetworkError(const QString &apiError);

private:
    QString post(const QString &endpoint, const QMap<QString, QString> &params);

    QNetworkAccessManager m_manager;
    QString m_lastError;
    QString m_baseUrl;
    QString m_masterPassword;
};

#endif
