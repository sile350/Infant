#include "src/infantwindow.h"
#include "src/licenseservice.h"
#include "src/repository.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_Use96Dpi);
#endif
    QApplication a(argc, argv);

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
