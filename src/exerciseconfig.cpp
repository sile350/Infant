#include "exerciseconfig.h"

namespace {

ExerciseDefinition def(
    const QString &id,
    ExerciseRunnerKind runner,
    ExerciseProtocolKind protocol,
    OnlyPictureSettings onlyPicture = {},
    bool multiSessionDateBlocks = false) {
    ExerciseDefinition item;
    item.id = id;
    item.runner = runner;
    item.protocol = protocol;
    item.onlyPicture = onlyPicture;
    item.usesMultiSessionDateBlocks = multiSessionDateBlocks;
    return item;
}

OnlyPictureSettings steps(const QStringList &ids, const QString &pattern = QStringLiteral("%1.png")) {
    OnlyPictureSettings settings;
    settings.stepIds = ids;
    settings.imagePattern = pattern;
    settings.pictureCount = ids.size();
    return settings;
}

OnlyPictureSettings single(const QString &pattern = QStringLiteral("p%1.png")) {
    OnlyPictureSettings settings;
    settings.pictureCount = 1;
    settings.imagePattern = pattern;
    return settings;
}

OnlyPictureSettings sequence(int count, bool answerButtons = false) {
    OnlyPictureSettings settings;
    settings.pictureCount = count;
    settings.imagePattern = QStringLiteral("p%1.png");
    settings.answerButtons = answerButtons;
    return settings;
}

const ExerciseDefinition kDefinitions[] = {
    def(QStringLiteral("1.1"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::TimedBalls, single()),
    def(QStringLiteral("1.2"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::PictureAnswers,
        sequence(5, true), true),
    def(QStringLiteral("1.4"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::TimedBallsWithPictureCount,
        sequence(4, false)),
    def(QStringLiteral("1.5"), ExerciseRunnerKind::E15, ExerciseProtocolKind::TimedBalls),
    def(QStringLiteral("1.6"), ExerciseRunnerKind::E15, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("1.7"), ExerciseRunnerKind::Paint, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("1.8"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::TimedBalls, single()),
    def(QStringLiteral("1.11"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2")})),
    def(QStringLiteral("1.12"), ExerciseRunnerKind::Paint, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("1.13"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::DoneTimeOrHlp, single()),
    def(QStringLiteral("1.14"), ExerciseRunnerKind::OnlyDemo, ExerciseProtocolKind::DoneTimeOrHlp,
        steps({QStringLiteral("1"), QStringLiteral("2")})),
    def(QStringLiteral("1.15"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("1.17"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("1.18"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("1.19"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("Матрешка 2"), QStringLiteral("Мишка 4"), QStringLiteral("Леопард 3"),
               QStringLiteral("Дом 4")})),
    def(QStringLiteral("1.20"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("Мяч 2"), QStringLiteral("Дом 3"), QStringLiteral("Мишка 4"),
               QStringLiteral("Машинка 5"), QStringLiteral("Чайник 6")})),
    def(QStringLiteral("1.21"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("2А"), QStringLiteral("2Б"), QStringLiteral("3А"), QStringLiteral("3Б"),
               QStringLiteral("4А"), QStringLiteral("4Б"), QStringLiteral("5А"), QStringLiteral("5Б"),
               QStringLiteral("6А"), QStringLiteral("6Б")})),
    def(QStringLiteral("1.22"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("Круг"), QStringLiteral("Квадрат"), QStringLiteral("Пирамида")})),
    def(QStringLiteral("1.24"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4")})),
    def(QStringLiteral("1.25"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::DoneTimeOrHlp, single()),
    def(QStringLiteral("1.26"), ExerciseRunnerKind::E126, ExerciseProtocolKind::DoneTimeOrHlp,
        steps({QStringLiteral("1"), QStringLiteral("2")})),
    def(QStringLiteral("1.27"), ExerciseRunnerKind::Remember, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("1.272"), ExerciseRunnerKind::E1272, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"),
               QStringLiteral("4"), QStringLiteral("5"), QStringLiteral("6")})),
    def(QStringLiteral("1.28"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2")})),
    def(QStringLiteral("1.29"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("2.1"), ExerciseRunnerKind::FindMark, ExerciseProtocolKind::DoneTimeOrHlp,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("2.2"), ExerciseRunnerKind::FindMark, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("2.3"), ExerciseRunnerKind::FindMark, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("2.8"), ExerciseRunnerKind::E28, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("2.9"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::DoneTimeOrHlp,
        steps({QStringLiteral("1"), QStringLiteral("2")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("2.10"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("2.11"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2")})),
    def(QStringLiteral("2.12"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2")})),
    def(QStringLiteral("3.1.1"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpRow,
        single(QStringLiteral("1.png"))),
    def(QStringLiteral("3.1.2"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpRow,
        steps({QStringLiteral("1"), QStringLiteral("2")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("3.1.7"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("3.1.8"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2")})),
    def(QStringLiteral("3.1.10"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4"),
               QStringLiteral("5"), QStringLiteral("6"), QStringLiteral("7"), QStringLiteral("8"),
               QStringLiteral("9"), QStringLiteral("10")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("3.1.11"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        single(QStringLiteral("10.png"))),
    def(QStringLiteral("3.1.12"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        sequence(5, false)),
    def(QStringLiteral("3.1.15"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::OrHlpBallsRow,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4")})),
    def(QStringLiteral("3.1.16"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4"),
               QStringLiteral("5"), QStringLiteral("6"), QStringLiteral("7")})),
    def(QStringLiteral("3.1.17"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        single(QStringLiteral("1.png"))),
    def(QStringLiteral("3.1.18"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        single(QStringLiteral("1.png"))),
    def(QStringLiteral("3.1.20"), ExerciseRunnerKind::Remember, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("3.1.21"), ExerciseRunnerKind::Remember, ExerciseProtocolKind::DoneTimeOrHlp,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("3.1.23"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("3.1.24"), ExerciseRunnerKind::Puzzles, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("Пример"), QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("3.2.1"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        steps({QStringLiteral("1"), QStringLiteral("2")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("3.2.2"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        single(QStringLiteral("1.png"))),
    def(QStringLiteral("3.2.3"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        steps({QStringLiteral("1"), QStringLiteral("2")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("3.2.4"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        single(QStringLiteral("1.png"))),
    def(QStringLiteral("3.2.5"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        single(QStringLiteral("1.png"))),
    def(QStringLiteral("3.2.11"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("3.3.1"), ExerciseRunnerKind::Paint, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("3.3.2"), ExerciseRunnerKind::Paint, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("3.3.3"), ExerciseRunnerKind::Paint, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("4.1.1"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::DoneTimeOrHlp,
        single(QStringLiteral("1.png"))),
    def(QStringLiteral("4.1.2"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::OrHlpBallsRow,
        steps({QStringLiteral("Пример"), QStringLiteral("1")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("4.1.4"), ExerciseRunnerKind::Remember2, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("4.1.5"), ExerciseRunnerKind::Cards, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("4.1.6"), ExerciseRunnerKind::Cards, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("4.1.7"), ExerciseRunnerKind::Remember, ExerciseProtocolKind::NumberedDoneTime,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")})),
    def(QStringLiteral("4.1.8"), ExerciseRunnerKind::Cards, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("4.2.1"), ExerciseRunnerKind::Digits, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("4.2.2"), ExerciseRunnerKind::WordsLearning, ExerciseProtocolKind::DoneTimeOrHlp),
    def(QStringLiteral("5.1.1"), ExerciseRunnerKind::E511, ExerciseProtocolKind::OrHlpBallsRow),
    def(QStringLiteral("5.2.1"), ExerciseRunnerKind::E521, ExerciseProtocolKind::DoneTimeOrHlp,
        steps({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4")})),
    def(QStringLiteral("5.3.1"), ExerciseRunnerKind::OnlyPicture, ExerciseProtocolKind::DoneTimeOrHlp,
        steps({QStringLiteral("1"), QStringLiteral("2")}, QStringLiteral("%1.png"))),
    def(QStringLiteral("5.4.2"), ExerciseRunnerKind::Wolf, ExerciseProtocolKind::DoneTimeOrHlp),
};

} // namespace

const ExerciseDefinition *ExerciseConfig::find(const QString &exerciseId) {
    const QString normalized = exerciseId.trimmed();
    for (const ExerciseDefinition &item : kDefinitions) {
        if (item.id == normalized) {
            return &item;
        }
    }
    return nullptr;
}

bool ExerciseConfig::isRunnable(const QString &exerciseId) {
    const ExerciseDefinition *item = find(exerciseId);
    return item && item->runner != ExerciseRunnerKind::NotImplemented;
}

QString ExerciseConfig::unsupportedMessage(const QString &exerciseId) {
    const ExerciseDefinition *item = find(exerciseId);
    if (!item) {
        return QStringLiteral("Упражнение «%1» отсутствует в оригинальной программе.").arg(exerciseId);
    }
    return QStringLiteral("Упражнение «%1» пока не поддерживается.").arg(exerciseId);
}
