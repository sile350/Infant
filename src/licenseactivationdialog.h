#ifndef LICENSEACTIVATIONDIALOG_H
#define LICENSEACTIVATIONDIALOG_H

#include <QString>

class QWidget;

class LicenseActivationDialog {
public:
    static QString promptForKey(QWidget *parent, bool *accepted);
};

#endif
