#ifndef MODELS_H
#define MODELS_H

#include <QString>

struct UserRecord {
    QString id;
    QString fio;
    QString role;
    QString mainId;
};

struct PatientRecord {
    QString id;
    QString fio;
    QString birthDate;
    qint64 createdAt = 0;
};

#endif
