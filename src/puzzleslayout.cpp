#include "puzzlelayout.h"

#include "exerciseassets.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {

void addSprite(
    PuzzleLayout *layout,
    const QString &file,
    int x,
    int y,
    int tx = -1,
    int ty = -1,
    const QString &name = QString()) {
    PuzzleSpriteDef sprite;
    sprite.file = file;
    sprite.x = x;
    sprite.y = y;
    sprite.targetX = tx;
    sprite.targetY = ty;
    sprite.name = name;
    layout->sprites.append(sprite);
}

bool loadLayoutJson(const QString &path, PuzzleLayout *layout) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }
    const QJsonObject root = doc.object();
    layout->rotateAllowed = root.value(QStringLiteral("rotate")).toBool(false);
    layout->selectMode = root.value(QStringLiteral("select")).toBool(false);
    layout->showTemplate = root.value(QStringLiteral("showTemplate")).toBool(true);
    if (root.contains(QStringLiteral("background"))) {
        layout->backgroundFile = root.value(QStringLiteral("background")).toString();
    }
    if (root.contains(QStringLiteral("template"))) {
        const QJsonObject tmpl = root.value(QStringLiteral("template")).toObject();
        layout->templateFile = tmpl.value(QStringLiteral("file")).toString();
        layout->templateX = tmpl.value(QStringLiteral("x")).toInt();
        layout->templateY = tmpl.value(QStringLiteral("y")).toInt();
    }
    const QJsonArray sprites = root.value(QStringLiteral("sprites")).toArray();
    for (const QJsonValue &value : sprites) {
        const QJsonObject obj = value.toObject();
        PuzzleSpriteDef sprite;
        sprite.file = obj.value(QStringLiteral("file")).toString();
        sprite.x = obj.value(QStringLiteral("x")).toInt();
        sprite.y = obj.value(QStringLiteral("y")).toInt();
        sprite.targetX = obj.value(QStringLiteral("tx")).toInt(-1);
        sprite.targetY = obj.value(QStringLiteral("ty")).toInt(-1);
        sprite.name = obj.value(QStringLiteral("name")).toString();
        sprite.clickable = obj.value(QStringLiteral("clickable")).toBool(true);
        sprite.closed = obj.value(QStringLiteral("closed")).toBool(false);
        sprite.closedFile = obj.value(QStringLiteral("closedFile")).toString();
        sprite.openFile = obj.value(QStringLiteral("openFile")).toString();
        layout->sprites.append(sprite);
    }
    return !layout->sprites.isEmpty() || !layout->templateFile.isEmpty();
}

bool autoGridLayout(const QString &exerciseId, const QString &stepId, PuzzleLayout *layout) {
    const QString dirPath = ExerciseAssets::exerciseDir(exerciseId);
    if (dirPath.isEmpty()) {
        return false;
    }

    QDir dir(dirPath);
    const QStringList pngs = dir.entryList({QStringLiteral("*.png")}, QDir::Files, QDir::Name);
    if (pngs.isEmpty()) {
        return false;
    }

    QStringList spriteFiles;
    const QRegularExpression numbered(QStringLiteral("^(\\d+)\\.png$"));
    for (const QString &name : pngs) {
        QRegularExpressionMatch match = numbered.match(name);
        if (match.hasMatch()) {
            spriteFiles.append(name);
        }
    }

    if (spriteFiles.isEmpty()) {
        for (const QString &name : pngs) {
            if (!name.startsWith(QStringLiteral("traf"), Qt::CaseInsensitive)
                && !name.startsWith(QStringLiteral("f"), Qt::CaseInsensitive)
                && !name.startsWith(QStringLiteral("et"), Qt::CaseInsensitive)
                && name != QStringLiteral("close.png")
                && name != QStringLiteral("hide.png")) {
                spriteFiles.append(name);
            }
        }
    }

    if (spriteFiles.isEmpty()) {
        return false;
    }

    const QString trafCandidates[] = {
        QStringLiteral("traf%1.png").arg(stepId),
        QStringLiteral("traf%1.png").arg(stepId.toLower()),
        QStringLiteral("fone%1.png").arg(stepId),
        QStringLiteral("f%1.png").arg(stepId),
        QStringLiteral("f1.png"),
    };
    for (const QString &candidate : trafCandidates) {
        if (dir.exists(candidate)) {
            layout->templateFile = candidate;
            layout->templateX = 500;
            layout->templateY = 70;
            break;
        }
    }

    int linex = 1000;
    int liney = 50;
    int col = 0;
    for (const QString &file : spriteFiles) {
        addSprite(layout, file, linex, liney);
        linex += 150;
        ++col;
        if (col >= 4) {
            col = 0;
            linex = 1000;
            liney += 180;
        }
    }
    return !layout->sprites.isEmpty();
}

bool builtinLayout(const QString &exerciseId, const QString &stepId, PuzzleLayout *layout) {
    if (exerciseId == QStringLiteral("1.11")) {
        if (stepId == QStringLiteral("1")) {
            layout->templateFile = QStringLiteral("f1.png");
            layout->templateX = 500;
            layout->templateY = 70;
            return true;
        }
        if (stepId == QStringLiteral("2")) {
            layout->templateFile = QStringLiteral("traf2.png");
            layout->templateX = 10;
            layout->templateY = 20;
            // Сетка 4×4: клетки на 50px ниже, шаг уменьшен в 1.2 раза.
            const int xs[] = {1000, 1125, 1292, 1417};
            const int ys[] = {85, 258, 475, 633};
            int index = 21;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    addSprite(layout, QString::number(index) + QStringLiteral(".png"), xs[col], ys[row]);
                    ++index;
                }
            }
            return true;
        }
    }

    if (exerciseId == QStringLiteral("3.1.7")) {
        int count = 1;
        int linex = 600;
        int liney = 50;
        const QString dirPath = ExerciseAssets::exerciseDir(exerciseId);
        QDir dir(dirPath);
        while (dir.exists(QString::number(count) + QStringLiteral(".png"))) {
            addSprite(layout, QString::number(count) + QStringLiteral(".png"), linex, liney);
            linex += 140;
            if (count % 8 == 0) {
                linex = 600;
                liney += 185;
            }
            ++count;
        }
        return count > 1;
    }

    if (exerciseId == QStringLiteral("2.11")) {
        layout->templateFile = QString();
        const QString prefix = stepId == QStringLiteral("2") ? QStringLiteral("2") : QStringLiteral("1");
        const int baseX = stepId == QStringLiteral("2") ? 200 : 370;
        for (int i = 1; i <= (stepId == QStringLiteral("2") ? 5 : 4); ++i) {
            addSprite(
                layout,
                prefix + QString::number(i) + QStringLiteral(".png"),
                27 + baseX + (i - 1) * 290,
                111);
        }
        PuzzleSpriteDef cover1;
        cover1.file = QStringLiteral("close.png");
        cover1.x = 500;
        cover1.y = 600;
        cover1.name = QStringLiteral("cover1");
        cover1.clickable = true;
        cover1.closed = true;
        cover1.closedFile = QStringLiteral("close.png");
        cover1.openFile = stepId == QStringLiteral("2") ? QStringLiteral("26.png")
                                                        : QStringLiteral("15.png");
        layout->sprites.append(cover1);

        PuzzleSpriteDef cover2;
        cover2.file = QStringLiteral("close.png");
        cover2.x = 800;
        cover2.y = 600;
        cover2.name = QStringLiteral("cover2");
        cover2.clickable = true;
        cover2.closed = true;
        cover2.closedFile = QStringLiteral("close.png");
        cover2.openFile = stepId == QStringLiteral("2") ? QStringLiteral("27.png")
                                                        : QStringLiteral("16.png");
        layout->sprites.append(cover2);
        return true;
    }

    if (exerciseId == QStringLiteral("1.19") || exerciseId == QStringLiteral("1.20")) {
        layout->rotateAllowed = true;
        layout->showTemplate = true;
    }

    return false;
}

QString layoutFileName(const QString &stepId) {
    QString safe = stepId;
    safe.replace(QLatin1Char('/'), QLatin1Char('_'));
    safe.replace(QLatin1Char('\\'), QLatin1Char('_'));
    safe.replace(QLatin1Char(':'), QLatin1Char('_'));
    return QStringLiteral("puzzle_%1.json").arg(safe);
}

} // namespace

bool loadPuzzleLayout(const QString &exerciseId, const QString &stepId, PuzzleLayout *layout) {
    if (!layout) {
        return false;
    }
    *layout = PuzzleLayout();

    const QString dir = ExerciseAssets::exerciseDir(exerciseId);
    if (dir.isEmpty()) {
        return false;
    }

    const QStringList candidates = {
        dir + QLatin1Char('/') + layoutFileName(stepId),
        dir + QStringLiteral("/puzzle_default.json"),
    };
    for (const QString &jsonPath : candidates) {
        if (QFile::exists(jsonPath) && loadLayoutJson(jsonPath, layout)) {
            return true;
        }
    }

    if (builtinLayout(exerciseId, stepId, layout)) {
        return true;
    }

    return autoGridLayout(exerciseId, stepId, layout);
}
