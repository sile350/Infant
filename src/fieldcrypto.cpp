#include "fieldcrypto.h"

#include <QCryptographicHash>

namespace {

constexpr char kEncryptedPrefix[] = "~enc~";
constexpr char kLicenseEncryptedPrefix[] = "~lic~";
constexpr char kPatientFioKeySeed[] = "thi1ss11lt";
constexpr char kLicenseKeySeed[] = "InfLtS@lt#2026";

QByteArray keyBytes(const char *seed) {
    return QCryptographicHash::hash(QByteArray(seed), QCryptographicHash::Sha256);
}

QByteArray xorBytes(const QByteArray &input, const char *seed) {
    const QByteArray key = keyBytes(seed);
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
    const QByteArray encrypted = xorBytes(plain.toUtf8(), kPatientFioKeySeed).toBase64();
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
    return QString::fromUtf8(xorBytes(decoded, kPatientFioKeySeed));
}

QString encryptLicenseBlob(const QString &plain) {
    if (plain.isEmpty()) {
        return plain;
    }
    const QByteArray encrypted = xorBytes(plain.toUtf8(), kLicenseKeySeed).toBase64();
    return QString::fromLatin1(kLicenseEncryptedPrefix) + QString::fromLatin1(encrypted);
}

QString decryptLicenseBlob(const QString &stored) {
    if (stored.isEmpty()) {
        return stored;
    }
    if (!stored.startsWith(QLatin1String(kLicenseEncryptedPrefix))) {
        return stored;
    }
    const QByteArray payload = stored.mid(QString::fromLatin1(kLicenseEncryptedPrefix).size()).toLatin1();
    const QByteArray decoded = QByteArray::fromBase64(payload);
    if (decoded.isEmpty() && !payload.isEmpty()) {
        return stored;
    }
    return QString::fromUtf8(xorBytes(decoded, kLicenseKeySeed));
}

} // namespace FieldCrypto
