#ifndef FIELDCRYPTO_H
#define FIELDCRYPTO_H

#include <QString>

namespace FieldCrypto {

QString encryptPatientFio(const QString &plain);
QString decryptPatientFio(const QString &stored);

} // namespace FieldCrypto

#endif
