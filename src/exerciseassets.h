#ifndef EXERCISEASSETS_H
#define EXERCISEASSETS_H

#include <QString>

class ExerciseAssets {
public:
    static QString exerciseDir(const QString &exerciseId);
    static QString exerciseFile(const QString &exerciseId, const QString &fileName);
    static QString sysImage(const QString &fileName);
    static QString prepareExerciseHtml(const QString &html, const QString &baseDir);
};

#endif
