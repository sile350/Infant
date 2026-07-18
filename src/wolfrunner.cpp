#include "wolfrunner.h"

#include "exerciseassets.h"
#include "imagebutton.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFont>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QTableWidget>
#include <QTimer>

namespace {

const QStringList &wolfQuestions() {
    static const QStringList questions = {
        QStringLiteral("О чем эта сказка?"),
        QStringLiteral("Кто самый главный герой в этой сказке"),
        QStringLiteral(
            "Какие чувства испытывал заяц, когда спас волка? Какие чувства испытывал волк, когда заяц помог ему?"),
        QStringLiteral("Какой характер у волка? Зайца? Лисы?"),
        QStringLiteral(
            "Почему? Кем бы ты хотел быть в этой сказке, на чье место встал бы?"),
        QStringLiteral("Какой герой тебе не понравился, почему? Как бы ты поступил на его месте?"),
        QStringLiteral("Как ты думаешь, захотят ли с волком дружить другие звери, почему?"),
    };
    return questions;
}

} // namespace

WolfRunner::WolfRunner(QWidget *parent) : ExerciseRunnerWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(false);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });

    auto *stopButton = new ImageButton(this);
    m_stop = stopButton;
    stopButton->setImagePath(ExerciseAssets::sysImage(QStringLiteral("stop.png")));
    connect(stopButton, &ImageButton::clicked, this, [this]() { finishSession(); });

    auto *template1 = new ImageButton(this);
    m_templateBtn1 = template1;
    connect(template1, &ImageButton::clicked, this, [this]() { toggleTemplate1(); });

    auto *template2 = new ImageButton(this);
    m_templateBtn2 = template2;
    connect(template2, &ImageButton::clicked, this, [this]() { toggleTemplate2(); });

    m_taleImage = new QLabel(this);
    m_taleImage->setScaledContents(true);
    m_taleImage->setStyleSheet(QStringLiteral("background:transparent;"));

    m_templateImage = new QLabel(this);
    m_templateImage->setScaledContents(false);
    m_templateImage->setStyleSheet(QStringLiteral("background:transparent;"));
    m_templateImage->hide();

    m_helpToLabel = new QLabel(QStringLiteral("Помощь к"), this);
    m_helpTypeLabel = new QLabel(QStringLiteral("Виды помощи"), this);
    const QFont labelFont(QStringLiteral("Microsoft Sans Serif"), 8);
    m_helpToLabel->setFont(labelFont);
    m_helpTypeLabel->setFont(labelFont);
    m_helpToLabel->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
    m_helpTypeLabel->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));

    // table.html: № | Вопросы | Ответы | Помощь, bgcolor #f8f8f8
    m_table = new QTableWidget(7, 3, this);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Вопросы"), QStringLiteral("Ответы"), QStringLiteral("Помощь")});
    m_table->verticalHeader()->setVisible(true);
    m_table->setWordWrap(true);
    m_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setStyleSheet(QStringLiteral(
        "QTableWidget {"
        "  background-color:#f8f8f8; gridline-color:#000000; color:#000000;"
        "  border:1px solid #000000;"
        "}"
        "QHeaderView::section {"
        "  background-color:#f8f8f8; color:#000000; padding:4px;"
        "  border:1px solid #000000;"
        "}"
        "QTableWidget::item { padding:4px; }"));
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setDefaultSectionSize(72);
    m_table->verticalHeader()->setMinimumWidth(40);
    for (int i = 0; i < 7; ++i) {
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(QString::number(i + 1)));
        auto *q = new QTableWidgetItem(wolfQuestions().at(i));
        q->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        q->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_table->setItem(i, 0, q);
        auto *a = new QTableWidgetItem;
        a->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        a->setFlags(a->flags() | Qt::ItemIsEditable);
        m_table->setItem(i, 1, a);
        auto *h = new QTableWidgetItem;
        h->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        h->setFlags(h->flags() | Qt::ItemIsEditable);
        m_table->setItem(i, 2, h);
    }

    m_episodeCombo = new QComboBox(this);
    m_helpCombo = new QComboBox(this);
    m_episodeCombo->setStyleSheet(QStringLiteral(
        "QComboBox { background:#ffffff; color:#000000; border:1px solid #7f9db9; }"));
    m_helpCombo->setStyleSheet(m_episodeCombo->styleSheet());
    for (int i = 1; i <= 7; ++i) {
        m_episodeCombo->addItem(QString::number(i));
    }
    m_helpCombo->addItems({
        QStringLiteral("Наводящие вопросы."),
        QStringLiteral(
            "Карточки с изображением лиц, выражающих те или иные эмоции, с предложением указать "
            "эмоции, которые, испытывает тот или иной герой."),
        QStringLiteral("Напоминает сюжета, с указанием на соответствующие сюжетные картинки."),
    });
    m_helpCombo->setCurrentIndex(-1);
    m_episodeCombo->setCurrentIndex(-1);
    connect(m_helpCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        appendHelpText();
    });
    connect(m_episodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        m_helpCombo->setCurrentIndex(-1);
    });

    setFocusPolicy(Qt::StrongFocus);
}

void WolfRunner::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    // WinForms Form BackColor = Control (~#F0F0F0); без заливки при WA_OpaquePaintEvent фон пустой.
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0xf0, 0xf0, 0xf0));
}

void WolfRunner::startSession(
    const QString &exerciseId,
    const ExerciseDefinition &definition,
    const QString &stepId) {
    Q_UNUSED(definition);
    Q_UNUSED(stepId);
    m_exerciseId = exerciseId;
    m_elapsed = 0;
    m_template1Visible = false;
    m_template2Visible = false;

    for (int i = 0; i < m_table->rowCount(); ++i) {
        if (QTableWidgetItem *a = m_table->item(i, 1)) {
            a->setText(QString());
        }
        if (QTableWidgetItem *h = m_table->item(i, 2)) {
            h->setText(QString());
        }
    }

    const QString talePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("tale.png"));
    if (!talePath.isEmpty()) {
        // pictureBox2.SizeMode = StretchImage, 805×248
        const QPixmap tale(talePath);
        m_taleImage->setPixmap(
            tale.scaled(805, 248, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
        btn1->setImagePath(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("shows.png")));
    }
    if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
        btn2->setImagePath(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("showe.png")));
    }

    m_episodeCombo->setCurrentIndex(-1);
    m_helpCombo->setCurrentIndex(-1);

    // wolf_Load → b1_Click: сразу показать traf1
    toggleTemplate1();
    layoutUi();
    setFocus();
    m_timer->start();
    show();
    raise();
}

void WolfRunner::toggleTemplate1() {
    const QString trafPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("traf1.png"));
    const QString showsPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("shows.png"));
    const QString hidePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("hidep.png"));
    const QString showePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("showe.png"));

    if (!m_template1Visible) {
        if (!trafPath.isEmpty()) {
            const QPixmap pix(trafPath);
            m_templateImage->setPixmap(pix);
            m_templateImage->setFixedSize(pix.size());
            m_templateImage->show();
        }
        if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
            btn1->setImagePath(hidePath);
        }
        if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
            btn2->setImagePath(showePath);
        }
        m_template2Visible = false;
        m_template1Visible = true;
    } else {
        m_templateImage->hide();
        if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
            btn1->setImagePath(showsPath);
        }
        m_template1Visible = false;
    }
    layoutUi();
}

void WolfRunner::toggleTemplate2() {
    const QString trafPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("traf2.png"));
    const QString showsPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("shows.png"));
    const QString hidePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("hidep.png"));
    const QString showePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("showe.png"));

    if (!m_template2Visible) {
        if (!trafPath.isEmpty()) {
            const QPixmap pix(trafPath);
            m_templateImage->setPixmap(pix);
            m_templateImage->setFixedSize(pix.size());
            m_templateImage->show();
        }
        if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
            btn2->setImagePath(hidePath);
        }
        if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
            btn1->setImagePath(showsPath);
        }
        m_template1Visible = false;
        m_template2Visible = true;
    } else {
        m_templateImage->hide();
        if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
            btn2->setImagePath(showePath);
        }
        m_template2Visible = false;
    }
    layoutUi();
}

void WolfRunner::appendHelpText() {
    const int episode = m_episodeCombo->currentText().toInt();
    const QString text = m_helpCombo->currentText().trimmed();
    if (episode < 1 || episode > 7 || text.isEmpty() || !m_table) {
        return;
    }
    QTableWidgetItem *item = m_table->item(episode - 1, 2);
    if (!item) {
        item = new QTableWidgetItem;
        m_table->setItem(episode - 1, 2, item);
    }
    const QString prev = item->text().trimmed();
    // Как в wolf.cs: новая строка с отступом при повторной помощи.
    item->setText(prev.isEmpty() ? text : (prev + QLatin1Char('\n') + text));
}

void WolfRunner::layoutUi() {
    // wolf_Load: pstop @ (970,70); b1/b2.Top = pstop.Top; Left из Designer 1204 / 1567
    m_stop->move(970, 70);
    m_templateBtn1->move(1204, 70);
    m_templateBtn2->move(1567, 70);

    // pictureBox2 @ (12,24,805,248)
    m_taleImage->setGeometry(12, 24, 805, 248);

    // label3/4 + combo @ Y≈278
    m_helpToLabel->move(9, 281);
    m_helpToLabel->adjustSize();
    m_episodeCombo->setGeometry(74, 278, 52, 21);
    m_helpTypeLabel->move(229, 281);
    m_helpTypeLabel->adjustSize();
    m_helpCombo->setGeometry(313, 278, 467, 21);

    // webBrowser1 @ (12,316,805,752)
    m_table->setGeometry(12, 316, 805, qMin(752, qMax(200, height() - 316 - 8)));

    // pictureBox1 @ (964,243), AutoSize
    if (m_templateImage->isVisible()) {
        m_templateImage->move(964, 243);
    }

    m_taleImage->raise();
    m_helpToLabel->raise();
    m_helpTypeLabel->raise();
    m_episodeCombo->raise();
    m_helpCombo->raise();
    m_table->raise();
    if (m_templateImage->isVisible()) {
        m_templateImage->raise();
    }
    m_stop->raise();
    m_templateBtn1->raise();
    m_templateBtn2->raise();
}

void WolfRunner::finishSession() {
    if (m_table) {
        m_table->setCurrentItem(nullptr);
        m_table->clearFocus();
    }
    ExerciseSessionResult result;
    result.elapsedSeconds = m_elapsed;
    QStringList helps;
    QStringList answers;
    for (int i = 0; i < 7; ++i) {
        const QTableWidgetItem *answer = m_table ? m_table->item(i, 1) : nullptr;
        const QTableWidgetItem *help = m_table ? m_table->item(i, 2) : nullptr;
        answers << (answer ? answer->text().trimmed() : QString());
        helps << (help ? help->text().trimmed() : QString());
    }
    // exbegin: arraydata1 (h1..h7) | arraydata2 (p1..p7)
    result.additional = helps.join(QLatin1Char(';')) + QLatin1Char('|') + answers.join(QLatin1Char(';'));
    m_timer->stop();
    hide();
    emitFinished(result);
}

void WolfRunner::stopSession() {
    finishSession();
}

void WolfRunner::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        finishSession();
        return;
    }
    ExerciseRunnerWidget::keyPressEvent(event);
}

void WolfRunner::resizeEvent(QResizeEvent *event) {
    ExerciseRunnerWidget::resizeEvent(event);
    layoutUi();
}
