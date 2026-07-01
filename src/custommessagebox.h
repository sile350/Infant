#ifndef CUSTOMMESSAGEBOX_H
#define CUSTOMMESSAGEBOX_H

#include <QString>

class QWidget;

class CustomMessageBox {
public:
    static void showError(QWidget *parent, const QString &message);
    static void showWarning(QWidget *parent, const QString &message);
    static void showInfo(QWidget *parent, const QString &message);
    static bool askConfirm(QWidget *parent, const QString &message);

private:
    static QString assetPath(const QString &name);
};

#endif
