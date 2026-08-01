#ifndef EXERCISESESSION_H
#define EXERCISESESSION_H

#include <QList>
#include <QString>

struct ExerciseSessionResult {
    int elapsedSeconds = 0;
    QString doneState;
    QString additional;
    QString capturedImagePath;
    QList<bool> answers;
    int picturesShown = 0;
};

struct ExerciseSessionOptions {
    bool e15SelectMode = false;
    bool showTemplate = true;
    bool showHint = true;
    bool rotateEnabled = true;
    int rotateW = 0;
    int rotateCW = 0;
    // 1.26: префикс портретов до старта («d» / «m») из превью мальчик/девочка.
    QString genderPrefix;
    // Dual: для 2.8 — компактная раскладка на правой половине первого экрана.
    bool dualScreen = false;
};

#endif
