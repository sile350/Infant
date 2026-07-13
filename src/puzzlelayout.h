#ifndef PUZZLELAYOUT_H
#define PUZZLELAYOUT_H

#include <QString>
#include <QVector>

struct PuzzleSpriteDef {
    QString file;
    int x = 0;
    int y = 0;
    int targetX = -1;
    int targetY = -1;
    QString name;
    bool clickable = true;
    bool closed = false;
    QString closedFile;
    QString openFile;
};

struct PuzzleLayout {
    bool rotateAllowed = false;
    bool selectMode = false;
    bool showTemplate = true;
    QString templateFile;
    int templateX = 0;
    int templateY = 0;
    QString backgroundFile;
    QVector<PuzzleSpriteDef> sprites;
    double scale = 1.0;
};

bool loadPuzzleLayout(const QString &exerciseId, const QString &stepId, PuzzleLayout *layout);

#endif
