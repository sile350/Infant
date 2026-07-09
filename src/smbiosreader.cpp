#include "smbiosreader.h"

#include <QFile>

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

QString processorModelFromCpuInfo() {
    QFile file(QStringLiteral("/proc/cpuinfo"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (!line.startsWith(QStringLiteral("model name"), Qt::CaseInsensitive)) {
            continue;
        }
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0) {
            continue;
        }
        const QString model = line.mid(colon + 1).trimmed();
        if (!model.isEmpty()) {
            return model;
        }
    }
    return {};
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

    ids->processorId = processorModelFromCpuInfo();
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
