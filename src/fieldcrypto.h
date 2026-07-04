#ifndef FIELDCRYPTO_H
#define FIELDCRYPTO_H

#include <QString>

namespace FieldCrypto {

QString encryptPatientFio(const QString &plain);
QString decryptPatientFio(const QString &stored);

QString encryptLicenseBlob(const QString &plain);
QString decryptLicenseBlob(const QString &stored);

} // namespace FieldCrypto

#endif
