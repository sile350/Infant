#include "wolfrunner.h"

#include "exerciseassets.h"
#include "imagebutton.h"
#include "patientdisplay.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFont>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QTableWidget>
#include <QTimer>

namespace {

void markPatientControl(QWidget *widget) {
    if (widget) {
        widget->setProperty("dokitPatientControl", true);
    }
}

void setButtonImage(QLabel *button, const QString &path) {
    if (!button || path.isEmpty()) {
        return;
    }
    const QPixmap pix(path);
    if (pix.isNull()) {
        return;
    }
    if (auto *imageButton = qobject_cast<ImageButton *>(button)) {
        imageButton->setImagePath(path);
    } else {
        button->setPixmap(pix);
    }
    button->setFixedSize(pix.size());
    button->setCursor(Qt::PointingHandCursor);
    button->show();
    button->raise();
}

QPixmap loadExercisePixmap(const QString &exerciseId, const QString &fileName) {
    const QString path = ExerciseAssets::exerciseFile(exerciseId, fileName);
    if (path.isEmpty()) {
        return {};
    }
    return QPixmap(path);
}

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
    markPatientControl(m_stop);
    setButtonImage(m_stop, ExerciseAssets::sysImage(QStringLiteral("stop.png")));
    connect(stopButton, &ImageButton::clicked, this, [this]() { finishSession(); });

    auto *template1 = new ImageButton(this);
    m_templateBtn1 = template1;
    markPatientControl(m_templateBtn1);
    m_templateBtn1->setCursor(Qt::PointingHandCursor);
    connect(template1, &ImageButton::clicked, this, [this]() { toggleTemplate1(); });

    auto *template2 = new ImageButton(this);
    m_templateBtn2 = template2;
    markPatientControl(m_templateBtn2);
    m_templateBtn2->setCursor(Qt::PointingHandCursor);
    connect(template2, &ImageButton::clicked, this, [this]() { toggleTemplate2(); });

    // Сказка и сюжетные картинки — видны пациенту (не markPatientControl).
    m_taleImage = new QLabel(this);
    m_taleImage->setScaledContents(true);
    m_taleImage->setStyleSheet(QStringLiteral("background:transparent;"));

    m_templateImage = new QLabel(this);
    m_templateImage->setScaledContents(true);
    m_templateImage->setAlignment(Qt::AlignCenter);
    m_templateImage->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    m_templateImage->hide();

    m_helpToLabel = new QLabel(QStringLiteral("Помощь к"), this);
    m_helpTypeLabel = new QLabel(QStringLiteral("Виды помощи"), this);
    markPatientControl(m_helpToLabel);
    markPatientControl(m_helpTypeLabel);
    const QFont labelFont(QStringLiteral("Microsoft Sans Serif"), 8);
    m_helpToLabel->setFont(labelFont);
    m_helpTypeLabel->setFont(labelFont);
    m_helpToLabel->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
    m_helpTypeLabel->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));

    // table.html: № | Вопросы | Ответы | Помощь (ширины 76+284+284+284), без verticalHeader.
    m_table = new QTableWidget(7, 4, this);
    markPatientControl(m_table);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("№"),
        QStringLiteral("Вопросы"),
        QStringLiteral("Ответы"),
        QStringLiteral("Помощь"),
    });
    m_table->verticalHeader()->setVisible(false);
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
        "  background-color:#f8f8f8; color:#000000; padding:2px;"
        "  border:1px solid #000000;"
        "}"
        "QTableWidget::item { padding:2px; }"));
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(1, 250);
    m_table->setColumnWidth(2, 250);
    m_table->setColumnWidth(3, 250);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->verticalHeader()->setDefaultSectionSize(96);
    for (int i = 0; i < 7; ++i) {
        auto *num = new QTableWidgetItem(QString::number(i + 1));
        num->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        num->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        m_table->setItem(i, 0, num);

        auto *q = new QTableWidgetItem(wolfQuestions().at(i));
        q->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        q->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_table->setItem(i, 1, q);

        auto *a = new QTableWidgetItem;
        a->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        a->setFlags(a->flags() | Qt::ItemIsEditable);
        m_table->setItem(i, 2, a);

        auto *h = new QTableWidgetItem;
        h->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        h->setFlags(h->flags() | Qt::ItemIsEditable);
        m_table->setItem(i, 3, h);

        m_table->setRowHeight(i, 96);
    }

    m_episodeCombo = new QComboBox(this);
    m_helpCombo = new QComboBox(this);
    markPatientControl(m_episodeCombo);
    markPatientControl(m_helpCombo);
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
    // WinForms Form BackColor = Control (~#F0F0F0).
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
    m_templateSource = QPixmap();

    for (int i = 0; i < m_table->rowCount(); ++i) {
        if (QTableWidgetItem *a = m_table->item(i, 2)) {
            a->setText(QString());
        }
        if (QTableWidgetItem *h = m_table->item(i, 3)) {
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

    setButtonImage(
        m_templateBtn1, ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("shows.png")));
    setButtonImage(
        m_templateBtn2, ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("showe.png")));

    m_episodeCombo->setCurrentIndex(-1);
    m_helpCombo->setCurrentIndex(-1);

    // wolf_Load → b1_Click: сразу показать traf1
    toggleTemplate1();
    layoutUi();
    syncPatientPicture();
    setFocus();
    m_timer->start();
    show();
    raise();
}

void WolfRunner::toggleTemplate1() {
    // Как wolf.cs b1_Click: Tag showed / "" — показ traf1 или скрытие.
    if (!m_template1Visible) {
        const QPixmap pix = loadExercisePixmap(m_exerciseId, QStringLiteral("traf1.png"));
        if (!pix.isNull()) {
            m_templateSource = pix;
            m_templateImage->setPixmap(pix);
            m_templateImage->show();
            m_templateImage->raise();
        }
        setButtonImage(
            m_templateBtn1, ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("hidep.png")));
        setButtonImage(
            m_templateBtn2, ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("showe.png")));
        m_template2Visible = false;
        m_template1Visible = true;
    } else {
        m_templateImage->hide();
        m_templateSource = QPixmap();
        setButtonImage(
            m_templateBtn1, ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("shows.png")));
        m_template1Visible = false;
    }
    layoutUi();
    syncPatientPicture();
}

void WolfRunner::toggleTemplate2() {
    // Как wolf.cs b2_Click: показ traf2 (эмоции) или скрытие.
    if (!m_template2Visible) {
        const QPixmap pix = loadExercisePixmap(m_exerciseId, QStringLiteral("traf2.png"));
        if (!pix.isNull()) {
            m_templateSource = pix;
            m_templateImage->setPixmap(pix);
            m_templateImage->show();
            m_templateImage->raise();
        }
        setButtonImage(
            m_templateBtn2, ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("hidep.png")));
        setButtonImage(
            m_templateBtn1, ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("shows.png")));
        m_template1Visible = false;
        m_template2Visible = true;
    } else {
        m_templateImage->hide();
        m_templateSource = QPixmap();
        setButtonImage(
            m_templateBtn2, ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("showe.png")));
        m_template2Visible = false;
    }
    layoutUi();
    syncPatientPicture();
}

void WolfRunner::appendHelpText() {
    const int episode = m_episodeCombo->currentText().toInt();
    const QString text = m_helpCombo->currentText().trimmed();
    if (episode < 1 || episode > 7 || text.isEmpty() || !m_table) {
        return;
    }
    QTableWidgetItem *item = m_table->item(episode - 1, 3);
    if (!item) {
        item = new QTableWidgetItem;
        m_table->setItem(episode - 1, 3, item);
    }
    const QString prev = item->text().trimmed();
    // Как в wolf.cs: новая строка с отступом при повторной помощи.
    item->setText(prev.isEmpty() ? text : (prev + QLatin1Char('\n') + text));
}

void WolfRunner::layoutUi() {
    const int hostW = qMax(width(), 1280);
    const int hostH = qMax(height(), 720);

    // Стоп и кнопки показа картинок — в один ряд, всегда в видимой зоне
    // (в Designer b2 @ 1567 уезжал за край на мониторах < ~1820 px).
    constexpr int kBtnY = 70;
    constexpr int kGap = 16;
    m_stop->move(970, kBtnY);
    const int stopRight = m_stop->x() + m_stop->width();
    int btn1X = stopRight + 50;
    int btn2X = btn1X + (m_templateBtn1->width() > 0 ? m_templateBtn1->width() : 274) + kGap;
    const int btn2W = m_templateBtn2->width() > 0 ? m_templateBtn2->width() : 254;
    if (btn2X + btn2W > hostW - 16) {
        btn2X = hostW - 16 - btn2W;
        btn1X = btn2X - kGap - (m_templateBtn1->width() > 0 ? m_templateBtn1->width() : 274);
        if (btn1X < stopRight + kGap) {
            btn1X = stopRight + kGap;
            btn2X = btn1X + (m_templateBtn1->width() > 0 ? m_templateBtn1->width() : 274) + kGap;
        }
    }
    m_templateBtn1->move(btn1X, kBtnY);
    m_templateBtn2->move(btn2X, kBtnY);

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
    const int tableH = qMax(200, hostH - 316 - 12);
    m_table->setGeometry(12, 316, 805, tableH);
    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(1, 250);
    m_table->setColumnWidth(2, 250);
    m_table->setColumnWidth(3, 250);

    // pictureBox1 справа: вписать traf в доступную область под кнопками.
    constexpr int kPicLeft = 964;
    constexpr int kPicTop = 120;
    const int picMaxW = qMax(200, hostW - kPicLeft - 16);
    const int picMaxH = qMax(200, hostH - kPicTop - 16);
    if (m_templateImage && !m_templateImage->isHidden() && !m_templateSource.isNull()) {
        QPixmap display = m_templateSource;
        if (display.width() > picMaxW || display.height() > picMaxH) {
            display = m_templateSource.scaled(
                picMaxW, picMaxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_templateImage->setPixmap(display);
        m_templateImage->setFixedSize(display.size());
        m_templateImage->move(kPicLeft, kPicTop);
        m_templateImage->show();
    } else if (m_templateImage) {
        m_templateImage->move(kPicLeft, kPicTop);
    }

    m_taleImage->raise();
    m_helpToLabel->raise();
    m_helpTypeLabel->raise();
    m_episodeCombo->raise();
    m_helpCombo->raise();
    m_table->raise();
    if (m_templateImage) {
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
        const QTableWidgetItem *answer = m_table ? m_table->item(i, 2) : nullptr;
        const QTableWidgetItem *help = m_table ? m_table->item(i, 3) : nullptr;
        answers << (answer ? answer->text().trimmed() : QString());
        helps << (help ? help->text().trimmed() : QString());
    }
    // exbegin: arraydata1 (h1..h7) | arraydata2 (p1..p7)
    result.additional = helps.join(QLatin1Char(';')) + QLatin1Char('|') + answers.join(QLatin1Char(';'));
    m_timer->stop();
    if (m_patientDisplay) {
        m_patientDisplay->hideDisplay();
    }
    if (m_patientRoot) {
        m_patientRoot->hide();
    }
    hide();
    emitFinished(result);
}

void WolfRunner::stopSession() {
    finishSession();
}

void WolfRunner::bindPatientDisplay(PatientDisplay *display) {
    m_patientDisplay = display;
    ensurePatientView();
    syncPatientPicture();
    if (display && m_patientRoot) {
        display->attachContentWidget(m_patientRoot);
        // После showOnSecondaryScreen геометрия обновится — перецентрировать кадр.
        QTimer::singleShot(0, this, [this]() { layoutPatientView(); });
        QTimer::singleShot(50, this, [this]() { layoutPatientView(); });
    }
}

void WolfRunner::ensurePatientView() {
    if (m_patientRoot) {
        return;
    }
    // Не создавать top-level окно: сразу скрытый виджет без родителя с Qt::Widget.
    m_patientRoot = new QWidget(nullptr, Qt::Widget);
    m_patientRoot->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_patientRoot->setAttribute(Qt::WA_StyledBackground, true);
    m_patientRoot->setAutoFillBackground(true);
    QPalette pal = m_patientRoot->palette();
    pal.setColor(QPalette::Window, Qt::white);
    pal.setColor(QPalette::Base, Qt::white);
    m_patientRoot->setPalette(pal);
    m_patientRoot->setStyleSheet(QStringLiteral("background-color:#ffffff; border:none;"));
    m_patientRoot->hide();

    m_patientPicture = new QLabel(m_patientRoot);
    m_patientPicture->setAlignment(Qt::AlignCenter);
    m_patientPicture->setStyleSheet(QStringLiteral("background-color:#ffffff; border:none;"));
    m_patientPicture->hide();
}

void WolfRunner::syncPatientPicture() {
    ensurePatientView();
    if (!m_patientPicture) {
        return;
    }
    // 38.8: текст сказки на 2-м экране скрыт; только сюжетные/эмоции (traf1/traf2).
    const bool showPic = m_templateImage && !m_templateImage->isHidden() && !m_templateSource.isNull()
        && (m_template1Visible || m_template2Visible);
    if (!showPic) {
        m_patientPicture->clear();
        m_patientPicture->hide();
        if (m_patientRoot) {
            m_patientRoot->update();
        }
        return;
    }
    m_patientPicture->setPixmap(m_templateSource);
    m_patientPicture->setFixedSize(m_templateSource.size());
    m_patientPicture->show();
    layoutPatientView();
    if (m_patientRoot) {
        m_patientRoot->update();
    }
}

void WolfRunner::layoutPatientView() {
    if (!m_patientRoot || !m_patientPicture) {
        return;
    }
    int w = m_patientRoot->width();
    int h = m_patientRoot->height();
    if (QWidget *parent = m_patientRoot->parentWidget()) {
        if (parent->width() > 0 && parent->height() > 0) {
            w = parent->width();
            h = parent->height();
            m_patientRoot->setGeometry(0, 0, w, h);
        }
    }
    if (w <= 0) {
        w = 1920;
    }
    if (h <= 0) {
        h = 1080;
    }
    const QPixmap pix = m_patientPicture->pixmap(Qt::ReturnByValue);
    if (m_patientPicture->isHidden() || pix.isNull()) {
        return;
    }
    // 38.9: картинка по центру экрана по горизонтали и вертикали.
    const QSize picSize = pix.size();
    m_patientPicture->setFixedSize(picSize);
    const int x = qMax(0, (w - picSize.width()) / 2);
    const int y = qMax(0, (h - picSize.height()) / 2);
    m_patientPicture->move(x, y);
    m_patientPicture->raise();
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
    if (m_patientRoot && m_patientDisplay) {
        layoutPatientView();
    }
}
