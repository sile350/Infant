#include "smbiosreader.h"

#include <QByteArray>
#include <QFile>

#include <fcntl.h>
#include <unistd.h>

namespace {

constexpr int kTypeBaseboard = 2;
constexpr int kTypeProcessor = 4;

bool readPhysicalMemory(quint64 address, quint32 length, QByteArray *out) {
    const int fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) {
        return false;
    }
    out->resize(static_cast<int>(length));
    const ssize_t bytesRead = pread(fd, out->data(), length, static_cast<off_t>(address));
    close(fd);
    return bytesRead == static_cast<ssize_t>(length);
}

bool loadDmiTableFromSysfs(QByteArray *out) {
    QFile file(QStringLiteral("/sys/firmware/dmi/tables/DMI"));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    *out = file.readAll();
    return !out->isEmpty();
}

bool loadDmiTableFromDevMem(QByteArray *out) {
    QByteArray block;
    for (quint64 address = 0x000F0000; address < 0x00100000; address += 16) {
        if (!readPhysicalMemory(address, 32, &block)) {
            return false;
        }

        if (block.startsWith("_SM3_") && block.size() >= 24) {
            const quint8 version = static_cast<quint8>(block.at(6));
            if (version != 0) {
                continue;
            }
            const quint64 tableAddress =
                static_cast<quint8>(block.at(16))
                | (static_cast<quint64>(static_cast<quint8>(block.at(17))) << 8)
                | (static_cast<quint64>(static_cast<quint8>(block.at(18))) << 16)
                | (static_cast<quint64>(static_cast<quint8>(block.at(19))) << 24)
                | (static_cast<quint64>(static_cast<quint8>(block.at(20))) << 32)
                | (static_cast<quint64>(static_cast<quint8>(block.at(21))) << 40)
                | (static_cast<quint64>(static_cast<quint8>(block.at(22))) << 48)
                | (static_cast<quint64>(static_cast<quint8>(block.at(23))) << 56);
            const quint32 tableLength =
                static_cast<quint8>(block.at(12))
                | (static_cast<quint32>(static_cast<quint8>(block.at(13))) << 8)
                | (static_cast<quint32>(static_cast<quint8>(block.at(14))) << 16)
                | (static_cast<quint32>(static_cast<quint8>(block.at(15))) << 24);
            if (tableLength == 0 || tableAddress == 0) {
                continue;
            }
            return readPhysicalMemory(tableAddress, tableLength, out) && !out->isEmpty();
        }

        if (block.startsWith("_SM_") && block.size() >= 0x1F) {
            const quint32 tableLength =
                static_cast<quint8>(block.at(0x16))
                | (static_cast<quint32>(static_cast<quint8>(block.at(0x17))) << 8);
            const quint32 tableAddress =
                static_cast<quint8>(block.at(0x18))
                | (static_cast<quint32>(static_cast<quint8>(block.at(0x19))) << 8)
                | (static_cast<quint32>(static_cast<quint8>(block.at(0x1A))) << 16)
                | (static_cast<quint32>(static_cast<quint8>(block.at(0x1B))) << 24);
            if (tableLength == 0 || tableAddress == 0) {
                continue;
            }
            return readPhysicalMemory(tableAddress, tableLength, out) && !out->isEmpty();
        }
    }
    return false;
}

bool loadDmiTable(QByteArray *out) {
    if (loadDmiTableFromSysfs(out)) {
        return true;
    }
    return loadDmiTableFromDevMem(out);
}

QString getStructureString(const QByteArray &table, int structureOffset, int structureLength, quint8 stringIndex) {
    if (stringIndex == 0 || structureLength <= 0) {
        return {};
    }
    int stringOffset = structureOffset + structureLength;
    quint8 currentIndex = 1;
    while (stringOffset + 1 < table.size()) {
        if (table.at(stringOffset) == '\0') {
            if (currentIndex == stringIndex) {
                return {};
            }
            if (stringOffset + 1 < table.size() && table.at(stringOffset + 1) == '\0') {
                return {};
            }
            ++stringOffset;
            ++currentIndex;
            continue;
        }
        if (currentIndex == stringIndex) {
            const int end = table.indexOf('\0', stringOffset);
            if (end < 0) {
                return {};
            }
            return QString::fromLatin1(table.mid(stringOffset, end - stringOffset).trimmed());
        }
        const int end = table.indexOf('\0', stringOffset);
        if (end < 0) {
            return {};
        }
        stringOffset = end + 1;
        ++currentIndex;
    }
    return {};
}

QString formatProcessorId(const QByteArray &table, int offset) {
    if (offset + 16 > table.size()) {
        return {};
    }
    QString hex;
    hex.reserve(16);
    for (int i = 0; i < 8; ++i) {
        hex.append(QStringLiteral("%1").arg(static_cast<quint8>(table.at(offset + 8 + i)), 2, 16, QChar('0')));
    }
    return hex.toUpper();
}

QString normalizeBoardSerial(QString value) {
    value = value.trimmed();
    if (value.isEmpty()
        || value.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("NA"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("0000000000");
    }
    return value;
}

bool parseHardwareIds(const QByteArray &table, SmbiosReader::HardwareIds *ids) {
    int offset = 0;
    while (offset + 4 <= table.size()) {
        const quint8 type = static_cast<quint8>(table.at(offset));
        const quint8 length = static_cast<quint8>(table.at(offset + 1));
        if (length < 4) {
            break;
        }
        if (offset + length > table.size()) {
            break;
        }

        if (type == kTypeBaseboard && length >= 8 && ids->boardSerial.isEmpty()) {
            const quint8 serialIndex = static_cast<quint8>(table.at(offset + 7));
            ids->boardSerial = normalizeBoardSerial(
                getStructureString(table, offset, length, serialIndex));
        } else if (type == kTypeProcessor && length >= 16 && ids->processorId.isEmpty()) {
            ids->processorId = formatProcessorId(table, offset);
        }

        if (!ids->boardSerial.isEmpty() && !ids->processorId.isEmpty()) {
            return true;
        }

        offset += length;
        while (offset + 1 < table.size()) {
            if (table.at(offset) == '\0' && table.at(offset + 1) == '\0') {
                offset += 2;
                break;
            }
            ++offset;
        }
    }

    return !ids->processorId.isEmpty()
        || ids->boardSerial != QStringLiteral("0000000000");
}

} // namespace

namespace SmbiosReader {

bool readHardwareIds(HardwareIds *ids, QString *errorText) {
    if (!ids) {
        if (errorText) {
            *errorText = QStringLiteral("Internal SMBIOS reader error.");
        }
        return false;
    }

    ids->processorId.clear();
    ids->boardSerial.clear();

    QByteArray table;
    if (!loadDmiTable(&table)) {
        if (errorText) {
            *errorText = QStringLiteral(
                "Не удалось прочитать таблицу SMBIOS. ");
        }
        return false;
    }

    if (!parseHardwareIds(table, ids)) {
        if (errorText) {
            *errorText = QStringLiteral("Не удалось разобрать данные SMBIOS.");
        }
        return false;
    }

    if (ids->processorId.isEmpty()
        && (ids->boardSerial.isEmpty() || ids->boardSerial == QStringLiteral("0000000000"))) {
        if (errorText) {
            *errorText = QStringLiteral("Не удалось получить идентификатор оборудования.");
        }
        return false;
    }

    return true;
}

} // namespace SmbiosReader
