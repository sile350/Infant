#include "exercisehost.h"

#include "custommessagebox.h"
#include "exerciseassets.h"
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
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

#include <functional>

namespace {

constexpr int kLeftPanelWidth = 1100;

void applyOpaqueWhite(QWidget *widget) {
    if (!widget) {
        return;
    }
    widget->setAttribute(Qt::WA_StyledBackground, true);
    widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
    widget->setAutoFillBackground(true);
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, Qt::white);
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Button, Qt::white);
    widget->setPalette(pal);
    widget->setStyleSheet(QStringLiteral("background-color:#ffffff; color:#000000;"));
}

QTextBrowser *makeWhiteBrowser(QWidget *parent) {
    auto *browser = new QTextBrowser(parent);
    browser->setOpenExternalLinks(false);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    applyOpaqueWhite(browser);
    browser->setStyleSheet(QStringLiteral(
        "QTextBrowser { background-color:#ffffff; color:#000000; border:none; }"));
    if (browser->viewport()) {
        applyOpaqueWhite(browser->viewport());
        browser->viewport()->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
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

QString extractDivInnerHtml(const QString &html, const QString &divId) {
    const QRegularExpression openRe(
        QStringLiteral("<div\\s+id=['\"]%1['\"][^>]*>").arg(QRegularExpression::escape(divId)),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch openMatch = openRe.match(html);
    if (!openMatch.hasMatch()) {
        return {};
    }
    const int contentStart = openMatch.capturedEnd(0);
    const QRegularExpression closeRe(QStringLiteral("</div>"), QRegularExpression::CaseInsensitiveOption);
    int depth = 1;
    int pos = contentStart;
    while (pos < html.size() && depth > 0) {
        const QRegularExpressionMatch nextOpen = openRe.match(html, pos);
        const QRegularExpressionMatch nextClose = closeRe.match(html, pos);
        if (!nextClose.hasMatch()) {
            break;
        }
        const int openPos = nextOpen.hasMatch() ? nextOpen.capturedStart(0) : -1;
        const int closePos = nextClose.capturedStart(0);
        if (openPos >= 0 && openPos < closePos) {
            ++depth;
            pos = nextOpen.capturedEnd(0);
            continue;
        }
        --depth;
        if (depth == 0) {
            return html.mid(contentStart, closePos - contentStart).trimmed();
        }
        pos = nextClose.capturedEnd(0);
    }
    return {};
}

QString wrapSectionHtml(const QString &innerHtml) {
    return QStringLiteral(
               "<html><head><meta charset='utf-8'></head>"
               "<body style='background-color:#ffffff;color:#000000;"
               "font-family:\"Microsoft Sans Serif\",sans-serif;font-size:14px;margin:0;padding:0;'>"
               "%1</body></html>")
        .arg(innerHtml);
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
    applyOpaqueWhite(box);
    box->setStyleSheet(QStringLiteral(
        "QCheckBox { background-color:#ffffff; color:#000000;"
        "font-family:'Microsoft Sans Serif',sans-serif; font-size:14px; spacing:6px; }"
        "QCheckBox::indicator { width:13px; height:13px; background:#ffffff; }"));
    return box;
}

class CollapsibleSection final : public QWidget {
public:
    CollapsibleSection(
        const QString &title,
        const QString &bodyHtml,
        const std::function<void()> &onToggled,
        QWidget *parent = nullptr)
        : QWidget(parent), m_onToggled(onToggled) {
        applyOpaqueWhite(this);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 8);
        layout->setSpacing(4);

        m_header = new QPushButton(title, this);
        m_header->setFlat(true);
        m_header->setCursor(Qt::PointingHandCursor);
        applyOpaqueWhite(m_header);
        m_header->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "text-align:left;"
            "background-color:#ffffff;"
            "color:#000000;"
            "border:none;"
            "font-family:'Microsoft Sans Serif',sans-serif;"
            "font-size:16px;"
            "font-weight:bold;"
            "padding:0;"
            "}"
            "QPushButton:hover { text-decoration:underline; }"));

        m_body = makeWhiteBrowser(this);
        m_body->setHtml(wrapSectionHtml(bodyHtml));
        applyCompactLineHeight(m_body->document());
        m_body->setVisible(false);
        m_body->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        m_body->document()->setTextWidth(1040);

        layout->addWidget(m_header);
        layout->addWidget(m_body);

        connect(m_header, &QPushButton::clicked, this, [this]() {
            m_expanded = !m_expanded;
            m_body->setVisible(m_expanded);
            if (m_expanded) {
                const int h = m_body->document()->size().height() + 8;
                m_body->setMinimumHeight(h);
                m_body->setMaximumHeight(h);
            }
            if (m_onToggled) {
                m_onToggled();
            }
        });
    }

private:
    QPushButton *m_header = nullptr;
    QTextBrowser *m_body = nullptr;
    std::function<void()> m_onToggled;
    bool m_expanded = false;
};

} // namespace

ExerciseHost::ExerciseHost(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(true);
    applyOpaqueWhite(this);

    m_opaqueBackground = new QWidget(this);
    applyOpaqueWhite(m_opaqueBackground);
    m_opaqueBackground->lower();

    m_rightBackground = new QWidget(this);
    applyOpaqueWhite(m_rightBackground);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    applyOpaqueWhite(m_scrollArea);
    m_scrollArea->setStyleSheet(QStringLiteral("QScrollArea { background-color:#ffffff; border:none; }"));
    if (m_scrollArea->viewport()) {
        applyOpaqueWhite(m_scrollArea->viewport());
        m_scrollArea->viewport()->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
    }

    m_scrollContent = new QWidget;
    applyOpaqueWhite(m_scrollContent);

    auto *layout = new QVBoxLayout(m_scrollContent);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    m_methodologyPanel = new QWidget(m_scrollContent);
    applyOpaqueWhite(m_methodologyPanel);
    auto *methodologyLayout = new QVBoxLayout(m_methodologyPanel);
    methodologyLayout->setContentsMargins(0, 0, 0, 0);
    methodologyLayout->setSpacing(0);

    m_evaluationPanel = new QWidget(m_scrollContent);
    applyOpaqueWhite(m_evaluationPanel);
    auto *evaluationLayout = new QVBoxLayout(m_evaluationPanel);
    evaluationLayout->setContentsMargins(0, 12, 0, 0);
    evaluationLayout->setSpacing(4);

    auto *hrLine = new QFrame(m_evaluationPanel);
    hrLine->setFrameShape(QFrame::HLine);
    hrLine->setFrameShadow(QFrame::Plain);
    hrLine->setStyleSheet(QStringLiteral("color:#000000; background:#000000; max-height:1px;"));
    evaluationLayout->addWidget(hrLine);

    auto *evalTitle = new QLabel(QStringLiteral("Оценка результатов"), m_evaluationPanel);
    evalTitle->setAlignment(Qt::AlignCenter);
    applyOpaqueWhite(evalTitle);
    evalTitle->setStyleSheet(QStringLiteral(
        "QLabel { background:#ffffff; color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:16px; font-weight:bold; line-height:100%; padding:4px 0; }"));
    evaluationLayout->addWidget(evalTitle);

    auto *activityTitle = new QLabel(QStringLiteral("Характер деятельности ребенка:"), m_evaluationPanel);
    activityTitle->setAlignment(Qt::AlignCenter);
    applyOpaqueWhite(activityTitle);
    activityTitle->setStyleSheet(QStringLiteral(
        "QLabel { background:#ffffff; color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:15px; font-weight:bold; line-height:100%; padding:2px 0; }"));
    evaluationLayout->addWidget(activityTitle);

    m_checkboxPanel = new QWidget(m_evaluationPanel);
    applyOpaqueWhite(m_checkboxPanel);
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
    applyOpaqueWhite(helpTitle);
    helpTitle->setStyleSheet(QStringLiteral(
        "QLabel { background:#ffffff; color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:15px; font-weight:bold; line-height:100%; padding:8px 0 2px 0; }"));
    checkboxLayout->addWidget(helpTitle);

    auto *stimHelpLabel = new QLabel(QStringLiteral("Стимулирующая помощь"), m_checkboxPanel);
    applyOpaqueWhite(stimHelpLabel);
    stimHelpLabel->setStyleSheet(QStringLiteral(
        "QLabel { background:#ffffff; color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:14px; font-weight:bold; padding:4px 0 0 24px; }"));
    checkboxLayout->addWidget(stimHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(0));

    auto *directHelpLabel = new QLabel(QStringLiteral("Направляющая помощь:"), m_checkboxPanel);
    applyOpaqueWhite(directHelpLabel);
    directHelpLabel->setStyleSheet(stimHelpLabel->styleSheet());
    checkboxLayout->addWidget(directHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(1));
    checkboxLayout->addWidget(m_helpChecks.at(2));

    auto *teachHelpLabel = new QLabel(QStringLiteral("Обучающая помощь:"), m_checkboxPanel);
    applyOpaqueWhite(teachHelpLabel);
    teachHelpLabel->setStyleSheet(stimHelpLabel->styleSheet());
    checkboxLayout->addWidget(teachHelpLabel);
    checkboxLayout->addWidget(m_helpChecks.at(3));
    checkboxLayout->addWidget(m_helpChecks.at(4));

    evaluationLayout->addWidget(m_checkboxPanel);

    m_beginButton = new QPushButton(QStringLiteral("Начать"), m_evaluationPanel);
    m_beginButton->setFixedHeight(34);
    applyOpaqueWhite(m_beginButton);
    m_beginButton->setStyleSheet(QStringLiteral(
        "QPushButton { background:#ffffff; color:#000000; border:1px solid #000000;"
        "font-family:'Microsoft Sans Serif',sans-serif; font-size:14px; font-weight:bold; padding:4px 18px; }"));
    evaluationLayout->addSpacing(8);
    evaluationLayout->addWidget(m_beginButton, 0, Qt::AlignLeft);

    m_templatePanel = new QWidget(m_scrollContent);
    applyOpaqueWhite(m_templatePanel);
    auto *templateLayout = new QVBoxLayout(m_templatePanel);
    templateLayout->setContentsMargins(0, 12, 0, 0);
    templateLayout->setSpacing(8);

    m_templateBrowser = makeWhiteBrowser(m_templatePanel);
    templateLayout->addWidget(m_templateBrowser);

    m_formProtocolButton = new QPushButton(QStringLiteral("Сформировать протокол"), m_templatePanel);
    m_formProtocolButton->setFixedHeight(34);
    applyOpaqueWhite(m_formProtocolButton);
    m_formProtocolButton->setStyleSheet(m_beginButton->styleSheet());
    templateLayout->addWidget(m_formProtocolButton, 0, Qt::AlignLeft);

    layout->addWidget(m_methodologyPanel);
    layout->addWidget(m_evaluationPanel);
    layout->addWidget(m_templatePanel);
    layout->addStretch();
    m_scrollArea->setWidget(m_scrollContent);

    m_previewImage = new QLabel(this);
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setScaledContents(true);
    applyOpaqueWhite(m_previewImage);

    m_rightCountLabel = new QLabel(this);
    applyOpaqueWhite(m_rightCountLabel);
    m_rightCountLabel->setStyleSheet(QStringLiteral(
        "QLabel { font:bold 17px Arial; color:#000000; background:#ffffff; }"));
    m_rightCountLabel->hide();

    m_wrongCountLabel = new QLabel(this);
    applyOpaqueWhite(m_wrongCountLabel);
    m_wrongCountLabel->setStyleSheet(m_rightCountLabel->styleSheet());
    m_wrongCountLabel->hide();

    m_onlyP = new OnlyPExercise(this);
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

void ExerciseHost::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    QWidget::paintEvent(event);
}

void ExerciseHost::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_opaqueBackground) {
        m_opaqueBackground->setGeometry(rect());
    }
    if (m_rightBackground) {
        m_rightBackground->setGeometry(kLeftPanelWidth, 0, qMax(0, width() - kLeftPanelWidth), height());
    }
    if (m_scrollArea) {
        m_scrollArea->setGeometry(0, 0, kLeftPanelWidth, height());
    }
    if (m_previewImage) {
        m_previewImage->setGeometry(kLeftPanelWidth + 20, 75, qMax(200, width() - kLeftPanelWidth - 40), 400);
    }
    if (m_rightCountLabel) {
        m_rightCountLabel->setGeometry(kLeftPanelWidth + 20, 250, 300, 40);
    }
    if (m_wrongCountLabel) {
        m_wrongCountLabel->setGeometry(kLeftPanelWidth + 20, 350, 300, 40);
    }
}

void ExerciseHost::buildMethodologySections(const QString &orHtml) {
    if (!m_methodologyPanel) {
        return;
    }
    QLayoutItem *child = nullptr;
    while ((child = m_methodologyPanel->layout()->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    auto *methodologyLayout = qobject_cast<QVBoxLayout *>(m_methodologyPanel->layout());
    if (!methodologyLayout) {
        return;
    }

    const QString methodBody = extractDivInnerHtml(orHtml, QStringLiteral("div1"));
    const QString procedureBody = extractDivInnerHtml(orHtml, QStringLiteral("div2"));
    const QString analisBody = extractDivInnerHtml(orHtml, QStringLiteral("div3"));

    const auto relayout = [this]() { layoutContent(); };
    auto *methodSection = new CollapsibleSection(
        QStringLiteral("Методика \"Что на картинке не дорисовано?\" (Забрамная С.Д., Боровик О.В.) описание"),
        methodBody,
        relayout,
        m_methodologyPanel);
    auto *procedureSection = new CollapsibleSection(
        QStringLiteral("Процедура проведения."),
        procedureBody,
        relayout,
        m_methodologyPanel);
    auto *analisSection = new CollapsibleSection(
        QStringLiteral("Анализируемые показатели и нормативы выполнения."),
        analisBody,
        relayout,
        m_methodologyPanel);

    methodologyLayout->addWidget(methodSection);
    methodologyLayout->addSpacing(6);
    methodologyLayout->addWidget(procedureSection);
    methodologyLayout->addSpacing(6);
    methodologyLayout->addWidget(analisSection);
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
    buildMethodologySections(rawOr);

    const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
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
    if (!m_templateBrowser) {
        return;
    }
    m_templateBrowser->document()->setTextWidth(qMax(200, m_templateBrowser->viewport()->width()));
    const int templateHeight = static_cast<int>(m_templateBrowser->document()->size().height()) + 12;
    m_templateBrowser->setMinimumHeight(templateHeight);
    m_templateBrowser->setMaximumHeight(templateHeight);
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
    return QString();
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
