#include "fieldcrypto.h"

#include <QCryptographicHash>

namespace {

constexpr char kEncryptedPrefix[] = "~enc~";
constexpr char kPatientFioKeySeed[] = "thi1ss11lt";

QByteArray keyBytes() {
    return QCryptographicHash::hash(QByteArray(kPatientFioKeySeed), QCryptographicHash::Sha256);
}

QByteArray xorBytes(const QByteArray &input) {
    const QByteArray key = keyBytes();
    if (key.isEmpty()) {
        return input;
    }
    QByteArray output = input;
    for (int i = 0; i < output.size(); ++i) {
        output[i] = static_cast<char>(output.at(i) ^ key.at(i % key.size()));
    }
    return output;
}

} // namespace

namespace FieldCrypto {

QString encryptPatientFio(const QString &plain) {
    if (plain.isEmpty()) {
        return plain;
    }
    const QByteArray encrypted = xorBytes(plain.toUtf8()).toBase64();
    return QString::fromLatin1(kEncryptedPrefix) + QString::fromLatin1(encrypted);
}

QString decryptPatientFio(const QString &stored) {
    if (stored.isEmpty()) {
        return stored;
    }
    if (!stored.startsWith(QLatin1String(kEncryptedPrefix))) {
        return stored;
    }
    const QByteArray payload = stored.mid(QString::fromLatin1(kEncryptedPrefix).size()).toLatin1();
    const QByteArray decoded = QByteArray::fromBase64(payload);
    if (decoded.isEmpty() && !payload.isEmpty()) {
        return stored;
    }
    return QString::fromUtf8(xorBytes(decoded));
}

} // namespace FieldCrypto
