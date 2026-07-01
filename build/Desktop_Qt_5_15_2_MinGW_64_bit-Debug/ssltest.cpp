#include <QCoreApplication>
#include <QSslSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <stdio.h>
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    printf("supportsSsl=%d\n", QSslSocket::supportsSsl());
    QNetworkAccessManager mgr;
    QNetworkRequest req(QUrl("https://dokitlab.ru/dload.php"));
    QNetworkReply* reply = mgr.post(req, QByteArray("req=SELECT%201&mp=mainpassword"));
    QEventLoop loop; QTimer::singleShot(15000, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    printf("error=%d errStr=%s bytes=%lld\n", (int)reply->error(), reply->errorString().toUtf8().constData(), (long long)reply->readAll().size());
    return reply->error() == QNetworkReply::NoError ? 0 : 1;
}
