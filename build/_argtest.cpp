#include <QCoreApplication>
#include <QDebug>
int main(int argc, char **argv) {
  QCoreApplication a(argc, argv);
  qDebug() << QStringLiteral("f1.png").arg(1);
  qDebug() << QStringLiteral("p%1.png").arg(1);
  return 0;
}
