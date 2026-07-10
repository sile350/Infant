#include "singleinstance.h"

#include <QDir>
#include <QLockFile>
#include <QStandardPaths>

namespace {

QString lockFilePath() {
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    return QDir(tempDir).absoluteFilePath(QStringLiteral("DokitLabInfant.lock"));
}

} // namespace

class SingleInstance::Private {
public:
    explicit Private(const QString &path)
        : lock(path) {
        lock.setStaleLockTime(30000);
        primary = lock.tryLock(100);
    }

    QLockFile lock;
    bool primary = false;
};

SingleInstance::SingleInstance()
    : m_private(new Private(lockFilePath())) {
}

SingleInstance::~SingleInstance() {
    delete m_private;
    m_private = nullptr;
}

bool SingleInstance::isPrimary() const {
    return m_private && m_private->primary;
}
