#include "exercisehost.h"

#include "appsettings.h"
#include "custommessagebox.h"
#include "exerciseassets.h"
#include "exerciseconfig.h"
#include "exerciseprotocolcreate.h"
#include "exerciserunnerwidget.h"
#include "exercisesession.h"
#include "imagebutton.h"
#include "onlypexercise.h"
#include "patientdisplay.h"
#include "protocoleditguard.h"
#include "puzzlelayout.h"
#include "repository.h"

#include <QAbstractTextDocumentLayout>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPolygonF>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
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
constexpr int kTemplateViewportPadding = 4;

QString stepComboArrowCss() {
    static QString cachedPath;
    if (cachedPath.isEmpty()) {
        const QStringList roots = {
            QCoreApplication::applicationDirPath() + QStringLiteral("/../assets/sysImages"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/../../assets/sysImages"),
            QDir::currentPath() + QStringLiteral("/assets/sysImages"),
        };
        for (const QString &root : roots) {
            const QString candidate = QDir(root).filePath(QStringLiteral("combo_arrow.png"));
            if (QFile::exists(candidate)) {
                cachedPath = QDir::fromNativeSeparators(candidate);
                break;
            }
        }
        if (cachedPath.isEmpty()) {
            const QString filePath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                        .filePath(QStringLiteral("infant_step_combo_arrow.png"));
            if (!QFile::exists(filePath)) {
                QPixmap pixmap(10, 6);
                pixmap.fill(Qt::transparent);
                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0x33, 0x33, 0x33));
                QPolygonF arrow;
                arrow << QPointF(0.0, 0.0) << QPointF(10.0, 0.0) << QPointF(5.0, 6.0);
                painter.drawPolygon(arrow);
                pixmap.save(filePath, "PNG");
            }
            cachedPath = QDir::fromNativeSeparators(filePath);
        }
    }
    QString path = cachedPath;
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return QStringLiteral("width:10px; height:6px; image:url(\"%1\");").arg(path);
}

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
    m_scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_scrollContent->setStyleSheet(QStringLiteral("background-color:#ffffff;"));

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
    evaluationLayout->addSpacing(12);

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
    checkboxLayout->addSpacing(12);

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

    for (const ExerciseCheckRow &row : m_helpChecks) {
        connect(row.box, &QCheckBox::toggled, this, [this](bool) {});
    }

    m_donePanel = new OpaquePanel(kDocumentBg, m_evaluationPanel);
    auto *doneOuter = new QHBoxLayout(m_donePanel);
    doneOuter->setContentsMargins(0, 8, 0, 0);
    doneOuter->setSpacing(16);
    auto *doneTitle = new WhiteLabel(QStringLiteral("Выполнение"), m_donePanel);
    doneTitle->setStyleSheet(QStringLiteral("font: 10pt 'Microsoft Sans Serif'; color:#000000;"));
    doneOuter->addWidget(doneTitle, 0, Qt::AlignTop);
    auto *doneChecksHost = new QWidget(m_donePanel);
    auto *doneChecksLayout = new QVBoxLayout(doneChecksHost);
    doneChecksLayout->setContentsMargins(8, 4, 8, 4);
    doneChecksLayout->setSpacing(4);
    doneChecksHost->setStyleSheet(
        QStringLiteral("QWidget { background:#ffffff; border:1px solid #808080; }"));
    m_doneChecks << makeCheckRow(QStringLiteral("Выполнено"), doneChecksLayout, 220)
                 << makeCheckRow(QStringLiteral("Выполнено частично"), doneChecksLayout, 220)
                 << makeCheckRow(QStringLiteral("Не выполнено"), doneChecksLayout, 220);
    for (const ExerciseCheckRow &row : m_doneChecks) {
        connect(row.box, &QCheckBox::toggled, this, [this, row](bool checked) {
            if (!checked) {
                return;
            }
            for (const ExerciseCheckRow &other : m_doneChecks) {
                if (other.box && other.box != row.box) {
                    other.box->setChecked(false);
                }
            }
        });
    }
    doneOuter->addWidget(doneChecksHost, 0, Qt::AlignLeft | Qt::AlignTop);
    doneOuter->addStretch(1);
    m_donePanel->hide();
    evaluationLayout->addWidget(m_donePanel);

    evaluationLayout->addWidget(m_checkboxPanel);

    m_templatePanel = new OpaquePanel(kDocumentBg, m_scrollContent);
    auto *templateLayout = new QVBoxLayout(m_templatePanel);
    templateLayout->setContentsMargins(0, 24, 0, 8);
    templateLayout->setSpacing(0);

    m_formProtocolButton = new ImageButton(m_templatePanel);
    const QString formpPath = ExerciseAssets::sysImage(QStringLiteral("formp.png"));
    if (!formpPath.isEmpty()) {
        m_formProtocolButton->setImagePath(formpPath);
        m_formProtocolButton->setFixedSize(196, 33);
    }
    templateLayout->addWidget(m_formProtocolButton, 0, Qt::AlignHCenter);
    // Оригинал: PictureBox bsum = Resources.im1 («Подвести итог»), 135×30.
    m_sumButton = new ImageButton(m_templatePanel);
    const QString sumPath = ExerciseAssets::sysImage(QStringLiteral("im1.png"));
    if (!sumPath.isEmpty()) {
        m_sumButton->setImagePath(sumPath);
        m_sumButton->setFixedSize(135, 30);
    } else {
        m_sumButton->setText(QStringLiteral("Подвести итог"));
        m_sumButton->setFixedSize(135, 30);
    }
    m_sumButton->setToolTip(QStringLiteral("Подвести итог"));
    m_sumButton->hide();
    templateLayout->addSpacing(8);
    templateLayout->addWidget(m_sumButton, 0, Qt::AlignHCenter);
    templateLayout->addSpacing(12);
    templateLayout->addSpacing(12);

    m_templateBrowser = makeHtmlEditor(m_templatePanel);
    ProtocolEditGuard::install(m_templateBrowser, ProtocolEditGuard::Mode::ReadOnly);
    templateLayout->addWidget(m_templateBrowser);

    m_protocolSaveTimer = new QTimer(this);
    m_protocolSaveTimer->setSingleShot(true);
    m_protocolSaveTimer->setInterval(700);
    connect(m_protocolSaveTimer, &QTimer::timeout, this, &ExerciseHost::saveProtocolEdits);
    connect(m_templateBrowser->document(), &QTextDocument::contentsChanged, this, [this]() {
        if (!m_protocolSavedThisSession || m_currentProtocolId.isEmpty()) {
            return;
        }
        if (m_protocolSaveTimer) {
            m_protocolSaveTimer->start();
        }
    });

    layout->addWidget(orPanel);
    layout->addWidget(m_evaluationPanel);
    layout->addWidget(m_templatePanel);
    layout->addSpacing(120);
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

    m_rightCountLabel = new QLabel(m_rightPanel);
    m_rightCountLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_rightCountLabel->setAutoFillBackground(false);
    m_rightCountLabel->setStyleSheet(QStringLiteral(
        "QLabel { font: bold 20pt 'Arial'; color: #000000; background: transparent; border: none; padding: 0px; }"));
    m_rightCountLabel->hide();

    m_wrongCountLabel = new QLabel(m_rightPanel);
    m_wrongCountLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_wrongCountLabel->setAutoFillBackground(false);
    m_wrongCountLabel->setStyleSheet(m_rightCountLabel->styleSheet());
    m_wrongCountLabel->hide();

    m_timeResultLabel = new QLabel(m_rightPanel);
    m_timeResultLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_timeResultLabel->setStyleSheet(QStringLiteral(
        "QLabel { font: bold 16pt 'Arial'; color: #000000; background: transparent; border: none; }"));
    m_timeResultLabel->hide();

    m_stepCombo = new QComboBox(this);
    m_stepCombo->setStyleSheet(
        QStringLiteral(
            "QComboBox {"
            "  background:#ffffff; color:#000000; border:1px solid #7f9db9;"
            "  font-family:'Microsoft Sans Serif'; font-size:14px;"
            "  padding:1px 28px 1px 6px; min-height:20px;"
            "}"
            "QComboBox::drop-down {"
            "  subcontrol-origin: padding; subcontrol-position: top right;"
            "  width:22px; border:none; border-left:1px solid #b0b0b0; background:#ececec;"
            "}"
            "QComboBox::down-arrow { %1 }"
            "QComboBox QAbstractItemView {"
            "  background:#ffffff; color:#000000; selection-background-color:#cce8ff;"
            "  outline:0; border:1px solid #808080;"
            "}")
            .arg(stepComboArrowCss()));
    m_stepCombo->hide();

    m_exerciseOptionsPanel = new QWidget(m_rightPanel);
    auto *optionsLayout = new QVBoxLayout(m_exerciseOptionsPanel);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(6);

    m_shardButton = new QPushButton(QStringLiteral("Режим"), m_exerciseOptionsPanel);
    m_shardButton->hide();
    optionsLayout->addWidget(m_shardButton);

    m_e15ModeGroup = new QGroupBox(QStringLiteral("Режим выбора"), m_exerciseOptionsPanel);
    auto *e15Layout = new QVBoxLayout(m_e15ModeGroup);
    m_e15HighlightRadio = new QRadioButton(
        QStringLiteral("Выделение \"подсвечивание\" фрагментов при выборе"), m_e15ModeGroup);
    m_e15SelectRadio = new QRadioButton(QStringLiteral("Только выбор фрагментов"), m_e15ModeGroup);
    m_e15HighlightRadio->setChecked(true);
    e15Layout->addWidget(m_e15HighlightRadio);
    e15Layout->addWidget(m_e15SelectRadio);
    m_e15ModeGroup->hide();
    optionsLayout->addWidget(m_e15ModeGroup);

    m_showHintCheck = new QCheckBox(QStringLiteral("Показать пример (showp)"), m_exerciseOptionsPanel);
    m_showTemplateCheck = new QCheckBox(QStringLiteral("Показать трафарет (showt)"), m_exerciseOptionsPanel);
    m_rotateEnableCheck = new QCheckBox(QStringLiteral("Поворот фрагментов"), m_exerciseOptionsPanel);
    m_showHintCheck->setChecked(true);
    m_showTemplateCheck->setChecked(true);
    m_rotateEnableCheck->setChecked(true);
    m_showHintCheck->hide();
    m_showTemplateCheck->hide();
    m_rotateEnableCheck->hide();
    optionsLayout->addWidget(m_showHintCheck);
    optionsLayout->addWidget(m_showTemplateCheck);
    optionsLayout->addWidget(m_rotateEnableCheck);

    auto *rotateRow = new QHBoxLayout();
    m_rotateWCombo = new QComboBox(m_exerciseOptionsPanel);
    m_rotateCWCombo = new QComboBox(m_exerciseOptionsPanel);
    rotateRow->addWidget(new QLabel(QStringLiteral("По ширине:"), m_exerciseOptionsPanel));
    rotateRow->addWidget(m_rotateWCombo);
    rotateRow->addWidget(new QLabel(QStringLiteral("По часовой:"), m_exerciseOptionsPanel));
    rotateRow->addWidget(m_rotateCWCombo);
    optionsLayout->addLayout(rotateRow);
    m_rotateWCombo->hide();
    m_rotateCWCombo->hide();
    m_exerciseOptionsPanel->hide();

    connect(m_shardButton, &QPushButton::clicked, this, [this]() {
        m_shardPanelVisible = !m_shardPanelVisible;
        if (m_e15ModeGroup) {
            m_e15ModeGroup->setVisible(m_shardPanelVisible);
        }
    });
    connect(m_stepCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index < 0) {
            return;
        }
        m_sessionStepId = currentStepId();
        refreshRotateCombos();
        reloadPreviewForCurrentStep();
        if (m_exerciseRunning && m_onlyP && m_onlyP->isVisible() && !m_sessionStepId.isEmpty()) {
            m_onlyP->switchStep(m_sessionStepId);
        }
        if (m_exerciseRunning && m_sessionRunner
            && (m_sessionRunnerKind == ExerciseRunnerKind::E126
                || m_sessionRunnerKind == ExerciseRunnerKind::E1272)
            && !m_sessionStepId.isEmpty()) {
            m_sessionRunner->switchStep(m_sessionStepId);
        }
        layoutStepCombo();
        QTimer::singleShot(0, this, [this]() { layoutStepCombo(); });
    });
    connect(m_rotateWCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_rotateWCombo || !m_rotateCWCombo) {
            return;
        }
        const int fragmentCount = puzzleFragmentCount();
        const int w = m_rotateWCombo->currentText().toInt();
        m_rotateCWCombo->blockSignals(true);
        m_rotateCWCombo->clear();
        for (int i = 0; i <= qMax(0, fragmentCount - w); ++i) {
            m_rotateCWCombo->addItem(QString::number(i));
        }
        if (m_rotateCWCombo->count() > 0) {
            m_rotateCWCombo->setCurrentIndex(0);
        }
        m_rotateCWCombo->blockSignals(false);
    });
    connect(m_rotateEnableCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (m_rotateWCombo) {
            m_rotateWCombo->setEnabled(enabled);
        }
        if (m_rotateCWCombo) {
            m_rotateCWCombo->setEnabled(enabled);
        }
    });

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
        // 1.26: не трогаем уже сохранённый протокол в БД перед заданием 2 —
        // иначе merge из редактора может повредить блок задания 1.
        if (m_exerciseId == QStringLiteral("1.26")) {
            const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
            const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
            if (m_templateBrowser) {
                m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
                applyCompactLineHeight(m_templateBrowser->document());
            }
            m_currentProtocolId.clear();
            m_protocolSavedThisSession = false;
            updateProtocolEditMode();
        } else {
            resetProtocolToInitialTemplate();
        }
        runExerciseSession();
    });
    connect(m_formProtocolButton, &ImageButton::clicked, this, [this]() { formProtocol(); });
    connect(m_sumButton, &ImageButton::clicked, this, [this]() { sumProtocol126(); });
    connect(m_onlyP, &OnlyPExercise::finished, this, [this](const QList<bool> &answers, int elapsedSeconds) {
        m_answers = answers;
        m_elapsedSeconds = elapsedSeconds;
        if (m_onlyP) {
            m_stepElapsedSeconds = m_onlyP->stepElapsedSeconds();
            m_picturesShown = m_onlyP->picturesShown();
        }
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
            if (m_previewImage) {
                m_previewImage->hide();
            }
        } else {
            restoreExerciseOverlay();
        }
        setExerciseChromeVisible(true);
        updateChromeLayout();
        showResultLabels(answers, elapsedSeconds);
        emit exerciseOverlayChanged(false);
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
        layoutStepCombo();
        return;
    }

    if (m_leftBackdrop) {
        m_leftBackdrop->setGeometry(0, 0, kPanelX + kScrollWidth + kScrollBarGutter, height());
        m_leftBackdrop->lower();
    }
    if (m_scrollArea) {
        m_scrollArea->setGeometry(kPanelX, kPanelY, kScrollWidth, qMax(100, height() - kPanelY));
        m_scrollArea->raise();
        if (m_scrollContent && m_scrollArea->viewport()) {
            m_scrollContent->setMinimumWidth(m_scrollArea->viewport()->width());
        }
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
        m_beginButton->setVisible(!m_exerciseRunning);
        m_beginButton->raise();
    }
    if (m_exerciseOptionsPanel && m_rightPanel) {
        m_exerciseOptionsPanel->setGeometry(12, 52, qMax(120, m_rightPanel->width() - 24), 220);
        m_exerciseOptionsPanel->raise();
    }
    if (m_previewImage) {
        updatePreviewLayout();
    }
    if (m_rightPanel && m_rightCountLabel && m_wrongCountLabel) {
        const int panelW = qMax(120, m_rightPanel->width());
        constexpr int kCounterLabelW = 320;
        constexpr int kCounterLabelH = 44;
        const int labelX = qMax(0, (panelW - kCounterLabelW) / 2);
        m_rightCountLabel->setGeometry(labelX, 250, kCounterLabelW, kCounterLabelH);
        m_wrongCountLabel->setGeometry(labelX, 250 + kCounterLabelH + 10, kCounterLabelW, kCounterLabelH);
        m_rightCountLabel->raise();
        m_wrongCountLabel->raise();
    }

    layoutStepCombo();
}

void ExerciseHost::layoutStepCombo() {
    if (!m_stepCombo) {
        return;
    }
    if (m_stepCombo->count() <= 0) {
        m_stepCombo->hide();
        return;
    }

    // Во время упражнения оверлей на parent (m_root) — селект тоже там и поверх оверлея.
    QWidget *host = this;
    if (m_exerciseRunning && parentWidget()) {
        host = parentWidget();
    }
    if (m_stepCombo->parentWidget() != host) {
        m_stepCombo->setParent(host);
    }

    constexpr int kComboW = 121;
    constexpr int kComboH = 33;
    constexpr int kComboY = 12;
    constexpr int kRightMargin = 24;
    int comboX = qMax(0, host->width() - kComboW - kRightMargin);
    // 1.26/1.272: не поверх правой панели задания 2; +100px правее прежней позиции.
    if (m_exerciseRunning
        && (m_exerciseId == QStringLiteral("1.26") || m_exerciseId == QStringLiteral("1.272"))) {
        comboX = qMax(0, qMin(comboX, 1220));
    }
    const int comboY = (host == this) ? kComboY : (y() + kComboY);
    m_stepCombo->setStyleSheet(
        QStringLiteral(
            "QComboBox { background-color:#ffffff; background-image:none; color:#000000;"
            " border:1px solid #808080; padding-left:6px; }"
            "QComboBox QAbstractItemView { background-color:#ffffff; background-image:none; color:#000000; }"
            "%1")
            .arg(stepComboArrowCss()));
    m_stepCombo->setGeometry(comboX, comboY, kComboW, kComboH);
    m_stepCombo->setVisible(true);
    m_stepCombo->raise();
    // Выпадающий список тоже поверх оверлея.
    if (QWidget *popup = m_stepCombo->view() ? m_stepCombo->view()->window() : nullptr) {
        popup->raise();
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
    m_protocolSavedThisSession = false;
    m_partly = false;
    if (m_repository && !patientId.trimmed().isEmpty()) {
        const QString existingBody = m_repository->loadLastExerciseProtocolBody(patientId, exerciseId);
        if (!existingBody.trimmed().isEmpty()) {
            m_partly = true;
        }
    }
    m_sessionAdditional.clear();
    m_additionalByStep.clear();
    m_sessionStepId.clear();
    m_picturesShown = 0;
    m_currentProtocolId.clear();
    m_orOpen1 = false;
    m_orOpen2 = false;
    m_orOpen3 = false;
    m_answers.clear();
    m_elapsedSeconds = 0;
    m_stepElapsedSeconds.clear();
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();
    if (m_timeResultLabel) {
        m_timeResultLabel->hide();
    }
    setExerciseChromeVisible(true);

    for (const ExerciseCheckRow &row : m_activityChecks) {
        row.box->setChecked(false);
    }
    for (const ExerciseCheckRow &row : m_helpChecks) {
        row.box->setChecked(false);
    }

    if (const ExerciseDefinition *definition = ExerciseConfig::find(exerciseId)) {
        if (definition->availableInVersion
            && definition->runner != ExerciseRunnerKind::NotImplemented) {
            loadExercise();
        }
    }
    show();
    raise();
}

void ExerciseHost::updatePreviewLayout() {
    if (!m_previewImage) {
        return;
    }
    if (m_dualScreen && m_exerciseRunning) {
        m_previewImage->hide();
        return;
    }
    if (m_previewSource.isNull()) {
        m_previewImage->hide();
        return;
    }

    constexpr int kPreviewAbsLeft = 1100;
    constexpr int kPreviewAbsTop = 75;
    const int rightPanelLeft = kPanelX + kScrollWidth;
    const int localX = kPreviewAbsLeft - rightPanelLeft;
    const int localY = kPreviewAbsTop;
    const int maxW = qMax(120, width() - kPreviewAbsLeft - 16);
    const int maxH = qMax(120, height() - kPreviewAbsTop - 16);
    QPixmap display = m_previewSource;
    if (m_exerciseId == QStringLiteral("1.1")) {
        display = m_previewSource.scaled(
            qMax(1, m_previewSource.width() / 2),
            qMax(1, m_previewSource.height() / 2),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
    } else if (display.width() > maxW || display.height() > maxH) {
        display = m_previewSource.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_previewImage->setPixmap(display);
    m_previewImage->setFixedSize(display.size());
    m_previewImage->move(qMax(0, localX), localY);
    m_previewImage->show();
    m_previewImage->raise();
    if (m_timeResultLabel && m_timeResultLabel->isVisible()) {
        // По центру правой панели (правее превью по горизонтали).
        const int rightPanelWidth = qMax(1, width() - rightPanelLeft);
        m_timeResultLabel->adjustSize();
        const int timerX = qMax(0, (rightPanelWidth - m_timeResultLabel->width()) / 2);
        const int timerY = qMax(8, localY - 36);
        m_timeResultLabel->move(timerX, timerY);
        m_timeResultLabel->raise();
    }
}

void ExerciseHost::reloadPreviewForCurrentStep() {
    m_previewSource = QPixmap();
    const QString step = currentStepId();
    QStringList candidates;
    if (m_exerciseId == QStringLiteral("1.26")) {
        // Оригинал: задание 1 → d1.png, задание 2 → f2.png
        if (step == QStringLiteral("2")) {
            candidates << QStringLiteral("f2.png");
        } else {
            candidates << QStringLiteral("d1.png") << QStringLiteral("f1.png");
        }
    } else if (!step.isEmpty()) {
        candidates << QStringLiteral("f") + step + QStringLiteral(".png")
                   << step + QStringLiteral(".png")
                   << QStringLiteral("p") + step + QStringLiteral(".png")
                   << QStringLiteral("f1.png");
    } else {
        candidates << QStringLiteral("f1.png") << QStringLiteral("p1.png") << QStringLiteral("1.png");
    }
    for (const QString &name : candidates) {
        const QString path = ExerciseAssets::exerciseFile(m_exerciseId, name);
        if (!path.isEmpty() && m_previewSource.load(path)) {
            break;
        }
    }
    updatePreviewLayout();
}

void ExerciseHost::loadExercise() {
    m_evaluationPanel->show();

    m_rawOrHtml = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("or.html"));
    reloadOrBrowser();

    const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
    if (m_repository && m_partly) {
        const QString lastBody = m_repository->loadLastExerciseProtocolBody(m_patientId, m_exerciseId);
        const QString lastId = m_repository->loadLastExerciseProtocolId(m_patientId, m_exerciseId);
        if (!lastBody.trimmed().isEmpty()) {
            m_currentProtocolId = lastId;
            const QString viewHtml = m_repository->loadProtocolViewHtml(
                m_exerciseId, lastId, m_patientFio, m_patientBirthDate);
            if (!viewHtml.trimmed().isEmpty()) {
                m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
            } else {
                m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
            }
        } else {
            m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
        }
    } else {
        m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
    }
    applyCompactLineHeight(m_templateBrowser->document());
    updateProtocolEditMode();

    if (m_donePanel) {
        m_donePanel->setVisible(needsDoneStatePanel());
    }
    for (const ExerciseCheckRow &row : m_doneChecks) {
        if (row.box) {
            row.box->setChecked(false);
        }
    }

    m_previewSource = QPixmap();
    reloadPreviewForCurrentStep();

    if (m_stepCombo) {
        m_stepCombo->blockSignals(true);
        m_stepCombo->clear();
        if (const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId)) {
            if (!definition->onlyPicture.stepIds.isEmpty()) {
                m_stepCombo->addItems(definition->onlyPicture.stepIds);
                m_stepCombo->setCurrentIndex(0);
                m_sessionStepId = m_stepCombo->currentText();
            }
        }
        m_stepCombo->blockSignals(false);
        reloadPreviewForCurrentStep();
        layoutStepCombo();
    }

    updateExerciseOptionsPanel();
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
        if (QTextDocument *doc = m_templateBrowser->document()) {
            doc->setDocumentMargin(kTemplateViewportPadding / 2);
            doc->setTextWidth(kTemplateTableWidth);
        }
        const int templateHeight = static_cast<int>(qCeil(m_templateBrowser->document()->size().height())) + 2;
        const int templateViewportWidth = kTemplateTableWidth + kTemplateViewportPadding;
        m_templateBrowser->setMinimumHeight(templateHeight);
        m_templateBrowser->setMaximumHeight(templateHeight);
        m_templateBrowser->setMinimumWidth(templateViewportWidth);
        m_templateBrowser->setMaximumWidth(templateViewportWidth);
        m_templatePanel->setMinimumWidth(templateViewportWidth);
        m_templatePanel->setMaximumWidth(templateViewportWidth + 16);
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
        if (m_scrollArea && m_scrollArea->viewport()) {
            m_scrollContent->setMinimumWidth(m_scrollArea->viewport()->width());
        }
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
        m_beginButton->setVisible(visible && !m_exerciseRunning);
    }
    if (m_rightPanel) {
        m_rightPanel->setVisible(visible);
    }
    if (m_previewImage) {
        m_previewImage->setVisible(visible && !m_previewSource.isNull() && !(m_dualScreen && m_exerciseRunning));
    }
    if (m_rightCountLabel) {
        m_rightCountLabel->setVisible(visible && m_exerciseDone && m_rightCountLabel->text().startsWith(QStringLiteral("Верно")));
    }
    if (m_wrongCountLabel) {
        m_wrongCountLabel->setVisible(visible && m_exerciseDone && m_wrongCountLabel->text().startsWith(QStringLiteral("Неверно")));
    }
    if (m_timeResultLabel) {
        m_timeResultLabel->setVisible(visible && m_exerciseDone && !m_timeResultLabel->text().isEmpty());
    }
}

void ExerciseHost::updateExerciseOverlayGeometry() {
    if (m_sessionRunner && m_exerciseRunning) {
        QWidget *overlayRoot = m_sessionRunner->parentWidget();
        if (overlayRoot) {
            m_sessionRunner->setGeometry(0, 0, overlayRoot->width(), overlayRoot->height());
        }
        return;
    }
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
    if (!overlayRoot) {
        return;
    }
    m_exerciseRunning = true;
    QWidget *overlayWidget = nullptr;
    if (m_sessionRunner && m_sessionRunner->isVisible()) {
        overlayWidget = m_sessionRunner;
    } else if (m_onlyP) {
        overlayWidget = m_onlyP;
    }
    if (!overlayWidget) {
        return;
    }
    overlayWidget->setParent(overlayRoot);
    updateExerciseOverlayGeometry();
    lower();
    emit exerciseOverlayChanged(true);
}

void ExerciseHost::restoreExerciseOverlay() {
    m_exerciseRunning = false;
    if (m_sessionRunner) {
        m_sessionRunner->hide();
        m_sessionRunner->setParent(this);
        m_sessionRunner->setGeometry(0, 0, width(), height());
    }
    if (m_stepCombo && m_stepCombo->parentWidget() != this) {
        m_stepCombo->setParent(this);
    }
    if (!m_onlyP) {
        raise();
        layoutStepCombo();
        emit exerciseOverlayChanged(false);
        return;
    }
    m_onlyP->setParent(this);
    m_onlyP->setGeometry(0, 0, width(), height());
    m_onlyP->hide();
    raise();
    layoutStepCombo();
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
    if (!m_dualScreen || !m_exerciseRunning) {
        m_patientDisplay->hideDisplay();
        return;
    }
    if (m_onlyP && m_onlyP->isVisible()) {
        m_patientDisplay->attachExercise(m_onlyP);
        m_patientDisplay->showOnSecondaryScreen();
        return;
    }
    if (m_sessionRunner && m_sessionRunner->isVisible()) {
        m_patientDisplay->attachMirrorWidget(m_sessionRunner);
        m_patientDisplay->showOnSecondaryScreen();
    }
}

void ExerciseHost::runExerciseSession() {
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    if (!definition) {
        return;
    }
    // Для numbered OnlyPicture (1.17/1.18) при старте сбрасываем на 1-е задание.
    // Для 1.26 и др. — уважаем выбранное в селекте задание.
    if (m_stepCombo && m_stepCombo->count() > 0
        && definition->runner == ExerciseRunnerKind::OnlyPicture
        && definition->protocol == ExerciseProtocolKind::NumberedDoneTime) {
        m_stepCombo->blockSignals(true);
        m_stepCombo->setCurrentIndex(0);
        m_stepCombo->blockSignals(false);
        m_sessionStepId = m_stepCombo->currentText().trimmed();
    } else {
        m_sessionStepId = currentStepId();
    }
    if (definition->runner == ExerciseRunnerKind::OnlyPicture) {
        runOnlyPExercise();
        return;
    }

    m_protocolFormed = false;
    m_protocolSavedThisSession = false;
    m_stepElapsedSeconds.clear();
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();
    m_exerciseRunning = true;
    if (m_beginButton) {
        m_beginButton->hide();
    }
    emit exerciseOverlayChanged(true);

    if (!m_sessionRunner || m_sessionRunnerKind != definition->runner) {
        delete m_sessionRunner;
        m_sessionRunner = nullptr;
        m_sessionRunner = createExerciseRunner(definition->runner, this);
        m_sessionRunnerKind = definition->runner;
        connect(m_sessionRunner, &ExerciseRunnerWidget::sessionFinished, this,
            [this](const ExerciseSessionResult &result) {
                m_answers = result.answers;
                m_elapsedSeconds = result.elapsedSeconds;
                const QString step = currentStepId();
                if (!step.isEmpty()) {
                    m_stepElapsedSeconds.insert(step, result.elapsedSeconds);
                }
                m_sessionAdditional = result.additional;
                if (m_exerciseId == QStringLiteral("1.26") || m_exerciseId == QStringLiteral("1.272")) {
                    const QStringList parts = result.additional.split(QLatin1Char(';'));
                    const QString stepKey = parts.isEmpty() || parts.at(0).trimmed().isEmpty()
                        ? QStringLiteral("1")
                        : parts.at(0).trimmed();
                    m_additionalByStep.insert(stepKey, result.additional);
                }
                m_picturesShown = result.picturesShown;
                m_capturedImagePath = result.capturedImagePath;
                if (!result.capturedImagePath.isEmpty()) {
                    m_previewSource.load(result.capturedImagePath);
                    updatePreviewLayout();
                }
                m_exerciseDone = true;
                m_protocolFormed = false;
                m_exerciseRunning = false;
                if (m_patientDisplay) {
                    m_patientDisplay->hideDisplay();
                }
                restoreExerciseOverlay();
                setExerciseChromeVisible(true);
                updateChromeLayout();
                showResultLabels(result.answers, result.elapsedSeconds);
                emit exerciseOverlayChanged(false);
            });
    } else if (m_sessionRunner->parent() != this) {
        m_sessionRunner->setParent(this);
    }

    setExerciseChromeVisible(false);
    QWidget *overlayRoot = parentWidget();
    if (!overlayRoot) {
        return;
    }
    m_sessionStepId = currentStepId();
    m_sessionRunner->setParent(overlayRoot);
    m_sessionRunner->setGeometry(0, 0, overlayRoot->width(), overlayRoot->height());
    lower();
    m_sessionRunner->setSessionOptions(buildSessionOptions());
    m_dualScreen = AppSettings::dualScreenEnabled();
    m_sessionRunner->startSession(m_exerciseId, *definition, m_sessionStepId);
    m_sessionRunner->show();
    m_sessionRunner->raise();
    layoutStepCombo();
    QTimer::singleShot(0, this, [this]() { layoutStepCombo(); });
    syncPatientDisplay();
}

void ExerciseHost::runOnlyPExercise() {
    m_protocolFormed = false;
    m_protocolSavedThisSession = false;
    m_stepElapsedSeconds.clear();
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();
    m_dualScreen = AppSettings::dualScreenEnabled();
    m_exerciseRunning = true;
    if (m_beginButton) {
        m_beginButton->hide();
    }
    emit exerciseOverlayChanged(true);

    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    const OnlyPictureSettings settings =
        definition ? definition->onlyPicture : OnlyPictureSettings();
    m_sessionStepId = currentStepId();
    const QString stepId = m_sessionStepId;

    if (m_dualScreen) {
        if (m_previewImage) {
            m_previewImage->hide();
        }
        m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Headless);
        m_onlyP->start(m_exerciseId, settings, stepId);

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
        layoutStepCombo();
        return;
    }

    setExerciseChromeVisible(false);
    showExerciseOverlay();
    m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Primary);
    m_onlyP->start(m_exerciseId, settings, stepId);
    m_onlyP->raise();
    layoutStepCombo();
    QTimer::singleShot(0, this, [this]() { layoutStepCombo(); });
}

void ExerciseHost::showResultLabels(const QList<bool> &answers, int elapsedSeconds) {
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    const int minutes = elapsedSeconds / 60;
    const int seconds = elapsedSeconds % 60;
    const QString timeText = QStringLiteral("%1:%2 сек")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));

    const bool showAnswerCounts = definition && definition->onlyPicture.answerButtons;
    if (showAnswerCounts) {
        int right = 0;
        int wrong = 0;
        for (bool answer : answers) {
            if (answer) {
                ++right;
            } else {
                ++wrong;
            }
        }
        m_rightCountLabel->setText(QStringLiteral("Верно %1").arg(right));
        m_wrongCountLabel->setText(QStringLiteral("Неверно %1").arg(wrong));
        m_rightCountLabel->show();
        m_wrongCountLabel->show();
        m_rightCountLabel->raise();
        m_wrongCountLabel->raise();
    } else {
        m_rightCountLabel->hide();
        m_wrongCountLabel->hide();
    }

    // Таймер результата — не для 1.26/1.272 (в оригинале не показывают).
    const bool hideResultTimer = m_exerciseId == QStringLiteral("1.26")
        || m_exerciseId == QStringLiteral("1.272");
    if (m_timeResultLabel && !showAnswerCounts && !hideResultTimer && elapsedSeconds >= 0
        && m_exerciseDone) {
        m_timeResultLabel->setText(timeText);
        m_timeResultLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        m_timeResultLabel->adjustSize();
        m_timeResultLabel->show();
        m_timeResultLabel->raise();
        updatePreviewLayout();
    } else if (m_timeResultLabel) {
        m_timeResultLabel->hide();
    }
}

bool ExerciseHost::needsDoneStatePanel() const {
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    if (!definition) {
        return false;
    }
    return definition->protocol == ExerciseProtocolKind::DoneTimeOrHlp
        || definition->protocol == ExerciseProtocolKind::NumberedDoneTime;
}

QString ExerciseHost::selectedDoneState() const {
    for (const ExerciseCheckRow &row : m_doneChecks) {
        if (row.box && row.box->isChecked() && row.label) {
            return row.label->text();
        }
    }
    const QString fromOr = readDoneStateFromOrHtml(orHtmlSnapshot());
    if (!fromOr.isEmpty() && fromOr != QStringLiteral("не определено")) {
        return fromOr;
    }
    return QStringLiteral("не определено");
}

QStringList ExerciseHost::numberedStepIds() const {
    if (m_stepCombo && m_stepCombo->count() > 0) {
        QStringList ids;
        for (int i = 0; i < m_stepCombo->count(); ++i) {
            const QString text = m_stepCombo->itemText(i).trimmed();
            if (!text.isEmpty()) {
                ids << text;
            }
        }
        if (!ids.isEmpty()) {
            return ids;
        }
    }
    if (const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId)) {
        return definition->onlyPicture.stepIds;
    }
    return {};
}

QString ExerciseHost::currentStepId() const {
    if (m_stepCombo && m_stepCombo->count() > 0) {
        const QString text = m_stepCombo->currentText().trimmed();
        if (!text.isEmpty()) {
            return text;
        }
    }
    return m_sessionStepId;
}

ProtocolSessionInput ExerciseHost::buildProtocolSession() const {
    ProtocolSessionInput session;
    session.doneState = selectedDoneState();
    session.stepId = currentStepId();
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    if (m_exerciseId == QStringLiteral("1.4")) {
        session.picturesShown = m_picturesShown > 0 ? m_picturesShown - 1 : 0;
    } else {
        session.picturesShown = m_picturesShown;
    }

    // Раннер уже положил данные (ответы, цифры, сказка и т.п.) — не затираем.
    const bool keepRunnerAdditional = !m_sessionAdditional.isEmpty()
        && (m_exerciseId == QStringLiteral("1.26") || m_exerciseId == QStringLiteral("1.272")
            || m_exerciseId == QStringLiteral("4.1.8") || m_exerciseId == QStringLiteral("4.2.1")
            || m_exerciseId == QStringLiteral("4.2.2") || m_exerciseId == QStringLiteral("5.1.1")
            || m_exerciseId == QStringLiteral("5.2.1") || m_exerciseId == QStringLiteral("5.4.2"));

    if (keepRunnerAdditional) {
        session.additional = m_sessionAdditional;
    } else if (m_exerciseId == QStringLiteral("1.26") || m_exerciseId == QStringLiteral("1.272")) {
        // Как в оригинале: param1 + ";" + answers[0..12].join(";")
        const QString step = session.stepId.trimmed().isEmpty() ? QStringLiteral("1") : session.stepId;
        QStringList emptyAnswers;
        for (int i = 0; i < 13; ++i) {
            emptyAnswers << QString();
        }
        session.additional = step + QLatin1Char(';') + emptyAnswers.join(QLatin1Char(';'));
    } else if (definition && definition->protocol == ExerciseProtocolKind::NumberedDoneTime) {
        // № = номера заданий из селекта (1/2/3…); время — отдельно на каждое в stepElapsedSeconds.
        session.stepIds = numberedStepIds();
        session.stepElapsedSeconds = m_stepElapsedSeconds;
        if (session.stepId.isEmpty()) {
            session.stepId = QStringLiteral("1");
        }
        if (session.stepElapsedSeconds.isEmpty() && !session.stepId.isEmpty()) {
            session.stepElapsedSeconds.insert(session.stepId, m_elapsedSeconds);
        }
        if (session.stepIds.isEmpty()) {
            session.stepIds << session.stepId;
        }
        // Для совместимости с fallback/шаблонами без multi-row.
        session.additional = session.stepId + QLatin1Char(';') + session.doneState;
    } else if (definition && definition->protocol == ExerciseProtocolKind::DoneTimeOrHlp) {
        session.additional = session.doneState;
    } else if (!m_sessionAdditional.isEmpty()) {
        session.additional = m_sessionAdditional;
    } else if (m_exerciseId == QStringLiteral("1.2")) {
        QStringList parts;
        for (bool answer : m_answers) {
            parts << (answer ? QStringLiteral("True") : QStringLiteral("False"));
        }
        session.additional = parts.join(QLatin1Char(';'));
    }
    session.capturedImagePath = m_capturedImagePath;
    session.orHtml = orHtmlSnapshot();
    return session;
}

ExerciseProtocol::CheckboxValues ExerciseHost::checkboxValues() const {
    ExerciseProtocol::CheckboxValues values;
    QStringList activityValues;
    for (const ExerciseCheckRow &row : m_activityChecks) {
        if (row.box && row.box->isChecked() && row.label) {
            activityValues << row.label->text();
        }
    }
    values.activity = activityValues.join(QStringLiteral("\n"));
    QStringList helpValues;
    for (const ExerciseCheckRow &row : m_helpChecks) {
        if (row.box && row.box->isChecked() && row.label) {
            helpValues << row.label->text();
        }
    }
    values.help = helpValues.join(QStringLiteral("\n"));
    return values;
}

QString ExerciseHost::orHtmlSnapshot() const {
    return m_orBrowser ? m_orBrowser->toHtml() : QString();
}

void ExerciseHost::resetProtocolToInitialTemplate() {
    if (!m_templateBrowser) {
        return;
    }
    // Сохраняем правки «Результат/Примечание», но не даём merge сломать разметку:
    // дописывание следующей сессии идёт через appendRowsToStoredBody.
    if (m_protocolSavedThisSession && !m_currentProtocolId.isEmpty()) {
        saveProtocolEdits();
    }
    const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
    m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
    applyCompactLineHeight(m_templateBrowser->document());
    if (QTextDocument *doc = m_templateBrowser->document()) {
        doc->setDocumentMargin(kTemplateViewportPadding / 2);
        doc->setTextWidth(kTemplateTableWidth);
    }
    const int templateViewportWidth = kTemplateTableWidth + kTemplateViewportPadding;
    m_templateBrowser->setFixedWidth(templateViewportWidth);
    if (m_templatePanel) {
        m_templatePanel->setMaximumWidth(templateViewportWidth + 16);
    }
    m_currentProtocolId.clear();
    m_protocolSavedThisSession = false;
    updateProtocolEditMode();
    layoutContent();
    QTimer::singleShot(80, this, [this]() { updateContentHeights(); });
}

void ExerciseHost::updateProtocolEditMode() {
    if (!m_templateBrowser) {
        return;
    }
    // Редактирование только после формирования протокола в текущей сессии.
    const ProtocolEditGuard::Mode mode = m_protocolSavedThisSession
        ? ProtocolEditGuard::Mode::LimitedEdit
        : ProtocolEditGuard::Mode::ReadOnly;
    ProtocolEditGuard::setMode(m_templateBrowser, mode);
    updateSumButtonVisibility();
}

void ExerciseHost::updateSumButtonVisibility() {
    if (!m_sumButton) {
        return;
    }
    const bool show = m_exerciseId == QStringLiteral("1.26") && m_protocolSavedThisSession;
    m_sumButton->setVisible(show);
}

namespace {

void commitTextEditChanges(QTextEdit *editor, bool preserveFocus) {
    if (!editor) {
        return;
    }
    const QTextCursor cursor = editor->textCursor();
    editor->setTextCursor(cursor);
    if (!preserveFocus && editor->hasFocus()) {
        editor->clearFocus();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

} // namespace

void ExerciseHost::sumProtocol126() {
    if (m_exerciseId != QStringLiteral("1.26") || !m_repository || m_currentProtocolId.isEmpty()
        || !m_templateBrowser) {
        return;
    }
    commitTextEditChanges(m_templateBrowser, true);
    QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }
    // Сначала обычные поля, затем баллы + сумма как bsum в оригинале.
    storedBody = ExerciseProtocol::mergeLimitedEditableFieldsIntoStoredBody(
        storedBody, m_templateBrowser->document());
    storedBody = ExerciseProtocol::applyProtocol126SumFromDocument(
        storedBody, m_templateBrowser->document(), true);
    storedBody = ExerciseProtocol::normalizeStoredProtocolBody(storedBody);

    QString error;
    if (!m_repository->updateProtocolBody(m_currentProtocolId, storedBody, &error)) {
        CustomMessageBox::showError(this, error);
        return;
    }

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    applyCompactLineHeight(m_templateBrowser->document());
    if (QTextDocument *doc = m_templateBrowser->document()) {
        doc->setDocumentMargin(kTemplateViewportPadding / 2);
        doc->setTextWidth(kTemplateTableWidth);
    }
    updateContentHeights();
    updateProtocolEditMode();
}

void ExerciseHost::saveProtocolEdits() {
    if (!m_protocolSavedThisSession) {
        return;
    }
    if (!m_repository || m_currentProtocolId.isEmpty() || !m_templateBrowser) {
        return;
    }
    commitTextEditChanges(m_templateBrowser, true);
    const QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }

    // Для 1.1 и прочих полная пересборка через QTextDocument ломает границы сессий
    // (3-й+ протокол оказывается внутри таблицы). Для них — только Результат/Примечание.
    QString body;
    if (m_exerciseId == QStringLiteral("1.2")) {
        body = ExerciseProtocol::mergeEditorDocumentIntoStoredBody(
            storedBody, m_templateBrowser->document(), 0);
    } else if (m_exerciseId == QStringLiteral("1.26")) {
        body = ExerciseProtocol::mergeLimitedEditableFieldsIntoStoredBody(
            storedBody, m_templateBrowser->document());
        body = ExerciseProtocol::applyProtocol126SumFromDocument(
            body, m_templateBrowser->document(), false);
    } else {
        body = ExerciseProtocol::mergeLimitedEditableFieldsIntoStoredBody(
            storedBody, m_templateBrowser->document());
    }
    body = ExerciseProtocol::normalizeStoredProtocolBody(body);
    QString error;
    m_repository->updateProtocolBody(m_currentProtocolId, body, &error);
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
    if (m_protocolSavedThisSession) {
        CustomMessageBox::showError(
            this, QStringLiteral("Формирование протокола невозможно без выполнения упражнения"));
        return;
    }

    saveProtocolEdits();

    const bool partlySave = m_partly;
    QString existingBody = partlySave
        ? m_repository->loadLastExerciseProtocolBody(m_patientId, m_exerciseId)
        : QString();

    ProtocolSessionInput session = buildProtocolSession();
    if (m_exerciseId == QStringLiteral("1.26") && !session.additional.trimmed().isEmpty()) {
        const QStringList parts = session.additional.split(QLatin1Char(';'));
        const QString stepKey = parts.isEmpty() || parts.at(0).trimmed().isEmpty()
            ? QStringLiteral("1")
            : parts.at(0).trimmed();
        m_additionalByStep.insert(stepKey, session.additional);
    }

    QString protocolBody;
    bool reconstructed126 = false;
    if (m_exerciseId == QStringLiteral("1.26")) {
        const QStringList parts = session.additional.split(QLatin1Char(';'));
        const QString stepKey = parts.isEmpty() || parts.at(0).trimmed().isEmpty()
            ? QStringLiteral("1")
            : parts.at(0).trimmed();

        // Если формируем задание 2, а в сохранённом протоколе нет задания 1 —
        // сначала восстанавливаем блок 1 из ответов этой сессии.
        if (stepKey == QStringLiteral("2")
            && !existingBody.contains(QStringLiteral("Задание 1"))
            && m_additionalByStep.contains(QStringLiteral("1"))) {
            ProtocolSessionInput session1 = session;
            session1.additional = m_additionalByStep.value(QStringLiteral("1"));
            const QString body1 = ExerciseProtocol::createProtocolHtml(
                m_exerciseId,
                m_specialistFio,
                m_elapsedSeconds,
                false,
                QString(),
                m_answers,
                checkboxValues(),
                session1);
            protocolBody = ExerciseProtocol::createProtocolHtml(
                m_exerciseId,
                m_specialistFio,
                m_elapsedSeconds,
                true,
                body1,
                m_answers,
                checkboxValues(),
                session);
            reconstructed126 = true;
        } else {
            protocolBody = ExerciseProtocol::createProtocolHtml(
                m_exerciseId,
                m_specialistFio,
                m_elapsedSeconds,
                partlySave && !existingBody.trimmed().isEmpty(),
                existingBody,
                m_answers,
                checkboxValues(),
                session);
        }
    } else {
        protocolBody = ExerciseProtocol::createProtocolHtml(
            m_exerciseId,
            m_specialistFio,
            m_elapsedSeconds,
            partlySave,
            existingBody,
            m_answers,
            checkboxValues(),
            session);
    }

    QString error;
    QString protocolId;
    const bool hasExistingProtocol =
        !m_repository->loadLastExerciseProtocolId(m_patientId, m_exerciseId).isEmpty();
    bool saveAsPartly = (partlySave || reconstructed126) && hasExistingProtocol;

    if (!m_repository->saveExerciseProtocol(
            m_patientId, m_exerciseId, protocolBody, saveAsPartly, &error, &protocolId)) {
        if (saveAsPartly) {
            error.clear();
            if (!m_repository->saveExerciseProtocol(
                    m_patientId, m_exerciseId, protocolBody, false, &error, &protocolId)) {
                CustomMessageBox::showError(this, error);
                return;
            }
        } else {
            CustomMessageBox::showError(this, error);
            return;
        }
    }
    m_currentProtocolId = protocolId;

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, protocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    applyCompactLineHeight(m_templateBrowser->document());
    if (QTextDocument *doc = m_templateBrowser->document()) {
        doc->setDocumentMargin(kTemplateViewportPadding / 2);
        doc->setTextWidth(kTemplateTableWidth);
    }
    const int templateViewportWidth = kTemplateTableWidth + kTemplateViewportPadding;
    m_templateBrowser->setFixedWidth(templateViewportWidth);
    m_templatePanel->setMaximumWidth(templateViewportWidth + 16);
    layoutContent();
    QTimer::singleShot(80, this, [this]() { updateContentHeights(); });

    for (const ExerciseCheckRow &row : m_activityChecks) {
        if (row.box) {
            row.box->setChecked(false);
        }
    }
    for (const ExerciseCheckRow &row : m_helpChecks) {
        if (row.box) {
            row.box->setChecked(false);
        }
    }

    m_protocolFormed = true;
    m_protocolSavedThisSession = true;
    m_partly = true;
    m_stepElapsedSeconds.clear();
    updateProtocolEditMode();
    emit protocolSaved();
}

int ExerciseHost::puzzleFragmentCount() const {
    const QString step = currentStepId();
    if (m_exerciseId == QStringLiteral("1.19")) {
        if (step == QStringLiteral("Матрешка 2")) {
            return 2;
        }
        if (step == QStringLiteral("Леопард 3")) {
            return 3;
        }
        if (step == QStringLiteral("Мишка 4") || step == QStringLiteral("Дом 4")) {
            return 4;
        }
    }
    if (m_exerciseId == QStringLiteral("1.20")) {
        if (step == QStringLiteral("Мяч 2")) {
            return 2;
        }
        if (step == QStringLiteral("Дом 3")) {
            return 3;
        }
        if (step == QStringLiteral("Мишка 4")) {
            return 4;
        }
        if (step == QStringLiteral("Машинка 5")) {
            return 5;
        }
        if (step == QStringLiteral("Чайник 6")) {
            return 6;
        }
    }
    if (m_exerciseId == QStringLiteral("1.21")) {
        if (step.endsWith(QStringLiteral("А")) || step.endsWith(QStringLiteral("Б"))) {
            const QChar digit = step.at(0);
            if (digit.isDigit()) {
                return digit.digitValue();
            }
        }
    }
    PuzzleLayout layout;
    if (loadPuzzleLayout(m_exerciseId, step, &layout) && !layout.sprites.isEmpty()) {
        return layout.sprites.size();
    }
    return 4;
}

void ExerciseHost::refreshRotateCombos() {
    if (!m_rotateWCombo || !m_rotateCWCombo) {
        return;
    }
    const int fragmentCount = puzzleFragmentCount();
    m_rotateWCombo->blockSignals(true);
    m_rotateWCombo->clear();
    for (int i = 0; i <= fragmentCount; ++i) {
        m_rotateWCombo->addItem(QString::number(i));
    }
    m_rotateWCombo->setCurrentIndex(0);
    m_rotateWCombo->blockSignals(false);

    m_rotateCWCombo->blockSignals(true);
    m_rotateCWCombo->clear();
    for (int i = 0; i <= fragmentCount; ++i) {
        m_rotateCWCombo->addItem(QString::number(i));
    }
    m_rotateCWCombo->setCurrentIndex(0);
    m_rotateCWCombo->blockSignals(false);
}

void ExerciseHost::updateExerciseOptionsPanel() {
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    const bool isE15 = m_exerciseId == QStringLiteral("1.5") || m_exerciseId == QStringLiteral("1.6");
    const bool isPuzzleRotate = m_exerciseId == QStringLiteral("1.19") || m_exerciseId == QStringLiteral("1.20")
        || m_exerciseId == QStringLiteral("1.21");
    const bool showPanel = isE15 || isPuzzleRotate;

    if (m_exerciseOptionsPanel) {
        m_exerciseOptionsPanel->setVisible(showPanel);
    }
    if (m_shardButton) {
        m_shardButton->setVisible(isE15);
    }
    if (m_e15ModeGroup) {
        m_e15ModeGroup->setVisible(isE15 && m_shardPanelVisible);
    }
    if (m_showHintCheck) {
        m_showHintCheck->setVisible(isPuzzleRotate);
        m_showHintCheck->setChecked(true);
    }
    if (m_showTemplateCheck) {
        m_showTemplateCheck->setVisible(isPuzzleRotate);
        m_showTemplateCheck->setChecked(true);
    }
    if (m_rotateEnableCheck) {
        m_rotateEnableCheck->setVisible(isPuzzleRotate);
        m_rotateEnableCheck->setChecked(true);
    }
    if (m_rotateWCombo) {
        m_rotateWCombo->setVisible(isPuzzleRotate);
    }
    if (m_rotateCWCombo) {
        m_rotateCWCombo->setVisible(isPuzzleRotate);
    }
    if (isPuzzleRotate) {
        refreshRotateCombos();
    }
    Q_UNUSED(definition);
}

ExerciseSessionOptions ExerciseHost::buildSessionOptions() const {
    ExerciseSessionOptions options;
    if (m_e15SelectRadio && m_e15SelectRadio->isChecked()) {
        options.e15SelectMode = true;
    }
    if (m_showHintCheck && m_showHintCheck->isVisible()) {
        options.showHint = m_showHintCheck->isChecked();
    }
    if (m_showTemplateCheck && m_showTemplateCheck->isVisible()) {
        options.showTemplate = m_showTemplateCheck->isChecked();
    }
    if (m_rotateEnableCheck && m_rotateEnableCheck->isVisible()) {
        options.rotateEnabled = m_rotateEnableCheck->isChecked();
        if (m_rotateWCombo) {
            options.rotateW = m_rotateWCombo->currentText().toInt();
        }
        if (m_rotateCWCombo) {
            options.rotateCW = m_rotateCWCombo->currentText().toInt();
        }
    }
    return options;
}
