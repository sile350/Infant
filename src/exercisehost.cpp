#include "exercisehost.h"

#include "custommessagebox.h"
#include "exerciseassets.h"
#include "onlypexercise.h"
#include "patientdisplay.h"
#include "repository.h"

#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QTextBlock>
#include <functional>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

namespace {

constexpr auto kWhitePanelStyle = "background-color:#ffffff;";
constexpr auto kWhiteButtonStyle =
    "QPushButton {"
    "background-color:#ffffff;"
    "color:#000000;"
    "border:1px solid #000000;"
    "font-family:'Microsoft Sans Serif',sans-serif;"
    "font-size:14px;"
    "font-weight:bold;"
    "padding:4px 18px;"
    "}"
    "QPushButton:hover { background-color:#ffffff; }"
    "QPushButton:pressed { background-color:#f0f0f0; }";

constexpr auto kWhiteCheckStyle =
    "QCheckBox {"
    "background-color:#ffffff;"
    "color:#000000;"
    "font-family:'Microsoft Sans Serif',sans-serif;"
    "font-size:14px;"
    "spacing:6px;"
    "}"
    "QCheckBox::indicator { width:13px; height:13px; }";

constexpr auto kEvalTitleStyle =
    "QLabel {"
    "background-color:#ffffff;"
    "color:#000000;"
    "font-family:'Microsoft Sans Serif',sans-serif;"
    "font-size:16px;"
    "font-weight:bold;"
    "padding:0;"
    "margin:0;"
    "}";

constexpr auto kEvalSubtitleStyle =
    "QLabel {"
    "background-color:#ffffff;"
    "color:#000000;"
    "font-family:'Microsoft Sans Serif',sans-serif;"
    "font-size:15px;"
    "font-weight:bold;"
    "padding:0;"
    "margin:0;"
    "}";

constexpr auto kHelpGroupStyle =
    "QLabel {"
    "background-color:#ffffff;"
    "color:#000000;"
    "font-family:'Microsoft Sans Serif',sans-serif;"
    "font-size:14px;"
    "font-weight:bold;"
    "padding:8px 0 0 24px;"
    "margin:0;"
    "}";

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
    box->setStyleSheet(kWhiteCheckStyle);
    box->setAttribute(Qt::WA_StyledBackground, true);
    box->setAutoFillBackground(true);
    return box;
}

class ExerciseOrBrowser final : public QTextBrowser {
public:
    explicit ExerciseOrBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {
        setOpenExternalLinks(false);
        setFrameShape(QFrame::NoFrame);
        setAutoFillBackground(true);
        setStyleSheet(QStringLiteral("QTextBrowser { background-color:#ffffff; border:none; color:#000000; }"));
        if (viewport()) {
            viewport()->setAutoFillBackground(true);
            viewport()->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
        }
        connect(this, &QTextBrowser::anchorClicked, this, &ExerciseOrBrowser::handleAnchorClicked);
    }

    void setBaseHtml(const QString &html) {
        m_baseHtml = html;
        m_expandedSections.clear();
        renderHtml();
    }

    void setContentChangedHandler(std::function<void()> handler) {
        m_contentChangedHandler = std::move(handler);
    }

private:
    void handleAnchorClicked(const QUrl &url) {
        const QString section = url.fragment();
        if (section == QStringLiteral("method")) {
            toggleSection(QStringLiteral("div1"));
        } else if (section == QStringLiteral("procedure")) {
            toggleSection(QStringLiteral("div2"));
        } else if (section == QStringLiteral("analis")) {
            toggleSection(QStringLiteral("div3"));
        }
    }

    void toggleSection(const QString &divId) {
        if (m_expandedSections.contains(divId)) {
            m_expandedSections.remove(divId);
        } else {
            m_expandedSections.insert(divId);
        }
        renderHtml();
        if (m_contentChangedHandler) {
            m_contentChangedHandler();
        }
    }

    void renderHtml() {
        QString html = m_baseHtml;
        const auto applyState = [&](const QString &divId) {
            const QString collapsed = QStringLiteral("id='%1' style=\"position:relative;height:1px;overflow:hidden\"").arg(divId);
            const QString expanded = QStringLiteral("id='%1' style=\"position:relative;height:auto;overflow:visible\"").arg(divId);
            if (m_expandedSections.contains(divId)) {
                html.replace(collapsed, expanded);
            } else {
                html.replace(expanded, collapsed);
            }
        };
        applyState(QStringLiteral("div1"));
        applyState(QStringLiteral("div2"));
        applyState(QStringLiteral("div3"));
        setHtml(html);
        applyCompactLineHeight(document());
    }

    QString m_baseHtml;
    QSet<QString> m_expandedSections;
    std::function<void()> m_contentChangedHandler;
};

} // namespace

ExerciseHost::ExerciseHost(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setStyleSheet(kWhitePanelStyle);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setGeometry(0, 0, 1100, 1000);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setAttribute(Qt::WA_StyledBackground, true);
    m_scrollArea->setAutoFillBackground(true);
    m_scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background-color:#ffffff; border:none; }"));
    if (m_scrollArea->viewport()) {
        m_scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, true);
        m_scrollArea->viewport()->setAutoFillBackground(true);
        m_scrollArea->viewport()->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
    }

    m_scrollContent = new QWidget;
    m_scrollContent->setAttribute(Qt::WA_StyledBackground, true);
    m_scrollContent->setAutoFillBackground(true);
    m_scrollContent->setStyleSheet(kWhitePanelStyle);

    auto *layout = new QVBoxLayout(m_scrollContent);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    m_orBrowser = new ExerciseOrBrowser(m_scrollContent);
    if (auto *orBrowser = static_cast<ExerciseOrBrowser *>(m_orBrowser)) {
        orBrowser->setContentChangedHandler([this]() { layoutContent(); });
    }

    m_evaluationPanel = new QWidget(m_scrollContent);
    m_evaluationPanel->setAttribute(Qt::WA_StyledBackground, true);
    m_evaluationPanel->setAutoFillBackground(true);
    m_evaluationPanel->setStyleSheet(kWhitePanelStyle);
    auto *evaluationLayout = new QVBoxLayout(m_evaluationPanel);
    evaluationLayout->setContentsMargins(0, 8, 0, 0);
    evaluationLayout->setSpacing(2);

    auto *evalTitle = new QLabel(QStringLiteral("Оценка результатов"), m_evaluationPanel);
    evalTitle->setAlignment(Qt::AlignCenter);
    evalTitle->setStyleSheet(kEvalTitleStyle);
    evaluationLayout->addWidget(evalTitle);

    auto *activityTitle = new QLabel(QStringLiteral("Характер деятельности ребенка:"), m_evaluationPanel);
    activityTitle->setAlignment(Qt::AlignCenter);
    activityTitle->setStyleSheet(kEvalSubtitleStyle);
    evaluationLayout->addWidget(activityTitle);

    m_checkboxPanel = new QWidget(m_evaluationPanel);
    m_checkboxPanel->setAttribute(Qt::WA_StyledBackground, true);
    m_checkboxPanel->setAutoFillBackground(true);
    m_checkboxPanel->setStyleSheet(kWhitePanelStyle);
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
    helpTitle->setStyleSheet(kEvalSubtitleStyle);

    auto *stimHelpLabel = new QLabel(QStringLiteral("Стимулирующая помощь"), m_checkboxPanel);
    stimHelpLabel->setStyleSheet(kHelpGroupStyle);
    checkboxLayout->addSpacing(8);
    checkboxLayout->addWidget(helpTitle);
    checkboxLayout->addWidget(stimHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(0));

    auto *directHelpLabel = new QLabel(QStringLiteral("Направляющая помощь:"), m_checkboxPanel);
    directHelpLabel->setStyleSheet(kHelpGroupStyle);
    checkboxLayout->addWidget(directHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(1));
    checkboxLayout->addWidget(m_helpChecks.at(2));

    auto *teachHelpLabel = new QLabel(QStringLiteral("Обучающая помощь:"), m_checkboxPanel);
    teachHelpLabel->setStyleSheet(kHelpGroupStyle);
    checkboxLayout->addWidget(teachHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(3));
    checkboxLayout->addWidget(m_helpChecks.at(4));

    evaluationLayout->addWidget(m_checkboxPanel);

    m_beginButton = new QPushButton(QStringLiteral("Начать"), m_evaluationPanel);
    m_beginButton->setFixedHeight(34);
    m_beginButton->setStyleSheet(kWhiteButtonStyle);
    m_beginButton->setAutoFillBackground(true);
    evaluationLayout->addSpacing(8);
    evaluationLayout->addWidget(m_beginButton, 0, Qt::AlignLeft);

    m_templatePanel = new QWidget(m_scrollContent);
    m_templatePanel->setAttribute(Qt::WA_StyledBackground, true);
    m_templatePanel->setAutoFillBackground(true);
    m_templatePanel->setStyleSheet(kWhitePanelStyle);
    auto *templateLayout = new QVBoxLayout(m_templatePanel);
    templateLayout->setContentsMargins(0, 12, 0, 0);
    templateLayout->setSpacing(8);

    m_templateBrowser = new QTextBrowser(m_templatePanel);
    m_templateBrowser->setOpenExternalLinks(false);
    m_templateBrowser->setFrameShape(QFrame::NoFrame);
    m_templateBrowser->setAutoFillBackground(true);
    m_templateBrowser->setStyleSheet(QStringLiteral("QTextBrowser { background-color:#ffffff; border:none; }"));
    if (m_templateBrowser->viewport()) {
        m_templateBrowser->viewport()->setAutoFillBackground(true);
        m_templateBrowser->viewport()->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
    }
    m_templateBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    templateLayout->addWidget(m_templateBrowser);

    m_formProtocolButton = new QPushButton(QStringLiteral("Сформировать протокол"), m_templatePanel);
    m_formProtocolButton->setFixedHeight(34);
    m_formProtocolButton->setStyleSheet(kWhiteButtonStyle);
    m_formProtocolButton->setAutoFillBackground(true);
    templateLayout->addWidget(m_formProtocolButton, 0, Qt::AlignLeft);

    layout->addWidget(m_orBrowser);
    layout->addWidget(m_evaluationPanel);
    layout->addWidget(m_templatePanel);
    layout->addStretch();
    m_scrollArea->setWidget(m_scrollContent);

    m_previewImage = new QLabel(this);
    m_previewImage->setGeometry(1100, 75, 400, 400);
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setScaledContents(true);
    m_previewImage->setStyleSheet(kWhitePanelStyle);
    m_previewImage->setAutoFillBackground(true);

    m_rightCountLabel = new QLabel(this);
    m_rightCountLabel->setGeometry(1100, 250, 300, 40);
    m_rightCountLabel->setStyleSheet(QStringLiteral("font: bold 17px Arial; color: #000000; background:#ffffff;"));
    m_rightCountLabel->hide();

    m_wrongCountLabel = new QLabel(this);
    m_wrongCountLabel->setGeometry(1100, 350, 300, 40);
    m_wrongCountLabel->setStyleSheet(QStringLiteral("font: bold 17px Arial; color: #000000; background:#ffffff;"));
    m_wrongCountLabel->hide();

    m_onlyP = new OnlyPExercise(this);
    m_onlyP->setGeometry(0, 0, 1920, 1080);
    m_onlyP->hide();

    m_patientDisplay = new PatientDisplay;

    connect(m_beginButton, &QPushButton::clicked, this, [this]() {
        if (!m_protocolFormed) {
            CustomMessageBox::showError(this, QStringLiteral("Сначала необходимо сформировать отчет"));
            return;
        }
        runOnlyPExercise();
    });
    connect(m_formProtocolButton, &QPushButton::clicked, this, [this]() { formProtocol(); });
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

    const QString rawOr = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("or.html"));
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
    if (auto *orBrowser = static_cast<ExerciseOrBrowser *>(m_orBrowser)) {
        orBrowser->setBaseHtml(ExerciseAssets::prepareOrHtml(rawOr, baseDir));
    }

    const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
    m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
    applyCompactLineHeight(m_templateBrowser->document());

    const QString previewPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("f1.png"));
    if (!previewPath.isEmpty()) {
        m_previewImage->setPixmap(QPixmap(previewPath));
        m_previewImage->show();
    } else {
        m_previewImage->hide();
    }
    layoutContent();
}

void ExerciseHost::layoutContent() {
    QTimer::singleShot(0, this, [this]() { updateContentHeights(); });
}

void ExerciseHost::updateContentHeights() {
    if (!m_orBrowser || !m_templateBrowser) {
        return;
    }
    const int orHeight = m_orBrowser->document()->size().toSize().height() + 16;
    const int templateHeight = m_templateBrowser->document()->size().toSize().height() + 16;
    m_orBrowser->setMinimumHeight(orHeight);
    m_orBrowser->setMaximumHeight(orHeight);
    m_templateBrowser->setMinimumHeight(templateHeight);
    m_templateBrowser->setMaximumHeight(templateHeight);
    m_scrollContent->adjustSize();
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
