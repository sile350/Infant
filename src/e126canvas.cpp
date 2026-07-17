#include "e126canvas.h"

#include "exerciseassets.h"

#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QResizeEvent>
#include <QTextEdit>

#include <functional>

namespace {

class ClickLabel final : public QLabel {
public:
    using QLabel::QLabel;
    std::function<void()> onClick;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick) {
            onClick();
        }
        QLabel::mouseReleaseEvent(event);
    }
};

void stylePlainWhite(QWidget *w) {
    if (!w) {
        return;
    }
    w->setAutoFillBackground(true);
    w->setStyleSheet(QStringLiteral(
        "background-color:#ffffff; color:#000000; border:none;"));
}

} // namespace

E126Canvas::E126Canvas(QWidget *parent) : QWidget(parent) {
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });
    setFocusPolicy(Qt::StrongFocus);
}

void E126Canvas::clearUi() {
    qDeleteAll(findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));
    m_groupBox1 = nullptr;
    m_groupBox2 = nullptr;
    m_groupBox3 = nullptr;
    m_imageLabel = nullptr;
    m_emotionsLabel = nullptr;
    m_nextButton = nullptr;
    m_showEmotionsButton = nullptr;
    m_answerCaption = nullptr;
    m_questionEdit = nullptr;
    m_answerEdit = nullptr;
    m_sceneCombo = nullptr;
    m_girlRadio = nullptr;
    m_boyRadio = nullptr;
}

void E126Canvas::startExercise(const QString &exerciseId, const QString &stepId) {
    m_exerciseId = exerciseId;
    m_stepId = stepId.trimmed().isEmpty() ? QStringLiteral("1") : stepId.trimmed();
    m_elapsed = 0;
    m_count = 1;
    m_genderPrefix = QStringLiteral("d");
    m_emotionsVisible = false;
    m_answers = QStringList();
    for (int i = 0; i < 13; ++i) {
        m_answers.append(QString());
    }

    m_questions.clear();
    if (exerciseId == QStringLiteral("1.272")) {
        m_questions
            << QStringLiteral(
                   "Ученица неправильно решила задачу, и учительница поставила ей в дневник "
                   "двойку. Ученица обрадовалась. Как ты думаешь, правильно ли ученица "
                   "отреагировала? Какое чувство на самом деле должна испытывать ученица? Почему?")
            << QStringLiteral(
                   "Ученик разбил окно в классе. Учительница узнала об этом и сказала, что "
                   "вызовет его родителей в школу. Ученик удивился. Как ты думаешь, правильно "
                   "ли ученик отреагировал? Какое чувство на самом деле должен испытывать "
                   "ученик? Почему?")
            << QStringLiteral(
                   "Ученица забыла принести на урок учебники и тетради. Учительница похвалила "
                   "ученицу за это. Ученица разозлилась. Как ты думаешь, правильно ли ученица "
                   "отреагировала? Какое чувство на самом деле должна испытывать ученица? Почему?")
            << QStringLiteral(
                   "На уроке рисования ученик Коля нарисовал красивую картинку, за которую "
                   "учительница поставила ему пятерку. Сосед по парте Миша разорвал Колину "
                   "картинку. Коля остался спокоен. Как ты думаешь, правильно ли ученик "
                   "отреагировал? Какое чувство на самом деле должен испытывать ученик? Почему?")
            << QStringLiteral(
                   "Учительница сказала ученикам, что сегодня они пойдут в цирк, потому что они "
                   "в течение года хорошо учились. Ученики загрустили и заплакали. Как ты "
                   "думаешь, правильно ли ученики отреагировали? Какое чувство на самом деле "
                   "должны испытывать ученики? Почему?")
            << QStringLiteral(
                   "Учительница сказала ученикам, что они закончили писать и сейчас будут "
                   "читать. Ученики испугались. Как ты думаешь, правильно ли ученики "
                   "отреагировали? Какое чувство на самом деле должны испытывать ученики? Почему?");
    } else {
        m_questions
            << QStringLiteral(
                   "Мама с Мишей пошли в садик. Вдруг Миша остановился, начал капризничать, "
                   "плакать и говорить, что не пойдет в садик. А мама очень опаздывает на "
                   "работу. Посмотри на маму. Скажи, что она чувствует. Какое у нее настроение? "
                   "Какая картинка ей подходит?")
            << QStringLiteral(
                   "Мама готовит ужин. А Маша и Миша вместо того, чтобы помочь ей, стали "
                   "баловаться и играть на кухне. Посмотри на маму. Какое у нее стало "
                   "настроение? Скажи, что она чувствует. Какая картинка ей подходит?")
            << QStringLiteral(
                   "Мальчик и девочка сажают деревья у себя во дворе. Мальчик копает, а "
                   "девочка помогает ему, держит дерево. Посмотри на детей. Какое у них "
                   "настроение? Что они чувствуют? Какая картинка им подходит?")
            << QStringLiteral(
                   "Миша и Саша построили кораблик и стали спорить, кто будет с ним играть. "
                   "Миша говорит: «Это мой кораблик!», а Саша – «Нет, это мой кораблик!». "
                   "Посмотри на мальчиков. Какое у них настроение? Что они чувствуют? Какая "
                   "картинка им подходит?")
            << QStringLiteral(
                   "Маша и Миша решили подкормить птичек. И вдруг птички заговорили с детьми "
                   "на человеческом языке! Какие лица стали у детей? Что бы они почувствовали? "
                   "Какая картинка им подходит?")
            << QStringLiteral(
                   "Мама с Мишей шли по улице и увидели маленького щенка. Миша стал просить "
                   "маму взять его к себе домой. И вдруг мама разрешила. Посмотри на Мишу. "
                   "Какое у него стало настроение? Скажи, что он чувствует. Какая картинка "
                   "ему подходит?")
            << QStringLiteral(
                   "Маша увидела маму на другой стороне дороги и решила перебежать на красный "
                   "цвет светофора прямо перед машиной. Водитель еле успел остановиться. "
                   "Посмотри на маму. Что она сейчас чувствует? Какая картинка ей подходит?")
            << QStringLiteral(
                   "Дети играют в песочнице: мальчики строят замок из песка, а девочки играют "
                   "с куклами. Посмотри на детей. Какое у них настроение? Что они чувствуют? "
                   "Какая картинка им подходит?")
            << QStringLiteral(
                   "Видишь, бабушка шла по дорожке, поскользнулась и упала. А в это время "
                   "мимо проходили дети, увидели, что бабушка упала, и стали смеяться. Что "
                   "почувствовала бабушка? Какое у нее стало настроение? Найди картинку с "
                   "таким же настроением.")
            << QStringLiteral(
                   "Зайчик залез в чужой огород, чтобы поесть вкусной капусты. Только он "
                   "откусил кусочек, как увидел гусеницу. Зайчик весь задрожал, ушки прижал. "
                   "Посмотри на зайку. Какое у него стало настроение? Скажи, что он "
                   "чувствует. Какая картинка ему подходит?")
            << QStringLiteral(
                   "Дети водят хоровод вокруг елки. Скоро Дед Мороз подарит им подарки. "
                   "Посмотри на детей. Какое у них настроение? Что они чувствуют? Какая "
                   "картинка им подходит?")
            << QStringLiteral(
                   "Маша и Саша развесили фотографии своих домашних животных. И вдруг все "
                   "фотографии ожили, начали лаять и мяукать. Представь, какие лица стали у "
                   "детей. Что они почувствовали? Какая картинка им подходит?");
    }

    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral(
        "E126Canvas {"
        "  background-color:#ffffff;"
        "  background-image:none;"
        "}"
        "QGroupBox {"
        "  background-color:#ffffff;"
        "  background-image:none;"
        "  border:1px solid #a0a0a0;"
        "  margin-top:6px;"
        "  color:#000000;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left:8px;"
        "  padding:0 3px;"
        "  background-color:#ffffff;"
        "  background-image:none;"
        "}"
        "QLabel { background-color:transparent; background-image:none; color:#000000; }"
        "QLineEdit {"
        "  background-color:#ffffff;"
        "  background-image:none;"
        "  color:#000000;"
        "  border:1px solid #7a7a7a;"
        "}"
        "QTextEdit {"
        "  background-color:#ffffff;"
        "  background-image:none;"
        "  color:#000000;"
        "  border:none;"
        "}"
        "QComboBox {"
        "  background-color:#ffffff;"
        "  background-image:none;"
        "  color:#000000;"
        "  border:1px solid #7a7a7a;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color:#ffffff;"
        "  background-image:none;"
        "  color:#000000;"
        "  selection-background-color:#cce8ff;"
        "}"
        "QRadioButton {"
        "  background-color:#ffffff;"
        "  background-image:none;"
        "  color:#000000;"
        "}"));

    clearUi();

    if (exerciseId == QStringLiteral("1.272")) {
        build272Mode();
    } else if (m_stepId == QStringLiteral("1")) {
        buildDemoMode();
    } else {
        buildStoryMode();
    }

    layoutUi();
    m_timer.start();
    setFocus();
}

void E126Canvas::applyPixmap(QLabel *label, const QString &fileName, bool autoSize) {
    if (!label) {
        return;
    }
    const QString path = ExerciseAssets::exerciseFile(m_exerciseId, fileName);
    if (path.isEmpty() && m_exerciseId == QStringLiteral("1.272")) {
        // 1.272 берёт часть картинок эмоций из 1.26
        const QString fallback = ExerciseAssets::exerciseFile(QStringLiteral("1.26"), fileName);
        if (!fallback.isEmpty()) {
            const QPixmap px(fallback);
            label->setPixmap(px);
            if (autoSize && !px.isNull()) {
                label->setFixedSize(px.size());
            }
            return;
        }
    }
    if (path.isEmpty()) {
        label->clear();
        return;
    }
    const QPixmap px(path);
    label->setPixmap(px);
    if (autoSize && !px.isNull()) {
        label->setFixedSize(px.size());
    }
}

void E126Canvas::buildDemoMode() {
    // initEx param=="1": groupBox1 Left=500 Top=200 Height=600
    m_groupBox1 = new QGroupBox(QStringLiteral(" "), this);
    m_girlRadio = new QRadioButton(QStringLiteral("Девочка"), m_groupBox1);
    m_boyRadio = new QRadioButton(QStringLiteral("Мальчик"), m_groupBox1);
    m_girlRadio->setChecked(true);
    m_answerEdit = new QLineEdit(m_groupBox1);
    m_answerCaption = new QLabel(QStringLiteral("Ответ ребенка"), m_groupBox1);
    m_answerCaption->setStyleSheet(QStringLiteral("font-weight:bold; background:transparent;"));
    m_imageLabel = new QLabel(m_groupBox1);
    m_imageLabel->setScaledContents(false);
    m_nextButton = new ClickLabel(m_groupBox1);
    applyPixmap(m_nextButton, QStringLiteral("next.png"));
    auto *next = static_cast<ClickLabel *>(m_nextButton);
    next->onClick = [this]() { advanceDemo(); };

    connect(m_girlRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            m_genderPrefix = QStringLiteral("d");
            showDemoImage();
        }
    });
    connect(m_boyRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            m_genderPrefix = QStringLiteral("m");
            showDemoImage();
        }
    });
    showDemoImage();
}

void E126Canvas::buildStoryMode() {
    // initEx param=="2": groupBox2 Left=300 Top=130; groupBox3 visible; pemotions 1300,180
    m_groupBox2 = new QGroupBox(QStringLiteral(" "), this);
    m_questionEdit = new QTextEdit(m_groupBox2);
    m_questionEdit->setReadOnly(true);
    m_questionEdit->setFrameShape(QFrame::NoFrame);
    m_questionEdit->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 10));
    m_answerEdit = new QLineEdit(m_groupBox2);
    m_answerCaption = new QLabel(QStringLiteral("Ответ ребенка"), m_groupBox2);
    m_answerCaption->setStyleSheet(QStringLiteral("font-weight:bold; background:transparent;"));
    m_imageLabel = new QLabel(m_groupBox2);
    m_imageLabel->setScaledContents(false);
    m_nextButton = new ClickLabel(m_groupBox2);
    applyPixmap(m_nextButton, QStringLiteral("next.png"));
    static_cast<ClickLabel *>(m_nextButton)->onClick = [this]() { advanceStory(); };

    m_groupBox3 = new QGroupBox(QStringLiteral(" "), this);
    m_girlRadio = new QRadioButton(QStringLiteral("Девочка"), m_groupBox3);
    m_boyRadio = new QRadioButton(QStringLiteral("Мальчик"), m_groupBox3);
    m_girlRadio->setChecked(true);
    m_sceneCombo = new QComboBox(m_groupBox3);
    for (int i = 1; i <= 12; ++i) {
        m_sceneCombo->addItem(QString::number(i));
    }
    m_showEmotionsButton = new ClickLabel(m_groupBox3);
    applyPixmap(m_showEmotionsButton, QStringLiteral("phide.png"));
    static_cast<ClickLabel *>(m_showEmotionsButton)->onClick = [this]() { toggleEmotions(); };

    m_emotionsLabel = new QLabel(this);
    m_emotionsLabel->setScaledContents(false);

    connect(m_girlRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (!checked) {
            return;
        }
        m_genderPrefix = QStringLiteral("d");
        if (m_emotionsVisible) {
            applyPixmap(m_emotionsLabel, QStringLiteral("dem.png"));
            layoutUi();
        }
    });
    connect(m_boyRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (!checked) {
            return;
        }
        m_genderPrefix = QStringLiteral("m");
        if (m_emotionsVisible) {
            applyPixmap(m_emotionsLabel, QStringLiteral("mem.png"));
            layoutUi();
        }
    });
    connect(m_sceneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_count = index + 1;
        showStoryImage();
        if (m_nextButton) {
            m_nextButton->setVisible(m_count < 12);
        }
    });

    showStoryImage();
    // Как в оригинале: сразу показываем эмоции (pshow_Click при старте).
    toggleEmotions();
}

void E126Canvas::build272Mode() {
    m_questionEdit = new QTextEdit(this);
    m_questionEdit->setReadOnly(true);
    m_questionEdit->setFrameShape(QFrame::NoFrame);
    m_questionEdit->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 14));
    stylePlainWhite(m_questionEdit);

    m_imageLabel = new QLabel(this);
    m_imageLabel->setScaledContents(false);

    m_emotionsLabel = new QLabel(this);
    m_emotionsLabel->setScaledContents(false);

    m_showEmotionsButton = new ClickLabel(this);
    applyPixmap(m_showEmotionsButton, QStringLiteral("pshow.png"));
    // 1.272: pshow лежит в папке 1.272
    {
        const QString path = ExerciseAssets::exerciseFile(QStringLiteral("1.272"), QStringLiteral("pshow.png"));
        if (!path.isEmpty()) {
            const QPixmap px(path);
            m_showEmotionsButton->setPixmap(px);
            m_showEmotionsButton->setFixedSize(px.size());
        }
    }
    static_cast<ClickLabel *>(m_showEmotionsButton)->onClick = [this]() { toggleEmotions(); };

    const int idx = qBound(0, m_stepId.toInt() - 1, m_questions.size() - 1);
    m_count = idx + 1;
    if (m_questionEdit) {
        m_questionEdit->setPlainText(m_questions.value(idx));
    }
    show272Image();
}

void E126Canvas::showDemoImage() {
    applyPixmap(m_imageLabel, m_genderPrefix + QString::number(m_count) + QStringLiteral(".png"));
    layoutUi();
}

void E126Canvas::showStoryImage() {
    // Картинки сцен: 1.png … 12.png / 10.PNG (ExerciseAssets обычно case-insensitive на Windows)
    applyPixmap(m_imageLabel, QString::number(m_count) + QStringLiteral(".png"));
    if (m_questionEdit && m_count >= 1 && m_count <= m_questions.size()) {
        m_questionEdit->setPlainText(m_questions.at(m_count - 1));
    }
    layoutUi();
}

void E126Canvas::show272Image() {
    applyPixmap(m_imageLabel, QString::number(m_count) + QStringLiteral(".png"));
    layoutUi();
}

void E126Canvas::toggleEmotions() {
    if (!m_emotionsLabel || !m_showEmotionsButton) {
        return;
    }
    if (!m_emotionsVisible) {
        if (m_exerciseId == QStringLiteral("1.272")) {
            // Оригинал всегда грузит mem.png из 1.26
            const QString path = ExerciseAssets::exerciseFile(QStringLiteral("1.26"), QStringLiteral("mem.png"));
            if (!path.isEmpty()) {
                const QPixmap px(path);
                m_emotionsLabel->setPixmap(px);
                m_emotionsLabel->setFixedSize(px.size());
            }
            const QString hidePath =
                ExerciseAssets::exerciseFile(QStringLiteral("1.272"), QStringLiteral("phide.png"));
            if (!hidePath.isEmpty()) {
                const QPixmap px(hidePath);
                m_showEmotionsButton->setPixmap(px);
                m_showEmotionsButton->setFixedSize(px.size());
            }
        } else {
            applyPixmap(
                m_emotionsLabel,
                m_genderPrefix == QStringLiteral("m") ? QStringLiteral("mem.png")
                                                      : QStringLiteral("dem.png"));
            applyPixmap(m_showEmotionsButton, QStringLiteral("phide.png"));
        }
        m_emotionsVisible = true;
    } else {
        m_emotionsLabel->clear();
        m_emotionsLabel->setFixedSize(0, 0);
        if (m_exerciseId == QStringLiteral("1.272")) {
            const QString showPath =
                ExerciseAssets::exerciseFile(QStringLiteral("1.272"), QStringLiteral("pshow.png"));
            if (showPath.isEmpty()) {
                applyPixmap(m_showEmotionsButton, QStringLiteral("pshow.png"));
            } else {
                const QPixmap px(showPath);
                m_showEmotionsButton->setPixmap(px);
                m_showEmotionsButton->setFixedSize(px.size());
            }
        } else {
            applyPixmap(m_showEmotionsButton, QStringLiteral("pshow.png"));
        }
        m_emotionsVisible = false;
    }
    layoutUi();
}

void E126Canvas::advanceDemo() {
    if (m_count >= 6) {
        return;
    }
    if (m_answerEdit) {
        m_answers[m_count] = m_answerEdit->text();
        m_answerEdit->clear();
    }
    ++m_count;
    if (m_count >= 6 && m_nextButton) {
        m_nextButton->hide();
    }
    showDemoImage();
}

void E126Canvas::advanceStory() {
    if (m_count >= 12) {
        return;
    }
    if (m_answerEdit) {
        m_answers[m_count] = m_answerEdit->text();
        m_answerEdit->clear();
    }
    ++m_count;
    if (m_sceneCombo) {
        m_sceneCombo->blockSignals(true);
        m_sceneCombo->setCurrentIndex(m_count - 1);
        m_sceneCombo->blockSignals(false);
    }
    if (m_count >= 12 && m_nextButton) {
        m_nextButton->hide();
    }
    showStoryImage();
}

QRect E126Canvas::designRect(int x, int y, int w, int h) const {
    const double sx = width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0;
    const double sy = height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0;
    return QRect(qRound(x * sx), qRound(y * sy), qMax(1, qRound(w * sx)), qMax(1, qRound(h * sy)));
}

QRect E126Canvas::localRect(int x, int y, int w, int h) const {
    // Локальные координаты внутри GroupBox — тот же масштаб 1920×1080.
    return designRect(x, y, w, h);
}

void E126Canvas::layoutUi() {
    if (width() <= 0 || height() <= 0) {
        return;
    }

    if (m_exerciseId == QStringLiteral("1.272")) {
        if (m_questionEdit) {
            m_questionEdit->setGeometry(designRect(273, 118, 719, 123));
        }
        if (m_imageLabel) {
            const QPixmap px = m_imageLabel->pixmap(Qt::ReturnByValue);
            const QSize sz = px.isNull() ? QSize(147, 128) : px.size();
            m_imageLabel->setFixedSize(sz);
            m_imageLabel->move(designRect(273, 266, 1, 1).topLeft());
        }
        if (m_emotionsLabel && m_emotionsVisible) {
            const QPixmap px = m_emotionsLabel->pixmap(Qt::ReturnByValue);
            const QSize sz = px.isNull() ? QSize(100, 50) : px.size();
            m_emotionsLabel->setFixedSize(sz);
            m_emotionsLabel->move(designRect(1320, 118, 1, 1).topLeft());
            m_emotionsLabel->show();
        } else if (m_emotionsLabel) {
            m_emotionsLabel->hide();
        }
        if (m_showEmotionsButton) {
            const QPixmap px = m_showEmotionsButton->pixmap(Qt::ReturnByValue);
            const QSize sz = px.isNull() ? QSize(134, 29) : px.size();
            m_showEmotionsButton->setFixedSize(sz);
            m_showEmotionsButton->move(designRect(1345, 27, 1, 1).topLeft());
        }
        return;
    }

    if (m_stepId == QStringLiteral("1")) {
        // Runtime: Left=500 Top=200 Height=600 Width=943
        if (m_groupBox1) {
            m_groupBox1->setGeometry(designRect(500, 200, 943, 600));
        }
        if (m_girlRadio) {
            m_girlRadio->setGeometry(localRect(617, 41, 90, 20));
        }
        if (m_boyRadio) {
            m_boyRadio->setGeometry(localRect(716, 41, 90, 20));
        }
        if (m_answerEdit) {
            m_answerEdit->setGeometry(localRect(93, 81, 516, 20));
        }
        if (m_answerCaption) {
            m_answerCaption->setGeometry(localRect(273, 103, 120, 16));
        }
        if (m_nextButton) {
            const QSize sz = m_nextButton->pixmap(Qt::ReturnByValue).isNull()
                ? QSize(104, 25)
                : m_nextButton->pixmap(Qt::ReturnByValue).size();
            m_nextButton->setFixedSize(sz);
            m_nextButton->move(localRect(681, 75, 1, 1).topLeft());
        }
        if (m_imageLabel) {
            const QSize sz = m_imageLabel->pixmap(Qt::ReturnByValue).isNull()
                ? QSize(147, 128)
                : m_imageLabel->pixmap(Qt::ReturnByValue).size();
            m_imageLabel->setFixedSize(sz);
            m_imageLabel->move(localRect(624, 167, 1, 1).topLeft());
        }
        return;
    }

    // Step 2 — сказка
    if (m_groupBox2) {
        m_groupBox2->setGeometry(designRect(300, 130, 901, 788));
    }
    if (m_questionEdit) {
        m_questionEdit->setGeometry(localRect(27, 32, 713, 71));
    }
    if (m_answerEdit) {
        m_answerEdit->setGeometry(localRect(26, 109, 714, 20));
    }
    if (m_answerCaption) {
        m_answerCaption->setGeometry(localRect(136, 132, 120, 16));
    }
    if (m_nextButton) {
        const QSize sz = m_nextButton->pixmap(Qt::ReturnByValue).isNull()
            ? QSize(104, 25)
            : m_nextButton->pixmap(Qt::ReturnByValue).size();
        m_nextButton->setFixedSize(sz);
        m_nextButton->move(localRect(765, 104, 1, 1).topLeft());
    }
    if (m_imageLabel) {
        const QSize sz = m_imageLabel->pixmap(Qt::ReturnByValue).isNull()
            ? QSize(147, 128)
            : m_imageLabel->pixmap(Qt::ReturnByValue).size();
        m_imageLabel->setFixedSize(sz);
        m_imageLabel->move(localRect(38, 162, 1, 1).topLeft());
    }
    if (m_groupBox3) {
        m_groupBox3->setGeometry(designRect(1396, 12, 445, 147));
    }
    if (m_girlRadio) {
        m_girlRadio->setGeometry(localRect(6, 41, 90, 20));
    }
    if (m_boyRadio) {
        m_boyRadio->setGeometry(localRect(105, 41, 90, 20));
    }
    if (m_sceneCombo) {
        m_sceneCombo->setGeometry(localRect(67, 114, 55, 21));
    }
    if (m_showEmotionsButton) {
        const QSize sz = m_showEmotionsButton->pixmap(Qt::ReturnByValue).isNull()
            ? QSize(104, 25)
            : m_showEmotionsButton->pixmap(Qt::ReturnByValue).size();
        m_showEmotionsButton->setFixedSize(sz);
        m_showEmotionsButton->move(localRect(180, 36, 1, 1).topLeft());
    }
    if (m_emotionsLabel && m_emotionsVisible) {
        const QSize sz = m_emotionsLabel->pixmap(Qt::ReturnByValue).isNull()
            ? QSize(100, 50)
            : m_emotionsLabel->pixmap(Qt::ReturnByValue).size();
        m_emotionsLabel->setFixedSize(sz);
        m_emotionsLabel->move(designRect(1300, 180, 1, 1).topLeft());
        m_emotionsLabel->show();
        m_emotionsLabel->raise();
    } else if (m_emotionsLabel) {
        m_emotionsLabel->hide();
    }
}

QString E126Canvas::answersSnapshot() const {
    QStringList parts = m_answers;
    if (m_answerEdit && m_count >= 0 && m_count < parts.size()) {
        parts[m_count] = m_answerEdit->text();
    }
    return parts.join(QLatin1Char(';'));
}

void E126Canvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    QWidget::paintEvent(event);
}

void E126Canvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        emit stopRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

void E126Canvas::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    layoutUi();
}
