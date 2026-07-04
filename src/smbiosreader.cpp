#include "smbiosreader.h"

#include <QFile>

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#if defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#define INFANT_HAS_CPUID 1
#elif defined(_MSC_VER)
#include <intrin.h>
#define INFANT_HAS_CPUID 1
#endif
#endif

namespace {

QString normalizeBoardSerial(QString value) {
    value = value.trimmed();
    if (value.isEmpty()
        || value.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("NA"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("0000000000");
    }
    return value;
}

QString boardSerialFromSysfs() {
    QFile file(QStringLiteral("/sys/class/dmi/id/board_serial"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return normalizeBoardSerial(QString::fromUtf8(file.readAll()));
}

QString processorIdFromCpuid() {
#if defined(INFANT_HAS_CPUID)
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;

#if defined(__GNUC__) || defined(__clang__)
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return {};
    }
#elif defined(_MSC_VER)
    int cpuInfo[4] = {0, 0, 0, 0};
    __cpuid(cpuInfo, 1);
    eax = static_cast<unsigned int>(cpuInfo[0]);
    ebx = static_cast<unsigned int>(cpuInfo[1]);
    ecx = static_cast<unsigned int>(cpuInfo[2]);
    edx = static_cast<unsigned int>(cpuInfo[3]);
#endif

    return QStringLiteral("%1%2%3%4")
        .arg(eax, 8, 16, QChar('0'))
        .arg(ebx, 8, 16, QChar('0'))
        .arg(ecx, 8, 16, QChar('0'))
        .arg(edx, 8, 16, QChar('0'))
        .toUpper();
#else
    return {};
#endif
}

} // namespace

namespace SmbiosReader {

bool readHardwareIds(HardwareIds *ids, QString *errorText) {
    if (!ids) {
        if (errorText) {
            *errorText = QStringLiteral("Internal hardware reader error.");
        }
        return false;
    }

    ids->processorId = processorIdFromCpuid();
    ids->boardSerial = boardSerialFromSysfs();

    if (ids->processorId.isEmpty()
        && (ids->boardSerial.isEmpty() || ids->boardSerial == QStringLiteral("0000000000"))) {
        if (errorText) {
            *errorText = QStringLiteral("Не удалось получить идентификатор оборудования.");
        }
        return false;
    }

    if (ids->boardSerial.isEmpty()) {
        ids->boardSerial = QStringLiteral("0000000000");
    }

    return true;
}

} // namespace SmbiosReader
