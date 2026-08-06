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
    const QString &name = QString(),
    const QString &selectFile = QString()) {
    PuzzleSpriteDef sprite;
    sprite.file = file;
    sprite.x = x;
    sprite.y = y;
    sprite.targetX = tx;
    sprite.targetY = ty;
    sprite.name = name;
    sprite.selectFile = selectFile;
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
    if (root.contains(QStringLiteral("template2"))) {
        const QJsonObject tmpl = root.value(QStringLiteral("template2")).toObject();
        layout->template2File = tmpl.value(QStringLiteral("file")).toString();
        layout->template2X = tmpl.value(QStringLiteral("x")).toInt();
        layout->template2Y = tmpl.value(QStringLiteral("y")).toInt();
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
        sprite.selectFile = obj.value(QStringLiteral("selectFile")).toString();
        sprite.name = obj.value(QStringLiteral("name")).toString();
        sprite.clickable = obj.value(QStringLiteral("clickable")).toBool(true);
        sprite.closed = obj.value(QStringLiteral("closed")).toBool(false);
        sprite.returnable = obj.value(QStringLiteral("returnable")).toBool(false);
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
    // 1.21 «Сложи круг»: детали слева, трафарет @ (700,190); t/et переключаются опциями.
    if (exerciseId == QStringLiteral("1.21")) {
        layout->rotateAllowed = true;
        layout->selectMode = false;
        layout->showTemplate = true;
        layout->templateX = 700;
        layout->templateY = 190;
        const QString slug = stepId;
        layout->templateFile = QStringLiteral("t") + slug + QStringLiteral(".png");

        int pieceCount = 0;
        if (slug == QStringLiteral("2А") || slug == QStringLiteral("2Б")) {
            pieceCount = 2;
        } else if (slug == QStringLiteral("3А") || slug == QStringLiteral("3Б")) {
            pieceCount = 3;
        } else if (slug == QStringLiteral("4А") || slug == QStringLiteral("4Б")) {
            pieceCount = 4;
        } else if (slug == QStringLiteral("5А") || slug == QStringLiteral("5Б")) {
            pieceCount = 5;
        } else if (slug == QStringLiteral("6А") || slug == QStringLiteral("6Б")) {
            pieceCount = 6;
        }
        if (pieceCount <= 0) {
            return false;
        }
        for (int i = 1; i <= pieceCount; ++i) {
            int x = 100;
            int y = 100;
            if (i >= 3 && i <= 4) {
                x = 120;
                y = 120;
            } else if (i >= 5) {
                x = 130;
                y = 130;
            }
            addSprite(layout, slug + QString::number(i) + QStringLiteral(".png"), x, y);
        }
        return true;
    }

    // 1.22: координаты как puzzles.cs (Круг / Квадрат / Пирамида).
    if (exerciseId == QStringLiteral("1.22")) {
        layout->rotateAllowed = false;
        layout->selectMode = true;
        layout->showTemplate = true;
        layout->templateX = 800;
        layout->templateY = 150;
        if (stepId == QStringLiteral("Круг")) {
            layout->templateFile = QStringLiteral("tкруг.png");
            static const struct {
                const char *file;
                const char *select;
                int x;
                int y;
            } kSprites[] = {
                {"круг1.png", "вкруг1.png", 130, 100},
                {"круг2.png", "вкруг2.png", 100, 280},
                {"круг3.png", "вкруг3.png", 150, 500},
                {"круг4.png", "вкруг4.png", 100, 600},
                {"круг5.png", "вкруг5.png", 330, 600},
                {"круг6.png", "вкруг6.png", 500, 600},
            };
            for (const auto &item : kSprites) {
                addSprite(
                    layout,
                    QString::fromUtf8(item.file),
                    item.x,
                    item.y,
                    -1,
                    -1,
                    QString(),
                    QString::fromUtf8(item.select));
            }
            return true;
        }
        if (stepId == QStringLiteral("Квадрат")) {
            layout->templateFile = QStringLiteral("tквадрат.png");
            static const struct {
                const char *file;
                const char *select;
                int x;
                int y;
            } kSprites[] = {
                {"квадрат1.png", "вквадрат1.png", 100, 100},
                {"квадрат2.png", "вквадрат2.png", 300, 80},
                {"квадрат3.png", "вквадрат3.png", 400, 380},
                {"квадрат4.png", "вквадрат4.png", 100, 680},
                {"квадрат5.png", "вквадрат5.png", 300, 720},
            };
            for (const auto &item : kSprites) {
                addSprite(
                    layout,
                    QString::fromUtf8(item.file),
                    item.x,
                    item.y,
                    -1,
                    -1,
                    QString(),
                    QString::fromUtf8(item.select));
            }
            return true;
        }
        if (stepId == QStringLiteral("Пирамида")) {
            layout->templateFile = QStringLiteral("tпирамида.png");
            static const struct {
                const char *file;
                const char *select;
                int x;
                int y;
            } kSprites[] = {
                {"пирамида1.png", "впирамида1.png", 100, 100},
                {"пирамида2.png", "впирамида2.png", 300, 80},
                {"пирамида3.png", "впирамида3.png", 400, 380},
                {"пирамида4.png", "впирамида4.png", 100, 680},
                {"пирамида5.png", "впирамида5.png", 300, 520},
                {"пирамида6.png", "впирамида6.png", 550, 700},
                {"пирамида7.png", "впирамида7.png", 750, 700},
            };
            for (const auto &item : kSprites) {
                addSprite(
                    layout,
                    QString::fromUtf8(item.file),
                    item.x,
                    item.y,
                    -1,
                    -1,
                    QString(),
                    QString::fromUtf8(item.select));
            }
            return true;
        }
        return false;
    }

    // 1.14 шаг 2: детали справа от подсказки (200..671) и пустого прямоугольника (700..1176).
    if (exerciseId == QStringLiteral("1.14") && stepId == QStringLiteral("2")) {
        layout->rotateAllowed = true;
        layout->showTemplate = false;
        layout->templateFile = QStringLiteral("et2.png");
        layout->templateX = 700;
        layout->templateY = 140;
        static const struct {
            const char *file;
            int x;
            int y;
        } kSprites[] = {
            {"1.png", 1220, 230},
            {"2.png", 1235, 215},
            {"3.png", 1430, 310},
            {"4.png", 1440, 440},
            {"5.png", 1260, 260},
            {"6.png", 1455, 450},
            {"7.png", 1340, 261},
            {"8.png", 1290, 320},
            {"9.png", 1490, 310},
            {"10.png", 1385, 255},
            {"11.png", 1375, 355},
        };
        for (const auto &item : kSprites) {
            addSprite(layout, QString::fromUtf8(item.file), item.x, item.y);
        }
        return true;
    }

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

    // 1.22 / 1.21: координаты из puzzles.cs.
    if (exerciseId == QStringLiteral("1.22") || exerciseId == QStringLiteral("1.21")
        || (exerciseId == QStringLiteral("1.14") && stepId == QStringLiteral("2"))) {
        return builtinLayout(exerciseId, stepId, layout);
    }

    const QString dir = ExerciseAssets::exerciseDir(exerciseId);
    if (dir.isEmpty()) {
        return builtinLayout(exerciseId, stepId, layout) || autoGridLayout(exerciseId, stepId, layout);
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
