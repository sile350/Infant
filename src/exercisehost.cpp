#include "exercisehost.h"

#include "appsettings.h"
#include "custommessagebox.h"
#include "exerciseassets.h"
#include "imagebutton.h"
#include "onlypexercise.h"
#include "patientdisplay.h"
#include "repository.h"

#include <QAbstractTextDocumentLayout>
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
#include <QTextEdit>
#include <QTextCursor>
#include <QTimer>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStyleOption>
#include <QStyle>
#include <QVBoxLayout>
#include <QtMath>

namespace {

constexpr QColor kExerciseBg(0xf8, 0xf8, 0xf8);
constexpr QColor kDocumentBg(0xff, 0xff, 0xff);
constexpr int kPanelX = 51;
constexpr int kPanelY = 0;
constexpr int kScrollWidth = 870;

constexpr int kScrollBarGutter = 20;
constexpr int kTemplateTableWidth = 671;

const char *kScrollWhiteStyle =
    "QScrollArea { background-color:#ffffff; border:none; }"
    "QScrollBar:vertical { background-color:#ffffff; border:none; width:14px; margin:0; }"
    "QScrollBar::handle:vertical { background-color:#d0d0d0; min-height:20px; border-radius:2px; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { background-color:#ffffff; height:0px; border:none; }"
    "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background-color:#ffffff; }"
    "QScrollBar:horizontal { background-color:#ffffff; height:0px; }";

class WhiteLabel final : public QLabel {
public:
    using QLabel::QLabel;

    explicit WhiteLabel(const QString &text, QWidget *parent = nullptr) : QLabel(text, parent) {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
        setWordWrap(true);
        setAlignment(Qt::AlignLeft | Qt::AlignTop);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), kDocumentBg);
        QLabel::paintEvent(event);
    }
};

class WhiteViewportFilter final : public QObject {
public:
    explicit WhiteViewportFilter(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Paint) {
            auto *viewport = qobject_cast<QWidget *>(watched);
            if (viewport) {
                QPainter painter(viewport);
                painter.fillRect(viewport->rect(), kDocumentBg);
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

class WhiteCheckBox final : public QCheckBox {
public:
    using QCheckBox::QCheckBox;

    WhiteCheckBox(QWidget *parent = nullptr) : QCheckBox(parent) {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);

        const int indicatorSize = 16;
        QRect indicator(
            (width() - indicatorSize) / 2,
            (height() - indicatorSize) / 2,
            indicatorSize,
            indicatorSize);

        painter.fillRect(indicator, Qt::white);
        painter.setPen(QPen(QColor(0x4a, 0x4a, 0x4a)));
        painter.drawRect(indicator.adjusted(0, 0, -1, -1));

        if (isChecked()) {
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(indicator.left() + 3, indicator.top() + 8, indicator.left() + 6, indicator.top() + 11);
            painter.drawLine(indicator.left() + 6, indicator.top() + 11, indicator.right() - 3, indicator.top() + 4);
        }
    }
};

class OpaquePanel final : public QWidget {
public:
    explicit OpaquePanel(const QColor &color, QWidget *parent = nullptr)
        : QWidget(parent), m_color(color) {
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), m_color);
        QWidget::paintEvent(event);
    }

private:
    QColor m_color;
};

void applyWidgetBackground(QWidget *widget, const QColor &color) {
    if (!widget) {
        return;
    }
    widget->setAttribute(Qt::WA_StyledBackground, true);
    widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
    widget->setAutoFillBackground(true);
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, color);
    pal.setColor(QPalette::Base, color);
    pal.setColor(QPalette::Button, color);
    widget->setPalette(pal);
}

QTextEdit *makeHtmlEditor(QWidget *parent) {
    auto *editor = new QTextEdit(parent);
    editor->setReadOnly(false);
    editor->setAcceptRichText(true);
    editor->setFrameShape(QFrame::NoFrame);
    editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    applyWidgetBackground(editor, kDocumentBg);
    editor->setStyleSheet(QStringLiteral(
        "QTextEdit { background-color:#ffffff; color:#000000; border:none; }"));
    if (editor->viewport()) {
        applyWidgetBackground(editor->viewport(), kDocumentBg);
        editor->viewport()->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
    }
    return editor;
}

void resizeCheckLabel(QLabel *label, int width) {
    if (!label || width <= 0) {
        return;
    }
    label->setFixedWidth(width);
    const int height = label->heightForWidth(width);
    label->setFixedHeight(qMax(20, height));
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
    doc->setDocumentMargin(0);
    QTextFrameFormat rootFormat = doc->rootFrame()->frameFormat();
    rootFormat.setMargin(0);
    rootFormat.setPadding(0);
    rootFormat.setBorder(0);
    doc->rootFrame()->setFrameFormat(rootFormat);

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        fmt.setLineHeight(100, QTextBlockFormat::ProportionalHeight);
        fmt.setTopMargin(0);
        fmt.setBottomMargin(0);
        cursor.setPosition(block.position());
        cursor.mergeBlockFormat(fmt);
    }
    cursor.endEditBlock();
}

int tightDocumentHeight(QTextDocument *doc) {
    if (!doc) {
        return 0;
    }
    return static_cast<int>(qCeil(doc->documentLayout()->documentSize().height()));
}

ExerciseCheckRow makeCheckRow(const QString &text, QVBoxLayout *layout, int contentWidth) {
    ExerciseCheckRow row;
    auto *wrap = new OpaquePanel(kDocumentBg);
    wrap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *rowLayout = new QHBoxLayout(wrap);
    rowLayout->setContentsMargins(0, 2, 0, 2);
    rowLayout->setSpacing(8);

    row.box = new WhiteCheckBox(wrap);
    row.box->setFixedSize(18, 18);

    row.label = new WhiteLabel(text, wrap);
    row.label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    if (contentWidth > 40) {
        resizeCheckLabel(row.label, contentWidth - 34);
    }
    row.label->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif; font-size:14px;"));

    rowLayout->addWidget(row.box, 0, Qt::AlignTop);
    rowLayout->addWidget(row.label, 1);
    layout->addWidget(wrap);
    return row;
}

class ExerciseOrBrowser final : public QTextBrowser {
public:
    explicit ExerciseOrBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {
        setOpenExternalLinks(false);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        applyWidgetBackground(this, kDocumentBg);
        setStyleSheet(QStringLiteral(
            "QTextBrowser { background-color:#ffffff; color:#000000; border:none; margin:0; padding:0; }"));
        if (viewport()) {
            applyWidgetBackground(viewport(), kDocumentBg);
            viewport()->setStyleSheet(QStringLiteral("background-color:#ffffff; margin:0; padding:0;"));
        }
        document()->setDocumentMargin(0);
    }
};

} // namespace

ExerciseHost::ExerciseHost(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(true);
    applyWidgetBackground(this, kExerciseBg);

    m_leftBackdrop = new OpaquePanel(kDocumentBg, this);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    applyWidgetBackground(m_scrollArea, kDocumentBg);
    m_scrollArea->setStyleSheet(QString::fromLatin1(kScrollWhiteStyle));
    if (m_scrollArea->viewport()) {
        applyWidgetBackground(m_scrollArea->viewport(), kDocumentBg);
        m_scrollArea->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_scrollArea->viewport()->installEventFilter(new WhiteViewportFilter(m_scrollArea));
    }
    if (m_scrollArea->verticalScrollBar()) {
        applyWidgetBackground(m_scrollArea->verticalScrollBar(), kDocumentBg);
        m_scrollArea->verticalScrollBar()->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
    }

    m_scrollContent = new OpaquePanel(kDocumentBg);
    m_scrollContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    auto *layout = new QVBoxLayout(m_scrollContent);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *orPanel = new OpaquePanel(kDocumentBg, m_scrollContent);
    auto *orLayout = new QVBoxLayout(orPanel);
    orLayout->setContentsMargins(0, 0, 0, 0);
    orLayout->setSpacing(0);

    m_orBrowser = new ExerciseOrBrowser(orPanel);
    m_orBrowser->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    connect(m_orBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        toggleOrSection(url.fragment());
    });
    orLayout->addWidget(m_orBrowser);

    m_evaluationPanel = new OpaquePanel(kDocumentBg, m_scrollContent);
    m_evaluationPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    auto *evaluationLayout = new QVBoxLayout(m_evaluationPanel);
    evaluationLayout->setContentsMargins(8, 0, 8, 12);
    evaluationLayout->setSpacing(4);

    auto *hrLine = new QFrame(m_evaluationPanel);
    hrLine->setFrameShape(QFrame::HLine);
    hrLine->setFrameShadow(QFrame::Plain);
    hrLine->setFixedHeight(1);
    hrLine->setStyleSheet(QStringLiteral("background-color:#000000; border:none;"));
    evaluationLayout->addWidget(hrLine);

    auto *evalTitle = new WhiteLabel(QStringLiteral("Оценка результатов"), m_evaluationPanel);
    evalTitle->setAlignment(Qt::AlignCenter);
    evalTitle->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:16px; font-weight:bold; padding:0;"));
    evaluationLayout->addWidget(evalTitle);

    auto *activityTitle = new WhiteLabel(QStringLiteral("Характер деятельности ребенка:"), m_evaluationPanel);
    activityTitle->setAlignment(Qt::AlignCenter);
    activityTitle->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:15px; font-weight:bold; padding:0;"));
    evaluationLayout->addWidget(activityTitle);

    m_checkboxPanel = new OpaquePanel(kDocumentBg, m_evaluationPanel);
    m_checkboxPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *checkboxLayout = new QVBoxLayout(m_checkboxPanel);
    checkboxLayout->setContentsMargins(8, 0, 8, 0);
    checkboxLayout->setSpacing(2);

    const int initialCheckWidth = 760;
    m_activityChecks << makeCheckRow(QStringLiteral("Ребенок не понимает инструкцию."), checkboxLayout, initialCheckWidth)
                     << makeCheckRow(
                            QStringLiteral("Ребенок понимает инструкцию, но не может выполнить задание."),
                            checkboxLayout,
                            initialCheckWidth)
                     << makeCheckRow(
                            QStringLiteral("Целенаправленное выполнение задания."),
                            checkboxLayout,
                            initialCheckWidth);
    for (const ExerciseCheckRow &row : m_activityChecks) {
        connect(row.box, &QCheckBox::toggled, this, [this, row](bool checked) {
            if (!checked) {
                return;
            }
            for (const ExerciseCheckRow &other : m_activityChecks) {
                if (other.box != row.box) {
                    other.box->setChecked(false);
                }
            }
        });
    }

    auto *helpTitle = new WhiteLabel(QStringLiteral("Виды возможной помощи:"), m_checkboxPanel);
    helpTitle->setAlignment(Qt::AlignCenter);
    helpTitle->setStyleSheet(activityTitle->styleSheet());
    checkboxLayout->addWidget(helpTitle);

    auto *stimHelpLabel = new WhiteLabel(QStringLiteral("Стимулирующая помощь"), m_checkboxPanel);
    stimHelpLabel->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:14px; font-weight:bold; padding:4px 0 0 16px;"));
    checkboxLayout->addWidget(stimHelpLabel);
    m_helpChecks << makeCheckRow(
        QStringLiteral(
            "Одобрение или неодобрение действий ребенка, стимуляция с помощью слов "
            "«хорошо», «правильно», «неправильно, подумай еще»."),
        checkboxLayout,
        initialCheckWidth);

    auto *directHelpLabel = new WhiteLabel(QStringLiteral("Направляющая помощь:"), m_checkboxPanel);
    directHelpLabel->setStyleSheet(stimHelpLabel->styleSheet());
    checkboxLayout->addWidget(directHelpLabel);
    m_helpChecks << makeCheckRow(
        QStringLiteral(
            "Вопросы к испытуемому о том, почему он сделал то или иное действие, с целью "
            "повышения уровня осознания смысла задания и ориентировки в задании."),
        checkboxLayout,
        initialCheckWidth);
    m_helpChecks << makeCheckRow(
        QStringLiteral("Подсказка, совет действовать тем или иным образом."),
        checkboxLayout,
        initialCheckWidth);

    auto *teachHelpLabel = new WhiteLabel(QStringLiteral("Обучающая помощь:"), m_checkboxPanel);
    teachHelpLabel->setStyleSheet(stimHelpLabel->styleSheet());
    checkboxLayout->addWidget(teachHelpLabel);
    m_helpChecks << makeCheckRow(
        QStringLiteral("Показ способа выполнения задания с просьбой повторить это действие."),
        checkboxLayout,
        initialCheckWidth);
    m_helpChecks << makeCheckRow(
        QStringLiteral(
            "Совместно-раздельная деятельность: специалист начинает выполнять задание, а ребенок "
            "продолжает."),
        checkboxLayout,
        initialCheckWidth);

    evaluationLayout->addWidget(m_checkboxPanel);

    m_templatePanel = new OpaquePanel(kDocumentBg, m_scrollContent);
    auto *templateLayout = new QVBoxLayout(m_templatePanel);
    templateLayout->setContentsMargins(0, 16, 0, 8);
    templateLayout->setSpacing(12);

    m_templateBrowser = makeHtmlEditor(m_templatePanel);
    templateLayout->addWidget(m_templateBrowser);

    m_formProtocolButton = new ImageButton(m_templatePanel);
    const QString formpPath = ExerciseAssets::sysImage(QStringLiteral("formp.png"));
    if (!formpPath.isEmpty()) {
        m_formProtocolButton->setImagePath(formpPath);
        m_formProtocolButton->setFixedSize(196, 33);
    }
    templateLayout->addWidget(m_formProtocolButton, 0, Qt::AlignHCenter);

    layout->addWidget(orPanel);
    layout->addWidget(m_evaluationPanel);
    layout->addWidget(m_templatePanel);
    m_scrollArea->setWidget(m_scrollContent);

    m_beginButton = new ImageButton(this);
    const QString beginPath = ExerciseAssets::sysImage(QStringLiteral("beginu.png"));
    if (!beginPath.isEmpty()) {
        m_beginButton->setImagePath(beginPath);
    }

    m_rightPanel = new OpaquePanel(kExerciseBg, this);

    m_previewImage = new QLabel(m_rightPanel);
    m_previewImage->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_previewImage->setScaledContents(false);
    m_previewImage->setStyleSheet(QStringLiteral("background: transparent;"));

    m_rightCountLabel = new QLabel(this);
    applyWidgetBackground(m_rightCountLabel, kExerciseBg);
    m_rightCountLabel->setStyleSheet(QStringLiteral(
        "QLabel { font:bold 17px Arial; color:#000000; background:#f8f8f8; }"));
    m_rightCountLabel->hide();

    m_wrongCountLabel = new QLabel(this);
    applyWidgetBackground(m_wrongCountLabel, kExerciseBg);
    m_wrongCountLabel->setStyleSheet(m_rightCountLabel->styleSheet());
    m_wrongCountLabel->hide();

    m_onlyP = new OnlyPExercise(this);
    m_onlyP->hide();

    m_specialistExercise = new OnlyPExercise(m_rightPanel);
    m_specialistExercise->setDisplayRole(OnlyPExercise::DisplayRole::Specialist);
    m_specialistExercise->setMirrorMode(true);
    m_specialistExercise->hide();
    connect(m_onlyP, &OnlyPExercise::pictureChanged, m_specialistExercise, &OnlyPExercise::showPicture, Qt::UniqueConnection);
    connect(
        m_specialistExercise,
        &OnlyPExercise::mirrorAnswerRequested,
        m_onlyP,
        &OnlyPExercise::submitAnswer,
        Qt::UniqueConnection);
    connect(
        m_specialistExercise,
        &OnlyPExercise::mirrorStopRequested,
        m_onlyP,
        &OnlyPExercise::stopExercise,
        Qt::UniqueConnection);

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
        m_exerciseRunning = false;
        if (m_dualScreen) {
            if (m_specialistExercise) {
                m_specialistExercise->hide();
            }
            if (m_onlyP) {
                m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Primary);
                m_onlyP->hide();
            }
            if (m_previewImage && !m_previewSource.isNull()) {
                m_previewImage->show();
                updatePreviewLayout();
            }
        } else {
            restoreExerciseOverlay();
        }
        setExerciseChromeVisible(true);
        updateChromeLayout();
        showResultLabels(answers, elapsedSeconds);
        if (m_patientDisplay) {
            m_patientDisplay->hideDisplay();
        }
    });
}

void ExerciseHost::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    const int leftWhiteWidth = kPanelX + kScrollWidth + kScrollBarGutter;
    painter.fillRect(QRect(0, 0, leftWhiteWidth, height()), kDocumentBg);
    if (leftWhiteWidth < width()) {
        painter.fillRect(QRect(leftWhiteWidth, 0, width() - leftWhiteWidth, height()), kExerciseBg);
    }
    QWidget::paintEvent(event);
}

void ExerciseHost::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateChromeLayout();
    updateExerciseOverlayGeometry();
}

void ExerciseHost::updateChromeLayout() {
    if (m_exerciseRunning && !m_dualScreen) {
        updateExerciseOverlayGeometry();
        if (m_onlyP && m_onlyP->isVisible()) {
            m_onlyP->raise();
        }
        return;
    }

    if (m_leftBackdrop) {
        m_leftBackdrop->setGeometry(0, 0, kPanelX + kScrollWidth + kScrollBarGutter, height());
        m_leftBackdrop->lower();
    }
    if (m_scrollArea) {
        m_scrollArea->setGeometry(kPanelX, kPanelY, kScrollWidth, qMax(100, height() - kPanelY));
        m_scrollArea->raise();
    }
    if (m_rightPanel) {
        m_rightPanel->setGeometry(kPanelX + kScrollWidth, 0, qMax(0, width() - kPanelX - kScrollWidth), height());
        m_rightPanel->raise();
    }
    if (m_dualScreen && m_exerciseRunning && m_specialistExercise && m_rightPanel) {
        m_specialistExercise->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        m_specialistExercise->raise();
    }
    if (m_beginButton) {
        m_beginButton->setGeometry(976, 12, 158, 33);
        m_beginButton->raise();
    }
    if (m_previewImage) {
        updatePreviewLayout();
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
    setExerciseChromeVisible(true);

    for (const ExerciseCheckRow &row : m_activityChecks) {
        row.box->setChecked(false);
    }
    for (const ExerciseCheckRow &row : m_helpChecks) {
        row.box->setChecked(false);
    }

    if (exerciseId == QStringLiteral("1.2")) {
        loadStaticPictureExercise();
    }
    show();
    raise();
}

void ExerciseHost::updatePreviewLayout() {
    if (!m_previewImage || m_previewSource.isNull()) {
        if (m_previewImage) {
            m_previewImage->hide();
        }
        return;
    }

    constexpr int kPreviewAbsLeft = 1100;
    constexpr int kPreviewAbsTop = 75;
    const int rightPanelLeft = kPanelX + kScrollWidth;
    const int localX = kPreviewAbsLeft - rightPanelLeft;
    const int localY = kPreviewAbsTop;
    const int maxW = qMax(120, width() - kPreviewAbsLeft - 16);
    const int maxH = qMax(120, height() - kPreviewAbsTop - 16);
    const QPixmap scaled = m_previewSource.scaled(
        maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewImage->setPixmap(scaled);
    m_previewImage->setFixedSize(scaled.size());
    m_previewImage->move(qMax(0, localX), localY);
    m_previewImage->show();
    m_previewImage->raise();
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
    QPixmap previewPixmap;
    if (!previewPath.isEmpty()) {
        previewPixmap.load(previewPath);
    }
    if (previewPixmap.isNull() || previewPixmap.width() < 32 || previewPixmap.height() < 32) {
        const QString fallbackPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("p1.png"));
        if (!fallbackPath.isEmpty()) {
            previewPixmap.load(fallbackPath);
        }
    }
    if (!previewPixmap.isNull()) {
        m_previewSource = previewPixmap;
        m_rightPanel->show();
        updatePreviewLayout();
    } else {
        m_previewSource = QPixmap();
        m_previewImage->hide();
    }
    layoutContent();
    QTimer::singleShot(50, this, [this]() { updateContentHeights(); });
    updateChromeLayout();
}

void ExerciseHost::layoutContent() {
    QTimer::singleShot(0, this, [this]() { updateContentHeights(); });
}

void ExerciseHost::updateContentHeights() {
    const int textWidth = m_scrollArea && m_scrollArea->viewport()
        ? qMax(200, m_scrollArea->viewport()->width() - 24)
        : 716;

    if (m_orBrowser) {
        m_orBrowser->document()->setTextWidth(textWidth);
        const int orHeight = tightDocumentHeight(m_orBrowser->document());
        m_orBrowser->setMinimumHeight(orHeight);
        m_orBrowser->setMaximumHeight(orHeight);
        if (QWidget *orPanel = m_orBrowser->parentWidget()) {
            orPanel->setMinimumHeight(orHeight);
            orPanel->setMaximumHeight(orHeight);
        }
    }
    if (m_evaluationPanel) {
        m_evaluationPanel->setMinimumHeight(0);
        m_evaluationPanel->setMaximumHeight(QWIDGETSIZE_MAX);
        m_evaluationPanel->adjustSize();
    }
    if (m_templateBrowser) {
        m_templateBrowser->document()->setTextWidth(kTemplateTableWidth);
        const int templateHeight = static_cast<int>(qCeil(m_templateBrowser->document()->size().height())) + 2;
        m_templateBrowser->setMinimumHeight(templateHeight);
        m_templateBrowser->setMaximumHeight(templateHeight);
        m_templateBrowser->setMinimumWidth(kTemplateTableWidth);
        m_templateBrowser->setMaximumWidth(kTemplateTableWidth);
        m_templatePanel->setMinimumWidth(kTemplateTableWidth);
        m_templatePanel->setMaximumWidth(kTemplateTableWidth + 16);
    }
    const int checkWidth = textWidth - 16;
    const int labelWidth = qMax(120, checkWidth - 34);
    for (const ExerciseCheckRow &row : m_activityChecks) {
        if (row.label) {
            resizeCheckLabel(row.label, labelWidth);
            if (row.label->parentWidget()) {
                row.label->parentWidget()->setMinimumWidth(textWidth);
                row.label->parentWidget()->adjustSize();
            }
        }
    }
    for (const ExerciseCheckRow &row : m_helpChecks) {
        if (row.label) {
            resizeCheckLabel(row.label, labelWidth);
            if (row.label->parentWidget()) {
                row.label->parentWidget()->setMinimumWidth(textWidth);
                row.label->parentWidget()->adjustSize();
            }
        }
    }
    if (m_checkboxPanel) {
        m_checkboxPanel->setMinimumWidth(textWidth);
    }
    if (m_evaluationPanel) {
        m_evaluationPanel->setMinimumWidth(textWidth);
    }
    if (m_scrollContent) {
        m_scrollContent->adjustSize();
        m_scrollContent->updateGeometry();
    }
}

void ExerciseHost::setExerciseChromeVisible(bool visible) {
    if (m_leftBackdrop) {
        m_leftBackdrop->setVisible(visible);
    }
    if (m_scrollArea) {
        m_scrollArea->setVisible(visible);
    }
    if (m_beginButton) {
        m_beginButton->setVisible(visible);
    }
    if (m_rightPanel) {
        m_rightPanel->setVisible(visible);
    }
    if (m_previewImage) {
        m_previewImage->setVisible(visible && !m_previewSource.isNull());
    }
    if (m_rightCountLabel) {
        m_rightCountLabel->setVisible(visible && m_exerciseDone);
    }
    if (m_wrongCountLabel) {
        m_wrongCountLabel->setVisible(visible && m_exerciseDone);
    }
}

void ExerciseHost::updateExerciseOverlayGeometry() {
    if (!m_onlyP || !m_exerciseRunning) {
        return;
    }
    QWidget *overlayRoot = m_onlyP->parentWidget();
    if (!overlayRoot) {
        return;
    }
    m_onlyP->setGeometry(0, 0, overlayRoot->width(), overlayRoot->height());
}

void ExerciseHost::showExerciseOverlay() {
    QWidget *overlayRoot = parentWidget();
    if (!m_onlyP || !overlayRoot) {
        return;
    }
    m_exerciseRunning = true;
    m_onlyP->setParent(overlayRoot);
    updateExerciseOverlayGeometry();
    lower();
    emit exerciseOverlayChanged(true);
}

void ExerciseHost::restoreExerciseOverlay() {
    m_exerciseRunning = false;
    if (!m_onlyP) {
        return;
    }
    m_onlyP->setParent(this);
    m_onlyP->setGeometry(0, 0, width(), height());
    m_onlyP->hide();
    raise();
    emit exerciseOverlayChanged(false);
}

void ExerciseHost::setDualScreenEnabled(bool enabled) {
    const bool wasDual = m_dualScreen;
    m_dualScreen = enabled;
    if (!enabled && m_patientDisplay) {
        m_patientDisplay->hideDisplay();
    }
    if (enabled && !wasDual && m_exerciseRunning) {
        syncPatientDisplay();
    }
}

void ExerciseHost::syncPatientDisplay() {
    if (!m_patientDisplay) {
        return;
    }
    if (!m_dualScreen || !m_exerciseRunning || !m_onlyP) {
        m_patientDisplay->hideDisplay();
        return;
    }
    m_patientDisplay->attachExercise(m_onlyP);
    m_patientDisplay->showOnSecondaryScreen();
}

void ExerciseHost::runOnlyPExercise() {
    m_protocolFormed = false;
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();
    m_dualScreen = AppSettings::dualScreenEnabled();
    m_exerciseRunning = true;

    if (m_dualScreen) {
        if (m_previewImage) {
            m_previewImage->hide();
        }
        m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Headless);
        m_onlyP->start(m_exerciseId);

        if (m_specialistExercise && m_rightPanel) {
            m_specialistExercise->setDisplayRole(OnlyPExercise::DisplayRole::Specialist);
            m_specialistExercise->setMirrorMode(true);
            m_specialistExercise->prepareMirrorUi(m_exerciseId);
            m_specialistExercise->showPicture(1);
            m_specialistExercise->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
            m_specialistExercise->show();
            m_specialistExercise->raise();
        }

        if (m_patientDisplay) {
            m_patientDisplay->attachExercise(m_onlyP);
        }
        syncPatientDisplay();
        updateChromeLayout();
        return;
    }

    setExerciseChromeVisible(false);
    showExerciseOverlay();
    m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Primary);
    m_onlyP->start(m_exerciseId);
    m_onlyP->raise();
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
    for (const ExerciseCheckRow &row : m_activityChecks) {
        if (row.box && row.box->isChecked() && row.label) {
            values.activity = row.label->text();
            break;
        }
    }
    QStringList helpValues;
    for (const ExerciseCheckRow &row : m_helpChecks) {
        if (row.box && row.box->isChecked() && row.label) {
            helpValues << row.label->text();
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
        m_partly ? m_repository->loadLastExerciseProtocolBody(m_patientId, m_exerciseId)
                 : QString(),
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
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    applyCompactLineHeight(m_templateBrowser->document());
    if (m_templateBrowser->document()) {
        m_templateBrowser->document()->setTextWidth(kTemplateTableWidth);
    }
    m_templateBrowser->setFixedWidth(kTemplateTableWidth);
    m_templatePanel->setMaximumWidth(kTemplateTableWidth + 16);
    layoutContent();
    QTimer::singleShot(80, this, [this]() { updateContentHeights(); });

    m_protocolFormed = true;
    m_partly = true;
    emit protocolSaved();
}
