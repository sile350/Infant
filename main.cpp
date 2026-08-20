#include "src/approotpaths.h"
#include "src/custommessagebox.h"
#include "src/infantwindow.h"
#include "src/licenseservice.h"
#include "src/repository.h"
#include "src/singleinstance.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_Use96Dpi);
#endif
    QApplication a(argc, argv);

    const SingleInstance singleInstance;
    if (!singleInstance.isPrimary()) {
        return 0;
    }

    // data/, data/scans/, key/ рядом с exe — и на Windows, и на Linux.
    QString layoutError;
    if (!AppRootPaths::ensureWritableLayout(&layoutError)) {
        CustomMessageBox::showError(nullptr, layoutError);
        return 0;
    }

    ApiClient api;
    Repository repository(&api);
    LicenseService licenseService(&repository);
    if (!licenseService.ensureActivated(nullptr)) {
        return 0;
    }

    InfantWindow w(licenseService.key(), licenseService.freshActivation());
    w.show();
    return a.exec();
}
