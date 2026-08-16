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

bool builtinLayout(const QString &exerciseId, const QString &stepId, PuzzleLayout *layout, const QString &aparam) {
    // 1.28 «Восприятие величины»: как puzzles.cs (traf1 / traf22+traf2+детали).
    // ТЗ 15.5: все картинки на 100 px ниже.
    if (exerciseId == QStringLiteral("1.28")) {
        constexpr int kDy = 100;
        layout->rotateAllowed = false;
        layout->selectMode = false;
        layout->showTemplate = true;
        if (stepId.trimmed() == QStringLiteral("1")) {
            layout->templateFile = QStringLiteral("traf1.png");
            layout->templateX = 500;
            layout->templateY = 20 + kDy;
            return true;
        }
        if (stepId.trimmed() == QStringLiteral("2")) {
            layout->templateFile = QStringLiteral("traf22.png");
            layout->templateX = 900;
            layout->templateY = 20 + kDy;
            layout->template2File = QStringLiteral("traf2.png");
            layout->template2X = 400;
            layout->template2Y = 20 + kDy;
            addSprite(layout, QStringLiteral("21.png"), 945, 72 + kDy, -1, -1, QStringLiteral("smalltree"));
            addSprite(layout, QStringLiteral("22.png"), 1097, 61 + kDy, -1, -1, QStringLiteral("bigmashroom"));
            addSprite(layout, QStringLiteral("23.png"), 909, 258 + kDy, -1, -1, QStringLiteral("bighouse"));
            addSprite(layout, QStringLiteral("24.png"), 1119, 346 + kDy, -1, -1, QStringLiteral("smallcar"));
            addSprite(layout, QStringLiteral("25.png"), 903, 572 + kDy, -1, -1, QStringLiteral("bigcar"));
            addSprite(layout, QStringLiteral("26.png"), 1089, 503 + kDy, -1, -1, QStringLiteral("bigtree"));
            addSprite(layout, QStringLiteral("27.png"), 937, 796 + kDy, -1, -1, QStringLiteral("smallhouse"));
            addSprite(layout, QStringLiteral("28.png"), 1124, 813 + kDy, -1, -1, QStringLiteral("smallmashroom"));
            return true;
        }
        return false;
    }

    // 1.21 «Сложи круг»: трафарет на высоте подсказки (y=250);
    // детали — правая половина экрана, на 200 px ниже исходных.
    if (exerciseId == QStringLiteral("1.21")) {
        layout->rotateAllowed = true;
        layout->selectMode = false;
        layout->showTemplate = true;
        layout->templateX = 1380;
        layout->templateY = 250; // как pexample.Top
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
        constexpr int kRightShift = 830; // 900 − 70 влево
        constexpr int kDownShift = 200;
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
            addSprite(
                layout,
                slug + QString::number(i) + QStringLiteral(".png"),
                x + kRightShift,
                y + kDownShift);
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
        // Одна высота с подсказкой p2 (hintY=230).
        layout->templateY = 230;
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
            // Опустить поле/фрагменты ~100px под «Стоп» (не ниже низа 1080).
            layout->templateFile = QStringLiteral("traf2.png");
            layout->templateX = 10;
            layout->templateY = 120;
            const int xs[] = {1000, 1150, 1350, 1500};
            const int ys[] = {135, 350, 610, 800};
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
        // 20.3: все картинки на 100 px ниже (оригинал y=111 / 600).
        constexpr int kDown = 100;
        for (int i = 1; i <= (stepId == QStringLiteral("2") ? 5 : 4); ++i) {
            addSprite(
                layout,
                prefix + QString::number(i) + QStringLiteral(".png"),
                27 + baseX + (i - 1) * 290,
                111 + kDown);
        }
        PuzzleSpriteDef cover1;
        cover1.file = QStringLiteral("close.png");
        cover1.x = 500;
        cover1.y = 600 + kDown;
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
        cover2.y = 600 + kDown;
        cover2.name = QStringLiteral("cover2");
        cover2.clickable = true;
        cover2.closed = true;
        cover2.closedFile = QStringLiteral("close.png");
        cover2.openFile = stepId == QStringLiteral("2") ? QStringLiteral("27.png")
                                                        : QStringLiteral("16.png");
        layout->sprites.append(cover2);
        return true;
    }

    if (exerciseId == QStringLiteral("2.12")) {
        layout->showTemplate = false;
        layout->templateFile = QString();
        // 21.3: все картинки на 100 px ниже (оригинал y=200).
        constexpr int kDown = 100;
        if (stepId == QStringLiteral("1")) {
            addSprite(layout, QStringLiteral("11.png"), 300, 200 + kDown);
            addSprite(layout, QStringLiteral("12.png"), 600, 200 + kDown);
            addSprite(layout, QStringLiteral("13.png"), 900, 200 + kDown);
            addSprite(layout, QStringLiteral("14.png"), 1200, 200 + kDown);
            return true;
        }
        if (stepId == QStringLiteral("2")) {
            addSprite(layout, QStringLiteral("21.png"), 100, 200 + kDown);
            addSprite(layout, QStringLiteral("22.png"), 400, 200 + kDown);
            addSprite(layout, QStringLiteral("23.png"), 700, 200 + kDown);
            addSprite(layout, QStringLiteral("24.png"), 1000, 200 + kDown);
            addSprite(layout, QStringLiteral("25.png"), 1300, 200 + kDown);
            return true;
        }
        return false;
    }

    if (exerciseId == QStringLiteral("3.1.8")) {
        layout->rotateAllowed = false;
        layout->showTemplate = true;
        // 23.5: во время сессии −50 по Y и +50 по X (оба задания, один/два экрана).
        constexpr int kDx = 50;
        constexpr int kDy = 50;
        if (stepId == QStringLiteral("1")) {
            layout->templateFile = QStringLiteral("traf.png");
            layout->templateX = 425 + kDx;
            layout->templateY = 65 + kDy;
            static const struct {
                const char *file;
                int x;
                int y;
            } kSprites[] = {
                {"1.png", 0, 75},
                {"2.png", 200, 75},
                {"5.png", 0, 275},
                {"6.png", 200, 275},
                {"3.png", 0, 475},
                {"4.png", 200, 475},
                {"15.png", 0, 675},
                {"16.png", 200, 675},
                {"7.png", 1300, 75},
                {"8.png", 1500, 75},
                {"9.png", 1300, 275},
                {"10.png", 1500, 275},
                {"11.png", 1300, 475},
                {"12.png", 1500, 475},
                {"13.png", 1300, 675},
                {"14.png", 1500, 675},
            };
            for (const auto &item : kSprites) {
                addSprite(layout, QString::fromUtf8(item.file), item.x + kDx, item.y + kDy);
            }
            return true;
        }
        if (stepId == QStringLiteral("2")) {
            layout->templateFile = QStringLiteral("traf2.png");
            layout->templateX = 0 + kDx;
            layout->templateY = 70 + kDy;
            static const char *const kRow1[] = {
                "1.png", "5.png", "2.png", "3.png", "16.png", "4.png", "14.png", "6.png"};
            static const char *const kRow2[] = {
                "7.png", "8.png", "9.png", "10.png", "11.png", "12.png", "13.png", "14.png"};
            for (int k = 0; k < 8; ++k) {
                addSprite(layout, QString::fromUtf8(kRow1[k]), 27 + 200 * k + kDx, 510 + kDy);
            }
            for (int k = 0; k < 8; ++k) {
                addSprite(layout, QString::fromUtf8(kRow2[k]), 27 + 200 * k + kDx, 710 + kDy);
            }
            return true;
        }
        return false;
    }

    if (exerciseId == QStringLiteral("3.1.15")) {
        layout->showTemplate = true;
        layout->templateFile = QStringLiteral("traf.png");
        // 24.2: опустить задание на 70 px (оригинал templateY=30, sprites y=830).
        constexpr int kDy = 70;
        layout->templateX = 425;
        layout->templateY = 30 + kDy;
        addSprite(layout, QStringLiteral("1.png"), 400, 830 + kDy);
        addSprite(layout, QStringLiteral("2.png"), 550, 830 + kDy);
        addSprite(layout, QStringLiteral("3.png"), 700, 830 + kDy);
        addSprite(layout, QStringLiteral("4.png"), 850, 830 + kDy);
        return true;
    }

    if (exerciseId == QStringLiteral("3.1.16")) {
        layout->showTemplate = true;
        const int step = stepId.toInt();
        const QString mode = aparam.trimmed().isEmpty() ? QStringLiteral("1") : aparam.trimmed();
        if (step == 1) {
            // 25.5: задания 1–2 — опустить на 50 px (оригинал dy=110).
            const int dy = 110 + 50;
            layout->templateFile = QStringLiteral("traf1.png");
            layout->templateX = 750;
            layout->templateY = dy;
            addSprite(layout, QStringLiteral("11.png"), 457, 1 + dy);
            addSprite(layout, QStringLiteral("12.png"), 457, 248 + dy);
            addSprite(layout, QStringLiteral("13.png"), 457, 500 + dy);
            return true;
        }
        if (step == 2) {
            const int dy = 110 + 50;
            layout->templateFile = QStringLiteral("traf2.png");
            layout->templateX = 750;
            layout->templateY = dy;
            addSprite(layout, QStringLiteral("21.png"), 457, 1 + dy);
            addSprite(layout, QStringLiteral("22.png"), 457, 248 + dy);
            addSprite(layout, QStringLiteral("23.png"), 457, 500 + dy);
            return true;
        }
        if (step < 3 || step > 7) {
            return false;
        }
        // 25.6: ниже Стоп (≥70+50) и +100 px вправо.
        constexpr int kDx = 100;
        constexpr int kBelowStop = 70 + 50;
        const int dy = 40;
        const int y0 = kBelowStop; // было templateY 0/40 — перекрывало Стоп
        layout->templateFile = QStringLiteral("traf%1.png").arg(step);
        layout->templateX = 650 + kDx;
        layout->templateY = y0 + ((step == 3) ? 0 : dy);
        auto sx = [kDx](int x) { return x + kDx; };
        auto sy = [y0](int y) { return y + y0; };
        if (mode == QStringLiteral("1")) {
            // puzzles.cs aparam=="1": все фрагменты заданий 3–7.
            addSprite(layout, QStringLiteral("31.png"), sx(457), sy(43));
            addSprite(layout, QStringLiteral("32.png"), sx(457), sy(248 + dy));
            addSprite(layout, QStringLiteral("41.png"), sx(257), sy(1 + dy));
            addSprite(layout, QStringLiteral("42.png"), sx(257), sy(248 + dy));
            addSprite(layout, QStringLiteral("51.png"), sx(0), sy(1 + dy));
            addSprite(layout, QStringLiteral("52.png"), sx(0), sy(248 + dy));
            addSprite(layout, QStringLiteral("61.png"), sx(0), sy(500 + dy));
            addSprite(layout, QStringLiteral("62.png"), sx(257), sy(500 + dy));
            addSprite(layout, QStringLiteral("71.png"), sx(457), sy(500 + dy));
            addSprite(layout, QStringLiteral("72.png"), sx(0), sy(750 + dy));
            return true;
        }
        // aparam == "2": только фрагменты текущего задания.
        if (step == 3) {
            addSprite(layout, QStringLiteral("31.png"), sx(457), sy(51 + dy));
            addSprite(layout, QStringLiteral("32.png"), sx(457), sy(248 + dy));
            return true;
        }
        if (step == 4) {
            addSprite(layout, QStringLiteral("41.png"), sx(457), sy(51));
            addSprite(layout, QStringLiteral("42.png"), sx(457), sy(248 + dy));
            return true;
        }
        if (step == 5) {
            addSprite(layout, QStringLiteral("51.png"), sx(457), sy(1 + dy));
            addSprite(layout, QStringLiteral("52.png"), sx(457), sy(248 + dy));
            return true;
        }
        if (step == 6) {
            addSprite(layout, QStringLiteral("61.png"), sx(457), sy(51));
            addSprite(layout, QStringLiteral("62.png"), sx(457), sy(248 + dy));
            return true;
        }
        addSprite(layout, QStringLiteral("71.png"), sx(457), sy(1 + dy));
        addSprite(layout, QStringLiteral("72.png"), sx(457), sy(248 + dy));
        return true;
    }

    if (exerciseId == QStringLiteral("3.1.23")) {
        layout->showTemplate = true;
        layout->templateFile = QStringLiteral("fone.png");
        layout->templateX = 600;
        // 28.1: Стоп@70 → трафарет и фрагменты не выше 70+50.
        constexpr int kBelowStop = 70 + 50;
        constexpr int kDy = kBelowStop - 50; // исходный traf.y=50
        layout->templateY = kBelowStop;
        addSprite(layout, QStringLiteral("11.png"), 600, 700 + kDy);
        addSprite(layout, QStringLiteral("12.png"), 720, 700 + kDy);
        addSprite(layout, QStringLiteral("13.png"), 840, 700 + kDy);
        addSprite(layout, QStringLiteral("14.png"), 960, 700 + kDy);
        addSprite(layout, QStringLiteral("15.png"), 600, 820 + kDy);
        addSprite(layout, QStringLiteral("16.png"), 720, 820 + kDy);
        addSprite(layout, QStringLiteral("17.png"), 840, 820 + kDy);
        addSprite(layout, QStringLiteral("18.png"), 960, 820 + kDy);
        return true;
    }

    if (exerciseId == QStringLiteral("1.19")) {
        layout->rotateAllowed = true;
        layout->selectMode = false;
        layout->showTemplate = true;
        const QString step = stepId.trimmed();
        // Горизонтальные: Леопард/Дом — трафарет 1150×310;
        // вертикальные: Матрешка/Мишка — 1250×240 (+100 вправо).
        // Подсказка слева (50×…); фрагменты между подсказкой и трафаретом.
        const bool horizontal =
            step == QStringLiteral("Леопард 3") || step == QStringLiteral("Дом 4");
        const int templateY = horizontal ? 310 : 240;
        layout->templateX = horizontal ? 1150 : 1250;
        layout->templateY = templateY;
        if (step == QStringLiteral("Матрешка 2")) {
            layout->templateFile = QStringLiteral("tматрешка2.png");
            addSprite(layout, QStringLiteral("матрешка21.png"), 600, 240);
            addSprite(layout, QStringLiteral("матрешка22.png"), 720, 280);
            return true;
        }
        if (step == QStringLiteral("Мишка 4")) {
            layout->templateFile = QStringLiteral("tмишка4.png");
            addSprite(layout, QStringLiteral("мишка41.png"), 580, 240);
            addSprite(layout, QStringLiteral("мишка42.png"), 700, 260);
            addSprite(layout, QStringLiteral("мишка43.png"), 820, 280);
            addSprite(layout, QStringLiteral("мишка44.png"), 700, 400);
            return true;
        }
        if (step == QStringLiteral("Дом 4")) {
            layout->templateFile = QStringLiteral("tдом4.png");
            addSprite(layout, QStringLiteral("дом41.png"), 580, 310);
            addSprite(layout, QStringLiteral("дом42.png"), 720, 330);
            addSprite(layout, QStringLiteral("дом43.png"), 860, 350);
            addSprite(layout, QStringLiteral("дом44.png"), 720, 430);
            return true;
        }
        if (step == QStringLiteral("Леопард 3")) {
            layout->templateFile = QStringLiteral("tлеопард3.png");
            addSprite(layout, QStringLiteral("леопард31.png"), 600, 310);
            addSprite(layout, QStringLiteral("леопард32.png"), 760, 340);
            addSprite(layout, QStringLiteral("леопард33.png"), 900, 370);
            return true;
        }
        return false;
    }

    if (exerciseId == QStringLiteral("1.20")) {
        layout->rotateAllowed = true;
        layout->selectMode = false;
        layout->showTemplate = true;
        // Трафарет 1260×300; подсказка 40×300; фрагменты между ними (+150 вправо, +100 вниз).
        layout->templateX = 1260;
        layout->templateY = 300;
        const QString step = stepId.trimmed();
        if (step == QStringLiteral("Мяч 2")) {
            layout->templateFile = QStringLiteral("tмяч2.png");
            addSprite(layout, QStringLiteral("мяч21.png"), 570, 300);
            addSprite(layout, QStringLiteral("мяч22.png"), 770, 330);
            return true;
        }
        if (step == QStringLiteral("Дом 3")) {
            layout->templateFile = QStringLiteral("tдом3.png");
            addSprite(layout, QStringLiteral("дом31.png"), 530, 300);
            addSprite(layout, QStringLiteral("дом32.png"), 710, 320);
            addSprite(layout, QStringLiteral("дом33.png"), 890, 340);
            return true;
        }
        if (step == QStringLiteral("Мишка 4")) {
            layout->templateFile = QStringLiteral("tмишка4.png");
            addSprite(layout, QStringLiteral("мишка41.png"), 500, 300);
            addSprite(layout, QStringLiteral("мишка42.png"), 670, 320);
            addSprite(layout, QStringLiteral("мишка43.png"), 840, 340);
            addSprite(layout, QStringLiteral("мишка44.png"), 670, 460);
            return true;
        }
        if (step == QStringLiteral("Машинка 5")) {
            layout->templateFile = QStringLiteral("tмашинка5.png");
            addSprite(layout, QStringLiteral("машинка51.png"), 470, 300);
            addSprite(layout, QStringLiteral("машинка52.png"), 620, 320);
            addSprite(layout, QStringLiteral("машинка53.png"), 770, 340);
            addSprite(layout, QStringLiteral("машинка54.png"), 920, 320, 953, 354);
            addSprite(layout, QStringLiteral("машинка55.png"), 690, 460);
            return true;
        }
        if (step == QStringLiteral("Чайник 6")) {
            layout->templateFile = QStringLiteral("tчайник6.png");
            addSprite(layout, QStringLiteral("чайник61.png"), 450, 300);
            addSprite(layout, QStringLiteral("чайник62.png"), 580, 315);
            addSprite(layout, QStringLiteral("чайник63.png"), 710, 330);
            addSprite(layout, QStringLiteral("чайник64.png"), 840, 345);
            addSprite(layout, QStringLiteral("чайник65.png"), 970, 320);
            addSprite(layout, QStringLiteral("чайник66.png"), 690, 460);
            return true;
        }
        return false;
    }

    return false;
}

QString layoutFileName(const QString &stepId) {
    QString safe = stepId.trimmed();
    safe.replace(QLatin1Char(' '), QLatin1Char('_'));
    safe.replace(QLatin1Char('/'), QLatin1Char('_'));
    safe.replace(QLatin1Char('\\'), QLatin1Char('_'));
    safe.replace(QLatin1Char(':'), QLatin1Char('_'));
    return QStringLiteral("puzzle_%1.json").arg(safe);
}

} // namespace

bool loadPuzzleLayout(
    const QString &exerciseId,
    const QString &stepId,
    PuzzleLayout *layout,
    const QString &aparam) {
    if (!layout) {
        return false;
    }
    *layout = PuzzleLayout();

    // 1.19 / 1.20 / 1.21 / 1.22 / 1.14-2 / 1.28 / 2.11 / 2.12 / 3.1.*: координаты из puzzles.cs.
    if (exerciseId == QStringLiteral("1.22") || exerciseId == QStringLiteral("1.21")
        || exerciseId == QStringLiteral("1.20") || exerciseId == QStringLiteral("1.19")
        || (exerciseId == QStringLiteral("1.14") && stepId == QStringLiteral("2"))
        || exerciseId == QStringLiteral("1.28")
        || exerciseId == QStringLiteral("2.11") || exerciseId == QStringLiteral("2.12")
        || exerciseId == QStringLiteral("3.1.8") || exerciseId == QStringLiteral("3.1.15")
        || exerciseId == QStringLiteral("3.1.16") || exerciseId == QStringLiteral("3.1.23")) {
        return builtinLayout(exerciseId, stepId, layout, aparam);
    }

    bool loaded = false;
    const QString dir = ExerciseAssets::exerciseDir(exerciseId);
    if (dir.isEmpty()) {
        loaded = builtinLayout(exerciseId, stepId, layout, aparam)
            || autoGridLayout(exerciseId, stepId, layout);
    } else {
        const QStringList candidates = {
            dir + QLatin1Char('/') + layoutFileName(stepId),
            dir + QStringLiteral("/puzzle_default.json"),
        };
        for (const QString &jsonPath : candidates) {
            if (QFile::exists(jsonPath) && loadLayoutJson(jsonPath, layout)) {
                loaded = true;
                break;
            }
        }
        if (!loaded) {
            loaded = builtinLayout(exerciseId, stepId, layout, aparam)
                || autoGridLayout(exerciseId, stepId, layout);
        }
    }

    // 1.24: опустить трафарет и фрагменты на 70 px (до и во время выполнения).
    // 1.29: опустить задание ниже кнопок (кроме Стоп) на 50 px (ТЗ 16.1).
    // 3.1.24 задание 2: ниже Стоп@70 ещё на 50 px (было dy=80 → нужно ≥120).
    if (loaded
        && (exerciseId == QStringLiteral("1.24") || exerciseId == QStringLiteral("1.29"))) {
        const int kDy = exerciseId == QStringLiteral("1.29") ? 50 : 70;
        layout->templateY += kDy;
        layout->template2Y += kDy;
        for (PuzzleSpriteDef &sprite : layout->sprites) {
            sprite.y += kDy;
            if (sprite.targetY >= 0) {
                sprite.targetY += kDy;
            }
        }
    }
    if (loaded && exerciseId == QStringLiteral("3.1.24")
        && stepId.trimmed() == QStringLiteral("2")) {
        constexpr int kBelowStop = 70 + 50;
        const int lift = qMax(0, kBelowStop - layout->templateY);
        if (lift > 0) {
            layout->templateY += lift;
            for (PuzzleSpriteDef &sprite : layout->sprites) {
                sprite.y += lift;
                if (sprite.targetY >= 0) {
                    sprite.targetY += lift;
                }
            }
        }
    }
    return loaded;
}
