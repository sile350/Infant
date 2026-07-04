#ifndef SMBIOSREADER_H
#define SMBIOSREADER_H

#include <QString>

namespace SmbiosReader {

struct HardwareIds {
    QString processorId;
    QString boardSerial;
};

bool readHardwareIds(HardwareIds *ids, QString *errorText = nullptr);

} // namespace SmbiosReader

#endif
