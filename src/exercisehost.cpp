#include "exercisehost.h"

#include "appsettings.h"
#include "custommessagebox.h"
#include "exerciseassets.h"
#include "exerciseconfig.h"
#include "exerciseprotocol.h"
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
#include <QBrush>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextTable>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QTimer>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QStyle>
#include <QVBoxLayout>
#include <QWindow>
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
constexpr int kRemThumbW = 180;
constexpr int kRemThumbH = 233;
constexpr int kRemPanelDownShift = 200;

void layoutRemPanelWidget(QWidget *remPanel, QWidget *rightPanel) {
    if (!remPanel || !rightPanel || !remPanel->isVisible()) {
        return;
    }
    remPanel->adjustSize();
    const int pw = rightPanel->width();
    const int ph = rightPanel->height();
    const int w = remPanel->sizeHint().width();
    const int h = remPanel->sizeHint().height();
    const int x = qMax(8, (pw - w) / 2);
    const int y = qMin(ph - h - 8, qMax(48, (ph - h) / 2 + kRemPanelDownShift));
    remPanel->setGeometry(x, y, w, h);
    remPanel->raise();
}

class RemThumbLabel final : public QLabel {
public:
    explicit RemThumbLabel(QWidget *parent = nullptr) : QLabel(parent) {
        setCursor(Qt::PointingHandCursor);
    }
    QCheckBox *check = nullptr;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && check) {
            check->toggle();
            event->accept();
            return;
        }
        QLabel::mouseReleaseEvent(event);
    }
};

// QRadioButton в Qt не переносит текст — индикатор + QLabel с word wrap.
class RadioTextLabel final : public QLabel {
public:
    explicit RadioTextLabel(QWidget *parent = nullptr) : QLabel(parent) {
        setWordWrap(true);
        setAlignment(Qt::AlignLeft | Qt::AlignTop);
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    QRadioButton *radio = nullptr;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && radio && !radio->isChecked()) {
            radio->click();
        }
        QLabel::mouseReleaseEvent(event);
    }
};

QRadioButton *addWrappingRadio(
    QVBoxLayout *layout,
    const QString &text,
    QWidget *parent,
    QButtonGroup *group = nullptr) {
    auto *row = new QWidget(parent);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(6);
    auto *radio = new QRadioButton(row);
    radio->setText(QString());
    radio->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // У каждой radio свой parent-row → autoExclusive не работает; нужна группа.
    if (group) {
        group->addButton(radio);
    }
    auto *label = new RadioTextLabel(row);
    label->radio = radio;
    label->setText(text);
    rowLayout->addWidget(radio, 0, Qt::AlignTop);
    rowLayout->addWidget(label, 1);
    layout->addWidget(row);
    return radio;
}

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
    label->ensurePolished();
    label->setFixedWidth(width);
    // heightForWidth иногда врёт до первого show — считаем через FontMetrics.
    const QFontMetrics fm(label->font());
    const QRect bound = fm.boundingRect(QRect(0, 0, width, 10000), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, label->text());
    label->setFixedHeight(qMax(20, bound.height()));
}

QString stripHtmlTags(const QString &html) {
    QString result = html;
    result.remove(QRegularExpression(
        QStringLiteral("<[^>]+>"),
        QRegularExpression::CaseInsensitiveOption));
    // Обрывки вроде value='strong>…' (без '<') из старых or.html.
    result.remove(QRegularExpression(
        QStringLiteral("\\b/?[a-zA-Z][a-zA-Z0-9]*\\s*>"),
        QRegularExpression::CaseInsensitiveOption));
    return result.trimmed();
}

QString checkboxValueFromInputTag(const QString &inputTag) {
    static const QRegularExpression valueRe(
        QStringLiteral("\\bvalue\\s*=\\s*['\"]([^'\"]*)['\"]"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = valueRe.match(inputTag);
    if (!match.hasMatch()) {
        return {};
    }
    QString label = stripHtmlTags(match.captured(1)).trimmed();
    // Убрать мусорные символы (PUA/checkbox glyphs) и ведущие табы из старых or.html.
    QString cleaned;
    cleaned.reserve(label.size());
    for (const QChar ch : label) {
        const ushort u = ch.unicode();
        if (u == 0x09 || u == 0x0a || u == 0x0d) {
            if (!cleaned.isEmpty() && !cleaned.endsWith(QLatin1Char(' '))) {
                cleaned.append(QLatin1Char(' '));
            }
            continue;
        }
        if (u >= 0xE000 && u <= 0xF8FF) {
            continue;
        }
        if (ch.isPrint() || ch.isSpace()) {
            cleaned.append(ch);
        }
    }
    return cleaned.trimmed();
}

QString activitySectionFromOrHtml(const QString &html) {
    static const QRegularExpression startRe(
        QStringLiteral("Характер\\s+деятельности"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch startMatch = startRe.match(html);
    if (!startMatch.hasMatch()) {
        return {};
    }
    const int start = startMatch.capturedStart();
    static const QRegularExpression helpRe(
        QStringLiteral("Виды\\s+возможной\\s+помощи"),
        QRegularExpression::CaseInsensitiveOption);
    const int helpStart = html.indexOf(helpRe, start);
    if (helpStart > start) {
        return html.mid(start, helpStart - start);
    }
    return html.mid(start);
}

QStringList parseCheckboxLabelsByIdPrefix(const QString &section, const QString &idPrefix) {
    if (section.isEmpty() || idPrefix.isEmpty()) {
        return {};
    }

    QMap<int, QString> byId;
    // Атрибуты в кавычках могут содержать '>' (value='<strong>…</strong>…' в 1.12).
    static const QRegularExpression inputRe(
        QStringLiteral(R"((?i)<input\b(?:[^>"']|"[^"]*"|'[^']*')*>)"));
    static const QRegularExpression checkboxTypeRe(
        QStringLiteral("type\\s*=\\s*['\"]checkbox['\"]"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression idRe(
        QStringLiteral("\\bid\\s*=\\s*['\"]%1(\\d+)['\"]").arg(QRegularExpression::escape(idPrefix)),
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator it = inputRe.globalMatch(section);
    while (it.hasNext()) {
        const QString tag = it.next().captured(0);
        if (!checkboxTypeRe.match(tag).hasMatch()) {
            continue;
        }
        const QRegularExpressionMatch idMatch = idRe.match(tag);
        if (!idMatch.hasMatch()) {
            continue;
        }
        const QString label = checkboxValueFromInputTag(tag);
        if (label.isEmpty()) {
            continue;
        }
        byId.insert(idMatch.captured(1).toInt(), label);
    }

    QStringList labels;
    for (auto mapIt = byId.constBegin(); mapIt != byId.constEnd(); ++mapIt) {
        labels << mapIt.value();
    }
    return labels;
}

QStringList parseActivityLabelsFromOrHtml(const QString &html) {
    return parseCheckboxLabelsByIdPrefix(activitySectionFromOrHtml(html), QStringLiteral("idd"));
}

QString helpSectionFromOrHtml(const QString &html) {
    static const QRegularExpression startRe(
        QStringLiteral("Виды\\s+возможной\\s+помощи"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch startMatch = startRe.match(html);
    if (!startMatch.hasMatch()) {
        return {};
    }
    return html.mid(startMatch.capturedStart());
}

QStringList parseHelpLabelsFromOrHtml(const QString &html) {
    return parseCheckboxLabelsByIdPrefix(helpSectionFromOrHtml(html), QStringLiteral("idp"));
}

QStringList defaultHelpLabels() {
    return QStringList{
        QStringLiteral(
            "Одобрение или неодобрение действий ребенка, стимуляция с помощью слов "
            "«хорошо», «правильно», «неправильно, подумай еще»."),
        QStringLiteral(
            "Вопросы к испытуемому о том, почему он сделал то или иное действие, с целью "
            "повышения уровня осознания смысла задания и ориентировки в задании."),
        QStringLiteral("Подсказка, совет действовать тем или иным образом."),
        QStringLiteral("Показ способа выполнения задания с просьбой повторить это действие."),
        QStringLiteral(
            "Совместно-раздельная деятельность: специалист начинает выполнять задание, а ребенок "
            "продолжает."),
    };
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

void finalizeProtocolTemplateDocument(QTextDocument *doc) {
    applyCompactLineHeight(doc);
    ExerciseProtocol::forceProtocolDocumentTableWidths(doc, kTemplateTableWidth);
    if (doc) {
        doc->setDocumentMargin(0);
        doc->setTextWidth(kTemplateTableWidth);
    }
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
    // Maximum по вертикали — иначе setWidgetResizable раздувает строки лишним местом.
    wrap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *rowLayout = new QHBoxLayout(wrap);
    rowLayout->setContentsMargins(0, 2, 0, 2);
    rowLayout->setSpacing(8);

    row.box = new WhiteCheckBox(wrap);
    row.box->setFixedSize(18, 18);

    row.label = new WhiteLabel(text, wrap);
    row.label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    row.label->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif; font-size:14px;"));
    if (contentWidth > 40) {
        resizeCheckLabel(row.label, contentWidth - 34);
    }

    rowLayout->addWidget(row.box, 0, Qt::AlignTop);
    rowLayout->addWidget(row.label, 0, Qt::AlignTop);
    layout->addWidget(wrap, 0, Qt::AlignTop);
    return row;
}

class WrapCheckLabel final : public QLabel {
public:
    explicit WrapCheckLabel(QWidget *parent = nullptr) : QLabel(parent) {
        setWordWrap(true);
        setAlignment(Qt::AlignLeft | Qt::AlignTop);
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    QCheckBox *box = nullptr;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && box) {
            box->toggle();
        }
        QLabel::mouseReleaseEvent(event);
    }
};

// 1.12 «Оценка результатов»: кружки как ◦ / • на скрине, текст с <b>.
ExerciseCheckRow makeCircleBulletCheckRow(
    const QString &htmlText,
    QVBoxLayout *layout,
    int contentWidth,
    bool solidWhenChecked) {
    ExerciseCheckRow row;
    auto *wrap = new OpaquePanel(kDocumentBg);
    wrap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *rowLayout = new QHBoxLayout(wrap);
    rowLayout->setContentsMargins(8, 2, 0, 2);
    rowLayout->setSpacing(8);

    row.box = new WhiteCheckBox(wrap);
    row.box->setFixedSize(14, 14);
    const QString checkedFill = solidWhenChecked ? QStringLiteral("#000000") : QStringLiteral("#888888");
    row.box->setStyleSheet(QStringLiteral(
        "QCheckBox { background:transparent; spacing:0; }"
        "QCheckBox::indicator { width:12px; height:12px; border-radius:6px;"
        "  border:1px solid #000000; background:#ffffff; }"
        "QCheckBox::indicator:checked { background:%1; border:1px solid #000000; }")
                               .arg(checkedFill));

    auto *label = new WrapCheckLabel(wrap);
    label->box = row.box;
    label->setTextFormat(Qt::RichText);
    label->setText(htmlText);
    label->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif; font-size:14px;"
        "background:transparent;"));
    if (contentWidth > 40) {
        label->setFixedWidth(contentWidth - 34);
    }
    row.label = label;

    rowLayout->addWidget(row.box, 0, Qt::AlignTop);
    rowLayout->addWidget(row.label, 1, Qt::AlignTop);
    layout->addWidget(wrap, 0, Qt::AlignTop);
    return row;
}

QCheckBox *addWrappingCheckBox(QVBoxLayout *layout, const QString &text, QWidget *parent, int width) {
    auto *row = new QWidget(parent);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(6);
    auto *box = new QCheckBox(row);
    box->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto *label = new WrapCheckLabel(row);
    label->box = box;
    label->setText(text);
    if (width > 50) {
        label->setFixedWidth(width - 34);
    }
    rowLayout->addWidget(box, 0, Qt::AlignTop);
    rowLayout->addWidget(label, 1);
    layout->addWidget(row);
    return box;
}

ExerciseCheckRow makeDoneOptionRow(const QString &text, QVBoxLayout *layout, int optionWidth) {
    ExerciseCheckRow rowData;
    auto *rowLayout = new QHBoxLayout();
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    rowData.box = new WhiteCheckBox();
    rowData.box->setFixedSize(18, 18);

    rowData.label = new WhiteLabel(text);
    rowData.label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    rowData.label->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif; font-size:14px;"));
    if (optionWidth > 40) {
        resizeCheckLabel(rowData.label, optionWidth - 34);
    }

    rowLayout->addWidget(rowData.box, 0, Qt::AlignVCenter);
    rowLayout->addWidget(rowData.label, 1);
    layout->addLayout(rowLayout);
    return rowData;
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
    layout->setAlignment(Qt::AlignTop);

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
    m_evaluationPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *evaluationLayout = new QVBoxLayout(m_evaluationPanel);
    evaluationLayout->setContentsMargins(8, 0, 8, 12);
    evaluationLayout->setSpacing(4);
    evaluationLayout->setAlignment(Qt::AlignTop);

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

    m_activityTitle = new WhiteLabel(QStringLiteral("Характер деятельности ребенка:"), m_evaluationPanel);
    m_activityTitle->setAlignment(Qt::AlignCenter);
    m_activityTitle->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:15px; font-weight:bold; padding:0;"));

    m_activityChecksHost = new OpaquePanel(kDocumentBg, m_evaluationPanel);
    m_activityChecksHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_activityChecksLayout = new QVBoxLayout(m_activityChecksHost);
    m_activityChecksLayout->setContentsMargins(8, 0, 8, 0);
    m_activityChecksLayout->setSpacing(2);
    m_activityChecksLayout->setAlignment(Qt::AlignTop);

    m_checkboxPanel = new OpaquePanel(kDocumentBg, m_evaluationPanel);
    m_checkboxPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *checkboxLayout = new QVBoxLayout(m_checkboxPanel);
    checkboxLayout->setContentsMargins(8, 0, 8, 0);
    checkboxLayout->setSpacing(2);
    checkboxLayout->setAlignment(Qt::AlignTop);

    m_helpChecksLayout = checkboxLayout;

    auto *helpTitle = new WhiteLabel(QStringLiteral("Виды возможной помощи:"), m_checkboxPanel);
    helpTitle->setAlignment(Qt::AlignCenter);
    helpTitle->setStyleSheet(m_activityTitle->styleSheet());
    checkboxLayout->addWidget(helpTitle);

    m_helpPenaltyHintLabel = new WhiteLabel(
        QStringLiteral("За каждый вид помощи оценка снижается на 0,5 балла"), m_checkboxPanel);
    m_helpPenaltyHintLabel->setAlignment(Qt::AlignCenter);
    m_helpPenaltyHintLabel->setWordWrap(true);
    m_helpPenaltyHintLabel->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:13px; font-style:italic; font-weight:normal; padding:2px 8px 4px 8px;"));
    m_helpPenaltyHintLabel->hide();
    checkboxLayout->addWidget(m_helpPenaltyHintLabel);

    m_stimHelpLabel = new WhiteLabel(QStringLiteral("Стимулирующая помощь"), m_checkboxPanel);
    m_stimHelpLabel->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:14px; font-weight:bold; padding:4px 0 0 16px;"));
    checkboxLayout->addWidget(m_stimHelpLabel);

    m_directHelpLabel = new WhiteLabel(QStringLiteral("Направляющая помощь:"), m_checkboxPanel);
    m_directHelpLabel->setStyleSheet(m_stimHelpLabel->styleSheet());
    checkboxLayout->addWidget(m_directHelpLabel);

    m_teachHelpLabel = new WhiteLabel(QStringLiteral("Обучающая помощь:"), m_checkboxPanel);
    m_teachHelpLabel->setStyleSheet(m_stimHelpLabel->styleSheet());
    checkboxLayout->addWidget(m_teachHelpLabel);

    m_donePanel = new OpaquePanel(kDocumentBg, m_evaluationPanel);
    auto *doneOuter = new QHBoxLayout(m_donePanel);
    doneOuter->setContentsMargins(0, 8, 0, 12);
    doneOuter->addStretch(1);

    auto *doneTable = new QWidget(m_donePanel);
    doneTable->setAttribute(Qt::WA_StyledBackground, true);
    doneTable->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    doneTable->setStyleSheet(QStringLiteral(
        "QWidget { background:#ffffff; border:1px solid #000000; }"));
    auto *tableLayout = new QHBoxLayout(doneTable);
    tableLayout->setContentsMargins(4, 4, 4, 4);
    tableLayout->setSpacing(0);

    auto *doneTitleLabel = new QLabel(QStringLiteral("Выполнение"), doneTable);
    doneTitleLabel->setFrameShape(QFrame::NoFrame);
    doneTitleLabel->setFixedWidth(100);
    doneTitleLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    doneTitleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    doneTitleLabel->setStyleSheet(QStringLiteral(
        "background:#ffffff; border:none; color:#000000;"
        "font-family:'Microsoft Sans Serif',sans-serif; font-size:14px; padding:0; margin:0;"));
    tableLayout->addWidget(doneTitleLabel, 0, Qt::AlignTop);

    auto *doneOptionsLayout = new QVBoxLayout();
    doneOptionsLayout->setContentsMargins(4, 0, 4, 0);
    doneOptionsLayout->setSpacing(0);

    constexpr int kDoneOptionWidth = 240;
    m_doneChecks << makeDoneOptionRow(
                      QStringLiteral("Выполнено"), doneOptionsLayout, kDoneOptionWidth)
                 << makeDoneOptionRow(
                      QStringLiteral("Выполнено частично"), doneOptionsLayout, kDoneOptionWidth)
                 << makeDoneOptionRow(
                      QStringLiteral("Не выполнено"), doneOptionsLayout, kDoneOptionWidth);
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

    tableLayout->addLayout(doneOptionsLayout, 0);

    doneOuter->addWidget(doneTable, 0, Qt::AlignHCenter | Qt::AlignTop);
    doneOuter->addStretch(1);
    m_donePanel->hide();

    evaluationLayout->addWidget(m_donePanel);
    evaluationLayout->addWidget(m_activityTitle);
    evaluationLayout->addWidget(m_activityChecksHost);
    evaluationLayout->addSpacing(12);
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
    templateLayout->addSpacing(12);

    m_templateBrowser = makeHtmlEditor(m_templatePanel);
    ProtocolEditGuard::install(m_templateBrowser, ProtocolEditGuard::Mode::ReadOnly);
    ProtocolEditGuard::setScanAnchorHandler(m_templateBrowser, [this](const QString &anchor) {
        const QString path = Repository::resolveProtocolScanPathFromAnchor(anchor);
        if (path.isEmpty() || !QFileInfo::exists(path)) {
            CustomMessageBox::showWarning(
                this, QStringLiteral("Изображение ещё не загружено."));
            return true;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        return true;
    });
    templateLayout->addWidget(m_templateBrowser);
    // «Подвести итог» — под протоколом (как в руководстве для 1.26).
    templateLayout->addSpacing(8);
    templateLayout->addWidget(m_sumButton, 0, Qt::AlignHCenter);

    m_protocolSaveTimer = new QTimer(this);
    m_protocolSaveTimer->setSingleShot(true);
    m_protocolSaveTimer->setInterval(700);
    connect(m_protocolSaveTimer, &QTimer::timeout, this, &ExerciseHost::saveProtocolEdits);
    connect(m_templateBrowser->document(), &QTextDocument::contentsChanged, this, [this]() {
        if (m_suppressProtocolAutosave || !m_protocolSavedThisSession || m_currentProtocolId.isEmpty()) {
            return;
        }
        if (m_protocolSaveTimer) {
            m_protocolSaveTimer->start();
        }
    });
    connect(m_templateBrowser, &QTextEdit::cursorPositionChanged, this, [this]() {
        onProtocolCursorMoved();
    });
    m_templateBrowser->installEventFilter(this);
    if (m_templateBrowser->viewport()) {
        m_templateBrowser->viewport()->installEventFilter(this);
    }

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

    m_previewGenderPanel = new QWidget(m_rightPanel);
    m_previewGenderPanel->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *genderLayout = new QHBoxLayout(m_previewGenderPanel);
    genderLayout->setContentsMargins(0, 0, 0, 0);
    genderLayout->setSpacing(16);
    m_previewGirlRadio = new QRadioButton(QStringLiteral("Девочка"), m_previewGenderPanel);
    m_previewBoyRadio = new QRadioButton(QStringLiteral("Мальчик"), m_previewGenderPanel);
    m_previewGirlRadio->setChecked(true);
    m_previewGenderPrefix = QStringLiteral("d");
    genderLayout->addWidget(m_previewGirlRadio);
    genderLayout->addWidget(m_previewBoyRadio);
    genderLayout->addStretch(1);
    m_previewGenderPanel->hide();
    connect(m_previewGirlRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (!checked) {
            return;
        }
        m_previewGenderPrefix = QStringLiteral("d");
        reloadPreviewForCurrentStep();
    });
    connect(m_previewBoyRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (!checked) {
            return;
        }
        m_previewGenderPrefix = QStringLiteral("m");
        reloadPreviewForCurrentStep();
    });

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
    m_stepCombo->setMaxVisibleItems(12);
    m_stepCombo->setFocusPolicy(Qt::StrongFocus);
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

    m_shardButton = new QPushButton(QStringLiteral("Настройка уровня сложности ▾"), m_exerciseOptionsPanel);
    m_shardButton->setFlat(true);
    m_shardButton->setCursor(Qt::PointingHandCursor);
    m_shardButton->setStyleSheet(QStringLiteral(
        "QPushButton { color:#000000; font-family:'Microsoft Sans Serif'; font-size:11pt;"
        " font-weight:bold; text-decoration:underline; text-align:left; padding:0; border:none;"
        " background:transparent; }"
        "QPushButton:hover { color:#222222; }"));
    m_shardButton->hide();
    optionsLayout->addWidget(m_shardButton, 0, Qt::AlignLeft);

    // Отдельная ссылка 3.1.16 на корне окна (как label1 @ 1415,25) — поверх превью.
    m_shard316Button = new QPushButton(QStringLiteral("Настройка уровня сложности ▾"), this);
    m_shard316Button->setFlat(true);
    m_shard316Button->setCursor(Qt::PointingHandCursor);
    m_shard316Button->setStyleSheet(m_shardButton->styleSheet());
    m_shard316Button->hide();

    // 1.5 / 1.6: ссылка в правой области, в одну линию с «Начать» (~1550 abs / y=12).
    m_shard15Button = new QPushButton(QStringLiteral("Настройка уровня сложности ▾"), m_rightPanel);
    m_shard15Button->setFlat(true);
    m_shard15Button->setCursor(Qt::PointingHandCursor);
    m_shard15Button->setStyleSheet(m_shardButton->styleSheet());
    m_shard15Button->hide();

    // 3.1.16: всплывающее окно как в руководстве (img3) — рамка с двумя radio под ссылкой.
    m_aparamGroup = new QGroupBox(QString(), this);
    m_aparamGroup->setFixedWidth(280);
    m_aparamGroup->setStyleSheet(QStringLiteral(
        "QGroupBox { background:#ffffff; border:1px solid #000; margin-top:0; padding:6px 8px 8px 8px; }"
        "QGroupBox::title { height:0; width:0; margin:0; padding:0; }"));
    auto *aparamLayout = new QVBoxLayout(m_aparamGroup);
    aparamLayout->setContentsMargins(10, 10, 10, 8);
    aparamLayout->setSpacing(8);
    auto *aparamButtons = new QButtonGroup(m_aparamGroup);
    aparamButtons->setExclusive(true);
    m_aparamAllRadio = new QRadioButton(QStringLiteral("Показать все фрагменты"), m_aparamGroup);
    m_aparamCurrentRadio = new QRadioButton(QStringLiteral("Показать только к данному заданию"), m_aparamGroup);
    aparamButtons->addButton(m_aparamAllRadio);
    aparamButtons->addButton(m_aparamCurrentRadio);
    // Руководство: по умолчанию «Показать все фрагменты».
    m_aparamAllRadio->setChecked(true);
    aparamLayout->addWidget(m_aparamAllRadio);
    aparamLayout->addWidget(m_aparamCurrentRadio);
    m_aparamGroup->hide();

    // 4.1.7 (rem): выбор картинок 1–9 (как rem.cs; pfront скрыт).
    m_remPanel = new QWidget(m_rightPanel);
    m_remPanel->setAttribute(Qt::WA_StyledBackground, true);
    m_remPanel->setStyleSheet(QStringLiteral("background:#f8f8f8; border:1px solid #888;"));
    auto *remGrid = new QGridLayout(m_remPanel);
    remGrid->setContentsMargins(18, 18, 18, 18);
    remGrid->setHorizontalSpacing(21);
    remGrid->setVerticalSpacing(15);
    for (int i = 0; i < 9; ++i) {
        auto *cell = new QWidget(m_remPanel);
        auto *cellLay = new QVBoxLayout(cell);
        cellLay->setContentsMargins(0, 0, 0, 0);
        cellLay->setSpacing(6);
        m_remChecks[i] = new QCheckBox(cell);
        m_remChecks[i]->setChecked(true);
        auto *thumb = new RemThumbLabel(cell);
        thumb->check = m_remChecks[i];
        thumb->setFixedSize(kRemThumbW, kRemThumbH);
        thumb->setAlignment(Qt::AlignCenter);
        thumb->setStyleSheet(QStringLiteral("background:#fff; border:1px solid #ccc;"));
        const QString path = ExerciseAssets::exerciseFile(
            QStringLiteral("4.1.7"), QString::number(i + 1) + QStringLiteral(".png"));
        if (!path.isEmpty()) {
            thumb->setPixmap(QPixmap(path).scaled(
                kRemThumbW - 4,
                kRemThumbH - 4,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        }
        cellLay->addWidget(m_remChecks[i], 0, Qt::AlignLeft);
        cellLay->addWidget(thumb, 0, Qt::AlignLeft);
        remGrid->addWidget(cell, i / 3, i % 3);
    }
    m_remPanel->hide();

    // Как на форме e15: всплывающая группа поверх, без сдвига layout.
    m_e15ModeGroup = new QGroupBox(QStringLiteral("Настройки"), m_rightPanel);
    m_e15ModeGroup->setFixedWidth(300);
    m_e15ModeGroup->setStyleSheet(QStringLiteral(
        "QGroupBox { background:#ffffff; border:1px solid #000; margin-top:4px; padding:4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left:6px; padding:0 3px; }"));
    auto *e15Layout = new QVBoxLayout(m_e15ModeGroup);
    e15Layout->setContentsMargins(8, 10, 8, 6);
    e15Layout->setSpacing(4);
    auto *e15ModeGroup = new QButtonGroup(m_e15ModeGroup);
    e15ModeGroup->setExclusive(true);
    // radioButton1 в оригинале → param="select" (подсветка).
    m_e15HighlightRadio = addWrappingRadio(
        e15Layout,
        QStringLiteral("Выделение («подсвечивание») фрагментов при выборе"),
        m_e15ModeGroup,
        e15ModeGroup);
    // radioButton2 → перемещение на место (без анимации наклона).
    m_e15SelectRadio = addWrappingRadio(
        e15Layout,
        QStringLiteral("Перемещение фрагментов на основной рисунок для визуального сравнения узора"),
        m_e15ModeGroup,
        e15ModeGroup);
    m_e15HighlightRadio->setChecked(true);
    m_e15ModeGroup->hide();

    m_puzzleOptionsGroup = new QGroupBox(QStringLiteral("Настройки"), m_rightPanel);
    m_puzzleOptionsGroup->setFixedWidth(300);
    m_puzzleOptionsGroup->setStyleSheet(QStringLiteral(
        "QGroupBox { background:#ffffff; border:1px solid #000; margin-top:4px; padding:4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left:6px; padding:0 3px; }"));
    auto *puzzleLayout = new QVBoxLayout(m_puzzleOptionsGroup);
    puzzleLayout->setContentsMargins(8, 10, 8, 8);
    puzzleLayout->setSpacing(6);
    constexpr int kPuzzleOptionTextW = 300;

    m_showHintCheck = addWrappingCheckBox(
        puzzleLayout,
        QStringLiteral("Показать подсказку (демонстрация конечного результата)"),
        m_puzzleOptionsGroup,
        kPuzzleOptionTextW);
    m_showTemplateCheck = addWrappingCheckBox(
        puzzleLayout,
        QStringLiteral("Показать трафарет (накладывание деталей картинки на цельную картинку)"),
        m_puzzleOptionsGroup,
        kPuzzleOptionTextW);
    m_rotateEnableCheck = addWrappingCheckBox(
        puzzleLayout,
        QStringLiteral("Поворот фрагментов"),
        m_puzzleOptionsGroup,
        kPuzzleOptionTextW);
    m_showHintCheck->setChecked(true);
    m_showTemplateCheck->setChecked(true);
    m_rotateEnableCheck->setChecked(true);

    m_rotateWLabel = new QLabel(QStringLiteral("Фрагментов, повернутых на 90°"), m_puzzleOptionsGroup);
    m_rotateWLabel->setWordWrap(true);
    m_rotateCWLabel = new QLabel(QStringLiteral("Фрагментов, повернутых на 180°"), m_puzzleOptionsGroup);
    m_rotateCWLabel->setWordWrap(true);
    m_rotateWCombo = new QComboBox(m_puzzleOptionsGroup);
    m_rotateCWCombo = new QComboBox(m_puzzleOptionsGroup);
    puzzleLayout->addWidget(m_rotateWLabel);
    puzzleLayout->addWidget(m_rotateWCombo);
    puzzleLayout->addWidget(m_rotateCWLabel);
    puzzleLayout->addWidget(m_rotateCWCombo);
    m_puzzleOptionsGroup->hide();

    m_exerciseOptionsPanel->hide();

    connect(m_shardButton, &QPushButton::clicked, this, [this]() {
        m_shardPanelVisible = !m_shardPanelVisible;
        layoutE15ModePopup();
        updateExerciseOptionsPanel();
        updateChromeLayout();
    });
    connect(m_shard316Button, &QPushButton::clicked, this, [this]() {
        m_shardPanelVisible = !m_shardPanelVisible;
        layoutE15ModePopup();
        updateExerciseOptionsPanel();
        updateChromeLayout();
    });
    connect(m_shard15Button, &QPushButton::clicked, this, [this]() {
        m_shardPanelVisible = !m_shardPanelVisible;
        // Сначала показать/разложить ссылку, потом попап — иначе geometry (0,0).
        updateExerciseOptionsPanel();
        updateChromeLayout();
        layoutE15ModePopup();
    });
    auto applyE15ModeFromUi = [this]() {
        if (!m_exerciseRunning || !m_sessionRunner) {
            return;
        }
        const bool selectMode = m_e15HighlightRadio && m_e15HighlightRadio->isChecked();
        if (m_sessionRunnerKind == ExerciseRunnerKind::E15) {
            ExerciseSessionOptions opt = m_sessionRunner->sessionOptions();
            opt.e15SelectMode = selectMode;
            m_sessionRunner->setSessionOptions(opt);
            m_sessionRunner->applyE15SelectMode(selectMode);
            return;
        }
        if (m_exerciseId == QStringLiteral("1.22")
            && m_sessionRunnerKind == ExerciseRunnerKind::Puzzles) {
            ExerciseSessionOptions opt = m_sessionRunner->sessionOptions();
            opt.e15SelectMode = selectMode;
            m_sessionRunner->setSessionOptions(opt);
            m_sessionRunner->applyE15SelectMode(selectMode);
        }
    };
    connect(m_e15HighlightRadio, &QRadioButton::toggled, this, [applyE15ModeFromUi](bool checked) {
        if (checked) {
            applyE15ModeFromUi();
        }
    });
    connect(m_e15SelectRadio, &QRadioButton::toggled, this, [applyE15ModeFromUi](bool checked) {
        if (checked) {
            applyE15ModeFromUi();
        }
    });
    connect(m_stepCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index < 0) {
            return;
        }
        // Перед сменой задания сохраняем данные текущего (1.26 / 1.272 / 5.2.1).
        if (m_exerciseRunning && m_sessionRunner
            && (m_sessionRunnerKind == ExerciseRunnerKind::E126
                || m_sessionRunnerKind == ExerciseRunnerKind::E1272
                || m_sessionRunnerKind == ExerciseRunnerKind::E521)) {
            const QString snap = m_sessionRunner->currentAdditionalSnapshot();
            if (!snap.trimmed().isEmpty()) {
                const QStringList parts = snap.split(QLatin1Char(';'));
                const QString stepKey = parts.isEmpty() || parts.at(0).trimmed().isEmpty()
                    ? QStringLiteral("1")
                    : parts.at(0).trimmed();
                m_additionalByStep.insert(stepKey, snap);
                m_sessionAdditional = snap;
            }
        }
        m_sessionStepId = currentStepId();
        if (m_exerciseId == QStringLiteral("1.19")) {
            if (m_showHintCheck) {
                m_showHintCheck->setChecked(false);
            }
            if (m_showTemplateCheck) {
                m_showTemplateCheck->setChecked(false);
            }
        }
        refreshRotateCombos();
        reloadPreviewForCurrentStep();
        updateExerciseOptionsPanel();
        updateChromeLayout();
        if (m_exerciseRunning && m_onlyP && !m_sessionStepId.isEmpty()) {
            m_onlyP->switchStep(m_sessionStepId);
            if (m_specialistExercise) {
                m_specialistExercise->switchStep(m_sessionStepId);
            }
            if (m_patientDisplay) {
                m_patientDisplay->switchStep(m_sessionStepId);
            }
        }
        if (m_exerciseRunning && m_sessionRunner
            && (m_sessionRunnerKind == ExerciseRunnerKind::E126
                || m_sessionRunnerKind == ExerciseRunnerKind::E1272
                || m_sessionRunnerKind == ExerciseRunnerKind::E521)
            && !m_sessionStepId.isEmpty()) {
            m_sessionRunner->switchStep(m_sessionStepId);
            // Dual: session-runner зеркалится grab'ом — обновить кадр сразу после смены шага.
            if (m_patientDisplay && m_dualScreen) {
                syncPatientDisplay();
            }
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
        pushLivePuzzleOptionsToRunner();
    });
    auto pushPuzzleOption = [this](bool) { pushLivePuzzleOptionsToRunner(); };
    connect(m_showHintCheck, &QCheckBox::toggled, this, pushPuzzleOption);
    connect(m_showTemplateCheck, &QCheckBox::toggled, this, pushPuzzleOption);
    connect(m_rotateWCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        pushLivePuzzleOptionsToRunner();
    });
    connect(m_rotateCWCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        pushLivePuzzleOptionsToRunner();
    });
    connect(m_aparamAllRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            pushLivePuzzleOptionsToRunner();
        }
    });
    connect(m_aparamCurrentRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            pushLivePuzzleOptionsToRunner();
        }
    });

    m_onlyP = new OnlyPExercise(this);
    m_onlyP->hide();

    m_specialistExercise = new OnlyPExercise(m_rightPanel);
    m_specialistExercise->setDisplayRole(OnlyPExercise::DisplayRole::Specialist);
    m_specialistExercise->setMirrorMode(true);
    m_specialistExercise->hide();
    connect(m_onlyP, &OnlyPExercise::pictureChanged, this, [this](int index) {
        if (!m_specialistExercise || !m_onlyP) {
            return;
        }
        const QString stepId = m_onlyP->property("stepId").toString();
        const QString mirrorStep = m_specialistExercise->property("stepId").toString();
        if (!stepId.isEmpty() && stepId != mirrorStep) {
            m_specialistExercise->switchStep(stepId);
        }
        m_specialistExercise->showPicture(index);
    });
    connect(m_onlyP, &OnlyPExercise::browseStateChanged, this, [this](int index) {
        if (!m_specialistExercise || !m_onlyP) {
            return;
        }
        const QString stepId = m_onlyP->property("stepId").toString();
        const QString mirrorStep = m_specialistExercise->property("stepId").toString();
        if (!stepId.isEmpty() && stepId != mirrorStep) {
            m_specialistExercise->switchStep(stepId);
        }
        m_specialistExercise->applyBrowseIndex(index);
    });
    connect(
        m_specialistExercise,
        &OnlyPExercise::mirrorBrowseNextRequested,
        m_onlyP,
        &OnlyPExercise::browseNext,
        Qt::UniqueConnection);
    connect(
        m_specialistExercise,
        &OnlyPExercise::mirrorBrowseBackRequested,
        m_onlyP,
        &OnlyPExercise::browseBack,
        Qt::UniqueConnection);
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
        // Руководство: несколько заданий — выбрать № до Start, протокол не обнулять (как упр. 6).
        if (ExerciseConfig::usesAppendOnlyMultiStepLogic(m_exerciseId)) {
            bool newSession = false;
            // 1.26 / 1.272 / 5.2.1: смена сессии решается в formProtocol / createP.
            const bool deferSessionDecision = m_exerciseId == QStringLiteral("1.26")
                || m_exerciseId == QStringLiteral("1.272")
                || m_exerciseId == QStringLiteral("5.2.1")
                || m_exerciseId == QStringLiteral("5.3.1");
            if (!deferSessionDecision && m_repository && m_partly) {
                const QString step = currentStepId().trimmed().isEmpty()
                    ? QStringLiteral("1")
                    : currentStepId().trimmed();
                const QString existingBody =
                    m_repository->loadLastExerciseProtocolBody(m_patientId, m_exerciseId);
                const QString lastSession =
                    ExerciseProtocol::extractLastProtocol126Session(existingBody);
                const QString scope = lastSession.trimmed().isEmpty() ? existingBody : lastSession;
                if (m_exerciseId == QStringLiteral("3.1.10")) {
                    const QString idb = QStringLiteral("idb") + step;
                    newSession =
                        scope.contains(QStringLiteral("id='%1'").arg(idb), Qt::CaseInsensitive)
                        || scope.contains(QStringLiteral("id=\"%1\"").arg(idb), Qt::CaseInsensitive);
                } else if (m_exerciseId == QStringLiteral("1.20")) {
                    // В протоколе ids1..ids5 и подпись «Мяч из 2 частей», в combo — «Мяч 2».
                    static const QMap<QString, QString> kScoreIds = {
                        {QStringLiteral("Мяч 2"), QStringLiteral("1")},
                        {QStringLiteral("Дом 3"), QStringLiteral("2")},
                        {QStringLiteral("Мишка 4"), QStringLiteral("3")},
                        {QStringLiteral("Машинка 5"), QStringLiteral("4")},
                        {QStringLiteral("Чайник 6"), QStringLiteral("5")},
                    };
                    const QString scoreId = kScoreIds.value(step, step);
                    const QString idToken = QStringLiteral("ids") + scoreId;
                    newSession =
                        scope.contains(QStringLiteral("id='%1'").arg(idToken), Qt::CaseInsensitive)
                        || scope.contains(
                            QStringLiteral("id=\"%1\"").arg(idToken), Qt::CaseInsensitive)
                        || ExerciseProtocol::numberedStepPresentInSessionHtml(scope, step);
                } else {
                    newSession =
                        ExerciseProtocol::numberedStepPresentInSessionHtml(scope, step);
                }
            }
            m_forceNewProtocolSession = newSession;
            if (newSession) {
                resetProtocolToInitialTemplate();
            }
            m_sessionStepId = currentStepId();
            if (m_sessionStepId.trimmed().isEmpty()) {
                m_sessionStepId = QStringLiteral("1");
            }
            // Явно зафиксировать выбранный № до Start (5.2.1 / упр. 6).
            if (m_stepCombo && m_stepCombo->count() > 0) {
                const QString selected = m_stepCombo->currentText().trimmed();
                if (!selected.isEmpty()) {
                    m_sessionStepId = selected;
                }
            }
            reloadPreviewForCurrentStep();
            runExerciseSession();
            return;
        }
        if (forceNewProtocolSessionOnBegin()) {
            m_sessionAdditional.clear();
            m_additionalByStep.clear();
            m_forceNewProtocolSession = true;
            // 4.1.2: не сбрасывать выбранное задание («Пример» / «1») на индекс 0.
            if (m_stepCombo && m_stepCombo->count() > 0
                && m_exerciseId != QStringLiteral("4.1.2")) {
                m_stepCombo->blockSignals(true);
                m_stepCombo->setCurrentIndex(0);
                m_stepCombo->blockSignals(false);
                m_sessionStepId = m_stepCombo->currentText().trimmed();
                reloadPreviewForCurrentStep();
            }
        }
        // 3.1.12: одно задание (листание картинок) — после Begin протокол с нуля, без дописки.
        if (m_exerciseId == QStringLiteral("3.1.12")) {
            m_partly = false;
            m_forceNewProtocolSession = false;
        }
        resetProtocolToInitialTemplate();
        runExerciseSession();
    });
    connect(m_formProtocolButton, &ImageButton::clicked, this, [this]() { formProtocol(); });
    connect(m_sumButton, &ImageButton::clicked, this, [this]() { sumProtocolScores(); });
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
        if (ExerciseConfig::usesAppendOnlyMultiStepLogic(m_exerciseId)) {
            const QString stepKey = currentStepId().trimmed().isEmpty()
                ? QStringLiteral("1")
                : currentStepId().trimmed();
            m_additionalByStep.insert(stepKey, stepKey);
            m_stepElapsedSeconds.insert(stepKey, m_elapsedSeconds);
        }
        // 4.1.2 «Пример»: протокол не заполняется — сразу можно запускать задание «1».
        if (m_exerciseId == QStringLiteral("4.1.2")
            && currentStepId().trimmed() == QStringLiteral("Пример")) {
            m_protocolFormed = true;
            m_exerciseDone = false;
            m_elapsedSeconds = 0;
            m_stepElapsedSeconds.clear();
        }
        resetExerciseOverlays();
        clearRootExerciseOverlays();
        setExerciseChromeVisible(true);
        raise();
        updateChromeLayout();
        showResultLabels(answers, elapsedSeconds);
        emit exerciseOverlayChanged(false);
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

bool ExerciseHost::eventFilter(QObject *watched, QEvent *event) {
    // Ссылки «Показать изображение» — ProtocolEditGuard::setScanAnchorHandler.
    if (watched == m_templateBrowser && event
        && (event->type() == QEvent::FocusOut || event->type() == QEvent::Hide)) {
        if (m_cursorInBallsColumn) {
            m_cursorInBallsColumn = false;
            QTimer::singleShot(0, this, [this]() { syncProtocol317BallsToResult(); });
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ExerciseHost::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateChromeLayout();
    updateExerciseOverlayGeometry();
    layoutContent();
}

void ExerciseHost::updateChromeLayout() {
    // Dual + chrome намеренно открыт (задание на правой панели / OnlyP-зеркало).
    // Если chrome скрыт (полноэкранный оверлей: 5.2.1, 5.4.2, …) — не поднимать
    // описание и превью поверх таблицы/картинки.
    const bool dualKeepsChrome = m_dualScreen && m_exerciseRunning
        && m_scrollArea && m_scrollArea->isVisible()
        && ((m_sessionRunner && m_rightPanel
             && m_sessionRunner->parentWidget() == m_rightPanel)
            || (m_specialistExercise && m_specialistExercise->isVisible()));

    if (m_exerciseRunning && !dualKeepsChrome) {
        updateExerciseOverlayGeometry();
        if (m_sessionRunner && m_sessionRunner->isVisible()) {
            m_sessionRunner->raise();
        }
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
    if (m_dualScreen && m_exerciseRunning && m_specialistExercise) {
        if (m_exerciseId == QStringLiteral("4.1.1")) {
            m_specialistExercise->setGeometry(0, 0, width(), height());
        } else if (m_rightPanel) {
            m_specialistExercise->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        }
        m_specialistExercise->raise();
    }
    if (m_sessionRunner && m_sessionRunnerKind == ExerciseRunnerKind::Digits
        && m_rightPanel
        && m_sessionRunner->parentWidget() == m_rightPanel) {
        m_sessionRunner->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        m_sessionRunner->raise();
        if (m_beginButton && !m_exerciseRunning) {
            m_beginButton->raise();
        }
    } else if (m_dualScreen && m_exerciseRunning && m_sessionRunner
        && (m_sessionRunnerKind == ExerciseRunnerKind::E28
            || m_sessionRunnerKind == ExerciseRunnerKind::Remember2)
        && m_rightPanel
        && m_sessionRunner->parentWidget() == m_rightPanel) {
        m_sessionRunner->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        m_sessionRunner->raise();
    }
    if (m_beginButton) {
        m_beginButton->setGeometry(976, 12, 158, 33);
        m_beginButton->setVisible(!m_exerciseRunning);
        m_beginButton->raise();
    }
    if (m_exerciseOptionsPanel && m_rightPanel) {
        const bool isE15 = m_exerciseId == QStringLiteral("1.5") || m_exerciseId == QStringLiteral("1.6");
        const bool is122 = m_exerciseId == QStringLiteral("1.22");
        const bool isPuzzleShard = m_exerciseId == QStringLiteral("1.19")
            || m_exerciseId == QStringLiteral("1.20") || m_exerciseId == QStringLiteral("1.21")
            || (m_exerciseId == QStringLiteral("1.14") && currentStepId().trimmed() == QStringLiteral("2"));
        const bool is316Shard = m_exerciseId == QStringLiteral("3.1.16")
            && currentStepId().trimmed().toInt() >= 3;
        // 3.1.16: своя ссылка на корне окна; общая панель опций для неё не нужна.
        // 1.5 / 1.6 / 1.19 / 1.20 / 1.21: ссылка рядом с «Начать» — options-панель на правой не нужна.
        if (is316Shard || m_exerciseId == QStringLiteral("1.5")
            || m_exerciseId == QStringLiteral("1.6")
            || m_exerciseId == QStringLiteral("1.19")
            || m_exerciseId == QStringLiteral("1.20")
            || m_exerciseId == QStringLiteral("1.21")) {
            m_exerciseOptionsPanel->hide();
        } else {
            // Только ссылка; всплывающая группа — поверх (layoutE15ModePopup).
            const int panelH = (isE15 || is122 || isPuzzleShard) ? 28 : 220;
            const int panelW = (isE15 || is122 || isPuzzleShard)
                ? 340
                : qMax(120, m_rightPanel->width() - 24);
            m_exerciseOptionsPanel->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
            m_exerciseOptionsPanel->setGeometry(12, 52, panelW, panelH);
            m_exerciseOptionsPanel->raise();
            if (m_shardButton && m_shardButton->isVisible()) {
                m_shardButton->adjustSize();
                m_shardButton->raise();
            }
        }
        layoutE15ModePopup();
    }
    // 1.5 / 1.6 / 1.19 / 1.20 / 1.21: «Настройка уровня сложности» на правой панели (поверх превью), ~1550 abs.
    if (m_shard15Button && m_rightPanel
        && (m_exerciseId == QStringLiteral("1.5") || m_exerciseId == QStringLiteral("1.6")
            || m_exerciseId == QStringLiteral("1.19") || m_exerciseId == QStringLiteral("1.20")
            || m_exerciseId == QStringLiteral("1.21"))
        && !m_exerciseRunning) {
        m_shard15Button->adjustSize();
        constexpr int kLinkAbsX = 1550;
        constexpr int kLinkAbsY = 12;
        constexpr int kLinkH = 33;
        const int rightPanelLeft = kPanelX + kScrollWidth;
        const int linkW = qMax(m_shard15Button->sizeHint().width(), 280);
        // Без clamp по width панели: при первом layout width часто мал → ссылка уезжала в (8,12).
        const int localX = qMax(8, kLinkAbsX - rightPanelLeft);
        m_shard15Button->setGeometry(localX, kLinkAbsY, linkW, kLinkH);
        m_shard15Button->show();
        m_shard15Button->raise();
    }
    if (m_previewImage) {
        updatePreviewLayout();
    }
    // После превью снова поднять ссылку 1.5/1.6/1.19/1.20/1.21 (превью иначе перекрывает).
    if ((m_exerciseId == QStringLiteral("1.5") || m_exerciseId == QStringLiteral("1.6")
         || m_exerciseId == QStringLiteral("1.19") || m_exerciseId == QStringLiteral("1.20")
         || m_exerciseId == QStringLiteral("1.21"))
        && m_shard15Button && m_shard15Button->isVisible()) {
        m_shard15Button->raise();
        if (m_e15ModeGroup && m_e15ModeGroup->isVisible()) {
            m_e15ModeGroup->raise();
            m_shard15Button->raise();
        }
        if (m_puzzleOptionsGroup && m_puzzleOptionsGroup->isVisible()) {
            m_puzzleOptionsGroup->raise();
            m_shard15Button->raise();
        }
    }
    if (m_exerciseId == QStringLiteral("4.1.7") && m_remPanel && m_remPanel->isVisible() && m_rightPanel) {
        layoutRemPanelWidget(m_remPanel, m_rightPanel);
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
    // 3.1.16: ссылка поверх Start/combo/превью (как label1 @ 1415,25).
    if (m_shard316Button && m_shard316Button->isVisible()) {
        constexpr int kLinkAbsX = 1415;
        constexpr int kLinkAbsY = 25;
        constexpr int kLinkW = 300;
        constexpr int kLinkH = 24;
        const int maxX = qMax(8, width() - kLinkW - 8);
        const int linkX = qBound(8, kLinkAbsX, maxX);
        m_shard316Button->setGeometry(linkX, kLinkAbsY, kLinkW, kLinkH);
        m_shard316Button->raise();
    }
    if (m_aparamGroup && m_aparamGroup->isVisible()) {
        m_aparamGroup->raise();
    }
    if (m_exerciseOptionsPanel && m_exerciseOptionsPanel->isVisible()) {
        m_exerciseOptionsPanel->raise();
    }
}

void ExerciseHost::layoutStepCombo() {
    if (!m_stepCombo) {
        return;
    }
    if (m_stepCombo->count() <= 0) {
        m_stepCombo->hide();
        return;
    }

    const bool nearBeginCombo = ExerciseConfig::usesAppendOnlyMultiStepLogic(m_exerciseId)
        || m_exerciseId == QStringLiteral("4.1.2");

    // Во время выполнения: скрыть (как упр. 6 / руководство для 4.1.2 / 5.2.1).
    if (m_exerciseRunning && nearBeginCombo) {
        m_stepCombo->hide();
        return;
    }

    // Всегда на ExerciseHost — иначе popup выпадающего списка обрезается/ломается.
    QWidget *host = this;
    if (m_stepCombo->parentWidget() != host) {
        m_stepCombo->setParent(host);
    }

    constexpr int kComboH = 33;
    constexpr int kComboY = 15; // как param1 в exbegin.Designer (15)
    constexpr int kRightMargin = 24;
    // Руководство: ~70–100px между правым краем Start и combo (оригинал: begin@976, combo@1182).
    constexpr int kGapAfterBegin = 80;
    // 1.19: «Леопард 3»; 1.20: «Машинка 5» — шире стандартных 121.
    const int kComboW = (m_exerciseId == QStringLiteral("1.19")
                         || m_exerciseId == QStringLiteral("1.20"))
        ? 168
        : 121;
    int comboX = qMax(0, host->width() - kComboW - kRightMargin);
    if (!m_exerciseRunning && nearBeginCombo) {
        int beginRight = 976 + 158;
        if (m_beginButton && m_beginButton->isVisibleTo(this)) {
            beginRight = m_beginButton->x() + m_beginButton->width();
        }
        comboX = beginRight + kGapAfterBegin;
        const int maxX = qMax(0, host->width() - kComboW - kRightMargin);
        if (comboX > maxX) {
            comboX = maxX;
        }
    }
    const int comboY = kComboY;
    static const QString kStepComboStyle = QStringLiteral(
        "QComboBox { background-color:#ffffff; background-image:none; color:#000000;"
        " border:1px solid #808080; padding:1px 28px 1px 6px; min-height:20px;"
        " font-family:'Microsoft Sans Serif'; font-size:14px; }"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding; subcontrol-position: top right;"
        "  width:22px; border:none; border-left:1px solid #b0b0b0; background:#ececec;"
        "}"
        "QComboBox::down-arrow { %1 }"
        "QComboBox QAbstractItemView {"
        "  background-color:#ffffff; background-image:none; color:#000000;"
        "  selection-background-color:#cce8ff; outline:0; border:1px solid #808080;"
        "}")
        .arg(stepComboArrowCss());
    if (m_stepCombo->styleSheet() != kStepComboStyle) {
        m_stepCombo->setStyleSheet(kStepComboStyle);
    }
    m_stepCombo->setGeometry(comboX, comboY, kComboW, kComboH);
    m_stepCombo->setVisible(true);
    m_stepCombo->raise();
    if (m_beginButton && m_beginButton->isVisible()) {
        m_beginButton->raise();
        m_stepCombo->raise();
    }
    if (QAbstractItemView *view = m_stepCombo->view()) {
        view->setMinimumWidth(kComboW);
        view->setTextElideMode(Qt::ElideNone);
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
    if (m_orBrowser->document()) {
        m_orBrowser->document()->setBaseUrl(QUrl::fromLocalFile(baseDir + QLatin1Char('/')));
    }
    m_orBrowser->setHtml(ExerciseAssets::prepareOrHtml(
        m_rawOrHtml, baseDir, m_orOpen1, m_orOpen2, m_orOpen3));
    applyCompactLineHeight(m_orBrowser->document());
    // Раскрытие «Процедуры» (график по центру) не должно сдвигать следующий заголовок вправо.
    if (QTextDocument *doc = m_orBrowser->document()) {
        QTextCursor cursor(doc);
        cursor.beginEditBlock();
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            const QString text = block.text();
            if (!text.contains(QStringLiteral("Анализируемые показатели"))
                && !text.contains(QStringLiteral("Процедура проведения"))
                && !text.contains(QStringLiteral("Методика"))) {
                continue;
            }
            QTextBlockFormat fmt = block.blockFormat();
            fmt.setAlignment(Qt::AlignLeft);
            fmt.setLeftMargin(0);
            fmt.setRightMargin(0);
            fmt.setTextIndent(0);
            cursor.setPosition(block.position());
            cursor.setBlockFormat(fmt);
        }
        cursor.endEditBlock();
    }
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
    m_forceNewProtocolSession = false;
    if (m_repository && !patientId.trimmed().isEmpty()) {
        const QString existingBody = m_repository->loadLastExerciseProtocolBody(patientId, exerciseId);
        if (!existingBody.trimmed().isEmpty()) {
            // Методики со сканом: не обновлять прошлую запись — иначе загрузка файла
            // перезаписывает сканы и старые ссылки «Показать изображение» открывают новый файл.
            // Как в оригинале (partly=false при входе), каждый визит/повтор — свой id протокола.
            if (!supportsScanUpload(exerciseId)) {
                m_partly = true;
            }
        }
    }
    // 1.7 / 1.11 / 1.14 / 1.15 / 1.17 / 1.19 / 1.20 / 1.21: при каждом входе протоколы формируются с нуля.
    if (m_exerciseId == QStringLiteral("1.7")
        || m_exerciseId == QStringLiteral("1.11")
        || m_exerciseId == QStringLiteral("1.14")
        || m_exerciseId == QStringLiteral("1.15")
        || m_exerciseId == QStringLiteral("1.17")
        || m_exerciseId == QStringLiteral("1.19")
        || m_exerciseId == QStringLiteral("1.20")
        || m_exerciseId == QStringLiteral("1.21")
        || m_exerciseId == QStringLiteral("4.2.2")
        || m_exerciseId == QStringLiteral("5.1.1")
        || m_exerciseId == QStringLiteral("5.2.1")
        || m_exerciseId == QStringLiteral("5.3.1")
        || m_exerciseId == QStringLiteral("5.4.2")) {
        m_partly = false;
    }
    m_sessionAdditional.clear();
    m_additionalByStep.clear();
    m_sessionStepId.clear();
    m_picturesShown = 0;
    m_currentProtocolId.clear();
    m_previewGenderPrefix = QStringLiteral("d");
    if (m_previewGirlRadio) {
        m_previewGirlRadio->blockSignals(true);
        m_previewGirlRadio->setChecked(true);
        m_previewGirlRadio->blockSignals(false);
    }
    if (m_previewBoyRadio) {
        m_previewBoyRadio->blockSignals(true);
        m_previewBoyRadio->setChecked(false);
        m_previewBoyRadio->blockSignals(false);
    }
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
    shutdownSessionUi();

    for (const ExerciseCheckRow &row : m_activityChecks) {
        if (row.box) {
            row.box->setChecked(false);
        }
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

void ExerciseHost::ensureWords422Panel() {
    if (m_words422Panel || !m_rightPanel) {
        return;
    }
    m_words422Panel = new QWidget(m_rightPanel);
    m_words422Panel->setStyleSheet(QStringLiteral("background:transparent;"));

    m_words422Label = new QLabel(
        QStringLiteral(
            "дерево, кукла, вилка, цветок, телефон, стакан, птица, пальто, лампочка, картина, "
            "человек, книга."),
        m_words422Panel);
    m_words422Label->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 14));
    m_words422Label->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
    m_words422Label->setWordWrap(true);

    // Как table.html / table2.html: 2 колонки («№ попытки» / «Кол-во…»), без QHeaderView
    // (иначе на превью заголовки рисуются чёрными).
    m_words422Table = new QTableWidget(7, 2, m_words422Panel);
    m_words422Table->horizontalHeader()->setVisible(false);
    m_words422Table->verticalHeader()->setVisible(false);
    m_words422Table->setShowGrid(true);
    m_words422Table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_words422Table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_words422Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_words422Table->setFocusPolicy(Qt::NoFocus);
    m_words422Table->setSelectionMode(QAbstractItemView::NoSelection);
    m_words422Table->setStyleSheet(QStringLiteral(
        "QTableWidget {"
        "  background-color:#f0f0f0; color:#000000; gridline-color:#000000;"
        "  border:1px solid #000000; outline:none;"
        "}"
        "QTableWidget::item {"
        "  background-color:#f0f0f0; color:#000000;"
        "  border:none; padding:2px;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color:#f0f0f0; color:#000000;"
        "}"));
    m_words422Table->setColumnWidth(0, 76);
    m_words422Table->setColumnWidth(1, 284);
    auto makeCell = [](const QString &text, bool bold = false) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QBrush(Qt::black));
        item->setBackground(QBrush(QColor(240, 240, 240)));
        if (bold) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }
        item->setFlags(Qt::ItemIsEnabled);
        return item;
    };
    m_words422Table->setItem(0, 0, makeCell(QStringLiteral("№ попытки"), true));
    m_words422Table->setItem(0, 1, makeCell(QStringLiteral("Кол-во правильно названных слов"), true));
    m_words422Table->setRowHeight(0, 40);
    for (int i = 0; i < 6; ++i) {
        m_words422Table->setItem(i + 1, 0, makeCell(QString::number(i + 1)));
        auto *value = makeCell(QString());
        value->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_words422Table->setItem(i + 1, 1, value);
        m_words422Table->setRowHeight(i + 1, 36);
    }
    const int tableH = 40 + 36 * 6 + 2 * m_words422Table->frameWidth() + 2;
    m_words422Table->setFixedSize(76 + 284 + 2 * m_words422Table->frameWidth() + 2, tableH);

    m_words422Graph = new QLabel(m_words422Panel);
    m_words422Graph->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_words422Graph->setStyleSheet(QStringLiteral("background:transparent;"));
    const QString graphPath =
        ExerciseAssets::exerciseFile(QStringLiteral("4.2.2"), QStringLiteral("graph.png"));
    if (!graphPath.isEmpty()) {
        m_words422GraphBase.load(graphPath);
        m_words422Graph->setPixmap(m_words422GraphBase);
        m_words422Graph->setFixedSize(m_words422GraphBase.size());
    }

    connect(m_words422Table, &QTableWidget::cellChanged, this, [this](int, int) {
        if (m_exerciseId != QStringLiteral("4.2.2")) {
            return;
        }
        syncWords422AdditionalFromPanel();
        updateWords422Panel(m_sessionAdditional);
    });
}

namespace {

void setWords422TableEditable(QTableWidget *table, bool editable) {
    if (!table) {
        return;
    }
    table->setEditTriggers(editable ? QAbstractItemView::AllEditTriggers
                                    : QAbstractItemView::NoEditTriggers);
    table->setFocusPolicy(editable ? Qt::StrongFocus : Qt::NoFocus);
    table->setSelectionMode(editable ? QAbstractItemView::SingleSelection
                                     : QAbstractItemView::NoSelection);
    for (int r = 1; r < table->rowCount(); ++r) {
        QTableWidgetItem *item = table->item(r, 1);
        if (!item) {
            item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignCenter);
            item->setForeground(QBrush(Qt::black));
            item->setBackground(QBrush(QColor(240, 240, 240)));
            table->setItem(r, 1, item);
        }
        item->setFlags(editable
                           ? Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable
                           : Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
}

} // namespace

void ExerciseHost::layoutWords422Panel() {
    if (m_exerciseId != QStringLiteral("4.2.2") || !m_rightPanel) {
        if (m_words422Panel) {
            m_words422Panel->hide();
        }
        return;
    }
    ensureWords422Panel();
    // UserControl1 @ (900,150); в dual — на 100 px ниже (ТЗ 34.4).
    constexpr int kUcAbsLeft = 900;
    constexpr int kUcAbsTop = 150;
    const int dualNudgeY = AppSettings::dualScreenEnabled() ? 100 : 0;
    const int rightPanelLeft = kPanelX + kScrollWidth;
    const int localX = qMax(0, kUcAbsLeft - rightPanelLeft);
    const int localY = kUcAbsTop + dualNudgeY;
    const int panelW = qMax(400, m_rightPanel->width() - localX - 8);
    const int panelH = qMax(500, m_rightPanel->height() - localY - 8);
    m_words422Panel->setGeometry(localX, localY, panelW, panelH);
    if (m_words422Label) {
        m_words422Label->setGeometry(26, 16, qMin(886, panelW - 40), 48);
    }
    if (m_words422Table) {
        // UserControl1.webBrowser1 @ (190,60) внутри UC
        m_words422Table->move(190, 60);
    }
    if (m_words422Graph) {
        // pictureBox1 @ (160,361)
        m_words422Graph->move(160, 361);
    }
    m_words422Panel->show();
    m_words422Panel->raise();
    if (m_previewImage) {
        m_previewImage->hide();
    }
}

void ExerciseHost::syncWords422AdditionalFromPanel() {
    if (!m_words422Table) {
        return;
    }
    QStringList parts;
    for (int r = 0; r < 6; ++r) {
        const QTableWidgetItem *item = m_words422Table->item(r + 1, 1);
        parts << (item ? item->text().trimmed() : QString());
    }
    m_sessionAdditional = parts.join(QLatin1Char(';')) + QLatin1Char(';');
}

void ExerciseHost::updateWords422Panel(const QString &additional) {
    ensureWords422Panel();
    if (!m_words422Table) {
        return;
    }

    const QStringList parts = additional.split(QLatin1Char(';'));
    m_words422Table->blockSignals(true);
    for (int r = 0; r < 6; ++r) {
        QTableWidgetItem *item = m_words422Table->item(r + 1, 1);
        if (!item) {
            item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignCenter);
            item->setForeground(QBrush(Qt::black));
            item->setBackground(QBrush(QColor(240, 240, 240)));
            m_words422Table->setItem(r + 1, 1, item);
        }
        item->setText(r < parts.size() ? parts.at(r).trimmed() : QString());
    }
    m_words422Table->blockSignals(false);

    // exbegin.cs: линия по graph.png, coordByValue(value) при шкале 0..12.
    if (m_words422Graph && !m_words422GraphBase.isNull()) {
        QPixmap canvas = m_words422GraphBase;
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(QColor(239, 71, 227), 5);
        painter.setPen(pen);

        auto coordByValue = [](double value) -> int {
            const double x = value * 100.0 / 12.0;
            double xx = 215.0 * x / 100.0;
            xx = xx + 70.0;
            xx = 313.0 - xx;
            return qRound(xx);
        };
        auto readValue = [&parts](int index) -> double {
            if (index < 0 || index >= parts.size()) {
                return 0.0;
            }
            bool ok = false;
            const double v = parts.at(index).trimmed().toDouble(&ok);
            return ok ? v : 0.0;
        };

        const int xs[] = {110, 146, 184, 221, 256, 291};
        QPoint prev(xs[0], coordByValue(readValue(0)));
        for (int i = 1; i < 6; ++i) {
            const QPoint next(xs[i], coordByValue(readValue(i)));
            // Рисуем сегмент только если есть хотя бы одно ненулевое/введённое значение в паре.
            const bool hasData = (i - 1 < parts.size() && !parts.at(i - 1).trimmed().isEmpty())
                || (i < parts.size() && !parts.at(i).trimmed().isEmpty());
            if (hasData || readValue(i - 1) > 0 || readValue(i) > 0) {
                painter.drawLine(prev, next);
            }
            prev = next;
        }
        painter.end();
        m_words422Graph->setPixmap(canvas);
    }
    layoutWords422Panel();
}

namespace {

QStringList words511Groups() {
    return {
        QStringLiteral("Животные"),
        QStringLiteral("Растения"),
        QStringLiteral("Цвета предметов"),
        QStringLiteral("Формы предметов"),
        QStringLiteral("Другие признаки предметов, кроме формы и цвета."),
        QStringLiteral("Действия человека."),
        QStringLiteral("Способы выполнения человеком действий."),
        QStringLiteral("Качества выполняемых человеком действий."),
    };
}

} // namespace

void ExerciseHost::ensureWords511Panel() {
    if (m_words511Panel && m_words511Table && m_words511Table->rowCount() != words511Groups().size() + 1) {
        m_words511Panel->deleteLater();
        m_words511Panel = nullptr;
        m_words511Table = nullptr;
    }
    if (m_words511Panel || !m_rightPanel) {
        return;
    }
    m_words511Panel = new QWidget(m_rightPanel);
    m_words511Panel->setStyleSheet(QStringLiteral("background:transparent;"));

    const QStringList groups = words511Groups();
    // Как table.html: первая строка — заголовки, без QHeaderView (иначе чёрные ячейки).
    m_words511Table = new QTableWidget(groups.size() + 1, 2, m_words511Panel);
    m_words511Table->horizontalHeader()->setVisible(false);
    m_words511Table->verticalHeader()->setVisible(false);
    m_words511Table->setShowGrid(true);
    m_words511Table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_words511Table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_words511Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_words511Table->setFocusPolicy(Qt::NoFocus);
    m_words511Table->setSelectionMode(QAbstractItemView::NoSelection);
    m_words511Table->setWordWrap(true);
    m_words511Table->setStyleSheet(QStringLiteral(
        "QTableWidget {"
        "  background-color:#f6f6f6; color:#000000; gridline-color:#000000;"
        "  border:1px solid #000000; outline:none;"
        "}"
        "QTableWidget::item {"
        "  background-color:#f6f6f6; color:#000000;"
        "  border:none; padding:4px;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color:#f6f6f6; color:#000000;"
        "}"));
    m_words511Table->setColumnWidth(0, 266);
    m_words511Table->setColumnWidth(1, 464);
    auto makeCell = [](const QString &text, bool header = false) {
        auto *item = new QTableWidgetItem(text);
        item->setForeground(QBrush(Qt::black));
        item->setBackground(QBrush(QColor(246, 246, 246)));
        if (header) {
            item->setTextAlignment(Qt::AlignCenter);
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        } else {
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
        item->setFlags(Qt::ItemIsEnabled);
        return item;
    };
    m_words511Table->setItem(0, 0, makeCell(QStringLiteral("Группы"), true));
    m_words511Table->setItem(0, 1, makeCell(QStringLiteral("Названные ребенком слова"), true));
    m_words511Table->setRowHeight(0, 40);
    for (int i = 0; i < groups.size(); ++i) {
        m_words511Table->setItem(i + 1, 0, makeCell(groups.at(i)));
        auto *wordItem = makeCell(QString());
        wordItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_words511Table->setItem(i + 1, 1, wordItem);
        m_words511Table->setRowHeight(i + 1, 36);
    }
    const int tableH = 40 + 36 * groups.size() + 2 * m_words511Table->frameWidth() + 2;
    m_words511Table->setFixedSize(266 + 464 + 2 * m_words511Table->frameWidth() + 2, tableH);
}

void ExerciseHost::setWords511TableEditable(bool editable) {
    if (!m_words511Table) {
        return;
    }
    m_words511Table->setEditTriggers(editable ? QAbstractItemView::AllEditTriggers
                                              : QAbstractItemView::NoEditTriggers);
    m_words511Table->setFocusPolicy(editable ? Qt::StrongFocus : Qt::NoFocus);
    m_words511Table->setSelectionMode(editable ? QAbstractItemView::SingleSelection
                                               : QAbstractItemView::NoSelection);
    for (int r = 1; r < m_words511Table->rowCount(); ++r) {
        QTableWidgetItem *groupItem = m_words511Table->item(r, 0);
        if (groupItem) {
            groupItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        }
        QTableWidgetItem *wordItem = m_words511Table->item(r, 1);
        if (!wordItem) {
            wordItem = new QTableWidgetItem;
            wordItem->setForeground(QBrush(Qt::black));
            wordItem->setBackground(QBrush(QColor(246, 246, 246)));
            m_words511Table->setItem(r, 1, wordItem);
        }
        wordItem->setFlags(editable
                               ? Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable
                               : Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
}

void ExerciseHost::layoutWords511Panel() {
    if (m_exerciseId != QStringLiteral("5.1.1") || !m_rightPanel) {
        if (m_words511Panel) {
            m_words511Panel->hide();
        }
        return;
    }
    ensureWords511Panel();
    // Правая половина: +50 по X к центру; dual — ещё +100 по Y (ТЗ 35.1 / 35.3).
    constexpr int kPanelAbsLeft = 900;
    constexpr int kPanelAbsTop = 150;
    constexpr int kNudgeX = 50;
    const int dualNudgeY = AppSettings::dualScreenEnabled() ? 100 : 0;
    const int rightPanelLeft = kPanelX + kScrollWidth;
    const int localX = qMax(0, kPanelAbsLeft - rightPanelLeft) + kNudgeX;
    const int localY = kPanelAbsTop + dualNudgeY;
    const int panelW = qMax(400, m_rightPanel->width() - localX - 8);
    const int panelH = qMax(500, m_rightPanel->height() - localY - 8);
    m_words511Panel->setGeometry(localX, localY, panelW, panelH);
    if (m_words511Table) {
        m_words511Table->move(0, 0);
    }
    m_words511Panel->show();
    m_words511Panel->raise();
    if (m_previewImage) {
        m_previewImage->hide();
    }
}

void ExerciseHost::updatePreviewLayout() {
    if (m_exerciseId == QStringLiteral("4.2.2") || m_exerciseId == QStringLiteral("5.1.1")
        || m_exerciseId == QStringLiteral("4.2.1")) {
        if (m_previewImage) {
            m_previewImage->hide();
        }
        if (m_exerciseId == QStringLiteral("4.2.2")) {
            layoutWords422Panel();
        } else if (m_exerciseId == QStringLiteral("5.1.1")) {
            layoutWords511Panel();
        } else if (!m_exerciseRunning) {
            ensureDigitsPreviewRunner();
        }
        if (m_timeResultLabel && m_timeResultLabel->isVisible() && m_rightPanel) {
            const int rightPanelLeft = kPanelX + kScrollWidth;
            const int beginLocalX = qMax(0, 976 - rightPanelLeft);
            m_timeResultLabel->adjustSize();
            m_timeResultLabel->move(beginLocalX + 200, 24);
            m_timeResultLabel->raise();
        }
        return;
    }
    if (!m_previewImage) {
        return;
    }
    if (m_exerciseId == QStringLiteral("4.1.7")) {
        m_previewImage->hide();
        if (m_previewGenderPanel) {
            m_previewGenderPanel->hide();
        }
        if (m_remPanel && m_remPanel->isVisible() && m_rightPanel) {
            layoutRemPanelWidget(m_remPanel, m_rightPanel);
        }
        return;
    }
    if (m_exerciseRunning) {
        const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
        if (m_dualScreen
            || (definition
                && (definition->runner == ExerciseRunnerKind::Puzzles
                    || definition->runner == ExerciseRunnerKind::OnlyDemo
                    || definition->runner == ExerciseRunnerKind::Remember))) {
            m_previewImage->hide();
            if (m_previewGenderPanel) {
                m_previewGenderPanel->hide();
            }
            return;
        }
    }
    if (m_previewSource.isNull()) {
        m_previewImage->hide();
        if (m_previewGenderPanel) {
            m_previewGenderPanel->hide();
        }
        return;
    }

    const int rightPanelLeft = kPanelX + kScrollWidth;
    const int panelW = m_rightPanel ? m_rightPanel->width() : qMax(120, width() - rightPanelLeft);
    const int panelH = m_rightPanel ? m_rightPanel->height() : height();

    int localX = 0;
    int localY = 0;
    QPixmap display = m_previewSource;

    if (m_exerciseId == QStringLiteral("1.8")
        || m_exerciseId == QStringLiteral("1.17")
        || m_exerciseId == QStringLiteral("1.18")
        || m_exerciseId == QStringLiteral("1.25")
        || m_exerciseId == QStringLiteral("1.26")
        || m_exerciseId == QStringLiteral("2.8")
        || m_exerciseId == QStringLiteral("2.9")
        || m_exerciseId == QStringLiteral("2.10")
        || m_exerciseId == QStringLiteral("3.1.1")
        || m_exerciseId == QStringLiteral("3.1.2")
        || m_exerciseId == QStringLiteral("3.1.10")
        || m_exerciseId == QStringLiteral("3.1.11")
        || m_exerciseId == QStringLiteral("3.1.12")
        || m_exerciseId == QStringLiteral("3.1.17")
        || m_exerciseId == QStringLiteral("3.1.18")
        || m_exerciseId == QStringLiteral("3.2.1")
        || m_exerciseId == QStringLiteral("3.2.2")
        || m_exerciseId == QStringLiteral("3.2.4")
        || m_exerciseId == QStringLiteral("3.2.5")
        || m_exerciseId == QStringLiteral("3.2.11")
        || m_exerciseId == QStringLiteral("4.1.1")
        || m_exerciseId == QStringLiteral("4.1.2")
        || m_exerciseId == QStringLiteral("4.1.4")
        || m_exerciseId == QStringLiteral("5.2.1")
        || m_exerciseId == QStringLiteral("5.3.1")
        || m_exerciseId == QStringLiteral("5.4.2")) {
        // Как OnlyPExercise::Specialist при dual: та же область, масштаб и центрирование.
        constexpr int kPictureMargin = 12;
        constexpr int kSpecialistPictureShiftLeft = 15;
        constexpr int kButtonMargin = 12;
        int extraX = 0;
        int extraY = 0;
        if (m_exerciseId == QStringLiteral("1.18") && currentStepId() == QStringLiteral("3")) {
            extraX = -20;
            extraY = -10; // было +40; поднять на 50px (как Specialist при dual)
        } else if (m_exerciseId == QStringLiteral("1.25")) {
            extraY = -120;
        } else if (m_exerciseId == QStringLiteral("2.10")
                   || m_exerciseId == QStringLiteral("3.1.2")
                   || m_exerciseId == QStringLiteral("3.1.10")
                   || m_exerciseId == QStringLiteral("3.1.12")
                   || m_exerciseId == QStringLiteral("3.1.18")
                   || m_exerciseId == QStringLiteral("3.2.1")
                   || m_exerciseId == QStringLiteral("3.2.2")
                   || m_exerciseId == QStringLiteral("3.2.5")
                   || m_exerciseId == QStringLiteral("3.2.11")) {
            // Как 1.1: строго по центру правой панели.
            extraX = 0;
            extraY = 0;
        } else if (m_exerciseId == QStringLiteral("4.1.1")) {
            // 27.2: чуть ниже и левее → центр правой половины.
            extraX = -25;
            extraY = 40;
        } else if (m_exerciseId == QStringLiteral("4.1.2")) {
            // 28.5: обе картинки (Пример / 1) — ниже и правее → центр правой половины.
            extraX = 80;
            extraY = 40;
        } else if (m_exerciseId == QStringLiteral("4.1.4")) {
            // 29.1: чуть ниже → центр правой половины по вертикали.
            extraY = 40;
        } else if (m_exerciseId == QStringLiteral("5.2.1")
                   || m_exerciseId == QStringLiteral("5.3.1")
                   || m_exerciseId == QStringLiteral("5.4.2")) {
            // 36.14 / 37.10 / 38.2: чуть ниже и правее → центр правой половины.
            extraX = 80;
            extraY = 40;
        } else if (m_exerciseId == QStringLiteral("2.8")
                   || m_exerciseId == QStringLiteral("2.9")
                   || m_exerciseId == QStringLiteral("3.1.11")
                   || m_exerciseId == QStringLiteral("3.1.17")
                   || m_exerciseId == QStringLiteral("3.2.4")) {
            // До старта: по центру правой половины по вертикали.
            extraY = 40;
            if (m_exerciseId == QStringLiteral("3.1.17")) {
                // Чуть левее относительно базового specialist-сдвига.
                extraX = -25;
            } else if (m_exerciseId == QStringLiteral("3.2.4")
                       || m_exerciseId == QStringLiteral("3.1.11")) {
                extraX = 80;
            }
        } else if (m_exerciseId == QStringLiteral("3.1.1")) {
            // Как 1.1: строго по центру правой панели.
            extraX = 0;
            extraY = 0;
        }

        int contentTop = kButtonMargin;
        const bool showGender = m_exerciseId == QStringLiteral("1.26")
            && !m_exerciseRunning
            && currentStepId() != QStringLiteral("2");
        if (showGender && m_previewGenderPanel) {
            contentTop += 36;
        }
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        QPixmap stopPixmap;
        if (!stopPath.isEmpty() && stopPixmap.load(stopPath)) {
            contentTop += stopPixmap.height() + kButtonMargin;
        } else {
            contentTop += kButtonMargin;
        }

        const int availableW = qMax(40, panelW - 2 * kPictureMargin);
        const int availableH = qMax(40, panelH - contentTop - kPictureMargin);
        if (display.width() > availableW || display.height() > availableH) {
            display = m_previewSource.scaled(
                availableW, availableH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        if (m_exerciseId == QStringLiteral("2.10")
            || m_exerciseId == QStringLiteral("3.1.1")
            || m_exerciseId == QStringLiteral("3.1.2")
            || m_exerciseId == QStringLiteral("3.1.10")
            || m_exerciseId == QStringLiteral("3.1.12")
            || m_exerciseId == QStringLiteral("3.1.18")
            || m_exerciseId == QStringLiteral("3.2.1")
            || m_exerciseId == QStringLiteral("3.2.2")
            || m_exerciseId == QStringLiteral("3.2.5")
            || m_exerciseId == QStringLiteral("3.2.11")) {
            // Как 1.1 Specialist: строго геометрический центр панели.
            localX = qMax(kPictureMargin, (panelW - display.width()) / 2);
            localY = qMax(kPictureMargin, (panelH - display.height()) / 2);
        } else {
            localX = kPictureMargin + qMax(0, (panelW - display.width()) / 2) - kSpecialistPictureShiftLeft
                + extraX;
            if (localX + display.width() > panelW - kPictureMargin) {
                localX = qMax(kPictureMargin, panelW - kPictureMargin - display.width());
            }
            localX = qMax(kPictureMargin, localX);
            localY = qMax(contentTop, (panelH - display.height()) / 2 + extraY);
        }

        if (m_previewGenderPanel) {
            if (showGender) {
                constexpr int kGenderW = 220;
                constexpr int kGenderH = 28;
                const int genderX = qMax(0, (panelW - kGenderW) / 2);
                const int genderY = qMax(8, localY - 40);
                m_previewGenderPanel->setGeometry(genderX, genderY, kGenderW, kGenderH);
                m_previewGenderPanel->show();
                m_previewGenderPanel->raise();
            } else {
                m_previewGenderPanel->hide();
            }
        }
    } else {
        if (m_previewGenderPanel) {
            m_previewGenderPanel->hide();
        }
        constexpr int kPreviewAbsLeft = 1100;
        constexpr int kPreviewAbsTop = 75;
        // 1.5/1.6: чуть ниже ссылки «Настройка уровня сложности».
        int previewAbsTop =
            (m_exerciseId == QStringLiteral("1.5") || m_exerciseId == QStringLiteral("1.6"))
            ? 100
            : kPreviewAbsTop;
        int previewAbsLeft = kPreviewAbsLeft;
        if (m_exerciseId == QStringLiteral("1.14")) {
            // До старта: оба задания чуть ниже (~125) и правее (~50).
            previewAbsLeft =
                (currentStepId().trimmed() == QStringLiteral("1") ? 900 : 1100) + 50;
            previewAbsTop = kPreviewAbsTop + 125;
        }
        if (m_exerciseId == QStringLiteral("1.19")) {
            // Как трафарет на холсте: гориз. 1150×310, верт. 1250×240.
            const QString step = currentStepId().trimmed();
            const bool horizontal =
                step == QStringLiteral("Леопард 3") || step == QStringLiteral("Дом 4");
            previewAbsLeft = horizontal ? 1150 : 1250;
            previewAbsTop = horizontal ? 310 : 240;
        }
        if (m_exerciseId == QStringLiteral("1.20")) {
            // Трафарет/задание 1260×300.
            previewAbsLeft = 1260;
            previewAbsTop = 300;
        }
        // 3.1.16: как exbegin — не перекрывать ссылку сложности (Top=100/140).
        if (m_exerciseId == QStringLiteral("3.1.16")) {
            previewAbsLeft = 1100;
            previewAbsTop = currentStepId().trimmed().toInt() >= 3 ? 100 : 140;
        }
        localX = previewAbsLeft - rightPanelLeft;
        localY = previewAbsTop;
        const int maxW = qMax(120, width() - previewAbsLeft - 16);
        const int maxH = qMax(120, height() - previewAbsTop - 16);
        if (display.width() > maxW || display.height() > maxH) {
            display = m_previewSource.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }

    m_previewImage->setPixmap(display);
    m_previewImage->setFixedSize(display.size());
    m_previewImage->move(qMax(0, localX), localY);
    m_previewImage->show();
    // 1.20 до старта: инструкция поворота рядом с заданием (−80 влево от 1450).
    if (m_rightPanel) {
        QLabel *previewRotate = m_rightPanel->findChild<QLabel *>(QStringLiteral("dokitPreviewRotateHint"));
        if (m_exerciseId == QStringLiteral("1.20") && !m_exerciseRunning) {
            if (!previewRotate) {
                previewRotate = new QLabel(m_rightPanel);
                previewRotate->setObjectName(QStringLiteral("dokitPreviewRotateHint"));
                previewRotate->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                const QString rotateHintPath = ExerciseAssets::sysImage(QStringLiteral("rotate_hint.jpg"));
                if (!rotateHintPath.isEmpty()) {
                    const QPixmap hintPm(rotateHintPath);
                    previewRotate->setPixmap(hintPm);
                    previewRotate->setFixedSize(hintPm.size());
                }
            }
            constexpr int kHintAbsX = 1330; // 1450 − 80 − 50 + 10
            constexpr int kHintAbsY = 300;
            previewRotate->move(qMax(0, kHintAbsX - rightPanelLeft), kHintAbsY);
            previewRotate->show();
            previewRotate->raise();
        } else if (previewRotate) {
            previewRotate->hide();
        }
    }
    // Опции сложности / всплывающая группа поверх превью (1.5/1.6 / 3.1.16).
    if (m_exerciseOptionsPanel && m_exerciseOptionsPanel->isVisible()) {
        m_exerciseOptionsPanel->raise();
    }
    if (m_shard316Button && m_shard316Button->isVisible()) {
        m_shard316Button->raise();
    }
    if (m_shard15Button && m_shard15Button->isVisible()) {
        m_shard15Button->raise();
    }
    layoutE15ModePopup();
    if (m_aparamGroup && m_aparamGroup->isVisible()) {
        m_aparamGroup->raise();
    }
    if (m_timeResultLabel && m_timeResultLabel->isVisible()) {
        // По центру правой панели (правее превью по горизонтали).
        const int rightPanelWidth = qMax(1, width() - rightPanelLeft);
        m_timeResultLabel->adjustSize();
        int timerX = qMax(0, (rightPanelWidth - m_timeResultLabel->width()) / 2);
        // 1.22: сдвинуть таймер на 300px вправо в правой части.
        if (m_exerciseId == QStringLiteral("1.22")) {
            timerX += 300;
        }
        const int timerY = qMax(8, localY - 36);
        m_timeResultLabel->move(timerX, timerY);
        m_timeResultLabel->raise();
    }
}

bool ExerciseHost::supportsScanUpload(const QString &exerciseId) {
    Q_UNUSED(exerciseId);
    // Механизм «Показать изображение» / «Загрузить» отключён во всех упражнениях.
    return false;
}

bool ExerciseHost::supportsStimulusPrint(const QString &exerciseId) {
    return exerciseId == QStringLiteral("1.7")
        || exerciseId == QStringLiteral("1.12")
        || exerciseId == QStringLiteral("2.1")
        || exerciseId == QStringLiteral("2.2")
        || exerciseId == QStringLiteral("2.3")
        || exerciseId == QStringLiteral("3.3.1")
        || exerciseId == QStringLiteral("3.3.2")
        || exerciseId == QStringLiteral("3.3.3");
}

QString ExerciseHost::stimulusPrintImagePath() const {
    const QString step = currentStepId().trimmed().isEmpty()
        ? QStringLiteral("1")
        : currentStepId().trimmed();
    if (m_exerciseId == QStringLiteral("1.7")) {
        return ExerciseAssets::exerciseFile(
            m_exerciseId, QStringLiteral("f") + step + QStringLiteral(".png"));
    }
    if (m_exerciseId == QStringLiteral("1.12") || m_exerciseId == QStringLiteral("3.3.2")
        || m_exerciseId == QStringLiteral("3.3.3")) {
        return ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("f1.png"));
    }
    if (m_exerciseId == QStringLiteral("2.1")) {
        return ExerciseAssets::exerciseFile(
            m_exerciseId, QStringLiteral("traf") + step + QStringLiteral(".png"));
    }
    if (m_exerciseId == QStringLiteral("2.2")) {
        // Оригинал печатает traf1 из папки 2.1.
        const QString from21 = ExerciseAssets::exerciseFile(
            QStringLiteral("2.1"), QStringLiteral("traf1.png"));
        if (!from21.isEmpty()) {
            return from21;
        }
        return ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("traf1.png"));
    }
    if (m_exerciseId == QStringLiteral("2.3")) {
        return ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("toprint.png"));
    }
    if (m_exerciseId == QStringLiteral("3.3.1")) {
        // Основной кадр; печать двух повёрнутых картинок — в InfantWindow.
        return ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("traf1.png"));
    }
    return ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("f1.png"));
}

void ExerciseHost::refreshProtocolViewAfterScanUpload() {
    if (!m_repository || !m_templateBrowser) {
        return;
    }
    if (m_currentProtocolId.isEmpty() && !m_patientId.trimmed().isEmpty()) {
        m_currentProtocolId = m_repository->loadLastExerciseProtocolId(m_patientId, m_exerciseId);
    }
    if (m_currentProtocolId.isEmpty()) {
        return;
    }
    m_partly = true;
    m_suppressProtocolAutosave = true;
    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    if (!viewHtml.trimmed().isEmpty()) {
        m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
        finalizeProtocolTemplateDocument(m_templateBrowser->document());
        const int templateViewportWidth = kTemplateTableWidth + kTemplateViewportPadding;
        m_templateBrowser->setFixedWidth(templateViewportWidth);
        if (m_templatePanel) {
            m_templatePanel->setMaximumWidth(templateViewportWidth + 16);
        }
        layoutContent();
        QTimer::singleShot(80, this, [this]() { updateContentHeights(); });
    }
    updateProtocolEditMode();
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });
}

void ExerciseHost::reloadPreviewForCurrentStep() {
    if (m_exerciseId == QStringLiteral("4.1.7")) {
        // Как exbegin: pfront скрыт — только панель выбора картинок rem.
        m_previewSource = QPixmap();
        if (m_previewImage) {
            m_previewImage->hide();
        }
        updateExerciseOptionsPanel();
        return;
    }
    if (m_exerciseId == QStringLiteral("1.22")) {
        // Как просили: без картинки предварительного просмотра (fкруг/fквадрат/…).
        m_previewSource = QPixmap();
        if (m_previewImage) {
            m_previewImage->hide();
        }
        return;
    }
    if (m_exerciseId == QStringLiteral("4.2.2")) {
        m_previewSource = QPixmap();
        if (m_previewImage) {
            m_previewImage->hide();
        }
        ensureWords422Panel();
        setWords422TableEditable(m_words422Table, m_words422Editable);
        layoutWords422Panel();
        return;
    }
    if (m_exerciseId == QStringLiteral("5.1.1")) {
        m_previewSource = QPixmap();
        if (m_previewImage) {
            m_previewImage->hide();
        }
        ensureWords511Panel();
        setWords511TableEditable(false);
        layoutWords511Panel();
        return;
    }
    if (m_exerciseId == QStringLiteral("4.2.1")) {
        m_previewSource = QPixmap();
        if (m_previewImage) {
            m_previewImage->hide();
        }
        if (!m_exerciseRunning) {
            ensureDigitsPreviewRunner();
        }
        return;
    }
    m_previewSource = QPixmap();
    const QString step = currentStepId();
    QStringList candidates;
    if (m_exerciseId == QStringLiteral("1.26")) {
        // Оригинал: задание 1 → d1/m1.png, задание 2 → f2.png
        if (step == QStringLiteral("2")) {
            candidates << QStringLiteral("f2.png");
            if (m_previewGenderPanel) {
                m_previewGenderPanel->hide();
            }
        } else {
            const QString prefix = m_previewGenderPrefix.isEmpty()
                ? QStringLiteral("d")
                : m_previewGenderPrefix;
            candidates << prefix + QStringLiteral("1.png")
                       << QStringLiteral("d1.png")
                       << QStringLiteral("f1.png");
        }
    } else if (m_exerciseId == QStringLiteral("1.272")) {
        // Оригинал e1272: картинка задания = N.png (fN.png — только мелкое превью в списке).
        if (!step.isEmpty()) {
            candidates << step + QStringLiteral(".png") << QStringLiteral("f") + step + QStringLiteral(".png");
        } else {
            candidates << QStringLiteral("1.png") << QStringLiteral("f1.png");
        }
    } else if (m_exerciseId == QStringLiteral("5.2.1")
               || m_exerciseId == QStringLiteral("5.3.1")) {
        // Картинка задания = N.png (fN — превью того же кадра).
        if (!step.isEmpty()) {
            candidates << step + QStringLiteral(".png")
                       << QStringLiteral("f") + step + QStringLiteral(".png");
        } else {
            candidates << QStringLiteral("1.png") << QStringLiteral("f1.png");
        }
    } else if (m_exerciseId == QStringLiteral("5.4.2")) {
        candidates << QStringLiteral("f1.png") << QStringLiteral("traf1.png")
                   << QStringLiteral("tale.png");
    } else if (m_exerciseId == QStringLiteral("1.5")) {
        candidates << QStringLiteral("f1.5.png") << QStringLiteral("carpet1.png")
                   << QStringLiteral("carpet2.png") << QStringLiteral("f1.png")
                   << QStringLiteral("ready.png");
    } else if (m_exerciseId == QStringLiteral("1.6")) {
        // Первое задание + «Следующее задание» (как f1.6.png в оригинале).
        candidates << QStringLiteral("f1.6.png") << QStringLiteral("1/pole.png")
                   << QStringLiteral("next.png") << QStringLiteral("ready.png");
    } else if (m_exerciseId == QStringLiteral("1.8")) {
        // Тот же файл, что OnlyPExercise (single → p%1.png).
        candidates << QStringLiteral("p1.png") << QStringLiteral("f1.png") << QStringLiteral("1.png");
    } else if (m_exerciseId == QStringLiteral("1.25")) {
        candidates << QStringLiteral("p1.png") << QStringLiteral("f1.png") << QStringLiteral("1.png");
    } else if (m_exerciseId == QStringLiteral("1.17") || m_exerciseId == QStringLiteral("1.18")) {
        if (!step.isEmpty()) {
            candidates << QStringLiteral("p") + step + QStringLiteral(".png")
                       << step + QStringLiteral(".png")
                       << QStringLiteral("f") + step + QStringLiteral(".png");
        } else {
            candidates << QStringLiteral("p1.png") << QStringLiteral("1.png") << QStringLiteral("f1.png");
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

void ExerciseHost::clearActivityChecks() {
    for (const ExerciseCheckRow &row : m_activityChecks) {
        if (row.label && row.label->parentWidget()) {
            delete row.label->parentWidget();
        }
    }
    m_activityChecks.clear();
}

void ExerciseHost::clearHelpChecks() {
    for (const ExerciseCheckRow &row : m_helpChecks) {
        if (row.label && row.label->parentWidget()) {
            delete row.label->parentWidget();
        }
    }
    m_helpChecks.clear();
}

void ExerciseHost::syncHelpChecksFromOrHtml() {
    clearHelpChecks();
    if (!m_helpChecksLayout) {
        return;
    }

    // 32.1: 4.1.8 — виды помощи только в процессе выполнения; на странице описания скрыть.
    if (m_exerciseId == QStringLiteral("4.1.8")
        || m_exerciseId == QStringLiteral("5.4.2")) {
        if (m_checkboxPanel) {
            m_checkboxPanel->hide();
        }
        return;
    }
    if (m_checkboxPanel) {
        m_checkboxPanel->show();
    }

    QStringList labels = parseHelpLabelsFromOrHtml(m_rawOrHtml);
    // 1.272: свои 3 вида помощи из руководства (fallback при сбое парсинга).
    if (m_exerciseId == QStringLiteral("1.272") && labels.size() < 3) {
        labels = QStringList{
            QStringLiteral(
                "Демонстрация карточек с изображением лиц с основными эмоциональными состояниями, "
                "с предложением выбрать соответствующее ситуации состояние."),
            QStringLiteral("Наводящие вопросы, способствующие выделению причины."),
            QStringLiteral("Подсказывающий вопрос."),
        };
    }
    // 3.1.10: свои виды помощи (3 пункта, не стандартные 5).
    if (m_exerciseId == QStringLiteral("3.1.10") && labels.size() < 3) {
        labels = QStringList{
            QStringLiteral("Повтор более подробной и развернутой инструкции."),
            QStringLiteral(
                "Просьба назвать объекты, изображенные на таблице (в случае затруднения ребенка "
                "специалист называет объекты таблицы сам)."),
            QStringLiteral(
                "наводящие вопросы к выделению функциональных признаков или отнесению объектов "
                "к какой-либо группе."),
        };
    }
    if (labels.isEmpty()) {
        labels = defaultHelpLabels();
    }

    // 4.2.1: 3 вида помощи с заголовками Стимулирующая / Направляющая / Обучающая (как в РП).
    const bool categorizedThreeHelp = m_exerciseId == QStringLiteral("4.2.1") && labels.size() == 3;
    // Плоский список без подзаголовков — только явные исключения (не из‑за != 5 пунктов).
    // У 1.5 и др. в or.html часто 7 idp (1+2+4) — заголовки всё равно нужны.
    const bool flatCustom = !categorizedThreeHelp
        && (m_exerciseId == QStringLiteral("1.272")
            || m_exerciseId == QStringLiteral("1.21")
            || m_exerciseId == QStringLiteral("3.1.10"));
    // 1.12: заголовки из or.html (не Стимулирующая/Направляющая/Обучающая).
    const bool help112 = m_exerciseId == QStringLiteral("1.12");
    // 1.14: «после 1 серии» / «После 2 серии» (по 4 пункта), без «Обучающая».
    const bool help114 = m_exerciseId == QStringLiteral("1.14");
    const bool showPenaltyHint =
        m_exerciseId == QStringLiteral("3.1.10") || help112 || help114;
    if (m_helpPenaltyHintLabel) {
        m_helpPenaltyHintLabel->setText(
            QStringLiteral("За каждый вид помощи оценка снижается на 0,5 балла"));
        if (help112 || help114) {
            m_helpPenaltyHintLabel->setStyleSheet(QStringLiteral(
                "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
                "font-size:14px; font-style:normal; font-weight:bold; padding:2px 8px 8px 8px;"));
            m_helpPenaltyHintLabel->setAlignment(Qt::AlignCenter);
        } else {
            m_helpPenaltyHintLabel->setStyleSheet(QStringLiteral(
                "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
                "font-size:13px; font-style:italic; font-weight:normal; padding:2px 8px 4px 8px;"));
            m_helpPenaltyHintLabel->setAlignment(Qt::AlignCenter);
        }
        m_helpPenaltyHintLabel->setVisible(showPenaltyHint);
    }
    if (m_stimHelpLabel) {
        if (help112) {
            m_stimHelpLabel->setText(QStringLiteral("При выполнении задания на I уровне"));
            m_stimHelpLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_stimHelpLabel->setStyleSheet(QStringLiteral(
                "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
                "font-size:14px; font-style:italic; font-weight:normal; padding:8px 8px 2px 8px;"));
        } else if (help114) {
            m_stimHelpLabel->setText(QStringLiteral("после 1 серии"));
            m_stimHelpLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_stimHelpLabel->setStyleSheet(QStringLiteral(
                "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
                "font-size:14px; font-style:normal; font-weight:bold; padding:8px 8px 2px 8px;"));
        } else {
            m_stimHelpLabel->setText(QStringLiteral("Стимулирующая помощь"));
            m_stimHelpLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_stimHelpLabel->setStyleSheet(QStringLiteral(
                "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
                "font-size:14px; font-weight:bold; padding:4px 0 0 16px;"));
        }
        m_stimHelpLabel->setVisible(!flatCustom);
    }
    if (m_directHelpLabel) {
        if (help112) {
            m_directHelpLabel->setText(
                QStringLiteral("При выполнении задания с ошибками (II и III уровни)"));
            m_directHelpLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_directHelpLabel->setStyleSheet(m_stimHelpLabel->styleSheet());
        } else if (help114) {
            m_directHelpLabel->setText(QStringLiteral("После 2 серии"));
            m_directHelpLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_directHelpLabel->setStyleSheet(m_stimHelpLabel->styleSheet());
        } else {
            m_directHelpLabel->setText(QStringLiteral("Направляющая помощь:"));
            m_directHelpLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_directHelpLabel->setStyleSheet(QStringLiteral(
                "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
                "font-size:14px; font-weight:bold; padding:4px 0 0 16px;"));
        }
        m_directHelpLabel->setVisible(!flatCustom);
    }
    if (m_teachHelpLabel) {
        if (help112) {
            m_teachHelpLabel->setText(
                QStringLiteral("Если предыдущий вариант помощи не возымел действия"));
            m_teachHelpLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            m_teachHelpLabel->setStyleSheet(m_stimHelpLabel->styleSheet());
            m_teachHelpLabel->setVisible(!flatCustom);
        } else if (help114) {
            m_teachHelpLabel->hide();
        } else {
            m_teachHelpLabel->setText(QStringLiteral("Обучающая помощь:"));
            m_teachHelpLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_teachHelpLabel->setStyleSheet(QStringLiteral(
                "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
                "font-size:14px; font-weight:bold; padding:4px 0 0 16px;"));
            m_teachHelpLabel->setVisible(!flatCustom);
        }
    }

    const int checkWidth = m_scrollArea && m_scrollArea->viewport()
        ? qMax(200, m_scrollArea->viewport()->width() - 40)
        : 760;

    auto insertAfter = [this](QWidget *anchor) -> int {
        if (!m_helpChecksLayout || !anchor) {
            return m_helpChecksLayout ? m_helpChecksLayout->count() : 0;
        }
        const int idx = m_helpChecksLayout->indexOf(anchor);
        return idx >= 0 ? idx + 1 : m_helpChecksLayout->count();
    };

    if (flatCustom) {
        // Пункты сразу после заголовка; для 3.1.10 — после курсивной подсказки.
        // Нельзя опираться на isVisible(): при сборке UI родители ещё скрыты.
        int insertAt = 1;
        if (showPenaltyHint && m_helpPenaltyHintLabel) {
            m_helpChecksLayout->removeWidget(m_helpPenaltyHintLabel);
            m_helpChecksLayout->insertWidget(1, m_helpPenaltyHintLabel);
            insertAt = insertAfter(m_helpPenaltyHintLabel);
        }
        for (const QString &text : labels) {
            m_helpChecks << makeCheckRow(text, m_helpChecksLayout, checkWidth);
            if (QWidget *row = m_helpChecks.last().label
                    ? m_helpChecks.last().label->parentWidget()
                    : nullptr) {
                m_helpChecksLayout->removeWidget(row);
                m_helpChecksLayout->insertWidget(insertAt++, row);
            }
        }
        return;
    }

    if (categorizedThreeHelp) {
        // 4.2.1: по одному пункту под каждым заголовком (скрин РП).
        int stimAt = insertAfter(m_stimHelpLabel);
        m_helpChecks << makeCheckRow(labels.at(0), m_helpChecksLayout, checkWidth);
        if (QWidget *row = m_helpChecks.last().label ? m_helpChecks.last().label->parentWidget() : nullptr) {
            m_helpChecksLayout->removeWidget(row);
            m_helpChecksLayout->insertWidget(stimAt, row);
        }
        int directAt = insertAfter(m_directHelpLabel);
        m_helpChecks << makeCheckRow(labels.at(1), m_helpChecksLayout, checkWidth);
        if (QWidget *row = m_helpChecks.last().label ? m_helpChecks.last().label->parentWidget() : nullptr) {
            m_helpChecksLayout->removeWidget(row);
            m_helpChecksLayout->insertWidget(directAt, row);
        }
        int teachAt = insertAfter(m_teachHelpLabel);
        m_helpChecks << makeCheckRow(labels.at(2), m_helpChecksLayout, checkWidth);
        if (QWidget *row = m_helpChecks.last().label ? m_helpChecks.last().label->parentWidget() : nullptr) {
            m_helpChecksLayout->removeWidget(row);
            m_helpChecksLayout->insertWidget(teachAt, row);
        }
        return;
    }

    // Стимулирующая / Направляющая / Обучающая.
    // 5 пунктов (стандарт): 1+2+2; 7 пунктов (1.5 и др.): 1+2+4; иначе — 1 / до 2 / остаток.
    // 1.12: 4 пункта как в or.html — 1 + 1 + 2.
    // 1.14: 8 пунктов — 4 (после 1 серии) + 4 (После 2 серии).
    int stimCount = 1;
    int directCount = 2;
    if (help114 && labels.size() >= 8) {
        stimCount = 4;
        directCount = 4;
    } else if (help112 && labels.size() == 4) {
        stimCount = 1;
        directCount = 1;
    } else if (m_exerciseId == QStringLiteral("1.20") && labels.size() >= 10) {
        // or.html: 1 стимулирующая + 5 направляющих (в т.ч. демонстрация/ракурс/трафарет) + 4 обучающих.
        stimCount = 1;
        directCount = 5;
    } else if (labels.size() == 3) {
        stimCount = 1;
        directCount = 1;
    } else if (labels.size() == 7) {
        stimCount = 1;
        directCount = 2;
    } else if (labels.size() != 5 && labels.size() > 0) {
        stimCount = 1;
        directCount = qMin(2, labels.size() - 1);
    }

    int stimAt = insertAfter(m_stimHelpLabel);
    for (int i = 0; i < stimCount && i < labels.size(); ++i) {
        m_helpChecks << makeCheckRow(labels.at(i), m_helpChecksLayout, checkWidth);
        if (QWidget *row = m_helpChecks.last().label ? m_helpChecks.last().label->parentWidget() : nullptr) {
            m_helpChecksLayout->removeWidget(row);
            m_helpChecksLayout->insertWidget(stimAt++, row);
        }
    }

    int directAt = insertAfter(m_directHelpLabel);
    for (int i = stimCount; i < stimCount + directCount && i < labels.size(); ++i) {
        m_helpChecks << makeCheckRow(labels.at(i), m_helpChecksLayout, checkWidth);
        if (QWidget *row = m_helpChecks.last().label ? m_helpChecks.last().label->parentWidget() : nullptr) {
            m_helpChecksLayout->removeWidget(row);
            m_helpChecksLayout->insertWidget(directAt++, row);
        }
    }

    int teachAt = insertAfter(m_teachHelpLabel);
    if (!help114) {
        for (int i = stimCount + directCount; i < labels.size(); ++i) {
            m_helpChecks << makeCheckRow(labels.at(i), m_helpChecksLayout, checkWidth);
            if (QWidget *row = m_helpChecks.last().label ? m_helpChecks.last().label->parentWidget() : nullptr) {
                m_helpChecksLayout->removeWidget(row);
                m_helpChecksLayout->insertWidget(teachAt++, row);
            }
        }
    }
}

void ExerciseHost::syncActivityChecksFromOrHtml() {
    clearActivityChecks();

    const QStringList labels = parseActivityLabelsFromOrHtml(m_rawOrHtml);
    QStringList effectiveLabels = labels;
    // 5.4.2 «Сказка»: оценка — выбор балла (радио idd1..idd5 в or.html без value).
    if (m_exerciseId == QStringLiteral("5.4.2")) {
        effectiveLabels = QStringList{
            QStringLiteral(
                "4 балла (высокий уровень) – ребенок отвечает на вопрос, улавливает скрытый смысл "
                "сказки и правильно интерпретирует поступки героев, адекватно определяет чувства, "
                "переживаемые ими, способен к адекватной оценке поступков героев."),
            QStringLiteral(
                "3 балла (средний уровень) – в ответах ребенка есть неточности, чаще всего ребенок "
                "интерпретирует поступки и чувства героев исходя из конкретных ситуаций, пережитых "
                "им самим, рассуждениям ребенка присущ инфантилизм."),
            QStringLiteral(
                "2 балла (уровень ниже среднего) – ребенок может уловить смысл сказки и назвать "
                "некоторые эмоциональные состояния героев сказки при наводящих вопросах и "
                "подсказках экспериментатора."),
            QStringLiteral(
                "1 балл (низкий уровень) – ребенок не может объяснить поступки героев, правильно "
                "подставляет карточки с изображением соответствующих эмоциональных состояний, но не "
                "всегда может назвать их."),
            QStringLiteral(
                "0 баллов (очень низкий уровень) – ребенок не может выполнить задание даже в "
                "условиях предъявления помощи, не идентифицирует изображения эмоциональных "
                "состояний на картинках с эмоциями героев сказки."),
        };
    }
    // 3.1.21: оценка — radio idch1..5 (value 0–4), не idd.
    if (m_exerciseId == QStringLiteral("3.1.21")) {
        effectiveLabels = QStringList{
            QStringLiteral(
                "4 балла (высокий уровень) – ребенок полностью самостоятельно выполняет задание "
                "на установление причинно-следственных связей."),
            QStringLiteral(
                "3 балла (средний уровень) – для установления причинно-следственной связи ребенку "
                "требуется первый вид помощи – указание экспериментатором на ошибку."),
            QStringLiteral(
                "2 балла (уровень ниже среднего) – ребенку требуется второй вид помощи для "
                "выполнения задания."),
            QStringLiteral(
                "1 балл (низкий уровень) – ребенок способен выполнить задание только в случае "
                "применения третьего и (или четвертого) вида помощи."),
            QStringLiteral(
                "0 баллов (очень низкий уровень) – ни один из видов помощи не привел к правильному "
                "выполнению задания ребенком."),
        };
    }
    // 1.12: полный текст с «N баллов» (в value or.html баллов часто нет — только после input).
    if (m_exerciseId == QStringLiteral("1.12")) {
        effectiveLabels = QStringList{
            QStringLiteral(
                "I уровень – ребенок не принимает задачу, совершает неадекватные действия, "
                "например, рисует карандашами вне контура или др. (0 баллов)."),
            QStringLiteral(
                "II уровень – ребенок принимает задачу, но закрашивает предметы несоответственно, "
                "цвета назвать не может (0 баллов)."),
            QStringLiteral(
                "III уровень – ребенок выполняет задание, но с ошибками, например, неправильно "
                "закрашивает 1-2 предмета, может правильно назвать некоторые цвета (2 балла)."),
            QStringLiteral(
                "IV уровень – ребенок выполняет задание без ошибок, т.е. раскрашивает предметы "
                "соответственно, может назвать все цвета (3 балла)."),
        };
    }
    // 1.26 / 1.272 / 2.8 / 2.9 / 2.10: все 5 пунктов (fallback при сбое парсинга).
    if ((m_exerciseId == QStringLiteral("1.26") || m_exerciseId == QStringLiteral("1.272")
         || m_exerciseId == QStringLiteral("2.8") || m_exerciseId == QStringLiteral("2.9")
         || m_exerciseId == QStringLiteral("2.10"))
        && effectiveLabels.size() < 5) {
        effectiveLabels = QStringList{
            QStringLiteral("Ребенок не понимает инструкцию."),
            QStringLiteral("Ребенок понимает инструкцию, но не может выполнить задание."),
            QStringLiteral("Целенаправленное выполнение задания."),
            QStringLiteral("Хаотическая деятельность ребенка."),
            QStringLiteral("Метод «проб и ошибок»."),
        };
    }
    // 3.1.10: свои уровни характера деятельности (fallback при сбое парсинга).
    if (m_exerciseId == QStringLiteral("3.1.10") && effectiveLabels.size() < 3) {
        effectiveLabels = QStringList{
            QStringLiteral(
                "I уровень – полное непонимание ребенком инструкции и стоящей перед ним задачи "
                "(0 баллов)."),
            QStringLiteral(
                "II уровень – понимание ребенком поставленной перед ним цели, однако неверный "
                "выбор основания обобщения либо внешне правильное исключение, но неадекватное "
                "обоснование объединения объектов в одну группу при поиске лишнего (0 баллов)."),
            QStringLiteral(
                "III уровень – содержательное обобщение объектов (по функциональным признакам), "
                "сопровождающееся обоснованием своего выбора для объединения объектов (2 балла)."),
        };
    }
    // 3.2.1–3.2.5 / 3.2.11 / 4.1.1 / 4.1.5: 3 пункта характера деятельности (fallback при сбое парсинга).
    if ((m_exerciseId == QStringLiteral("3.2.1")
         || m_exerciseId == QStringLiteral("3.2.2")
         || m_exerciseId == QStringLiteral("3.2.3")
         || m_exerciseId == QStringLiteral("3.2.4")
         || m_exerciseId == QStringLiteral("3.2.5")
         || m_exerciseId == QStringLiteral("3.2.11")
         || m_exerciseId == QStringLiteral("4.1.1")
         || m_exerciseId == QStringLiteral("4.1.5")
         || m_exerciseId == QStringLiteral("4.1.6")
         || m_exerciseId == QStringLiteral("5.3.1"))
        && effectiveLabels.size() < 3) {
        effectiveLabels = QStringList{
            QStringLiteral("Ребенок не понимает инструкцию."),
            QStringLiteral("Ребенок понимает инструкцию, но не может выполнить задание."),
            QStringLiteral("Целенаправленное выполнение задания."),
        };
    }
    const bool hasActivity = !effectiveLabels.isEmpty();
    if (m_activityTitle) {
        if (m_exerciseId == QStringLiteral("5.4.2") || m_exerciseId == QStringLiteral("3.1.21")) {
            m_activityTitle->setText(QStringLiteral("Баллы"));
        } else {
            m_activityTitle->setText(QStringLiteral("Характер деятельности ребенка:"));
        }
        m_activityTitle->setVisible(hasActivity);
    }
    if (m_activityChecksHost) {
        m_activityChecksHost->setVisible(hasActivity);
    }
    if (!hasActivity || !m_activityChecksLayout) {
        return;
    }

    const int checkWidth = m_scrollArea && m_scrollArea->viewport()
        ? qMax(200, m_scrollArea->viewport()->width() - 40)
        : 760;

    for (const QString &text : effectiveLabels) {
        m_activityChecks << makeCheckRow(text, m_activityChecksLayout, checkWidth);
    }

    const bool allowMulti = m_exerciseId == QStringLiteral("1.13")
        || m_exerciseId == QStringLiteral("1.17")
        || m_exerciseId == QStringLiteral("1.18")
        || m_exerciseId == QStringLiteral("1.25")
        || m_exerciseId == QStringLiteral("1.26")
        || m_exerciseId == QStringLiteral("1.272")
        || m_exerciseId == QStringLiteral("2.8")
        || m_exerciseId == QStringLiteral("2.9")
        || m_exerciseId == QStringLiteral("2.10");
    for (const ExerciseCheckRow &row : m_activityChecks) {
        if (!row.box) {
            continue;
        }
        connect(row.box, &QCheckBox::toggled, this, [this, row, allowMulti](bool checked) {
            if (!checked || allowMulti) {
                return;
            }
            for (const ExerciseCheckRow &other : m_activityChecks) {
                if (other.box && other.box != row.box) {
                    other.box->setChecked(false);
                }
            }
        });
    }
}

void ExerciseHost::loadExercise() {
    m_evaluationPanel->show();
    m_attemptCount = 0;

    m_rawOrHtml = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("or.html"));
    reloadOrBrowser();
    syncActivityChecksFromOrHtml();
    syncHelpChecksFromOrHtml();

    const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
    // При входе форма протокола всегда чистая (прошлые прохождения — на странице «Протоколы»).
    m_currentProtocolId.clear();
    m_protocolSavedThisSession = false;
    m_cursorInBallsColumn = false;
    m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
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

    if (m_exerciseId == QStringLiteral("4.2.2")) {
        ensureWords422Panel();
        m_words422Editable = false;
        updateWords422Panel(QString());
        setWords422TableEditable(m_words422Table, false);
        layoutWords422Panel();
    } else if (m_words422Panel) {
        m_words422Panel->hide();
    }

    if (m_exerciseId == QStringLiteral("5.1.1")) {
        ensureWords511Panel();
        setWords511TableEditable(false);
        layoutWords511Panel();
    } else if (m_words511Panel) {
        m_words511Panel->hide();
    }

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

    applyPuzzleOptionsDefaults();
    updateExerciseOptionsPanel();
    updateChromeLayout();
    layoutContent();
    QTimer::singleShot(0, this, [this]() { updateContentHeights(); });
    QTimer::singleShot(50, this, [this]() { updateContentHeights(); });
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
            doc->setDocumentMargin(0);
            doc->setTextWidth(kTemplateTableWidth);
            ExerciseProtocol::forceProtocolDocumentTableWidths(doc, kTemplateTableWidth);
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
    if (m_activityChecksHost) {
        m_activityChecksHost->setMinimumWidth(textWidth);
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
        const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
        const bool hideDuringRun = m_exerciseRunning
            && (m_dualScreen
                || (definition
                    && (definition->runner == ExerciseRunnerKind::Puzzles
                        || definition->runner == ExerciseRunnerKind::OnlyDemo
                        || definition->runner == ExerciseRunnerKind::Remember)));
        m_previewImage->setVisible(
            visible && !m_previewSource.isNull() && !hideDuringRun);
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

void ExerciseHost::reparentOverlayWidget(QWidget *overlayWidget) {
    if (!overlayWidget) {
        return;
    }
    overlayWidget->hide();
    overlayWidget->setWindowFlags(Qt::Widget);
    overlayWidget->setParent(this);
    overlayWidget->setGeometry(0, 0, width(), height());
}

void ExerciseHost::destroySessionRunner() {
    if (!m_sessionRunner) {
        m_sessionRunnerKind = ExerciseRunnerKind::NotImplemented;
        return;
    }
    ExerciseRunnerWidget *runner = m_sessionRunner;
    m_sessionRunner = nullptr;
    m_sessionRunnerKind = ExerciseRunnerKind::NotImplemented;
    runner->hide();
    runner->setWindowFlags(Qt::Widget);
    if (runner->parentWidget() && runner->parentWidget() != this) {
        runner->setParent(this);
    }
    // deleteLater: безопасно из слота sessionFinished самого runner'а.
    runner->deleteLater();
}

void ExerciseHost::ensureDigitsPreviewRunner() {
    if (m_exerciseId != QStringLiteral("4.2.1") || !m_rightPanel || m_exerciseRunning) {
        return;
    }
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    if (!definition || definition->runner != ExerciseRunnerKind::Digits) {
        return;
    }
    if (!m_sessionRunner || m_sessionRunnerKind != ExerciseRunnerKind::Digits) {
        destroySessionRunner();
        m_sessionRunner = createExerciseRunner(ExerciseRunnerKind::Digits, m_rightPanel);
        m_sessionRunnerKind = ExerciseRunnerKind::Digits;
        connectSessionRunnerFinished();
    }
    if (m_sessionRunner->parentWidget() != m_rightPanel) {
        m_sessionRunner->setParent(m_rightPanel);
    }
    m_sessionRunner->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
    m_sessionRunner->setSessionOptions(buildSessionOptions());
    m_sessionRunner->prepareStaticPreview(m_exerciseId);
    if (m_beginButton) {
        m_beginButton->raise();
    }
}

void ExerciseHost::connectSessionRunnerFinished() {
    if (!m_sessionRunner) {
        return;
    }
    connect(m_sessionRunner, &ExerciseRunnerWidget::sessionFinished, this,
        &ExerciseHost::handleSessionRunnerFinished, Qt::UniqueConnection);
}

void ExerciseHost::handleSessionRunnerFinished(const ExerciseSessionResult &result) {
    m_answers = result.answers;
    m_elapsedSeconds = result.elapsedSeconds;
    const QString step = currentStepId();
    if (!step.isEmpty()) {
        m_stepElapsedSeconds.insert(step, result.elapsedSeconds);
    }
    m_sessionAdditional = result.additional;
    if (m_exerciseId == QStringLiteral("1.5") || m_exerciseId == QStringLiteral("1.6")) {
        // E15Canvas::doneState → «выполнено»/«не выполнено» (как additional True/False в оригинале).
        if (!result.doneState.trimmed().isEmpty()) {
            const bool ok =
                result.doneState.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
            m_sessionAdditional = ok ? QStringLiteral("выполнено") : QStringLiteral("не выполнено");
        }
    }
    if (m_exerciseId == QStringLiteral("4.2.2")) {
        m_words422Editable = true;
        updateWords422Panel(result.additional);
        setWords422TableEditable(m_words422Table, true);
    }
    if (m_exerciseId == QStringLiteral("5.2.1")) {
        const QString stepKey = currentStepId().trimmed().isEmpty()
            ? QStringLiteral("1")
            : currentStepId().trimmed();
        QString payload = result.additional.trimmed();
        if (!payload.startsWith(stepKey + QLatin1Char(';'))) {
            payload = stepKey + QLatin1Char(';') + payload;
        }
        m_additionalByStep.insert(stepKey, payload);
        m_sessionAdditional = payload;
        if (m_sessionRunner) {
            const QMap<QString, QString> byStep = m_sessionRunner->stepAdditionalMap();
            for (auto it = byStep.constBegin(); it != byStep.constEnd(); ++it) {
                const QString sid = it.key().trimmed();
                if (sid.isEmpty()) {
                    continue;
                }
                QString val = it.value();
                if (!val.startsWith(sid + QLatin1Char(';'))) {
                    val = sid + QLatin1Char(';') + val;
                }
                m_additionalByStep.insert(sid, val);
            }
        }
    }
    if (m_exerciseId == QStringLiteral("1.26")) {
        const QStringList parts = result.additional.split(QLatin1Char(';'));
        const QString stepKey = parts.isEmpty() || parts.at(0).trimmed().isEmpty()
            ? QStringLiteral("1")
            : parts.at(0).trimmed();
        m_additionalByStep.insert(stepKey, result.additional);
    } else if (m_exerciseId == QStringLiteral("1.272")) {
        const QString stepKey = currentStepId().trimmed().isEmpty()
            ? QStringLiteral("1")
            : currentStepId().trimmed();
        m_additionalByStep.insert(stepKey, stepKey);
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
    // 1.21 «2А»: обучающее задание — протокол не нужен, сразу можно запускать следующие.
    if (m_exerciseId == QStringLiteral("1.21")
        && currentStepId().trimmed() == QStringLiteral("2А")) {
        m_protocolFormed = true;
        m_exerciseDone = false;
        m_elapsedSeconds = 0;
        m_stepElapsedSeconds.clear();
    }
    updateExerciseOptionsPanel();
    if (m_patientDisplay) {
        m_patientDisplay->hideDisplay();
    }
    restoreExerciseOverlay();
    clearRootExerciseOverlays();
    setExerciseChromeVisible(true);
    raise();
    if (m_exerciseId == QStringLiteral("4.2.1") && m_sessionRunner
        && m_sessionRunnerKind == ExerciseRunnerKind::Digits) {
        if (m_sessionRunner->parentWidget() != m_rightPanel && m_rightPanel) {
            m_sessionRunner->setParent(m_rightPanel);
        }
        if (m_rightPanel) {
            m_sessionRunner->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        }
        m_sessionRunner->prepareStaticPreview(m_exerciseId);
        if (m_beginButton) {
            m_beginButton->raise();
        }
    } else {
        destroySessionRunner();
    }
    updateChromeLayout();
    showResultLabels(result.answers, result.elapsedSeconds);
    emit exerciseOverlayChanged(false);
}

void ExerciseHost::clearRootExerciseOverlays() {
    QWidget *overlayRoot = parentWidget();
    if (!overlayRoot) {
        return;
    }
    const QObjectList children = overlayRoot->children();
    for (QObject *child : children) {
        auto *widget = qobject_cast<QWidget *>(child);
        if (!widget || widget == this) {
            continue;
        }
        if (widget->objectName() != QLatin1String("dokitExerciseOverlay")) {
            continue;
        }
        widget->hide();
        if (widget == m_sessionRunner || widget == m_onlyP) {
            widget->setParent(this);
            continue;
        }
        // Потерянный оверлей после смены методики / аварийного выхода.
        widget->setParent(nullptr);
        widget->deleteLater();
    }
}

void ExerciseHost::shutdownSessionUi() {
    m_exerciseRunning = false;
    if (m_patientDisplay) {
        m_patientDisplay->hideDisplay();
    }
    // Не вызываем stopSession(): из openExercise/close это повторно шлёт sessionFinished
    // и портит состояние уже выбранной новой методики.
    if (m_sessionRunner) {
        m_sessionRunner->hide();
    }
    if (m_onlyP) {
        m_onlyP->hide();
    }
    clearRootExerciseOverlays();
    resetExerciseOverlays();
    destroySessionRunner();
    if (m_onlyP) {
        m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Primary);
        reparentOverlayWidget(m_onlyP);
        m_onlyP->hide();
    }
    if (m_specialistExercise) {
        m_specialistExercise->hide();
    }
    setExerciseChromeVisible(true);
    raise();
    emit exerciseOverlayChanged(false);
}

void ExerciseHost::presentOverlayWidget(QWidget *overlayWidget) {
    if (!overlayWidget) {
        return;
    }
    m_exerciseRunning = true;
    overlayWidget->setObjectName(QStringLiteral("dokitExerciseOverlay"));

    // Оверлей внутри главного окна (не отдельный fullscreen —
    // иначе на том же мониторе остаётся пустое окно Infant).
    QWidget *overlayRoot = parentWidget();
    if (!overlayRoot) {
        overlayRoot = this;
    }
    overlayWidget->hide();
    overlayWidget->setWindowFlags(Qt::Widget);
    if (overlayWidget->parentWidget() != overlayRoot) {
        overlayWidget->setParent(overlayRoot);
    }
    overlayWidget->setGeometry(0, 0, overlayRoot->width(), overlayRoot->height());
    overlayWidget->show();
    overlayWidget->raise();
}

void ExerciseHost::updateExerciseOverlayGeometry() {
    QWidget *overlayWidget = nullptr;
    if (m_sessionRunner && m_exerciseRunning) {
        overlayWidget = m_sessionRunner;
    } else if (m_onlyP && m_exerciseRunning) {
        overlayWidget = m_onlyP;
    }
    if (!overlayWidget) {
        return;
    }
    // 2.8 / 4.1.4 dual: runner на правой панели. 4.2.1 — всегда на правой панели.
    if (m_sessionRunner && m_exerciseRunning
        && m_sessionRunnerKind == ExerciseRunnerKind::Digits
        && m_rightPanel
        && overlayWidget->parentWidget() == m_rightPanel) {
        overlayWidget->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        return;
    }
    if (m_dualScreen && m_sessionRunner && m_exerciseRunning
        && (m_sessionRunnerKind == ExerciseRunnerKind::E28
            || m_sessionRunnerKind == ExerciseRunnerKind::Remember2)
        && m_rightPanel
        && overlayWidget->parentWidget() == m_rightPanel) {
        overlayWidget->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        return;
    }
    QWidget *overlayRoot = overlayWidget->parentWidget();
    if (!overlayRoot) {
        return;
    }
    // Dual: оверлей на корне; один экран — тоже на корне (без отдельного Window).
    if (!m_dualScreen || overlayRoot == parentWidget() || overlayRoot == this) {
        overlayWidget->setGeometry(0, 0, overlayRoot->width(), overlayRoot->height());
    }
}

void ExerciseHost::showExerciseOverlay() {
    QWidget *overlayWidget = nullptr;
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    // OnlyPicture никогда не должен поднимать «залипший» session-runner (напр. после 2.8).
    if (definition && definition->runner == ExerciseRunnerKind::OnlyPicture && m_onlyP) {
        overlayWidget = m_onlyP;
    } else if (m_sessionRunner && (m_exerciseRunning || m_sessionRunner->isVisible())) {
        overlayWidget = m_sessionRunner;
    } else if (m_onlyP) {
        overlayWidget = m_onlyP;
    }
    if (!overlayWidget) {
        return;
    }
    presentOverlayWidget(overlayWidget);
    lower();
    emit exerciseOverlayChanged(true);
}

void ExerciseHost::restoreExerciseOverlay() {
    m_exerciseRunning = false;
    resetExerciseOverlays();
    raise();
    layoutStepCombo();
    emit exerciseOverlayChanged(false);
}

void ExerciseHost::resetExerciseOverlays() {
    if (m_patientDisplay) {
        m_patientDisplay->hideDisplay();
    }
    if (m_specialistExercise) {
        m_specialistExercise->hide();
        if (m_rightPanel) {
            if (m_specialistExercise->parentWidget() != m_rightPanel) {
                m_specialistExercise->setParent(m_rightPanel);
            }
            m_specialistExercise->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        }
    }
    if (m_sessionRunner) {
        reparentOverlayWidget(m_sessionRunner);
        m_sessionRunner->hide();
    }
    if (m_stepCombo && m_stepCombo->parentWidget() != this) {
        m_stepCombo->setParent(this);
    }
    if (m_onlyP) {
        m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Primary);
        reparentOverlayWidget(m_onlyP);
        m_onlyP->hide();
    }
}

void ExerciseHost::setDualScreenEnabled(bool enabled) {
    const bool wasDual = m_dualScreen;
    m_dualScreen = enabled;

    if (!m_exerciseRunning) {
        if (!enabled && m_patientDisplay) {
            m_patientDisplay->hideDisplay();
        }
        return;
    }

    // Обычные сессии: только зеркало на втором мониторе.
    if (m_sessionRunner && m_sessionRunner->isVisible()) {
        if (enabled) {
            syncPatientDisplay();
        } else if (m_patientDisplay) {
            m_patientDisplay->hideDisplay();
        }
        return;
    }

    if (!m_onlyP) {
        if (enabled) {
            syncPatientDisplay();
        } else if (m_patientDisplay) {
            m_patientDisplay->hideDisplay();
        }
        return;
    }

    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    const OnlyPictureSettings settings =
        definition ? definition->onlyPicture : OnlyPictureSettings();
    const QString stepId = m_sessionStepId.trimmed().isEmpty()
        ? currentStepId()
        : m_sessionStepId;
    const int pictureIndex = qMax(1, m_onlyP->picturesShown());

    if (enabled && !wasDual) {
        if (m_previewImage) {
            m_previewImage->hide();
        }
        reparentOverlayWidget(m_onlyP);
        m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Headless);

        setExerciseChromeVisible(true);
        raise();

        if (m_patientDisplay) {
            m_patientDisplay->attachExercise(m_onlyP);
        }
        if (m_specialistExercise && m_rightPanel) {
            m_specialistExercise->setDisplayRole(OnlyPExercise::DisplayRole::Specialist);
            m_specialistExercise->setMirrorMode(true);
            m_specialistExercise->prepareMirrorUi(m_exerciseId);
            m_specialistExercise->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
            m_specialistExercise->show();
            m_specialistExercise->raise();
            m_specialistExercise->syncMirrorSession(m_exerciseId, settings, stepId);
            if (m_exerciseId != QStringLiteral("3.2.3")) {
                m_specialistExercise->showPicture(pictureIndex);
            }
        }
        syncPatientDisplay();
        updateChromeLayout();
        layoutStepCombo();
        emit exerciseOverlayChanged(false);
        return;
    }

    if (!enabled && wasDual) {
        if (m_patientDisplay) {
            m_patientDisplay->hideDisplay();
        }
        if (m_specialistExercise) {
            m_specialistExercise->hide();
        }
        m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Primary);
        setExerciseChromeVisible(false);
        showExerciseOverlay();
        m_onlyP->show();
        m_onlyP->raise();
        layoutStepCombo();
        return;
    }

    if (enabled) {
        syncPatientDisplay();
    } else if (m_patientDisplay) {
        m_patientDisplay->hideDisplay();
    }
}

void ExerciseHost::syncPatientDisplay() {
    if (!m_patientDisplay) {
        return;
    }
    // 4.2.1 / 4.2.2 / 5.1.1: без второго экрана даже в dual-режиме.
    if (m_exerciseId == QStringLiteral("4.2.1")
        || m_exerciseId == QStringLiteral("4.2.2")
        || m_exerciseId == QStringLiteral("5.1.1")) {
        m_patientDisplay->hideDisplay();
        return;
    }
    if (!m_dualScreen || !m_exerciseRunning) {
        m_patientDisplay->hideDisplay();
        return;
    }
    // OnlyPicture dual: Headless m_onlyP скрыт — нельзя требовать isVisible().
    if (m_onlyP
        && m_onlyP->displayRole() == OnlyPExercise::DisplayRole::Headless) {
        m_patientDisplay->attachExercise(m_onlyP);
        m_patientDisplay->showOnSecondaryScreen();
        return;
    }
    if (m_onlyP && m_onlyP->isVisible()) {
        m_patientDisplay->attachExercise(m_onlyP);
        m_patientDisplay->showOnSecondaryScreen();
        return;
    }
    if (m_sessionRunner && m_sessionRunner->isVisible()) {
        m_sessionRunner->bindPatientDisplay(m_patientDisplay);
        m_patientDisplay->showOnSecondaryScreen();
        // После show геометрия 2-го экрана уже известна — иначе paint/puzzle
        // первый раз рисует холст в мелком размере родителя 0×0.
        m_sessionRunner->bindPatientDisplay(m_patientDisplay);
        QTimer::singleShot(0, this, [this]() {
            if (m_dualScreen && m_exerciseRunning && m_sessionRunner && m_patientDisplay) {
                m_sessionRunner->bindPatientDisplay(m_patientDisplay);
            }
        });
    }
}

void ExerciseHost::runExerciseSession() {
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    if (!definition) {
        return;
    }
    // Для numbered OnlyPicture (1.17/1.18) уважаем выбранное в селекте задание.
    m_sessionStepId = currentStepId();
    if (definition->runner == ExerciseRunnerKind::OnlyPicture) {
        runOnlyPExercise();
        return;
    }
    // 1.11 задание 1 — одна статичная картинка по центру (как 1.1); задание 2 — пазл.
    if (m_exerciseId == QStringLiteral("1.11")
        && currentStepId().trimmed() == QStringLiteral("1")) {
        runOnlyPExercise();
        return;
    }

    m_protocolFormed = false;
    m_protocolSavedThisSession = false;
    m_stepElapsedSeconds.clear();
    if (m_exerciseId == QStringLiteral("4.1.7")) {
        ++m_attemptCount;
    }
    // Закрыть всплывающие «Настройки» перед стартом (1.5 / 1.22 / 1.14).
    m_shardPanelVisible = false;
    if (m_e15ModeGroup) {
        m_e15ModeGroup->hide();
    }
    if (m_puzzleOptionsGroup) {
        m_puzzleOptionsGroup->hide();
    }
    if (m_remPanel) {
        m_remPanel->hide();
    }
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();
    if (m_exerciseId == QStringLiteral("4.2.2")) {
        m_words422Editable = false;
        setWords422TableEditable(m_words422Table, false);
    }
    if (m_exerciseId == QStringLiteral("5.1.1")) {
        setWords511TableEditable(false);
    }
    m_exerciseRunning = true;
    if (m_beginButton) {
        m_beginButton->hide();
    }
    emit exerciseOverlayChanged(true);

    if (!m_sessionRunner || m_sessionRunnerKind != definition->runner) {
        destroySessionRunner();
        m_sessionRunner = createExerciseRunner(definition->runner, this);
        m_sessionRunnerKind = definition->runner;
        connectSessionRunnerFinished();
    } else if (m_sessionRunner->parent() != this || m_sessionRunner->isWindow()) {
        // Digits уже на правой панели из превью — не переносим на корень.
        if (!(definition->runner == ExerciseRunnerKind::Digits
              && m_sessionRunner->parentWidget() == m_rightPanel)) {
            reparentOverlayWidget(m_sessionRunner);
        }
    }

    setExerciseChromeVisible(false);
    m_sessionStepId = currentStepId();
    m_dualScreen = AppSettings::dualScreenEnabled();
    if (m_patientDisplay && !m_dualScreen) {
        m_patientDisplay->hideDisplay();
    }
    if (m_onlyP) {
        m_onlyP->hide();
    }
    if (m_specialistExercise) {
        m_specialistExercise->hide();
    }

    // 2.8 / 4.1.4 dual и 4.2.1 всегда: задание на правой панели (виджет, не f1.png).
    const bool digitsOnRight =
        definition->runner == ExerciseRunnerKind::Digits && m_rightPanel;
    const bool dualRunnerOnRight =
        (definition->runner == ExerciseRunnerKind::E28
         || definition->runner == ExerciseRunnerKind::Remember2)
        && m_dualScreen && m_rightPanel;
    if (digitsOnRight || dualRunnerOnRight) {
        if (m_previewImage) {
            m_previewImage->hide();
        }
        setExerciseChromeVisible(true);
        raise();
        if (m_sessionRunner->parentWidget() != m_rightPanel) {
            m_sessionRunner->setParent(m_rightPanel);
        }
        m_sessionRunner->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
        m_sessionRunner->setSessionOptions(buildSessionOptions());
        m_sessionRunner->startSession(m_exerciseId, *definition, m_sessionStepId);
        m_sessionRunner->show();
        m_sessionRunner->raise();
        emit exerciseOverlayChanged(false);
        layoutStepCombo();
        QTimer::singleShot(0, this, [this]() { layoutStepCombo(); });
        syncPatientDisplay();
        updateChromeLayout();
        return;
    }

    presentOverlayWidget(m_sessionRunner);
    lower();
    emit exerciseOverlayChanged(true);
    m_sessionRunner->setSessionOptions(buildSessionOptions());
    m_sessionRunner->startSession(m_exerciseId, *definition, m_sessionStepId);
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
    // После 1.26/1.272/2.8 session-runner мог остаться на корне окна — убрать до OnlyP.
    destroySessionRunner();
    clearRootExerciseOverlays();
    emit exerciseOverlayChanged(true);

    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    OnlyPictureSettings settings =
        definition ? definition->onlyPicture : OnlyPictureSettings();
    // 1.11/1: f1.png, без листания и кнопок верно/неверно.
    if (m_exerciseId == QStringLiteral("1.11")) {
        settings = OnlyPictureSettings();
        settings.pictureCount = 1;
        settings.imagePattern = QStringLiteral("f1.png");
    }
    m_sessionStepId = currentStepId();
    if (m_sessionStepId.trimmed().isEmpty()
        && m_stepCombo && m_stepCombo->count() > 0) {
        m_sessionStepId = m_stepCombo->currentText().trimmed();
    }
    if (m_sessionStepId.trimmed().isEmpty()) {
        m_sessionStepId = QStringLiteral("1");
    }
    const QString stepId = m_sessionStepId;

    if (m_dualScreen) {
        reparentOverlayWidget(m_onlyP);
        if (m_previewImage) {
            m_previewImage->hide();
        }
        m_onlyP->setDisplayRole(OnlyPExercise::DisplayRole::Headless);

        // Сначала подключаем зеркала, затем start — чтобы не потерять pictureChanged.
        if (m_patientDisplay) {
            m_patientDisplay->attachExercise(m_onlyP);
        }
        if (m_specialistExercise) {
            m_specialistExercise->setDisplayRole(OnlyPExercise::DisplayRole::Specialist);
            m_specialistExercise->setMirrorMode(true);
            m_specialistExercise->prepareMirrorUi(m_exerciseId);
            // 4.1.1: текст слева и картинка справа на всём экране специалиста.
            if (m_exerciseId == QStringLiteral("4.1.1")) {
                setExerciseChromeVisible(false);
                m_specialistExercise->setParent(this);
                m_specialistExercise->setGeometry(0, 0, width(), height());
            } else if (m_rightPanel) {
                if (m_specialistExercise->parentWidget() != m_rightPanel) {
                    m_specialistExercise->setParent(m_rightPanel);
                }
                m_specialistExercise->setGeometry(0, 0, m_rightPanel->width(), m_rightPanel->height());
            }
            m_specialistExercise->show();
            m_specialistExercise->raise();
        }

        m_onlyP->start(m_exerciseId, settings, stepId);

        if (m_specialistExercise) {
            m_specialistExercise->syncMirrorSession(m_exerciseId, settings, stepId);
            if (m_exerciseId != QStringLiteral("3.2.3")
                && m_exerciseId != QStringLiteral("4.1.1")) {
                m_specialistExercise->showPicture(1);
            }
        }
        syncPatientDisplay();
        updateChromeLayout();
        layoutStepCombo();
        return;
    }

    if (m_specialistExercise) {
        m_specialistExercise->hide();
    }
    if (m_patientDisplay) {
        m_patientDisplay->hideDisplay();
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

    // Таймер результата — не для методик без времени в протоколе.
    const bool hideResultTimer = m_exerciseId == QStringLiteral("1.26")
        || m_exerciseId == QStringLiteral("1.272")
        || m_exerciseId == QStringLiteral("3.1.10");
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
    // 1.12 / 1.14 / 1.15 / 1.20 / 1.21 / 1.27 / 2.1 / 2.2 / 2.3 / 2.11 / 2.12 / 3.3.x: нет блока «Выполнение».
    if (m_exerciseId == QStringLiteral("1.12")
        || m_exerciseId == QStringLiteral("1.14")
        || m_exerciseId == QStringLiteral("1.15")
        || m_exerciseId == QStringLiteral("1.20")
        || m_exerciseId == QStringLiteral("1.21")
        || m_exerciseId == QStringLiteral("1.27")
        || m_exerciseId == QStringLiteral("2.1")
        || m_exerciseId == QStringLiteral("2.2")
        || m_exerciseId == QStringLiteral("2.3")
        || m_exerciseId == QStringLiteral("2.11")
        || m_exerciseId == QStringLiteral("2.12")
        || m_exerciseId == QStringLiteral("3.3.1")
        || m_exerciseId == QStringLiteral("3.3.2")
        || m_exerciseId == QStringLiteral("3.3.3")) {
        return false;
    }
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

    // 1.5 / 1.6: факт выполнения с канваса E15, не с чекбоксов «выполнено».
    if ((m_exerciseId == QStringLiteral("1.5") || m_exerciseId == QStringLiteral("1.6"))
        && (m_sessionAdditional == QStringLiteral("выполнено")
            || m_sessionAdditional == QStringLiteral("не выполнено"))) {
        session.doneState = m_sessionAdditional;
        if (m_exerciseId == QStringLiteral("1.6")) {
            session.additional = m_sessionAdditional;
        }
    }

    // Раннер уже положил данные (ответы, цифры, сказка и т.п.) — не затираем.
    const bool keepRunnerAdditional = !m_sessionAdditional.isEmpty()
        && (m_exerciseId == QStringLiteral("1.26")
            || m_exerciseId == QStringLiteral("3.1.21")
            || m_exerciseId == QStringLiteral("4.1.8") || m_exerciseId == QStringLiteral("4.2.1")
            || m_exerciseId == QStringLiteral("4.2.2") || m_exerciseId == QStringLiteral("5.1.1")
            || m_exerciseId == QStringLiteral("5.2.1") || m_exerciseId == QStringLiteral("5.4.2"));

    if (m_exerciseId == QStringLiteral("1.272")) {
        // Оригинал createP("1.272"): additional = только param1.Text (№ задания).
        // Каждое задание формируется отдельно — в form только текущий №.
        const QString step = session.stepId.trimmed().isEmpty()
            ? QStringLiteral("1")
            : session.stepId.trimmed();
        session.stepId = step;
        session.additional = step;
        session.stepIds << step;
    } else if (keepRunnerAdditional) {
        session.additional = m_sessionAdditional;
        // 5.2.1: оригинал createP получает param1 + ";" + data1..data10; таблица на каждое №.
        if (m_exerciseId == QStringLiteral("5.2.1")) {
            const QString step = session.stepId.trimmed().isEmpty()
                ? QStringLiteral("1")
                : session.stepId.trimmed();
            session.stepId = step;
            if (!session.additional.startsWith(step + QLatin1Char(';'))) {
                session.additional = step + QLatin1Char(';') + session.additional;
            }
            // Как упр. 6: каждое формирование — только текущее выбранное задание.
            session.stepIds.clear();
            session.stepIds << step;
            session.additionalByStep.clear();
            session.additionalByStep.insert(step, session.additional);
        } else if (m_exerciseId == QStringLiteral("3.1.21")) {
            const QString step = session.stepId.trimmed().isEmpty()
                ? QStringLiteral("1")
                : session.stepId.trimmed();
            session.stepId = step;
            session.stepIds << step;
            if (!session.additional.startsWith(step + QLatin1Char(';'))) {
                session.additional = step + QLatin1Char(';') + session.additional;
            }
        }
    } else if (m_exerciseId == QStringLiteral("1.26")) {
        // Как в оригинале: param1 + ";" + answers[0..12].join(";")
        const QString step = session.stepId.trimmed().isEmpty() ? QStringLiteral("1") : session.stepId;
        QStringList emptyAnswers;
        for (int i = 0; i < 13; ++i) {
            emptyAnswers << QString();
        }
        session.additional = step + QLatin1Char(';') + emptyAnswers.join(QLatin1Char(';'));
    } else if (definition && definition->protocol == ExerciseProtocolKind::NumberedDoneTime) {
        if (m_exerciseId == QStringLiteral("4.1.7")) {
            // Оригинал: additional = ecount + ";" + doneState.
            const QString attempt = QString::number(qMax(1, m_attemptCount));
            session.stepId = attempt;
            session.stepIds << attempt;
            session.stepElapsedSeconds.insert(attempt, m_elapsedSeconds);
            session.additional = attempt + QLatin1Char(';') + session.doneState;
        } else if (ExerciseConfig::usesAppendOnlyMultiStepLogic(m_exerciseId)) {
            // Как упр. 6 / 1.272: каждое задание формируется отдельно — в form только текущий №.
            const QString step = session.stepId.trimmed().isEmpty()
                ? QStringLiteral("1")
                : session.stepId.trimmed();
            session.stepId = step;
            session.stepIds << step;
            session.stepElapsedSeconds.insert(step, m_elapsedSeconds);
            session.additional = step + QLatin1Char(';') + session.doneState;
        } else {
            // Как в оригинале: в протокол попадают задания, которые реально запускали
            // (время в stepElapsedSeconds).
            session.stepElapsedSeconds = m_stepElapsedSeconds;
            if (session.stepId.isEmpty()) {
                session.stepId = QStringLiteral("1");
            }
            if (session.stepElapsedSeconds.isEmpty() && !session.stepId.isEmpty()) {
                session.stepElapsedSeconds.insert(session.stepId, m_elapsedSeconds);
            }
            if (!session.stepElapsedSeconds.isEmpty()) {
                const QStringList order = numberedStepIds();
                for (const QString &sid : order) {
                    if (session.stepElapsedSeconds.contains(sid)) {
                        session.stepIds << sid;
                    }
                }
                if (session.stepIds.isEmpty()) {
                    session.stepIds = session.stepElapsedSeconds.keys();
                }
            }
            if (session.stepIds.isEmpty()) {
                session.stepIds << session.stepId;
            }
            session.additional = session.stepId + QLatin1Char(';') + session.doneState;
        }
    } else if (definition && definition->protocol == ExerciseProtocolKind::DoneTimeOrHlp) {
        if (m_exerciseId == QStringLiteral("1.6")
            && (session.additional == QStringLiteral("выполнено")
                || session.additional == QStringLiteral("не выполнено"))) {
            // Уже задано из E15Canvas — не затирать чекбоксами.
        } else if (ExerciseConfig::usesAppendOnlyMultiStepLogic(m_exerciseId)) {
            const QString step = session.stepId.trimmed().isEmpty()
                ? QStringLiteral("1")
                : session.stepId.trimmed();
            session.stepId = step;
            session.stepIds << step;
            session.stepElapsedSeconds.insert(step, m_elapsedSeconds);
            session.additional = step + QLatin1Char(';') + session.doneState;
        } else {
            session.additional = session.doneState;
        }
    } else if (definition && definition->protocol == ExerciseProtocolKind::OrHlpBallsRow) {
        // 3.1.10 и др.: номер картинки/задания; при логике упр. 6 — только текущий №.
        const QString step = session.stepId.trimmed().isEmpty()
            ? QStringLiteral("1")
            : session.stepId.trimmed();
        session.stepId = step;
        session.additional = step;
        if (ExerciseConfig::usesAppendOnlyMultiStepLogic(m_exerciseId)) {
            session.stepIds << step;
        }
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
    // повторная сессия дописывается через appendFullSessionToStoredBody (с «Дата/специалист»).
    if (m_protocolSavedThisSession && !m_currentProtocolId.isEmpty()) {
        saveProtocolEdits();
    }
    const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
    m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
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

void ExerciseHost::showLastProtocolInTemplate() {
    if (!m_templateBrowser) {
        return;
    }
    const QString rawTemplate = loadExerciseHtmlFile(m_exerciseId, QStringLiteral("template.html"));
    const QString baseDir = ExerciseAssets::exerciseDir(m_exerciseId);
    if (m_repository && !m_patientId.trimmed().isEmpty()) {
        const QString lastBody =
            m_repository->loadLastExerciseProtocolBody(m_patientId, m_exerciseId);
        const QString lastId =
            m_repository->loadLastExerciseProtocolId(m_patientId, m_exerciseId);
        if (!lastBody.trimmed().isEmpty() && !lastId.isEmpty()) {
            m_currentProtocolId = lastId;
            m_partly = true;
            const QString viewHtml = m_repository->loadProtocolViewHtml(
                m_exerciseId, lastId, m_patientFio, m_patientBirthDate);
            if (!viewHtml.trimmed().isEmpty()) {
                m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
                finalizeProtocolTemplateDocument(m_templateBrowser->document());
                const int templateViewportWidth = kTemplateTableWidth + kTemplateViewportPadding;
                m_templateBrowser->setFixedWidth(templateViewportWidth);
                if (m_templatePanel) {
                    m_templatePanel->setMaximumWidth(templateViewportWidth + 16);
                }
                updateProtocolEditMode();
                layoutContent();
                QTimer::singleShot(80, this, [this]() { updateContentHeights(); });
                return;
            }
        }
    }
    m_currentProtocolId.clear();
    m_templateBrowser->setHtml(ExerciseAssets::prepareTemplateHtml(rawTemplate, baseDir));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    updateProtocolEditMode();
}

void ExerciseHost::updateProtocolEditMode() {
    if (!m_templateBrowser) {
        return;
    }
    // Блокировать «Результат» только где он заполняется программой (не вручную).
    const bool lockResult = m_exerciseId == QStringLiteral("1.1")
        || m_exerciseId == QStringLiteral("1.4")
        || m_exerciseId == QStringLiteral("1.8")
        || m_exerciseId == QStringLiteral("4.1.2");
    m_templateBrowser->setProperty("protocolLockResultEdit", lockResult);
    // 4.1.2: запрет курсора в колонке «Баллы» (заполняется по времени).
    m_templateBrowser->setProperty(
        "protocolLockBallsEdit",
        m_exerciseId == QStringLiteral("4.1.2"));
    // 1.26: курсор только в ответ/баллы/характер/помощь/результат.
    m_templateBrowser->setProperty(
        "protocolStrict126Edit",
        m_exerciseId == QStringLiteral("1.26"));
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
    const bool show = m_protocolSavedThisSession
        && (m_exerciseId == QStringLiteral("1.26") || m_exerciseId == QStringLiteral("1.272")
            || m_exerciseId == QStringLiteral("3.1.10")
            || m_exerciseId == QStringLiteral("4.1.8")
            || m_exerciseId == QStringLiteral("1.14")
            || m_exerciseId == QStringLiteral("1.15")
            || m_exerciseId == QStringLiteral("1.20")
            || m_exerciseId == QStringLiteral("1.21")
            || m_exerciseId == QStringLiteral("1.27")
            || m_exerciseId == QStringLiteral("2.11")
            || m_exerciseId == QStringLiteral("2.12")
            || m_exerciseId == QStringLiteral("3.1.8")
            || m_exerciseId == QStringLiteral("3.1.21"));
    m_sumButton->setVisible(show);
}

bool ExerciseHost::usesLastProtocolSessionView() const {
    return m_exerciseId == QStringLiteral("1.26") || m_exerciseId == QStringLiteral("1.272")
        || m_exerciseId == QStringLiteral("2.8") || m_exerciseId == QStringLiteral("2.9")
        || m_exerciseId == QStringLiteral("2.10")
        || m_exerciseId == QStringLiteral("3.1.1") || m_exerciseId == QStringLiteral("3.1.2")
        || m_exerciseId == QStringLiteral("3.1.10") || m_exerciseId == QStringLiteral("3.1.11")
        || m_exerciseId == QStringLiteral("3.1.12") || m_exerciseId == QStringLiteral("3.1.17")
        || m_exerciseId == QStringLiteral("3.1.18") || m_exerciseId == QStringLiteral("3.2.1")
        || m_exerciseId == QStringLiteral("3.2.2") || m_exerciseId == QStringLiteral("3.2.3")
        || m_exerciseId == QStringLiteral("3.2.4") || m_exerciseId == QStringLiteral("3.2.5")
        || m_exerciseId == QStringLiteral("3.2.11") || m_exerciseId == QStringLiteral("4.1.1")
        || m_exerciseId == QStringLiteral("4.1.2")
        || m_exerciseId == QStringLiteral("4.1.4")
        || m_exerciseId == QStringLiteral("4.1.5")
        || m_exerciseId == QStringLiteral("4.1.6")
        || m_exerciseId == QStringLiteral("4.1.8")
        || m_exerciseId == QStringLiteral("4.2.1");
}

bool ExerciseHost::forceNewProtocolSessionOnBegin() const {
    // После Begin повторный form должен начинаться с «Дата/специалист».
    // Исключения: дописка строк внутри сессии (руководство / упр. 6).
    const ExerciseDefinition *definition = ExerciseConfig::find(m_exerciseId);
    if (!definition) {
        return false;
    }
    if (ExerciseConfig::usesAppendOnlyMultiStepLogic(m_exerciseId)) {
        return false;
    }
    if (definition->protocol == ExerciseProtocolKind::NumberedDoneTime
        || definition->protocol == ExerciseProtocolKind::DoneTimeOrHlp
        || definition->protocol == ExerciseProtocolKind::OrHlpBallsRow
        || definition->protocol == ExerciseProtocolKind::OrHlpRow
        || definition->protocol == ExerciseProtocolKind::TimedBalls
        || definition->protocol == ExerciseProtocolKind::TimedBallsWithPictureCount) {
        return true;
    }
    return m_exerciseId == QStringLiteral("3.1.1") || m_exerciseId == QStringLiteral("3.1.11")
        || m_exerciseId == QStringLiteral("3.1.17") || m_exerciseId == QStringLiteral("3.1.18")
        || m_exerciseId == QStringLiteral("4.1.8") || m_exerciseId == QStringLiteral("4.2.1")
        || m_exerciseId == QStringLiteral("4.2.2") || m_exerciseId == QStringLiteral("5.1.1")
        || m_exerciseId == QStringLiteral("5.4.2");
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

void ExerciseHost::sumProtocolScores() {
    if (m_exerciseId == QStringLiteral("1.26")) {
        sumProtocol126();
        return;
    }
    if (m_exerciseId == QStringLiteral("1.272")) {
        sumProtocol1272();
        return;
    }
    if (m_exerciseId == QStringLiteral("3.1.10")) {
        sumProtocol3110();
        return;
    }
    if (m_exerciseId == QStringLiteral("4.1.8")) {
        sumProtocol418();
        return;
    }
    if (m_exerciseId == QStringLiteral("1.14")) {
        sumProtocolOrHlpBalls(QStringLiteral("idb"), QStringLiteral("(6)"));
        return;
    }
    if (m_exerciseId == QStringLiteral("1.15")) {
        sumProtocolOrHlpBalls(QStringLiteral("ids"), QStringLiteral("(9)"));
        return;
    }
    if (m_exerciseId == QStringLiteral("1.20")) {
        sumProtocolOrHlpBalls(QStringLiteral("ids"), QStringLiteral("(20)"));
        return;
    }
    if (m_exerciseId == QStringLiteral("1.21")) {
        sumProtocolOrHlpBalls(QStringLiteral("ids"), QStringLiteral("(21)"));
        return;
    }
    if (m_exerciseId == QStringLiteral("1.27")) {
        sumProtocolOrHlpBalls(QStringLiteral("ids"), QStringLiteral("(9)"));
        return;
    }
    if (m_exerciseId == QStringLiteral("2.11")) {
        sumProtocolOrHlpBalls(QStringLiteral("ids"), QStringLiteral("(6)"));
        return;
    }
    if (m_exerciseId == QStringLiteral("2.12")) {
        sumProtocolOrHlpBalls(QStringLiteral("ids"), QStringLiteral("(4)"));
        return;
    }
    if (m_exerciseId == QStringLiteral("3.1.8")) {
        sumProtocolOrHlpBalls(QStringLiteral("idb"), QStringLiteral("(4)"));
        return;
    }
    if (m_exerciseId == QStringLiteral("3.1.21")) {
        sumProtocolOrHlpBalls(QStringLiteral("idb"), QStringLiteral("(12)"));
        return;
    }
}

void ExerciseHost::sumProtocol3110() {
    if (m_exerciseId != QStringLiteral("3.1.10") || !m_repository || m_currentProtocolId.isEmpty()
        || !m_templateBrowser) {
        return;
    }
    commitTextEditChanges(m_templateBrowser, true);
    QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }
    // Перенос правок (картинка/OR/HLP/…) + баллы по характеру деятельности −0.5/помощь
    // → idb; сумма → idsum / idvivod(20). Не затирать вручную внесённые OR/HLP.
    storedBody = ExerciseProtocol::applyProtocol3110SumFromDocument(
        storedBody, m_templateBrowser->document());

    QString error;
    if (!m_repository->updateProtocolBody(m_currentProtocolId, storedBody, &error)) {
        CustomMessageBox::showError(this, error);
        return;
    }

    if (m_protocolSaveTimer) {
        m_protocolSaveTimer->stop();
    }
    m_suppressProtocolAutosave = true;

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    updateContentHeights();
    updateProtocolEditMode();
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });
    emit protocolSaved();
}

void ExerciseHost::sumProtocol418() {
    if (m_exerciseId != QStringLiteral("4.1.8") || !m_repository || m_currentProtocolId.isEmpty()
        || !m_templateBrowser) {
        return;
    }
    // Как bsum: подтянуть b* из редактора и посчитать сумму.
    // Не вызывать saveProtocolEdits() отдельно — двойной merge мог затирать «Примечание».
    commitTextEditChanges(m_templateBrowser, true);

    auto plainFromIdnoteInner = [](QString inner) {
        inner.remove(QRegularExpression(QStringLiteral("<[^>]+>")));
        inner.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
        inner.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        inner.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
        inner.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
        inner.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
        return inner.trimmed();
    };
    auto readNotePlainFromDoc = [](QTextDocument *doc) -> QString {
        if (!doc) {
            return {};
        }
        QString note;
        std::function<void(QTextFrame *)> walk;
        walk = [&](QTextFrame *frame) {
            if (!frame) {
                return;
            }
            if (auto *table = qobject_cast<QTextTable *>(frame)) {
                for (int r = 0; r < table->rows(); ++r) {
                    if (table->columns() < 2) {
                        continue;
                    }
                    QTextCursor c0 = table->cellAt(r, 0).firstCursorPosition();
                    c0.setPosition(table->cellAt(r, 0).lastCursorPosition().position(),
                                   QTextCursor::KeepAnchor);
                    QString first = c0.selectedText();
                    first.replace(QChar(0x2029), QLatin1Char(' '));
                    first.replace(QChar::ParagraphSeparator, QLatin1Char(' '));
                    first = first.trimmed();
                    if (!first.contains(QStringLiteral("Примечание"), Qt::CaseInsensitive)) {
                        continue;
                    }
                    QTextCursor c1 = table->cellAt(r, 1).firstCursorPosition();
                    c1.setPosition(table->cellAt(r, 1).lastCursorPosition().position(),
                                   QTextCursor::KeepAnchor);
                    QString second = c1.selectedText();
                    second.replace(QChar(0x2029), QLatin1Char('\n'));
                    second.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
                    second = second.trimmed();
                    if (second.contains(QStringLiteral("Стимульные"), Qt::CaseInsensitive)
                        || second.contains(QStringLiteral("Выбранная"), Qt::CaseInsensitive)) {
                        continue;
                    }
                    note = second;
                }
            }
            for (QTextFrame::iterator it = frame->begin(); !it.atEnd(); ++it) {
                if (QTextFrame *child = it.currentFrame()) {
                    walk(child);
                }
            }
        };
        walk(doc->rootFrame());
        return note;
    };

    const QString editorNotePlain = readNotePlainFromDoc(m_templateBrowser->document());

    QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }

    const QRegularExpression idnoteRe(
        QStringLiteral("(<div\\b[^>]*\\bid\\s*=\\s*['\"]idnote['\"][^>]*>)([\\s\\S]*?)(</div>)"),
        QRegularExpression::CaseInsensitiveOption);
    QString preservedNotePlain = editorNotePlain;
    if (preservedNotePlain.isEmpty()) {
        QRegularExpressionMatchIterator it = idnoteRe.globalMatch(storedBody);
        QRegularExpressionMatch last;
        while (it.hasNext()) {
            last = it.next();
        }
        if (last.hasMatch()) {
            preservedNotePlain = plainFromIdnoteInner(last.captured(2));
        }
    }

    storedBody = ExerciseProtocol::mergeProtocol418EditorIntoStoredBody(
        storedBody, m_templateBrowser->document());
    storedBody = ExerciseProtocol::applyProtocolBPrefixSum(storedBody);

    // После sum всегда вернуть «Примечание» из редактора (или БД), если оно было.
    if (!preservedNotePlain.isEmpty()) {
        QRegularExpressionMatchIterator it = idnoteRe.globalMatch(storedBody);
        QRegularExpressionMatch last;
        while (it.hasNext()) {
            last = it.next();
        }
        if (last.hasMatch()) {
            storedBody.replace(
                last.capturedStart(0),
                last.capturedLength(0),
                last.captured(1) + preservedNotePlain.toHtmlEscaped() + last.captured(3));
        }
    }

    QString error;
    if (!m_repository->updateProtocolBody(m_currentProtocolId, storedBody, &error)) {
        CustomMessageBox::showError(this, error);
        return;
    }

    if (m_protocolSaveTimer) {
        m_protocolSaveTimer->stop();
    }
    m_suppressProtocolAutosave = true;

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    updateContentHeights();
    updateProtocolEditMode();
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });
    emit protocolSaved();
}

void ExerciseHost::sumProtocol126() {
    if (m_exerciseId != QStringLiteral("1.26") || !m_repository || m_currentProtocolId.isEmpty()
        || !m_templateBrowser) {
        return;
    }
    // Сначала сохранить правки (примечание, характер, виды помощи, ответы), затем суммы.
    saveProtocolEdits();
    commitTextEditChanges(m_templateBrowser, true);
    QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }
    // Не вызываем merge/joinClosed — они пересобирают таблицы и «сжимают» вёрстку.
    // Как bsum в оригинале: только подстановка баллов/сумм в существующие div по id.
    storedBody = ExerciseProtocol::applyProtocol126SumFromDocument(
        storedBody, m_templateBrowser->document(), true);

    QString error;
    if (!m_repository->updateProtocolBody(m_currentProtocolId, storedBody, &error)) {
        CustomMessageBox::showError(this, error);
        return;
    }

    // Не даём autosave сразу после setHtml перезаписать тело из QTextDocument без id.
    if (m_protocolSaveTimer) {
        m_protocolSaveTimer->stop();
    }
    m_suppressProtocolAutosave = true;

    // Последняя сессия (оба задания 1.26 внутри неё), не все исторические протоколы.
    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    updateContentHeights();
    updateProtocolEditMode();
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });
    emit protocolSaved();
}

void ExerciseHost::sumProtocol1272() {
    if (m_exerciseId != QStringLiteral("1.272") || !m_repository || m_currentProtocolId.isEmpty()
        || !m_templateBrowser) {
        return;
    }
    // Сначала сохранить правки (примечание, характер, виды помощи, баллы), затем суммы.
    saveProtocolEdits();
    commitTextEditChanges(m_templateBrowser, true);
    QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }
    // Ещё раз подтянуть баллы ids* / idsum из редактора (на случай потери id в autosave).
    storedBody = ExerciseProtocol::mergeProtocol1272EditorIntoStoredBody(
        storedBody, m_templateBrowser->document());
    // Сумма ids* → «Итоговая оценка»; то же значение → «Результат: баллы (макс.)» как N(24).
    storedBody = ExerciseProtocol::applyProtocolIdbSum(
        storedBody, QStringLiteral("(24)"), QStringLiteral("ids"));

    QString error;
    if (!m_repository->updateProtocolBody(m_currentProtocolId, storedBody, &error)) {
        CustomMessageBox::showError(this, error);
        return;
    }
    if (m_protocolSaveTimer) {
        m_protocolSaveTimer->stop();
    }
    m_suppressProtocolAutosave = true;

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    updateContentHeights();
    updateProtocolEditMode();
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });
    emit protocolSaved();
}

void ExerciseHost::sumProtocolOrHlpBalls(const QString &idPrefix, const QString &maxSuffix) {
    if (!m_repository || m_currentProtocolId.isEmpty() || !m_templateBrowser) {
        return;
    }
    saveProtocolEdits();
    commitTextEditChanges(m_templateBrowser, true);
    QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }
    // Введённые в QTextEdit баллы (idb*/ids*) иначе не попадают в stored body → сумма 0.
    storedBody = ExerciseProtocol::mergeProtocol1272EditorIntoStoredBody(
        storedBody, m_templateBrowser->document(), idPrefix);
    storedBody = ExerciseProtocol::applyProtocolIdbSum(storedBody, maxSuffix, idPrefix);

    QString error;
    if (!m_repository->updateProtocolBody(m_currentProtocolId, storedBody, &error)) {
        CustomMessageBox::showError(this, error);
        return;
    }
    if (m_protocolSaveTimer) {
        m_protocolSaveTimer->stop();
    }
    m_suppressProtocolAutosave = true;

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    updateContentHeights();
    updateProtocolEditMode();
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });
    emit protocolSaved();
}

void ExerciseHost::sumProtocol318() {
    if (m_exerciseId != QStringLiteral("3.1.18")
        || !m_repository || m_currentProtocolId.isEmpty() || !m_templateBrowser) {
        return;
    }
    commitTextEditChanges(m_templateBrowser, true);
    QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }
    storedBody = ExerciseProtocol::applyProtocol318SumFromDocument(
        storedBody, m_templateBrowser->document());

    QString error;
    if (!m_repository->updateProtocolBody(m_currentProtocolId, storedBody, &error)) {
        CustomMessageBox::showError(this, error);
        return;
    }
    if (m_protocolSaveTimer) {
        m_protocolSaveTimer->stop();
    }
    m_suppressProtocolAutosave = true;

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    updateContentHeights();
    updateProtocolEditMode();
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });
    emit protocolSaved();
}

bool ExerciseHost::isCursorInProtocolBallsColumn() const {
    if (!m_templateBrowser) {
        return false;
    }
    const QTextCursor cursor = m_templateBrowser->textCursor();
    QTextTable *table = cursor.currentTable();
    if (!table) {
        return false;
    }
    const QTextTableCell cell = table->cellAt(cursor.position());
    if (!cell.isValid()) {
        return false;
    }
    const int row = cell.row();
    const int col = cell.column();
    int ballsCol = -1;
    int headerRow = -1;
    for (int r = 0; r < table->rows() && ballsCol < 0; ++r) {
        for (int c = 0; c < table->columns(); ++c) {
            QTextCursor probe = table->cellAt(r, c).firstCursorPosition();
            probe.setPosition(table->cellAt(r, c).lastCursorPosition().position(), QTextCursor::KeepAnchor);
            QString text = probe.selectedText();
            text.replace(QChar(0x2029), QLatin1Char(' '));
            text.replace(QChar::ParagraphSeparator, QLatin1Char(' '));
            text = text.trimmed();
            if (text.compare(QStringLiteral("Баллы"), Qt::CaseInsensitive) == 0
                || (text.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive) && text.length() <= 12)) {
                ballsCol = c;
                headerRow = r;
                break;
            }
        }
    }
    return ballsCol >= 0 && col == ballsCol && row > headerRow;
}

void ExerciseHost::onProtocolCursorMoved() {
    const bool syncBalls = m_exerciseId == QStringLiteral("2.2")
        || m_exerciseId == QStringLiteral("2.3")
        || m_exerciseId == QStringLiteral("3.1.17")
        || m_exerciseId == QStringLiteral("3.1.18")
        || m_exerciseId == QStringLiteral("3.3.1")
        || m_exerciseId == QStringLiteral("3.3.2")
        || m_exerciseId == QStringLiteral("3.3.3")
        || m_exerciseId == QStringLiteral("4.1.4")
        || m_exerciseId == QStringLiteral("4.2.1")
        || m_exerciseId == QStringLiteral("4.2.2")
        || m_exerciseId == QStringLiteral("5.1.1");
    if (!syncBalls || !m_protocolSavedThisSession) {
        m_cursorInBallsColumn = false;
        return;
    }
    const bool inBalls = isCursorInProtocolBallsColumn();
    if (m_cursorInBallsColumn && !inBalls) {
        QTimer::singleShot(0, this, [this]() { syncProtocol317BallsToResult(); });
    }
    m_cursorInBallsColumn = inBalls;
}

void ExerciseHost::syncProtocol317BallsToResult() {
    const bool syncBalls = m_exerciseId == QStringLiteral("2.2")
        || m_exerciseId == QStringLiteral("2.3")
        || m_exerciseId == QStringLiteral("3.1.17")
        || m_exerciseId == QStringLiteral("3.1.18")
        || m_exerciseId == QStringLiteral("3.3.1")
        || m_exerciseId == QStringLiteral("3.3.2")
        || m_exerciseId == QStringLiteral("3.3.3")
        || m_exerciseId == QStringLiteral("4.1.4")
        || m_exerciseId == QStringLiteral("4.2.1")
        || m_exerciseId == QStringLiteral("4.2.2")
        || m_exerciseId == QStringLiteral("5.1.1");
    if (!syncBalls
        || !m_protocolSavedThisSession
        || m_suppressProtocolAutosave
        || !m_repository
        || m_currentProtocolId.isEmpty()
        || !m_templateBrowser) {
        return;
    }
    commitTextEditChanges(m_templateBrowser, true);
    QString storedBody = m_repository->loadProtocolBodyById(m_currentProtocolId);
    if (storedBody.trimmed().isEmpty()) {
        return;
    }
    const QString updated = ExerciseProtocol::applyProtocol318SumFromDocument(
        storedBody, m_templateBrowser->document());
    if (updated != storedBody) {
        QString error;
        if (!m_repository->updateProtocolBody(m_currentProtocolId, updated, &error)) {
            return;
        }
    }
    // Всегда обновляем UI: autosave мог уже записать БД, а «Результат» на экране упражнения
    // оставался старым из‑за early-return при updated == storedBody.
    if (m_protocolSaveTimer) {
        m_protocolSaveTimer->stop();
    }
    m_suppressProtocolAutosave = true;
    m_cursorInBallsColumn = false;

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, m_currentProtocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    updateContentHeights();
    updateProtocolEditMode();
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });
    emit protocolSaved();
}

void ExerciseHost::saveProtocolEdits() {
    if (!m_protocolSavedThisSession || m_suppressProtocolAutosave) {
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
        // Результат/Примечание/OR/HLP/ответы/баллы — без пересборки таблиц.
        body = ExerciseProtocol::mergeProtocol126EditorIntoStoredBody(
            storedBody, m_templateBrowser->document());
    } else if (m_exerciseId == QStringLiteral("1.272")) {
        body = ExerciseProtocol::mergeProtocol1272EditorIntoStoredBody(
            storedBody, m_templateBrowser->document(), QStringLiteral("ids"));
    } else if (m_exerciseId == QStringLiteral("1.14")
               || m_exerciseId == QStringLiteral("3.1.8")
               || m_exerciseId == QStringLiteral("3.1.21")) {
        body = ExerciseProtocol::mergeProtocol1272EditorIntoStoredBody(
            storedBody, m_templateBrowser->document(), QStringLiteral("idb"));
    } else if (m_exerciseId == QStringLiteral("1.15")
               || m_exerciseId == QStringLiteral("1.20")
               || m_exerciseId == QStringLiteral("1.21")
               || m_exerciseId == QStringLiteral("1.27")
               || m_exerciseId == QStringLiteral("2.11")
               || m_exerciseId == QStringLiteral("2.12")) {
        body = ExerciseProtocol::mergeProtocol1272EditorIntoStoredBody(
            storedBody, m_templateBrowser->document(), QStringLiteral("ids"));
    } else if (m_exerciseId == QStringLiteral("3.1.10")) {
        body = ExerciseProtocol::mergeProtocol3110EditorIntoStoredBody(
            storedBody, m_templateBrowser->document());
        body = ExerciseProtocol::mergeOrHlpBallsEditorIntoStoredBody(
            body, m_templateBrowser->document());
    } else if (m_exerciseId == QStringLiteral("4.1.8")) {
        body = ExerciseProtocol::mergeProtocol418EditorIntoStoredBody(
            storedBody, m_templateBrowser->document());
    } else if (m_exerciseId == QStringLiteral("1.1")
               || m_exerciseId == QStringLiteral("1.2")
               || m_exerciseId == QStringLiteral("1.4")
               || m_exerciseId == QStringLiteral("1.5")
               || m_exerciseId == QStringLiteral("1.6")
               || m_exerciseId == QStringLiteral("1.7")
               || m_exerciseId == QStringLiteral("1.11")
               || m_exerciseId == QStringLiteral("1.12")
               || m_exerciseId == QStringLiteral("1.8")
               || m_exerciseId == QStringLiteral("1.13")
               || m_exerciseId == QStringLiteral("1.17")
               || m_exerciseId == QStringLiteral("1.18")
               || m_exerciseId == QStringLiteral("1.19")
               || m_exerciseId == QStringLiteral("1.25")
               || m_exerciseId == QStringLiteral("2.8")
               || m_exerciseId == QStringLiteral("2.9")
               || m_exerciseId == QStringLiteral("2.10")
               || m_exerciseId == QStringLiteral("3.1.1")
               || m_exerciseId == QStringLiteral("3.1.2")
               || m_exerciseId == QStringLiteral("3.1.10")
               || m_exerciseId == QStringLiteral("3.1.11")
               || m_exerciseId == QStringLiteral("3.1.12")
               || m_exerciseId == QStringLiteral("3.2.1")
               || m_exerciseId == QStringLiteral("3.2.2")
               || m_exerciseId == QStringLiteral("3.2.3")
               || m_exerciseId == QStringLiteral("3.2.4")
               || m_exerciseId == QStringLiteral("3.2.5")
               || m_exerciseId == QStringLiteral("3.2.11")
               || m_exerciseId == QStringLiteral("4.1.1")
               || m_exerciseId == QStringLiteral("4.1.2")
               || m_exerciseId == QStringLiteral("4.1.4")
               || m_exerciseId == QStringLiteral("4.1.5")
               || m_exerciseId == QStringLiteral("4.1.6")
               || m_exerciseId == QStringLiteral("4.2.1")
               || m_exerciseId == QStringLiteral("4.2.2")
               || m_exerciseId == QStringLiteral("5.1.1")
               || m_exerciseId == QStringLiteral("5.2.1")
               || m_exerciseId == QStringLiteral("5.3.1")
               || m_exerciseId == QStringLiteral("5.4.2")) {
        body = ExerciseProtocol::mergeOrHlpBallsEditorIntoStoredBody(
            storedBody, m_templateBrowser->document());
    } else if (m_exerciseId == QStringLiteral("3.1.17")
               || m_exerciseId == QStringLiteral("3.1.18")
               || m_exerciseId == QStringLiteral("3.3.1")
               || m_exerciseId == QStringLiteral("3.3.2")
               || m_exerciseId == QStringLiteral("3.3.3")) {
        body = ExerciseProtocol::applyProtocol318SumFromDocument(
            storedBody, m_templateBrowser->document());
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
    if (m_exerciseId == QStringLiteral("4.2.2")) {
        syncWords422AdditionalFromPanel();
    }
    // 4.1.2 / 3.1.24: по «Пример» протокол не формируется.
    // 1.21: «2А» — обучающее, в протокол не входит.
    if (((m_exerciseId == QStringLiteral("4.1.2") || m_exerciseId == QStringLiteral("3.1.24"))
         && currentStepId().trimmed() == QStringLiteral("Пример"))
        || (m_exerciseId == QStringLiteral("1.21")
            && currentStepId().trimmed() == QStringLiteral("2А"))) {
        m_protocolFormed = true;
        CustomMessageBox::showInfo(
            this,
            m_exerciseId == QStringLiteral("1.21")
                ? QStringLiteral(
                      "По заданию «2А» протокол не формируется. Выберите следующее задание и нажмите «Начать».")
                : QStringLiteral(
                      "По заданию «Пример» протокол не формируется. Выберите задание «1» и нажмите «Начать»."));
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
    bool saveAsPartly = partlySave && !existingBody.trimmed().isEmpty();

    if (m_exerciseId == QStringLiteral("1.26")) {
        // 1) Первое сохранение — новая запись с «Дата/специалист».
        // 2) То же посещение, задание 2 после задания 1 — дописать строки (partly/appendRows).
        // 3) Повторный протокол (ТЗ 14.2) — полная сессия с новой датой в то же тело.
        // 4) Оба задания за один прогон без промежуточного «Сформировать» — собрать оба блока.
        const QStringList parts = session.additional.split(QLatin1Char(';'));
        const QString stepKey = parts.isEmpty() || parts.at(0).trimmed().isEmpty()
            ? QStringLiteral("1")
            : parts.at(0).trimmed();

        const QString lastSessionHtml = saveAsPartly
            ? ExerciseProtocol::extractLastProtocol126Session(existingBody)
            : QString();
        const bool continueTask2 = saveAsPartly
            && !m_forceNewProtocolSession
            && stepKey == QStringLiteral("2")
            && lastSessionHtml.contains(QStringLiteral("Задание 1"), Qt::CaseInsensitive)
            && !lastSessionHtml.contains(QStringLiteral("Задание 2"), Qt::CaseInsensitive);

        const QString add1 = m_additionalByStep.value(QStringLiteral("1"));
        const QString add2 = m_additionalByStep.value(QStringLiteral("2"));
        const bool needBothTasks = !add1.trimmed().isEmpty()
            && (stepKey == QStringLiteral("2") || !add2.trimmed().isEmpty())
            && !continueTask2;

        auto sessionWithAdditional = [&](const QString &additional) {
            ProtocolSessionInput copy = session;
            copy.additional = additional;
            return copy;
        };

        if (continueTask2) {
            // Баллы задания 1 могли остаться только в редакторе (без «Подвести итог»).
            // Перед допиской задания 2 переносим их в HTML, не считая суммы.
            if (m_templateBrowser) {
                commitTextEditChanges(m_templateBrowser, true);
                existingBody = ExerciseProtocol::applyProtocol126SumFromDocument(
                    existingBody, m_templateBrowser->document(), false);
            }
            protocolBody = ExerciseProtocol::createProtocolHtml(
                m_exerciseId,
                m_specialistFio,
                m_elapsedSeconds,
                true,
                existingBody,
                m_answers,
                checkboxValues(),
                session);
            saveAsPartly = true;
            m_forceNewProtocolSession = false;
        } else if (needBothTasks) {
            const QString task2Additional = !add2.trimmed().isEmpty() ? add2 : session.additional;
            QString newSession = ExerciseProtocol::createProtocolHtml(
                m_exerciseId,
                m_specialistFio,
                m_elapsedSeconds,
                false,
                QString(),
                m_answers,
                checkboxValues(),
                sessionWithAdditional(add1));
            newSession = ExerciseProtocol::createProtocolHtml(
                m_exerciseId,
                m_specialistFio,
                m_elapsedSeconds,
                true,
                newSession,
                m_answers,
                checkboxValues(),
                sessionWithAdditional(task2Additional));
            if (saveAsPartly) {
                protocolBody =
                    ExerciseProtocol::appendFullSessionToStoredBody(existingBody, newSession);
                saveAsPartly = true;
            } else {
                protocolBody = newSession;
                saveAsPartly = false;
            }
            m_forceNewProtocolSession = false;
        } else if (saveAsPartly) {
            // Повтор: новый блок с даты, не затирая предыдущие сессии.
            const QString newSession = ExerciseProtocol::createProtocolHtml(
                m_exerciseId,
                m_specialistFio,
                m_elapsedSeconds,
                false,
                QString(),
                m_answers,
                checkboxValues(),
                session);
            protocolBody = ExerciseProtocol::appendFullSessionToStoredBody(existingBody, newSession);
            saveAsPartly = true;
            m_forceNewProtocolSession = false;
        } else {
            protocolBody = ExerciseProtocol::createProtocolHtml(
                m_exerciseId,
                m_specialistFio,
                m_elapsedSeconds,
                false,
                QString(),
                m_answers,
                checkboxValues(),
                session);
            saveAsPartly = false;
            m_forceNewProtocolSession = false;
        }
    } else if (m_forceNewProtocolSession && saveAsPartly) {
        // После Begin: новый блок со строки «Дата/специалист».
        const QString newSession = ExerciseProtocol::createProtocolHtml(
            m_exerciseId,
            m_specialistFio,
            m_elapsedSeconds,
            false,
            QString(),
            m_answers,
            checkboxValues(),
            session);
        if (supportsScanUpload(m_exerciseId)) {
            // Отдельная запись в БД — свои файлы data/scans/{id}[-N].JPG.
            protocolBody = newSession;
            saveAsPartly = false;
        } else {
            protocolBody = ExerciseProtocol::appendFullSessionToStoredBody(existingBody, newSession);
            saveAsPartly = true;
        }
        m_forceNewProtocolSession = false;
    } else {
        protocolBody = ExerciseProtocol::createProtocolHtml(
            m_exerciseId,
            m_specialistFio,
            m_elapsedSeconds,
            saveAsPartly,
            existingBody,
            m_answers,
            checkboxValues(),
            session);
        m_forceNewProtocolSession = false;
    }

    QString error;
    QString protocolId;
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

    m_suppressProtocolAutosave = true;
    {
        const QString viewHtml = m_repository->loadProtocolViewHtml(
            m_exerciseId, protocolId, m_patientFio, m_patientBirthDate);
        m_templateBrowser->setHtml(ExerciseAssets::buildProtocolDocumentHtml(viewHtml));
    }
    finalizeProtocolTemplateDocument(m_templateBrowser->document());
    const int templateViewportWidth = kTemplateTableWidth + kTemplateViewportPadding;
    m_templateBrowser->setFixedWidth(templateViewportWidth);
    m_templatePanel->setMaximumWidth(templateViewportWidth + 16);
    layoutContent();
    QTimer::singleShot(80, this, [this]() { updateContentHeights(); });
    QTimer::singleShot(900, this, [this]() { m_suppressProtocolAutosave = false; });

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
    for (const ExerciseCheckRow &row : m_doneChecks) {
        if (row.box) {
            row.box->blockSignals(true);
            row.box->setChecked(false);
            row.box->blockSignals(false);
        }
    }

    m_protocolFormed = true;
    m_protocolSavedThisSession = true;
    m_partly = true;
    m_stepElapsedSeconds.clear();
    if (m_exerciseId == QStringLiteral("1.26")
        || m_exerciseId == QStringLiteral("1.272")
        || ExerciseConfig::usesAppendOnlyMultiStepLogic(m_exerciseId)) {
        // Строки уже попали в протокол — не дублировать при следующем формировании.
        m_additionalByStep.clear();
    }
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
    const QString prevW = m_rotateWCombo->currentText();
    const QString prevCW = m_rotateCWCombo->currentText();
    const int fragmentCount = puzzleFragmentCount();
    m_rotateWCombo->blockSignals(true);
    m_rotateWCombo->clear();
    for (int i = 0; i <= fragmentCount; ++i) {
        m_rotateWCombo->addItem(QString::number(i));
    }
    const int wIndex = m_rotateWCombo->findText(prevW);
    if (wIndex >= 0) {
        m_rotateWCombo->setCurrentIndex(wIndex);
    } else if (m_exerciseId == QStringLiteral("1.14")
               && currentStepId().trimmed() == QStringLiteral("2")
               && m_rotateWCombo->count() > 1) {
        m_rotateWCombo->setCurrentIndex(1);
    } else {
        m_rotateWCombo->setCurrentIndex(0);
    }
    m_rotateWCombo->blockSignals(false);

    m_rotateCWCombo->blockSignals(true);
    m_rotateCWCombo->clear();
    for (int i = 0; i <= fragmentCount; ++i) {
        m_rotateCWCombo->addItem(QString::number(i));
    }
    const int cwIndex = m_rotateCWCombo->findText(prevCW);
    if (cwIndex >= 0) {
        m_rotateCWCombo->setCurrentIndex(cwIndex);
    } else if (m_exerciseId == QStringLiteral("1.14")
               && currentStepId().trimmed() == QStringLiteral("2")
               && m_rotateCWCombo->count() > 1) {
        m_rotateCWCombo->setCurrentIndex(1);
    } else {
        m_rotateCWCombo->setCurrentIndex(0);
    }
    m_rotateCWCombo->blockSignals(false);
}

void ExerciseHost::applyPuzzleOptionsDefaults() {
    const QString step = currentStepId().trimmed();
    const bool isPuzzleShard = m_exerciseId == QStringLiteral("1.19")
        || m_exerciseId == QStringLiteral("1.20") || m_exerciseId == QStringLiteral("1.21");
    if (m_exerciseId == QStringLiteral("1.14")) {
        if (m_showHintCheck) {
            m_showHintCheck->setChecked(true);
        }
        if (m_showTemplateCheck) {
            m_showTemplateCheck->setChecked(false);
        }
        if (m_rotateEnableCheck) {
            m_rotateEnableCheck->setChecked(true);
        }
        if (step == QStringLiteral("2")) {
            m_shardPanelVisible = false;
            refreshRotateCombos();
        }
    } else if (isPuzzleShard) {
        m_shardPanelVisible = false;
        if (m_showHintCheck) {
            m_showHintCheck->setChecked(false);
        }
        if (m_showTemplateCheck) {
            m_showTemplateCheck->setChecked(false);
        }
        if (m_rotateEnableCheck) {
            m_rotateEnableCheck->setChecked(true);
        }
        refreshRotateCombos();
    } else if (m_exerciseId == QStringLiteral("1.22")) {
        m_shardPanelVisible = false;
        if (m_e15HighlightRadio) {
            m_e15HighlightRadio->setChecked(true);
        }
    } else if (m_exerciseId == QStringLiteral("3.1.16")) {
        m_shardPanelVisible = false;
        if (m_aparamAllRadio) {
            m_aparamAllRadio->setChecked(true);
        }
        if (m_aparamGroup) {
            m_aparamGroup->hide();
        }
        if (m_shard316Button) {
            m_shard316Button->hide();
        }
    }
}

void ExerciseHost::layoutE15ModePopup() {
    if (!m_rightPanel) {
        return;
    }
    const QString step = currentStepId().trimmed();
    const bool is15 = m_exerciseId == QStringLiteral("1.5");
    const bool is16 = m_exerciseId == QStringLiteral("1.6");
    const bool isE15 = is15 || is16;
    const bool is122 = m_exerciseId == QStringLiteral("1.22");
    const bool isPuzzleShard = m_exerciseId == QStringLiteral("1.19")
        || m_exerciseId == QStringLiteral("1.20") || m_exerciseId == QStringLiteral("1.21")
        || (m_exerciseId == QStringLiteral("1.14") && step == QStringLiteral("2"));
    const bool is316 = m_exerciseId == QStringLiteral("3.1.16") && step.toInt() >= 3;

    if (m_e15ModeGroup) {
        const bool show15Popup = (is15 || is16) && m_shardPanelVisible && m_shard15Button
            && m_shard15Button->isVisible();
        const bool showOtherPopup = !(is15 || is16) && (isE15 || is122) && m_shardPanelVisible
            && m_shardButton && m_shardButton->isVisible();
        if (show15Popup || showOtherPopup) {
            m_e15ModeGroup->setFixedWidth(300);
            if (QLayout *lay = m_e15ModeGroup->layout()) {
                lay->activate();
            }
            const int groupH = qMax(m_e15ModeGroup->sizeHint().height(), 110);
            if (show15Popup) {
                // Попап на правой панели под ссылкой.
                if (m_e15ModeGroup->parentWidget() != m_rightPanel) {
                    m_e15ModeGroup->setParent(m_rightPanel);
                }
                // Гарантировать актуальные координаты ссылки до чтения x/y.
                m_shard15Button->adjustSize();
                constexpr int kLinkAbsX = 1550;
                constexpr int kLinkAbsY = 12;
                constexpr int kLinkH = 33;
                const int rightPanelLeft = kPanelX + kScrollWidth;
                const int linkW = qMax(m_shard15Button->sizeHint().width(), 280);
                const int localX = qMax(8, kLinkAbsX - rightPanelLeft);
                m_shard15Button->setGeometry(localX, kLinkAbsY, linkW, kLinkH);
                m_shard15Button->show();
                const int popupX = m_shard15Button->x();
                const int popupY = m_shard15Button->y() + m_shard15Button->height() + 2;
                m_e15ModeGroup->setGeometry(popupX, popupY, 300, groupH);
            } else {
                if (m_e15ModeGroup->parentWidget() != m_rightPanel) {
                    m_e15ModeGroup->setParent(m_rightPanel);
                }
                const QPoint below =
                    m_shardButton->mapTo(m_rightPanel, QPoint(0, m_shardButton->height() + 2));
                m_e15ModeGroup->setGeometry(below.x(), below.y(), 300, groupH);
            }
            m_e15ModeGroup->show();
            m_e15ModeGroup->raise();
            if (show15Popup && m_shard15Button) {
                m_shard15Button->raise();
            }
        } else {
            m_e15ModeGroup->hide();
            if (m_e15ModeGroup->parentWidget() != m_rightPanel) {
                m_e15ModeGroup->setParent(m_rightPanel);
            }
        }
    }

    if (m_aparamGroup) {
        if (is316 && m_shardPanelVisible && m_shard316Button && m_shard316Button->isVisible()) {
            m_aparamGroup->setFixedWidth(280);
            if (QLayout *lay = m_aparamGroup->layout()) {
                lay->activate();
            }
            const int groupH = qMax(m_aparamGroup->sizeHint().height(), 84);
            // Как sets в оригинале: Left=1415, Top≈43 под ссылкой.
            const int popupX = m_shard316Button->x();
            const int popupY = m_shard316Button->y() + m_shard316Button->height() + 2;
            m_aparamGroup->setGeometry(popupX, popupY, 280, groupH);
            m_aparamGroup->show();
            m_aparamGroup->raise();
        } else {
            m_aparamGroup->hide();
        }
    }

    if (!m_puzzleOptionsGroup) {
        return;
    }
    const bool puzzleAnchor15 =
        (m_exerciseId == QStringLiteral("1.19") || m_exerciseId == QStringLiteral("1.20")
         || m_exerciseId == QStringLiteral("1.21"))
        && m_shard15Button && m_shard15Button->isVisible();
    const bool puzzleAnchorLegacy = m_shardButton && m_shardButton->isVisible();
    if (isPuzzleShard && m_shardPanelVisible && (puzzleAnchor15 || puzzleAnchorLegacy)) {
        constexpr int kPopupW = 300;
        m_puzzleOptionsGroup->setFixedWidth(kPopupW);
        if (QLayout *lay = m_puzzleOptionsGroup->layout()) {
            lay->activate();
        }
        const int groupH = qMax(m_puzzleOptionsGroup->sizeHint().height(), 180);
        if (puzzleAnchor15) {
            if (m_puzzleOptionsGroup->parentWidget() != m_rightPanel) {
                m_puzzleOptionsGroup->setParent(m_rightPanel);
            }
            m_shard15Button->adjustSize();
            constexpr int kLinkAbsX = 1550;
            constexpr int kLinkAbsY = 12;
            constexpr int kLinkH = 33;
            const int rightPanelLeft = kPanelX + kScrollWidth;
            const int linkW = qMax(m_shard15Button->sizeHint().width(), 280);
            const int localX = qMax(8, kLinkAbsX - rightPanelLeft);
            m_shard15Button->setGeometry(localX, kLinkAbsY, linkW, kLinkH);
            m_shard15Button->show();
            const int popupX = m_shard15Button->x();
            const int popupY = m_shard15Button->y() + m_shard15Button->height() + 2;
            m_puzzleOptionsGroup->setGeometry(popupX, popupY, kPopupW, groupH);
        } else {
            const QPoint below =
                m_shardButton->mapTo(m_rightPanel, QPoint(0, m_shardButton->height() + 2));
            m_puzzleOptionsGroup->setGeometry(below.x(), below.y(), kPopupW, groupH);
        }
        m_puzzleOptionsGroup->show();
        m_puzzleOptionsGroup->raise();
        if (puzzleAnchor15) {
            m_shard15Button->raise();
        }
    } else if (isPuzzleShard) {
        m_puzzleOptionsGroup->hide();
    }
}

void ExerciseHost::updateExerciseOptionsPanel() {
    const QString step = currentStepId().trimmed();
    const bool isE15 = m_exerciseId == QStringLiteral("1.5") || m_exerciseId == QStringLiteral("1.6");
    const bool is15 = m_exerciseId == QStringLiteral("1.5");
    const bool is16 = m_exerciseId == QStringLiteral("1.6");
    const bool is122 = m_exerciseId == QStringLiteral("1.22");
    const bool isPuzzleShard = m_exerciseId == QStringLiteral("1.19")
        || m_exerciseId == QStringLiteral("1.20") || m_exerciseId == QStringLiteral("1.21")
        || (m_exerciseId == QStringLiteral("1.14") && step == QStringLiteral("2"));
    const bool is316 = m_exerciseId == QStringLiteral("3.1.16");
    // Руководство: ссылка только в заданиях 3–7.
    const bool show316Options = is316 && step.toInt() >= 3;
    const bool is417 = m_exerciseId == QStringLiteral("4.1.7");
    // 1.5/1.6/1.19/1.20/1.21 — отдельная ссылка m_shard15Button; не options-панель.
    const bool showShard = ((isE15 && !is15 && !is16) || is122 || isPuzzleShard)
        && m_exerciseId != QStringLiteral("1.19")
        && m_exerciseId != QStringLiteral("1.20")
        && m_exerciseId != QStringLiteral("1.21");

    if (m_exerciseOptionsPanel) {
        // 4.1.7 / 1.5 / 1.6 / 1.19 / 1.20 / 1.21: без options-панели на правой (своя ссылка / rem).
        m_exerciseOptionsPanel->setVisible(showShard && !show316Options);
    }
    if (m_shardButton) {
        m_shardButton->setVisible(showShard);
    }
    if (m_shard15Button) {
        const bool show15Link =
            (is15 || is16 || m_exerciseId == QStringLiteral("1.19")
             || m_exerciseId == QStringLiteral("1.20")
             || m_exerciseId == QStringLiteral("1.21"))
            && !m_exerciseRunning;
        m_shard15Button->setVisible(show15Link);
        if (show15Link && m_rightPanel) {
            m_shard15Button->adjustSize();
            constexpr int kLinkAbsX = 1550;
            constexpr int kLinkAbsY = 12;
            constexpr int kLinkH = 33;
            const int rightPanelLeft = kPanelX + kScrollWidth;
            const int linkW = qMax(m_shard15Button->sizeHint().width(), 280);
            const int localX = qMax(8, kLinkAbsX - rightPanelLeft);
            m_shard15Button->setGeometry(localX, kLinkAbsY, linkW, kLinkH);
            m_shard15Button->raise();
        }
    }
    if (m_shard316Button) {
        m_shard316Button->setVisible(show316Options && !m_exerciseRunning);
        if (m_shard316Button->isVisible()) {
            m_shard316Button->raise();
        }
    }
    if (m_aparamGroup && !show316Options) {
        m_aparamGroup->hide();
        if (is316) {
            m_shardPanelVisible = false;
        }
    }
    if (m_remPanel) {
        m_remPanel->setVisible(is417 && !m_exerciseRunning);
        if (is417 && m_rightPanel) {
            layoutRemPanelWidget(m_remPanel, m_rightPanel);
        }
    }
    if (m_e15ModeGroup) {
        if (isE15 || is122) {
            layoutE15ModePopup();
        } else {
            if (!isPuzzleShard && !show316Options) {
                m_shardPanelVisible = false;
            }
            m_e15ModeGroup->hide();
        }
    }
    if (m_puzzleOptionsGroup) {
        if (isPuzzleShard) {
            layoutE15ModePopup();
        } else {
            m_puzzleOptionsGroup->hide();
        }
    }
    if (show316Options) {
        layoutE15ModePopup();
    }
    if (isPuzzleShard) {
        refreshRotateCombos();
    }
}

void ExerciseHost::pushLivePuzzleOptionsToRunner() {
    if (!m_exerciseRunning || !m_sessionRunner) {
        return;
    }
    m_sessionRunner->setSessionOptions(buildSessionOptions());
    // 1.5 / 1.6 / 1.22: режим подсветка/перемещение тоже обновляем на обоих экранах.
    if (m_sessionRunnerKind == ExerciseRunnerKind::E15
        || m_exerciseId == QStringLiteral("1.22")) {
        const bool selectMode = m_e15HighlightRadio && m_e15HighlightRadio->isChecked();
        m_sessionRunner->applyE15SelectMode(selectMode);
    }
}

ExerciseSessionOptions ExerciseHost::buildSessionOptions() const {
    ExerciseSessionOptions options;
    options.dualScreen = m_dualScreen;
    if (m_e15HighlightRadio && m_e15HighlightRadio->isChecked()) {
        // Как radioButton1 → param="select": только подсветка.
        options.e15SelectMode = true;
    } else if (m_e15SelectRadio && m_e15SelectRadio->isChecked()) {
        options.e15SelectMode = false;
    }
    const QString step = currentStepId().trimmed();
    const bool is114Step2 = m_exerciseId == QStringLiteral("1.14") && step == QStringLiteral("2");
    const bool isPuzzleInline = m_exerciseId == QStringLiteral("1.19")
        || m_exerciseId == QStringLiteral("1.20") || m_exerciseId == QStringLiteral("1.21");
    const bool usePuzzleOptions = is114Step2 || isPuzzleInline;
    if (usePuzzleOptions && m_showHintCheck) {
        options.showHint = m_showHintCheck->isChecked();
    } else if (m_showHintCheck && m_showHintCheck->isVisible()) {
        options.showHint = m_showHintCheck->isChecked();
    }
    if (m_exerciseId == QStringLiteral("1.15")
        || m_exerciseId == QStringLiteral("1.22")
        || m_exerciseId == QStringLiteral("1.28")) {
        options.showHint = false;
    }
    if (m_exerciseId == QStringLiteral("1.22") && m_e15HighlightRadio) {
        options.e15SelectMode = m_e15HighlightRadio->isChecked();
    }
    if (usePuzzleOptions && m_showTemplateCheck) {
        options.showTemplate = m_showTemplateCheck->isChecked();
    } else if (m_showTemplateCheck && m_showTemplateCheck->isVisible()) {
        options.showTemplate = m_showTemplateCheck->isChecked();
    }
    if (usePuzzleOptions && m_rotateEnableCheck) {
        options.rotateEnabled = m_rotateEnableCheck->isChecked();
        if (m_rotateWCombo) {
            options.rotateW = m_rotateWCombo->currentText().toInt();
        }
        if (m_rotateCWCombo) {
            options.rotateCW = m_rotateCWCombo->currentText().toInt();
        }
    } else if (m_rotateEnableCheck && m_rotateEnableCheck->isVisible()) {
        options.rotateEnabled = m_rotateEnableCheck->isChecked();
        if (m_rotateWCombo) {
            options.rotateW = m_rotateWCombo->currentText().toInt();
        }
        if (m_rotateCWCombo) {
            options.rotateCW = m_rotateCWCombo->currentText().toInt();
        }
    }
    if (m_exerciseId == QStringLiteral("1.26")) {
        options.genderPrefix = m_previewGenderPrefix.isEmpty()
            ? QStringLiteral("d")
            : m_previewGenderPrefix;
    }
    if (m_exerciseId == QStringLiteral("3.1.16")) {
        options.puzzleAparam = (m_aparamAllRadio && m_aparamAllRadio->isChecked())
            ? QStringLiteral("1")
            : QStringLiteral("2");
    }
    if (m_exerciseId == QStringLiteral("4.1.7")) {
        QStringList parts;
        for (int i = 0; i < 9; ++i) {
            parts << ((m_remChecks[i] && m_remChecks[i]->isChecked())
                          ? QStringLiteral("1")
                          : QStringLiteral("0"));
        }
        options.remPictureMask = parts.join(QLatin1Char(','));
    }
    return options;
}
