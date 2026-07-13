#ifndef EXERCISECONFIG_H
#define EXERCISECONFIG_H

#include <QString>
#include <QStringList>

enum class ExerciseRunnerKind {
    NotImplemented,
    OnlyPicture,
    Puzzles,
    FindMark,
    Paint,
    Cards,
    Remember,
    Remember2,
    E15,
    E28,
    E126,
    E1272,
    Digits,
    WordsLearning,
    E511,
    E521,
    Wolf,
    OnlyDemo
};

enum class ExerciseProtocolKind {
    PictureAnswers,
    TimedBalls,
    TimedBallsWithPictureCount,
    DoneTimeOrHlp,
    NumberedDoneTime,
    OrHlpRow,
    OrHlpBallsRow
};

struct OnlyPictureSettings {
    int pictureCount = 1;
    QString imagePattern = QStringLiteral("p%1.png");
    bool answerButtons = false;
    bool autoAdvancePictures = false;
    QStringList stepIds;
    bool dualPicture = false;
    QString secondImagePattern;
};

struct ExerciseDefinition {
    QString id;
    ExerciseRunnerKind runner = ExerciseRunnerKind::NotImplemented;
    ExerciseProtocolKind protocol = ExerciseProtocolKind::DoneTimeOrHlp;
    OnlyPictureSettings onlyPicture;
    bool usesMultiSessionDateBlocks = false;
};

class ExerciseConfig {
public:
    static const ExerciseDefinition *find(const QString &exerciseId);
    static bool isRunnable(const QString &exerciseId);
    static QString unsupportedMessage(const QString &exerciseId);
};

#endif
