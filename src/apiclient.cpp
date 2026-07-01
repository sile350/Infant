#include "apiclient.h"

#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QTimer>
#include <QUrlQuery>

ApiClient::ApiClient(QObject *parent)
    : QObject(parent),
      m_baseUrl("https://dokitlab.ru"),
      m_masterPassword("mainpassword") {}

QString ApiClient::systemRequest(const QString &word, const QString &data) {
    return post("/dsystem.php", {{"word", word}, {"data", data}, {"mp", m_masterPassword}});
}

QString ApiClient::loadData(const QString &sql) {
    return post("/dload.php", {{"req", sql}, {"mp", m_masterPassword}});
}

QString ApiClient::loadOneData(const QString &sql) {
    return post("/dload1.php", {{"req", sql}, {"mp", m_masterPassword}});
}

QString ApiClient::insert(const QString &sql) {
    return post("/dsave.php", {{"req", sql}, {"mp", m_masterPassword}});
}

QString ApiClient::update(const QString &sql) {
    return post("/dupdate.php", {{"req", sql}, {"mp", m_masterPassword}});
}

QString ApiClient::remove(const QString &sql) {
    return post("/ddelete.php", {{"req", sql}, {"mp", m_masterPassword}});
}

QList<QStringList> ApiClient::parseRows(const QString &raw) {
    QList<QStringList> rows;
    const QString normalized = raw.trimmed();
    if (normalized.isEmpty()) {
        return rows;
    }

    QStringList rowChunks;
    if (normalized.contains(';')) {
        rowChunks = normalized.split(';', Qt::SkipEmptyParts);
    } else {
        rowChunks = normalized.split(']', Qt::SkipEmptyParts);
    }

    for (const QString &rowChunk : rowChunks) {
        QStringList cells = rowChunk.split('[', Qt::KeepEmptyParts);
        while (!cells.isEmpty() && cells.last().isEmpty()) {
            cells.removeLast();
        }
        if (!cells.isEmpty()) {
            rows.push_back(cells);
        }
    }
    return rows;
}

QString ApiClient::escapeSql(const QString &value) {
    QString escaped = value;
    escaped.replace("\\", "\\\\");
    escaped.replace("'", "''");
    return escaped;
}

QString ApiClient::lastError() const {
    return m_lastError;
}

QString ApiClient::baseUrl() const {
    return m_baseUrl;
}

bool ApiClient::supportsHttps() {
    return QSslSocket::supportsSsl();
}

QString ApiClient::userFacingNetworkError(const QString &apiError) {
    if (apiError.contains(QStringLiteral("TLS initialization failed"), Qt::CaseInsensitive)
        || apiError.contains(QStringLiteral("ssl"), Qt::CaseInsensitive)) {
        return QStringLiteral(
            "Не удается установить защищенное соединение (HTTPS).\n"
            "Для Windows положите libcrypto-1_1-x64.dll и libssl-1_1-x64.dll рядом с infant.exe "
            "(они копируются автоматически при сборке, если установлен OpenSSL в Qt Tools).");
    }
    if (apiError == QStringLiteral("timeout")) {
        return QStringLiteral("Не удается соединиться с сервером. Истекло время ожидания ответа.");
    }
    if (apiError.trimmed().isEmpty()) {
        return QStringLiteral("Не удается соединиться с сервером. Проверьте интернет-соединение.");
    }
    return QStringLiteral("Не удается соединиться с сервером. ") + apiError;
}

QString ApiClient::post(const QString &endpoint, const QMap<QString, QString> &params) {
    m_lastError.clear();

    QUrl url(m_baseUrl + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery query;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    const QByteArray body = query.toString(QUrl::FullyEncoded).toUtf8();

    QNetworkReply *reply = m_manager.post(request, body);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(15000);
    loop.exec();

    if (!timer.isActive()) {
        m_lastError = "timeout";
        reply->abort();
        reply->deleteLater();
        return {};
    }

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = reply->errorString();
        reply->deleteLater();
        return {};
    }

    const QString response = QString::fromUtf8(reply->readAll());
    reply->deleteLater();
    return response;
}
