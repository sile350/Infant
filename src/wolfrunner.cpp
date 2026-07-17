#include "wolfrunner.h"

#include "exerciseassets.h"
#include "imagebutton.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
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
    m_templateImage = new QLabel(this);

    m_table = new QTableWidget(7, 2, this);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Ответы"), QStringLiteral("Помощь")});
    m_table->verticalHeader()->setVisible(true);
    m_table->setWordWrap(true);
    m_table->setEditTriggers(
        QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked
        | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for (int i = 0; i < 7; ++i) {
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(wolfQuestions().at(i)));
        m_table->setItem(i, 0, new QTableWidgetItem);
        m_table->setItem(i, 1, new QTableWidgetItem);
    }

    m_episodeCombo = new QComboBox(this);
    m_helpCombo = new QComboBox(this);
    for (int i = 1; i <= 7; ++i) {
        m_episodeCombo->addItem(QString::number(i));
    }
    m_helpCombo->addItems({
        QStringLiteral("Наводящие вопросы."),
        QStringLiteral(
            "Карточки с изображением лиц, выражающих те или иные эмоции, с предложением указать "
            "эмоции, которые испытывает тот или иной герой."),
        QStringLiteral("Напоминает сюжета, с указанием на соответствующие сюжетные картинки."),
    });
    connect(m_helpCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        appendHelpText();
    });
    connect(m_episodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        m_helpCombo->setCurrentIndex(-1);
    });
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
        if (QTableWidgetItem *a = m_table->item(i, 0)) {
            a->setText(QString());
        }
        if (QTableWidgetItem *h = m_table->item(i, 1)) {
            h->setText(QString());
        }
    }

    const QString talePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("tale.png"));
    if (!talePath.isEmpty()) {
        m_taleImage->setPixmap(QPixmap(talePath));
    }

    if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
        btn1->setImagePath(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("shows.png")));
    }
    if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
        btn2->setImagePath(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("showe.png")));
    }

    toggleTemplate1();
    layoutUi();
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
            m_templateImage->setPixmap(QPixmap(trafPath));
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
            m_templateImage->setPixmap(QPixmap(trafPath));
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
    QTableWidgetItem *item = m_table->item(episode - 1, 1);
    if (!item) {
        item = new QTableWidgetItem;
        m_table->setItem(episode - 1, 1, item);
    }
    const QString prev = item->text().trimmed();
    item->setText(prev.isEmpty() ? text : (prev + QLatin1Char(' ') + text));
}

void WolfRunner::layoutUi() {
    m_stop->move(970, 70);
    m_templateBtn1->move(1204, 12);
    m_templateBtn2->move(1567, 12);
    m_taleImage->setGeometry(12, 24, 805, 248);
    m_episodeCombo->setGeometry(74, 278, 52, 24);
    m_helpCombo->setGeometry(313, 278, 467, 24);
    m_table->setGeometry(12, 316, 805, 752);
    m_templateImage->setGeometry(964, 243, 616, 399);
    m_stop->raise();
    m_templateBtn1->raise();
    m_templateBtn2->raise();
    m_table->raise();
}

void WolfRunner::finishSession() {
    ExerciseSessionResult result;
    result.elapsedSeconds = m_elapsed;
    QStringList helps;
    QStringList answers;
    for (int i = 0; i < 7; ++i) {
        const QTableWidgetItem *answer = m_table ? m_table->item(i, 0) : nullptr;
        const QTableWidgetItem *help = m_table ? m_table->item(i, 1) : nullptr;
        answers << (answer ? answer->text().trimmed() : QString());
        helps << (help ? help->text().trimmed() : QString());
    }
    // Как в exbegin: h1..h7 | p1..p7
    result.additional = helps.join(QLatin1Char(';')) + QLatin1Char('|') + answers.join(QLatin1Char(';'));
    m_timer->stop();
    hide();
    emitFinished(result);
}

void WolfRunner::stopSession() {
    finishSession();
}

void WolfRunner::resizeEvent(QResizeEvent *event) {
    ExerciseRunnerWidget::resizeEvent(event);
    layoutUi();
}
