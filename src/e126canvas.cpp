#include "e126canvas.h"

#include "exerciseassets.h"

#include <QComboBox>
#include <QFile>
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

const char *kPlainChrome =
    "background-color:#ffffff;"
    "background-image:none;"
    "color:#000000;";

} // namespace

E126Canvas::E126Canvas(QWidget *parent) : QWidget(parent) {
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(true);
}

double E126Canvas::scaleX() const {
    return width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0;
}

double E126Canvas::scaleY() const {
    return height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0;
}

QRect E126Canvas::designRect(int x, int y, int w, int h) const {
    return QRect(
        qRound(x * scaleX()),
        qRound(y * scaleY()),
        qMax(1, qRound(w * scaleX())),
        qMax(1, qRound(h * scaleY())));
}

QRect E126Canvas::localRect(int x, int y, int w, int h) const {
    return designRect(x, y, w, h);
}

QSize E126Canvas::scaledSize(const QSize &native) const {
    return QSize(
        qMax(1, qRound(native.width() * scaleX())),
        qMax(1, qRound(native.height() * scaleY())));
}

void E126Canvas::setScaledPixmap(QLabel *label, const QPixmap &nativePx) {
    if (!label) {
        return;
    }
    label->setProperty("nativeW", nativePx.isNull() ? 0 : nativePx.width());
    label->setProperty("nativeH", nativePx.isNull() ? 0 : nativePx.height());
    if (nativePx.isNull()) {
        label->clear();
        return;
    }
    const QSize sz = scaledSize(nativePx.size());
    label->setFixedSize(sz);
    label->setPixmap(
        (sz == nativePx.size())
            ? nativePx
            : nativePx.scaled(sz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    label->setStyleSheet(QString::fromUtf8(kPlainChrome));
}

void E126Canvas::placePixmapLabel(QLabel *label, int designX, int designY, bool localToParent) {
    if (!label) {
        return;
    }
    const int nw = label->property("nativeW").toInt();
    const int nh = label->property("nativeH").toInt();
    if (nw > 0 && nh > 0) {
        const QSize sz = scaledSize(QSize(nw, nh));
        if (label->size() != sz) {
            // Пересчитать pixmap только если сменился масштаб окна.
            const QString path = label->property("sourcePath").toString();
            if (!path.isEmpty() && QFile::exists(path)) {
                setScaledPixmap(label, QPixmap(path));
            } else {
                label->setFixedSize(sz);
            }
        }
    }
    const QPoint pos = localToParent ? localRect(designX, designY, 1, 1).topLeft()
                                     : designRect(designX, designY, 1, 1).topLeft();
    label->move(pos);
    label->show();
}

void E126Canvas::applyChromeStyles() {
    setStyleSheet(QStringLiteral(
        "E126Canvas { background-color:#ffffff; background-image:none; }"
        "QGroupBox {"
        "  background-color:#ffffff; background-image:none;"
        "  border:1px solid #a0a0a0; margin-top:6px; color:#000000;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin:margin; left:8px; padding:0 3px;"
        "  background-color:#ffffff; background-image:none;"
        "}"
        "QLabel { background-color:#ffffff; background-image:none; color:#000000; }"
        "QLineEdit {"
        "  background-color:#ffffff; background-image:none;"
        "  color:#000000; border:1px solid #7a7a7a;"
        "}"
        "QTextEdit {"
        "  background-color:#ffffff; background-image:none;"
        "  color:#000000; border:none;"
        "}"
        "QComboBox {"
        "  background-color:#ffffff; background-image:none;"
        "  color:#000000; border:1px solid #7a7a7a;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color:#ffffff; background-image:none;"
        "  color:#000000; selection-background-color:#cce8ff;"
        "}"
        "QRadioButton {"
        "  background-color:#ffffff; background-image:none; color:#000000;"
        "}"));
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

    applyChromeStyles();
    clearUi();

    if (exerciseId == QStringLiteral("1.272")) {
        build272Mode();
    } else if (m_stepId == QStringLiteral("1")) {
        buildDemoMode();
    } else {
        buildStoryMode();
    }

    layoutUi();
    if (!m_timer.isActive()) {
        m_timer.start();
    }
    setFocus();
}

void E126Canvas::switchStep(const QString &stepId) {
    const QString next = stepId.trimmed().isEmpty() ? QStringLiteral("1") : stepId.trimmed();
    if (next == m_stepId && ((next == QStringLiteral("1") && m_groupBox1)
                             || (next != QStringLiteral("1") && m_groupBox2))) {
        return;
    }
    // Сохраняем текущий ответ и пересобираем UI под другое задание.
    if (m_answerEdit && m_count >= 0 && m_count < m_answers.size()) {
        m_answers[m_count] = m_answerEdit->text();
    }
    const int savedElapsed = m_elapsed;
    const QStringList savedAnswers = m_answers;
    const QString gender = m_genderPrefix;
    startExercise(m_exerciseId, next);
    m_elapsed = savedElapsed;
    m_answers = savedAnswers;
    m_genderPrefix = gender;
    if (m_girlRadio && m_boyRadio) {
        const bool girl = m_genderPrefix != QStringLiteral("m");
        m_girlRadio->blockSignals(true);
        m_boyRadio->blockSignals(true);
        m_girlRadio->setChecked(girl);
        m_boyRadio->setChecked(!girl);
        m_girlRadio->blockSignals(false);
        m_boyRadio->blockSignals(false);
    }
    if (m_stepId == QStringLiteral("1")) {
        showDemoImage();
    } else if (m_exerciseId != QStringLiteral("1.272")) {
        showStoryImage();
    }
}

void E126Canvas::applyPixmap(QLabel *label, const QString &fileName, bool autoSize) {
    Q_UNUSED(autoSize);
    if (!label) {
        return;
    }
    QString path = ExerciseAssets::exerciseFile(m_exerciseId, fileName);
    if (path.isEmpty() && m_exerciseId == QStringLiteral("1.272")) {
        path = ExerciseAssets::exerciseFile(QStringLiteral("1.26"), fileName);
    }
    if (path.isEmpty()) {
        label->clear();
        label->setProperty("nativeW", 0);
        label->setProperty("nativeH", 0);
        label->setProperty("sourcePath", QString());
        return;
    }
    label->setProperty("sourcePath", path);
    setScaledPixmap(label, QPixmap(path));
}

void E126Canvas::buildDemoMode() {
    m_groupBox1 = new QGroupBox(QStringLiteral(" "), this);
    m_groupBox1->setStyleSheet(QString::fromUtf8(kPlainChrome));
    m_girlRadio = new QRadioButton(QStringLiteral("Девочка"), m_groupBox1);
    m_boyRadio = new QRadioButton(QStringLiteral("Мальчик"), m_groupBox1);
    m_girlRadio->setChecked(true);
    m_answerEdit = new QLineEdit(m_groupBox1);
    m_answerCaption = new QLabel(QStringLiteral("Ответ ребенка"), m_groupBox1);
    m_answerCaption->setStyleSheet(QStringLiteral("font-weight:bold; background:#ffffff; background-image:none;"));
    m_imageLabel = new QLabel(m_groupBox1);
    m_imageLabel->setStyleSheet(QString::fromUtf8(kPlainChrome));
    m_nextButton = new ClickLabel(m_groupBox1);
    applyPixmap(m_nextButton, QStringLiteral("next.png"));
    static_cast<ClickLabel *>(m_nextButton)->onClick = [this]() { advanceDemo(); };

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
    m_groupBox2 = new QGroupBox(QStringLiteral(" "), this);
    m_questionEdit = new QTextEdit(m_groupBox2);
    m_questionEdit->setReadOnly(true);
    m_questionEdit->setFrameShape(QFrame::NoFrame);
    m_questionEdit->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 10));
    m_answerEdit = new QLineEdit(m_groupBox2);
    m_answerCaption = new QLabel(QStringLiteral("Ответ ребенка"), m_groupBox2);
    m_answerCaption->setStyleSheet(QStringLiteral("font-weight:bold; background:#ffffff; background-image:none;"));
    m_imageLabel = new QLabel(m_groupBox2);
    m_imageLabel->setStyleSheet(QString::fromUtf8(kPlainChrome));
    m_nextButton = new ClickLabel(m_groupBox2);
    applyPixmap(m_nextButton, QStringLiteral("next.png"));
    static_cast<ClickLabel *>(m_nextButton)->onClick = [this]() { advanceStory(); };

    // groupBox3: пол / сцена / показать эмоции — без наложений.
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
    m_emotionsLabel->setStyleSheet(QString::fromUtf8(kPlainChrome));

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
        if (m_answerEdit && m_count >= 0 && m_count < m_answers.size()) {
            m_answers[m_count] = m_answerEdit->text();
        }
        m_count = index + 1;
        if (m_answerEdit) {
            m_answerEdit->setText(m_answers.value(m_count));
        }
        showStoryImage();
        if (m_nextButton) {
            m_nextButton->setVisible(m_count < 12);
        }
    });

    showStoryImage();
    toggleEmotions();
}

void E126Canvas::build272Mode() {
    m_questionEdit = new QTextEdit(this);
    m_questionEdit->setReadOnly(true);
    m_questionEdit->setFrameShape(QFrame::NoFrame);
    m_questionEdit->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 14));
    m_questionEdit->setStyleSheet(QString::fromUtf8(kPlainChrome));

    m_imageLabel = new QLabel(this);
    m_imageLabel->setStyleSheet(QString::fromUtf8(kPlainChrome));
    m_emotionsLabel = new QLabel(this);
    m_emotionsLabel->setStyleSheet(QString::fromUtf8(kPlainChrome));
    m_showEmotionsButton = new ClickLabel(this);
    {
        const QString path = ExerciseAssets::exerciseFile(QStringLiteral("1.272"), QStringLiteral("pshow.png"));
        if (!path.isEmpty()) {
            const QPixmap px(path);
            m_showEmotionsButton->setPixmap(px);
            m_showEmotionsButton->setFixedSize(scaledSize(px.size()));
            m_showEmotionsButton->setStyleSheet(QString::fromUtf8(kPlainChrome));
        } else {
            applyPixmap(m_showEmotionsButton, QStringLiteral("pshow.png"));
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
    // Как в оригинале: Image.FromFile(...\\{count}.png). Не кэшируем fitted-size в nativeW —
    // иначе placePixmapLabel может оставить старый pixmap при смене кадра.
    const QString fileName = QString::number(m_count) + QStringLiteral(".png");
    applyPixmap(m_imageLabel, fileName);
    if (m_imageLabel && m_imageLabel->property("sourcePath").toString().isEmpty()) {
        applyPixmap(m_imageLabel, QString::number(m_count) + QStringLiteral(".PNG"));
    }
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
            const QString path = ExerciseAssets::exerciseFile(QStringLiteral("1.26"), QStringLiteral("mem.png"));
            if (!path.isEmpty()) {
                setScaledPixmap(m_emotionsLabel, QPixmap(path));
            }
            const QString hidePath =
                ExerciseAssets::exerciseFile(QStringLiteral("1.272"), QStringLiteral("phide.png"));
            if (!hidePath.isEmpty()) {
                setScaledPixmap(m_showEmotionsButton, QPixmap(hidePath));
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
        m_emotionsLabel->setProperty("nativeW", 0);
        m_emotionsLabel->setProperty("nativeH", 0);
        m_emotionsLabel->setFixedSize(0, 0);
        if (m_exerciseId == QStringLiteral("1.272")) {
            const QString showPath =
                ExerciseAssets::exerciseFile(QStringLiteral("1.272"), QStringLiteral("pshow.png"));
            if (!showPath.isEmpty()) {
                setScaledPixmap(m_showEmotionsButton, QPixmap(showPath));
            } else {
                applyPixmap(m_showEmotionsButton, QStringLiteral("pshow.png"));
            }
        } else {
            applyPixmap(m_showEmotionsButton, QStringLiteral("pshow.png"));
        }
        m_emotionsVisible = false;
    }
    layoutUi();
}

void E126Canvas::advanceDemo() {
    // Оригинал bnext: if (count==5) hide; if (count<6) { count++; save answers[count-1]; }
    if (m_count >= 6) {
        return;
    }
    if (m_count == 5 && m_nextButton) {
        m_nextButton->hide();
    }
    if (m_count < 6) {
        ++m_count;
        if (m_answerEdit) {
            // После ++: answers[count-1] — ответ к только что покинутому кадру.
            if (m_count - 1 >= 0 && m_count - 1 < m_answers.size()) {
                m_answers[m_count - 1] = m_answerEdit->text();
            }
            m_answerEdit->clear();
        }
        showDemoImage();
    }
}

void E126Canvas::advanceStory() {
    // Оригинал bnext2: answers[count]=text; count++; combo=count; (картинка через SelectedIndexChanged)
    if (m_count >= 12) {
        return;
    }
    if (m_count == 11 && m_nextButton) {
        m_nextButton->hide();
    }
    if (m_count < 12) {
        if (m_answerEdit && m_count >= 0 && m_count < m_answers.size()) {
            m_answers[m_count] = m_answerEdit->text();
            m_answerEdit->clear();
        }
        ++m_count;
        if (m_sceneCombo) {
            m_sceneCombo->blockSignals(true);
            m_sceneCombo->setCurrentIndex(m_count - 1);
            m_sceneCombo->blockSignals(false);
        }
        showStoryImage();
    }
}

void E126Canvas::layoutUi() {
    if (width() <= 0 || height() <= 0) {
        return;
    }

    if (m_exerciseId == QStringLiteral("1.272")) {
        if (m_questionEdit) {
            m_questionEdit->setGeometry(designRect(273, 118, 719, 123));
            m_questionEdit->show();
        }
        placePixmapLabel(m_imageLabel, 273, 266, false);
        if (m_emotionsLabel && m_emotionsVisible) {
            placePixmapLabel(m_emotionsLabel, 1320, 118, false);
            m_emotionsLabel->raise();
        } else if (m_emotionsLabel) {
            m_emotionsLabel->hide();
        }
        placePixmapLabel(m_showEmotionsButton, 1345, 70, false);
        return;
    }

    if (m_stepId == QStringLiteral("1")) {
        if (m_groupBox1) {
            m_groupBox1->setGeometry(designRect(500, 200, 943, 600));
            m_groupBox1->show();
            m_groupBox1->raise();
        }
        if (m_girlRadio) {
            m_girlRadio->setGeometry(localRect(20, 30, 100, 22));
            m_girlRadio->show();
        }
        if (m_boyRadio) {
            m_boyRadio->setGeometry(localRect(130, 30, 100, 22));
            m_boyRadio->show();
        }
        if (m_answerEdit) {
            m_answerEdit->setGeometry(localRect(93, 81, 516, 24));
            m_answerEdit->show();
        }
        if (m_answerCaption) {
            m_answerCaption->setGeometry(localRect(273, 110, 140, 18));
            m_answerCaption->show();
        }
        placePixmapLabel(m_nextButton, 681, 75, true);
        placePixmapLabel(m_imageLabel, 624, 167, true);
        return;
    }

    // Задание 2 — координаты как в initEx оригинала.
    if (m_groupBox2) {
        m_groupBox2->setGeometry(designRect(300, 130, 901, 788));
        m_groupBox2->show();
        m_groupBox2->raise();
    }
    if (m_questionEdit) {
        m_questionEdit->setGeometry(localRect(27, 32, 713, 71));
        m_questionEdit->show();
    }
    if (m_answerEdit) {
        m_answerEdit->setGeometry(localRect(26, 109, 714, 24));
        m_answerEdit->show();
    }
    if (m_answerCaption) {
        m_answerCaption->setGeometry(localRect(136, 138, 140, 18));
        m_answerCaption->show();
    }
    placePixmapLabel(m_nextButton, 765, 104, true);
    placePixmapLabel(m_imageLabel, 38, 162, true);

    if (m_groupBox3) {
        m_groupBox3->setGeometry(designRect(1396, 12, 445, 147));
        m_groupBox3->show();
        m_groupBox3->raise();
    }
    if (m_girlRadio) {
        m_girlRadio->setGeometry(localRect(6, 28, 90, 22));
        m_girlRadio->show();
    }
    if (m_boyRadio) {
        m_boyRadio->setGeometry(localRect(105, 28, 90, 22));
        m_boyRadio->show();
    }
    placePixmapLabel(m_showEmotionsButton, 200, 55, true);
    if (m_sceneCombo) {
        m_sceneCombo->setGeometry(localRect(67, 100, 55, 24));
        m_sceneCombo->show();
    }
    if (m_emotionsLabel && m_emotionsVisible) {
        placePixmapLabel(m_emotionsLabel, 1300, 180, false);
    } else if (m_emotionsLabel) {
        m_emotionsLabel->hide();
    }
    // Группы поверх листа эмоций.
    if (m_groupBox2) {
        m_groupBox2->raise();
    }
    if (m_groupBox3) {
        m_groupBox3->raise();
    }
}

QString E126Canvas::answersSnapshot() const {
    QStringList parts = m_answers;
    while (parts.size() < 13) {
        parts.append(QString());
    }
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
