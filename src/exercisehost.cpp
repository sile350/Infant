#include "exercisehost.h"

#include "custommessagebox.h"
#include "exerciseassets.h"
#include "imagebutton.h"
#include "onlypexercise.h"
#include "patientdisplay.h"
#include "repository.h"

#include <QAbstractScrollArea>
#include <QCheckBox>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

namespace {

constexpr QColor kExerciseBg(0xf8, 0xf8, 0xf8);
constexpr int kPanelX = 51;
constexpr int kPanelY = 4;
constexpr int kScrollWidth = 870;

void applyExerciseBackground(QWidget *widget) {
    if (!widget) {
        return;
    }
    widget->setAttribute(Qt::WA_StyledBackground, true);
    widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
    widget->setAutoFillBackground(true);
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, kExerciseBg);
    pal.setColor(QPalette::Base, kExerciseBg);
    pal.setColor(QPalette::Button, kExerciseBg);
    widget->setPalette(pal);
}

QTextBrowser *makeHtmlBrowser(QWidget *parent) {
    auto *browser = new QTextBrowser(parent);
    browser->setOpenExternalLinks(false);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    applyExerciseBackground(browser);
    browser->setStyleSheet(QStringLiteral(
        "QTextBrowser { background-color:#f8f8f8; color:#000000; border:none; }"));
    if (browser->viewport()) {
        applyExerciseBackground(browser->viewport());
        browser->viewport()->setStyleSheet(QStringLiteral("background-color:#f8f8f8;"));
    }
    return browser;
}

QString loadExerciseHtmlFile(const QString &exerciseId, const QString &fileName) {
    const QString path = ExerciseAssets::exerciseFile(exerciseId, fileName);
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

void applyCompactLineHeight(QTextDocument *doc) {
    if (!doc) {
        return;
    }
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        fmt.setLineHeight(100, QTextBlockFormat::ProportionalHeight);
        cursor.setPosition(block.position());
        cursor.mergeBlockFormat(fmt);
    }
    cursor.endEditBlock();
}

QCheckBox *makeCheck(const QString &text, QWidget *parent) {
    auto *box = new QCheckBox(text, parent);
    applyExerciseBackground(box);
    box->setStyleSheet(QStringLiteral(
        "QCheckBox { background-color:#f8f8f8; color:#000000;"
        "font-family:'Microsoft Sans Serif',sans-serif; font-size:14px; spacing:6px; }"
        "QCheckBox::indicator { width:13px; height:13px; background:#f8f8f8; }"));
    return box;
}

class ExerciseOrBrowser final : public QTextBrowser {
public:
    explicit ExerciseOrBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {
        setOpenExternalLinks(false);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        applyExerciseBackground(this);
        setStyleSheet(QStringLiteral(
            "QTextBrowser { background-color:#f8f8f8; color:#000000; border:none; }"));
        if (viewport()) {
            applyExerciseBackground(viewport());
            viewport()->setStyleSheet(QStringLiteral("background-color:#f8f8f8;"));
        }
    }
};

} // namespace

ExerciseHost::ExerciseHost(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(true);
    applyExerciseBackground(this);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    applyExerciseBackground(m_scrollArea);
    m_scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background-color:#f8f8f8; border:none; }"));
    if (m_scrollArea->viewport()) {
        applyExerciseBackground(m_scrollArea->viewport());
        m_scrollArea->viewport()->setStyleSheet(QStringLiteral("background-color:#f8f8f8;"));
    }

    m_scrollContent = new QWidget;
    applyExerciseBackground(m_scrollContent);

    auto *layout = new QVBoxLayout(m_scrollContent);
    layout->setContentsMargins(40, 3, 16, 16);
    layout->setSpacing(8);

    m_orBrowser = new ExerciseOrBrowser(m_scrollContent);
    m_orBrowser->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    connect(m_orBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        toggleOrSection(url.fragment());
    });

    m_evaluationPanel = new QWidget(m_scrollContent);
    applyExerciseBackground(m_evaluationPanel);
    auto *evaluationLayout = new QVBoxLayout(m_evaluationPanel);
    evaluationLayout->setContentsMargins(0, 8, 0, 0);
    evaluationLayout->setSpacing(4);

    auto *hrLine = new QFrame(m_evaluationPanel);
    hrLine->setFrameShape(QFrame::HLine);
    hrLine->setFrameShadow(QFrame::Plain);
    hrLine->setStyleSheet(QStringLiteral("color:#000000; background:#000000; max-height:1px;"));
    evaluationLayout->addWidget(hrLine);

    auto *evalTitle = new QLabel(QStringLiteral("Оценка результатов"), m_evaluationPanel);
    evalTitle->setAlignment(Qt::AlignCenter);
    applyExerciseBackground(evalTitle);
    evalTitle->setStyleSheet(QStringLiteral(
        "QLabel { background:#f8f8f8; color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:16px; font-weight:bold; padding:4px 0; }"));
    evaluationLayout->addWidget(evalTitle);

    auto *activityTitle = new QLabel(QStringLiteral("Характер деятельности ребенка:"), m_evaluationPanel);
    activityTitle->setAlignment(Qt::AlignCenter);
    applyExerciseBackground(activityTitle);
    activityTitle->setStyleSheet(QStringLiteral(
        "QLabel { background:#f8f8f8; color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:15px; font-weight:bold; padding:2px 0; }"));
    evaluationLayout->addWidget(activityTitle);

    m_checkboxPanel = new QWidget(m_evaluationPanel);
    applyExerciseBackground(m_checkboxPanel);
    auto *checkboxLayout = new QVBoxLayout(m_checkboxPanel);
    checkboxLayout->setContentsMargins(24, 0, 8, 0);
    checkboxLayout->setSpacing(2);

    m_activityChecks << makeCheck(QStringLiteral("Ребенок не понимает инструкцию."), m_checkboxPanel)
                     << makeCheck(QStringLiteral("Ребенок понимает инструкцию, но не может выполнить задание."), m_checkboxPanel)
                     << makeCheck(QStringLiteral("Целенаправленное выполнение задания."), m_checkboxPanel);
    m_helpChecks << makeCheck(
                        QStringLiteral(
                            "Одобрение или неодобрение действий ребенка, стимуляция с помощью слов "
                            "«хорошо», «правильно», «неправильно, подумай еще»."),
                        m_checkboxPanel)
                 << makeCheck(
                        QStringLiteral(
                            "Вопросы к испытуемому о том, почему он сделал то или иное действие, с целью "
                            "повышения уровня осознания смысла задания и ориентировки в задании."),
                        m_checkboxPanel)
                 << makeCheck(QStringLiteral("Подсказка, совет действовать тем или иным образом."), m_checkboxPanel)
                 << makeCheck(
                        QStringLiteral("Показ способа выполнения задания с просьбой повторить это действие."),
                        m_checkboxPanel)
                 << makeCheck(
                        QStringLiteral(
                            "Совместно-раздельная деятельность: специалист начинает выполнять задание, а ребенок "
                            "продолжает."),
                        m_checkboxPanel);

    for (QCheckBox *box : m_activityChecks) {
        checkboxLayout->addWidget(box);
        connect(box, &QCheckBox::toggled, this, [this, box](bool checked) {
            if (!checked) {
                return;
            }
            for (QCheckBox *other : m_activityChecks) {
                if (other != box) {
                    other->setChecked(false);
                }
            }
        });
    }

    auto *helpTitle = new QLabel(QStringLiteral("Виды возможной помощи:"), m_checkboxPanel);
    helpTitle->setAlignment(Qt::AlignCenter);
    applyExerciseBackground(helpTitle);
    helpTitle->setStyleSheet(activityTitle->styleSheet());
    checkboxLayout->addWidget(helpTitle);

    auto *stimHelpLabel = new QLabel(QStringLiteral("Стимулирующая помощь"), m_checkboxPanel);
    applyExerciseBackground(stimHelpLabel);
    stimHelpLabel->setStyleSheet(QStringLiteral(
        "QLabel { background:#f8f8f8; color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:14px; font-weight:bold; padding:4px 0 0 24px; }"));
    checkboxLayout->addWidget(stimHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(0));

    auto *directHelpLabel = new QLabel(QStringLiteral("Направляющая помощь:"), m_checkboxPanel);
    applyExerciseBackground(directHelpLabel);
    directHelpLabel->setStyleSheet(stimHelpLabel->styleSheet());
    checkboxLayout->addWidget(directHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(1));
    checkboxLayout->addWidget(m_helpChecks.at(2));

    auto *teachHelpLabel = new QLabel(QStringLiteral("Обучающая помощь:"), m_checkboxPanel);
    applyExerciseBackground(teachHelpLabel);
    teachHelpLabel->setStyleSheet(stimHelpLabel->styleSheet());
    checkboxLayout->addWidget(teachHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(3));
    checkboxLayout->addWidget(m_helpChecks.at(4));

    evaluationLayout->addWidget(m_checkboxPanel);

    m_templatePanel = new QWidget(m_scrollContent);
    applyExerciseBackground(m_templatePanel);
    auto *templateLayout = new QVBoxLayout(m_templatePanel);
    templateLayout->setContentsMargins(0, 12, 0, 0);
    templateLayout->setSpacing(12);

    m_templateBrowser = makeHtmlBrowser(m_templatePanel);
    templateLayout->addWidget(m_templateBrowser);

    m_formProtocolButton = new ImageButton(m_templatePanel);
    const QString formpPath = ExerciseAssets::sysImage(QStringLiteral("formp.png"));
    if (!formpPath.isEmpty()) {
        m_formProtocolButton->setImagePath(formpPath);
        m_formProtocolButton->setFixedSize(196, 33);
    }
    templateLayout->addWidget(m_formProtocolButton, 0, Qt::AlignHCenter);

    layout->addWidget(m_orBrowser);
    layout->addWidget(m_evaluationPanel);
    layout->addWidget(m_templatePanel);
    layout->addStretch();
    m_scrollArea->setWidget(m_scrollContent);

    m_beginButton = new ImageButton(this);
    const QString beginPath = ExerciseAssets::sysImage(QStringLiteral("beginu.png"));
    if (!beginPath.isEmpty()) {
        m_beginButton->setImagePath(beginPath);
    }

    m_previewImage = new QLabel(this);
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setStyleSheet(QStringLiteral("background: transparent;"));

    m_rightCountLabel = new QLabel(this);
    applyExerciseBackground(m_rightCountLabel);
    m_rightCountLabel->setStyleSheet(QStringLiteral(
        "QLabel { font:bold 17px Arial; color:#000000; background:#f8f8f8; }"));
    m_rightCountLabel->hide();

    m_wrongCountLabel = new QLabel(this);
    applyExerciseBackground(m_wrongCountLabel);
    m_wrongCountLabel->setStyleSheet(m_rightCountLabel->styleSheet());
    m_wrongCountLabel->hide();

    m_onlyP = new OnlyPExercise(this);
    m_onlyP->hide();

    m_patientDisplay = new PatientDisplay;

    connect(m_beginButton, &ImageButton::clicked, this, [this]() {
        if (!m_protocolFormed) {
            CustomMessageBox::showError(this, QStringLiteral("Сначала необходимо сформировать отчет"));
            return;
        }
        runOnlyPExercise();
    });
    connect(m_formProtocolButton, &ImageButton::clicked, this, [this]() { formProtocol(); });
    connect(m_onlyP, &OnlyPExercise::finished, this, [this](const QList<bool> &answers, int elapsedSeconds) {
        m_answers = answers;
        m_elapsedSeconds = elapsedSeconds;
        m_exerciseDone = true;
        m_protocolFormed = false;
        showResultLabels(answers, elapsedSeconds);
        if (m_patientDisplay) {
            m_patientDisplay->hideDisplay();
        }
    });
}

void ExerciseHost::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), kExerciseBg);
    QWidget::paintEvent(event);
}

void ExerciseHost::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateChromeLayout();
}

void ExerciseHost::updateChromeLayout() {
    if (m_scrollArea) {
        m_scrollArea->setGeometry(kPanelX, kPanelY, kScrollWidth, qMax(100, height() - kPanelY - 4));
    }
    if (m_beginButton) {
        m_beginButton->setGeometry(976, 12, 158, 33);
    }
    if (m_previewImage) {
        const QPixmap pixmap = m_previewImage->pixmap(Qt::ReturnByValue);
        const int previewW = pixmap.isNull() ? 494 : pixmap.width();
        const int previewH = pixmap.isNull() ? 455 : pixmap.height();
        m_previewImage->setGeometry(1100, 75, previewW, previewH);
    }
    if (m_rightCountLabel) {
        m_rightCountLabel->setGeometry(1100, 250, 300, 40);
    }
    if (m_wrongCountLabel) {
        m_wrongCountLabel->setGeometry(1100, 350, 300, 40);
    }
}

void ExerciseHost::toggleOrSection(const QString &sectionId) {
    if (sectionId == QStringLiteral("method")) {
        m_orOpen1 = !m_orOpen1;
    } else if (sectionId == QStringLiteral("procedure")) {
        m_orOpen2 = !m_orOpen2;
    } else if (sectionId == QStringLiteral("analis")) {
        m_orOpen3 = !m_orOpen3;
    } else {
        return;
    }
    reloadOrBrowser();
}

void ExerciseHost::reloadOrBrowser() {
    if (!m_orBrowser || m_rawOrHtml.isEmpty()) {
        return;
    }
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
    m_orBrowser->setHtml(ExerciseAssets::prepareOrHtml(
        m_rawOrHtml, baseDir, m_orOpen1, m_orOpen2, m_orOpen3));
    applyCompactLineHeight(m_orBrowser->document());
    layoutContent();
}

void ExerciseHost::openExercise(
    const QString &exerciseId,
    const QString &patientId,
    const QString &specialistFio,
    const QString &patientFio,
    const QString &patientBirthDate,
    Repository *repository,
    bool dualScreen) {
    m_exerciseId = exerciseId;
    m_patientId = patientId;
    m_specialistFio = specialistFio;
    m_patientFio = patientFio;
    m_patientBirthDate = patientBirthDate;
    m_repository = repository;
    m_dualScreen = dualScreen;
    m_exerciseDone = false;
    m_protocolFormed = true;
    m_partly = false;
    m_orOpen1 = false;
    m_orOpen2 = false;
    m_orOpen3 = false;
    m_answers.clear();
    m_elapsedSeconds = 0;
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();

    for (QCheckBox *box : m_activityChecks) {
        box->setChecked(false);
    }
    for (QCheckBox *box : m_helpChecks) {
        box->setChecked(false);
    }

    if (exerciseId == QStringLiteral("1.2")) {
        loadStaticPictureExercise();
    }
    show();
    raise();
}

void ExerciseHost::loadStaticPictureExercise() {
    m_evaluationPanel->show();

    m_rawOrHtml = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("or.html"));
    reloadOrBrowser();

    const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
    m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
    applyCompactLineHeight(m_templateBrowser->document());

    const QString previewPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("f1.png"));
    if (!previewPath.isEmpty()) {
        const QPixmap pixmap(previewPath);
        m_previewImage->setPixmap(pixmap);
        m_previewImage->resize(pixmap.size());
        m_previewImage->show();
    } else {
        m_previewImage->hide();
    }
    layoutContent();
    updateChromeLayout();
}

void ExerciseHost::layoutContent() {
    QTimer::singleShot(0, this, [this]() { updateContentHeights(); });
}

void ExerciseHost::updateContentHeights() {
    if (m_orBrowser) {
        m_orBrowser->document()->setTextWidth(qMax(200, m_orBrowser->viewport()->width()));
        const int orHeight = static_cast<int>(m_orBrowser->document()->size().height()) + 8;
        m_orBrowser->setMinimumHeight(orHeight);
        m_orBrowser->setMaximumHeight(orHeight);
    }
    if (m_templateBrowser) {
        m_templateBrowser->document()->setTextWidth(qMax(200, m_templateBrowser->viewport()->width()));
        const int templateHeight = static_cast<int>(m_templateBrowser->document()->size().height()) + 12;
        m_templateBrowser->setMinimumHeight(templateHeight);
        m_templateBrowser->setMaximumHeight(templateHeight);
    }
    if (m_scrollContent) {
        m_scrollContent->adjustSize();
    }
}

void ExerciseHost::runOnlyPExercise() {
    m_protocolFormed = false;
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();
    m_onlyP->setGeometry(0, 0, width(), height());
    m_onlyP->start(m_exerciseId);
    m_onlyP->raise();

    if (m_dualScreen && m_patientDisplay) {
        m_patientDisplay->attachExercise(m_onlyP);
        m_patientDisplay->showOnSecondaryScreen();
    }
}

void ExerciseHost::showResultLabels(const QList<bool> &answers, int elapsedSeconds) {
    Q_UNUSED(elapsedSeconds);
    int right = 0;
    int wrong = 0;
    for (bool answer : answers) {
        if (answer) {
            ++right;
        } else {
            ++wrong;
        }
    }
    m_rightCountLabel->setText(QStringLiteral("Верно: %1").arg(right));
    m_wrongCountLabel->setText(QStringLiteral("Не верно: %1").arg(wrong));
    m_rightCountLabel->show();
    m_wrongCountLabel->show();
}

ExerciseProtocol::CheckboxValues ExerciseHost::checkboxValues() const {
    ExerciseProtocol::CheckboxValues values;
    for (QCheckBox *box : m_activityChecks) {
        if (box->isChecked()) {
            values.activity = box->text();
            break;
        }
    }
    QStringList helpValues;
    for (QCheckBox *box : m_helpChecks) {
        if (box->isChecked()) {
            helpValues << box->text();
        }
    }
    values.help = helpValues.join(QStringLiteral("; "));
    return values;
}

QString ExerciseHost::orHtmlSnapshot() const {
    return m_orBrowser ? m_orBrowser->toHtml() : QString();
}

void ExerciseHost::formProtocol() {
    if (!m_repository) {
        return;
    }
    if (!m_exerciseDone) {
        CustomMessageBox::showError(
            this, QStringLiteral("Формирование протокола невозможно без выполнения упражнения"));
        return;
    }

    const QString protocolBody = ExerciseProtocol::createProtocolHtml(
        m_exerciseId,
        m_specialistFio,
        m_elapsedSeconds,
        m_partly,
        m_templateBrowser->toHtml(),
        m_answers,
        checkboxValues());

    QString error;
    QString protocolId;
    if (!m_repository->saveExerciseProtocol(m_patientId, m_exerciseId, protocolBody, m_partly, &error, &protocolId)) {
        CustomMessageBox::showError(this, error);
        return;
    }

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, protocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(viewHtml);
    applyCompactLineHeight(m_templateBrowser->document());
    layoutContent();

    m_protocolFormed = true;
    m_partly = true;
    emit protocolSaved();
}
