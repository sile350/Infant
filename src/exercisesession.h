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
    // 3.1.16: «1» = все фрагменты, «2» = только к текущему заданию (pset2 / groupBox3).
    // По умолчанию как в оригинале (exbegin + radioButton1): все фрагменты.
    QString puzzleAparam = QStringLiteral("1");
    // 4.1.7: маска выбранных картинок «1,0,1,…» (как rem.state).
    QString remPictureMask;
};

#endif
