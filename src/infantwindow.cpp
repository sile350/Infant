#include "infantwindow.h"
#include "appsettings.h"
#include "custommessagebox.h"
#include "exerciseconfig.h"
#include "exerciseassets.h"
#include "protocoleditguard.h"
#include "exerciseprotocol.h"

#include <QClipboard>
#include <QMimeData>
#include <QTextCursor>
#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStandardPaths>
#include <QFileDialog>
#include <QFrame>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QPalette>
#include <QColor>
#include <QBrush>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPolygonF>
#include <QPrinter>
#include <QPrintDialog>
#include <QToolTip>
#include <QStandardPaths>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QTextCharFormat>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextDocumentWriter>
#include <QUrl>
#include <QVBoxLayout>
#include <QTimer>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QPropertyAnimation>
#include <QKeyEvent>
#include <QKeySequence>
#include <QShowEvent>
#include <QScreen>
#include <QScrollBar>
#include <QShortcut>
#include <QWindow>
#include <QSlider>
#include <memory>
#include <QRegularExpression>
#include <QMenu>
#include <QStyleFactory>
#include <QListView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QGraphicsOpacityEffect>
#include <QGroupBox>
#include <QTextDocument>
#include <algorithm>
#include <QCalendarWidget>
#include <QToolTip>
#include <QAbstractButton>
#include <QFileInfo>
#include <functional>

namespace {

struct UserSaveResult {
    bool ok = false;
    QString error;
    QString userId;
    QString login;
    QString password;
};

QPixmap tintedPixmap(const QString &path, const QColor &color, const QSize &size) {
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        return {};
    }
    pixmap = pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = image.pixel(x, y);
            if (qAlpha(pixel) == 0) {
                continue;
            }
            image.setPixel(
                x,
                y,
                qRgba(color.red(), color.green(), color.blue(), qAlpha(pixel))
            );
        }
    }
    return QPixmap::fromImage(image);
}

QString dropdownArrowImagePath() {
    static QString cachedPath;
    if (!cachedPath.isEmpty()) {
        return cachedPath;
    }

    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../assets/sysImages",
        QDir::currentPath() + "/assets/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath("combo_arrow.png");
        if (QFile::exists(candidate)) {
            cachedPath = QDir::fromNativeSeparators(candidate);
            return cachedPath;
        }
    }

    const QString filePath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                 .filePath("infant_combo_arrow.png");
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
    return cachedPath;
}

QString dropdownArrowCss() {
    QString path = dropdownArrowImagePath();
    path.replace('\\', '/');
    return QStringLiteral(
        "  width: 10px;"
        "  height: 6px;"
        "  image: url(\"%1\");"
    ).arg(path);
}

struct CheckBoxIndicatorPaths {
    QString unchecked;
    QString checked;
    QString disabledUnchecked;
    QString disabledChecked;
};

CheckBoxIndicatorPaths ensureCheckBoxIndicatorImages() {
    static CheckBoxIndicatorPaths paths;
    if (!paths.unchecked.isEmpty()) {
        return paths;
    }

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const auto makeIndicator = [&](bool checked, bool disabled, const QString &fileName) -> QString {
        const QString filePath = QDir(tempDir).filePath(fileName);
        if (!QFile::exists(filePath)) {
            QPixmap pixmap(13, 13);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            const QColor borderColor = disabled ? QColor(0xa0, 0xa0, 0xa0) : QColor(0x80, 0x80, 0x80);
            const QColor fillColor = disabled ? QColor(0xf4, 0xf4, 0xf4) : Qt::white;
            painter.fillRect(QRect(1, 1, 11, 11), fillColor);
            painter.setPen(QPen(borderColor, 1));
            painter.drawRect(QRect(1, 1, 11, 11));
            if (checked) {
                painter.setPen(QPen(disabled ? QColor(0x90, 0x90, 0x90) : QColor(0x22, 0x22, 0x22), 2));
                painter.drawLine(3, 7, 5, 9);
                painter.drawLine(5, 9, 10, 3);
            }
            pixmap.save(filePath, "PNG");
        }
        return QDir::fromNativeSeparators(filePath);
    };

    paths.unchecked = makeIndicator(false, false, QStringLiteral("infant_cb_unchecked.png"));
    paths.checked = makeIndicator(true, false, QStringLiteral("infant_cb_checked.png"));
    paths.disabledUnchecked = makeIndicator(false, true, QStringLiteral("infant_cb_disabled_unchecked.png"));
    paths.disabledChecked = makeIndicator(true, true, QStringLiteral("infant_cb_disabled_checked.png"));
    return paths;
}

QString checkBoxIndicatorCss() {
    const CheckBoxIndicatorPaths paths = ensureCheckBoxIndicatorImages();
    return QStringLiteral(
        "QCheckBox::indicator { width: 13px; height: 13px; }"
        "QCheckBox::indicator:unchecked { image: url(\"%1\"); }"
        "QCheckBox::indicator:checked { image: url(\"%2\"); }"
        "QCheckBox::indicator:unchecked:disabled { image: url(\"%3\"); }"
        "QCheckBox::indicator:checked:disabled { image: url(\"%4\"); }"
    ).arg(paths.unchecked, paths.checked, paths.disabledUnchecked, paths.disabledChecked);
}

QString panelCheckBoxStyleSheet() {
    return QStringLiteral(
        "QCheckBox, QRadioButton {"
        "  background: transparent;"
        "  color: #000000;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 8.25pt;"
        "  spacing: 4px;"
        "}"
        "QCheckBox:disabled, QRadioButton:disabled { color: rgba(0, 0, 0, 128); }"
    ) + checkBoxIndicatorCss();
}

void setPanelChildOpacity(QWidget *widget, bool enabled) {
    if (!widget) {
        return;
    }
    if (enabled) {
        widget->setGraphicsEffect(nullptr);
        return;
    }
    auto *effect = qobject_cast<QGraphicsOpacityEffect *>(widget->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
    }
    effect->setOpacity(0.45);
}

QString whiteScrollBarCss(const QString &widgetSelector) {
    return QStringLiteral(
        "%1 QScrollBar:vertical {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  width: 14px;"
        "  margin: 0px;"
        "}"
        "%1 QScrollBar::handle:vertical {"
        "  background-color: #c1c1c1;"
        "  background-image: none;"
        "  min-height: 20px;"
        "  border: none;"
        "}"
        "%1 QScrollBar::add-line:vertical, %1 QScrollBar::sub-line:vertical {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  height: 14px;"
        "  subcontrol-origin: margin;"
        "}"
        "%1 QScrollBar::up-arrow:vertical, %1 QScrollBar::down-arrow:vertical {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  width: 14px;"
        "  height: 14px;"
        "}"
        "%1 QScrollBar::add-page:vertical, %1 QScrollBar::sub-page:vertical {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "}"
        "%1 QScrollBar:horizontal {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  height: 14px;"
        "  margin: 0px;"
        "}"
        "%1 QScrollBar::handle:horizontal {"
        "  background-color: #c1c1c1;"
        "  background-image: none;"
        "  min-width: 20px;"
        "  border: none;"
        "}"
        "%1 QScrollBar::add-line:horizontal, %1 QScrollBar::sub-line:horizontal {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  width: 14px;"
        "  subcontrol-origin: margin;"
        "}"
        "%1 QScrollBar::left-arrow:horizontal, %1 QScrollBar::right-arrow:horizontal {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "  border: none;"
        "  width: 14px;"
        "  height: 14px;"
        "}"
        "%1 QScrollBar::add-page:horizontal, %1 QScrollBar::sub-page:horizontal {"
        "  background-color: #ffffff;"
        "  background-image: none;"
        "}"
    ).arg(widgetSelector);
}

class AdminUsersItemDelegate final : public QStyledItemDelegate {
public:
    explicit AdminUsersItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyledItemDelegate::paint(painter, option, index);
        if (!index.isValid() || index.column() == 0) {
            return;
        }

        painter->save();
        painter->setPen(QPen(Qt::white, 1));
        const QRect rect = option.rect;
        painter->drawLine(rect.bottomLeft(), rect.bottomRight());
        if (index.column() == 1 || index.column() == 2) {
            painter->drawLine(rect.topRight(), rect.bottomRight());
        }
        painter->restore();
    }
};

class WhiteComboHost final : public QWidget {
public:
    explicit WhiteComboHost(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        painter.setPen(QColor(0x7f, 0x9d, 0xb9));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
        QWidget::paintEvent(event);
    }
};

class WhitePanelWidget final : public QWidget {
public:
    explicit WhitePanelWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        QWidget::paintEvent(event);
    }
};

class ExercisesTreeHost final : public QWidget {
public:
    explicit ExercisesTreeHost(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        painter.setPen(QColor(0x7f, 0x9d, 0xb9));
        painter.drawRect(QRect(29, 55, 899, 944));
        QWidget::paintEvent(event);
    }
};

class ViewportWhitePaintFilter final : public QObject {
public:
    explicit ViewportWhitePaintFilter(QObject *parent = nullptr) : QObject(parent) {}

    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Paint) {
            if (auto *widget = qobject_cast<QWidget *>(watched)) {
                QPainter painter(widget);
                painter.fillRect(widget->rect(), Qt::white);
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

class WorkPanelWidget final : public QWidget {
public:
    explicit WorkPanelWidget(QWidget *parent = nullptr, const QColor &background = QColor(0xf2, 0xf0, 0xf0))
        : QWidget(parent), m_background(background) {
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), m_background);
        QWidget::paintEvent(event);
    }

private:
    QColor m_background;
};

class GrayTitleLabel final : public QLabel {
public:
    explicit GrayTitleLabel(QWidget *parent = nullptr) : QLabel(parent) {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(0xf2, 0xf0, 0xf0));
        QLabel::paintEvent(event);
    }
};

QString normalizeAnamnesisHtmlFonts(QString html) {
    html.replace(QStringLiteral("Calibri"), QStringLiteral("Times New Roman"));
    html.replace(QStringLiteral("Times New Roman CYR"), QStringLiteral("Times New Roman"));
    html.replace(
        QRegularExpression(QStringLiteral("font-size:\\s*\\d+(?:\\.\\d+)?pt")),
        QStringLiteral("font-size:12pt")
    );
    html.replace(
        QRegularExpression(QStringLiteral("line-height:\\s*[^;\"']+")),
        QStringLiteral("line-height:85%")
    );
    html.replace(
        QRegularExpression(QStringLiteral("margin-top:\\s*[^;\"']+")),
        QStringLiteral("margin-top:0pt")
    );
    html.replace(
        QRegularExpression(QStringLiteral("margin-bottom:\\s*[^;\"']+")),
        QStringLiteral("margin-bottom:0pt")
    );
    return html;
}

QByteArray compactAnamnesisRtf(const QByteArray &rtf) {
    QString data = QString::fromLatin1(rtf);
    data.replace(QRegularExpression(QStringLiteral("\\\\sa\\d+")), QStringLiteral("\\sa0"));
    data.replace(QRegularExpression(QStringLiteral("\\\\sb\\d+")), QStringLiteral("\\sb0"));
    data.replace(QStringLiteral("\\sl259\\slmult1"), QStringLiteral("\\sl200\\slmult0"));
    data.replace(QStringLiteral("\\sl276\\slmult1"), QStringLiteral("\\sl200\\slmult0"));
    data.replace(QStringLiteral("\\sl240\\slmult0"), QStringLiteral("\\sl200\\slmult0"));
    return data.toLatin1();
}

bool isRawRtfPlainText(const QString &plainText) {
    return plainText.trimmed().startsWith(QStringLiteral("{\\rtf"));
}

bool isAnamnesisTemplateLoaded(const QTextEdit *edit) {
    if (!edit) {
        return false;
    }
    const QString plain = edit->toPlainText().trimmed();
    if (plain.isEmpty() || isRawRtfPlainText(plain)) {
        return false;
    }
    return plain.contains(QStringLiteral("Психологический"))
        || plain.contains(QStringLiteral("Ф.И.О."))
        || plain.contains(QStringLiteral("Анамнез"));
}

bool isWordExportHtml(const QString &html) {
    return html.contains(QStringLiteral("schemas-microsoft-com:office"))
        || html.contains(QStringLiteral("ProgId content=\"Word.Document\""))
        || html.contains(QStringLiteral("class=MsoNormal"));
}

QString extractHtmlBodyContent(const QString &html) {
    const int bodyStart = html.indexOf(QStringLiteral("<body"), 0, Qt::CaseInsensitive);
    if (bodyStart < 0) {
        return html;
    }
    const int contentStart = html.indexOf(QLatin1Char('>'), bodyStart);
    if (contentStart < 0) {
        return html;
    }
    const int bodyEnd = html.indexOf(QStringLiteral("</body>"), contentStart, Qt::CaseInsensitive);
    if (bodyEnd < 0) {
        return html;
    }
    return html.mid(contentStart + 1, bodyEnd - contentStart - 1);
}

QString stripWordArtifacts(QString html) {
    html.remove(QStringLiteral("<o:p></o:p>"));
    html.remove(QStringLiteral("<o:p/>"));
    while (true) {
        const int start = html.indexOf(QStringLiteral("<o:p"));
        if (start < 0) {
            break;
        }
        const int end = html.indexOf(QStringLiteral("</o:p>"), start);
        if (end < 0) {
            break;
        }
        html.remove(start, end + 6 - start);
    }
    return html;
}

QString prepareAnamnesisHtml(QString html) {
    if (html.size() > 50000 || isWordExportHtml(html)) {
        html = stripWordArtifacts(extractHtmlBodyContent(html));
    }

    if (html.size() > 150000) {
        return {};
    }

    html = normalizeAnamnesisHtmlFonts(html);

    if (!html.contains(QStringLiteral("<html"), Qt::CaseInsensitive)) {
        html = QStringLiteral(
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<style>"
            "body { font-family: 'Times New Roman', serif; font-size: 12pt; color: #000000; background-color: #ffffff; }"
            "p { margin-top: 0pt; margin-bottom: 0pt; line-height: 85%; }"
            "</style></head><body>%1</body></html>"
        ).arg(html);
    }

    return html;
}

QString extractHelpStylesheet(const QString &html);
QString simplifyHelpInlineStyle(QString declarations);
QHash<QString, QString> parseHelpCssClassRules(const QString &stylesheet);
void appendStyleAttribute(QString *openTag, const QString &styleToAdd);
void applyHelpClassStylesToHtml(QString &html, const QHash<QString, QString> &classRules);
void reinforceHelpRichTextTags(QString &html);
QString buildHelpDefaultStylesheet(const QString &html);
void compactHelpSpacingStyles(QString &html);
void compactHelpDocumentSpacing(QTextDocument *doc);
QString normalizeHelpMarginsInCss(QString css);

QString prepareHelpHtml(QString html) {
    const QString stylesheet = extractHelpStylesheet(html);
    const QHash<QString, QString> classRules = parseHelpCssClassRules(stylesheet);
    applyHelpClassStylesToHtml(html, classRules);
    compactHelpSpacingStyles(html);
    reinforceHelpRichTextTags(html);
    return html;
}

QString extractHelpStylesheet(const QString &html) {
    const int styleStart = html.indexOf(QStringLiteral("<style"), 0, Qt::CaseInsensitive);
    if (styleStart < 0) {
        return {};
    }
    const int contentStart = html.indexOf(QLatin1Char('>'), styleStart);
    if (contentStart < 0) {
        return {};
    }
    const int styleEnd = html.indexOf(QStringLiteral("</style>"), contentStart, Qt::CaseInsensitive);
    if (styleEnd < 0) {
        return {};
    }
    QString css = html.mid(contentStart + 1, styleEnd - contentStart - 1);
    css.replace(QStringLiteral("<!--"), QString());
    css.replace(QStringLiteral("-->"), QString());
    return css;
}

QString normalizeHelpMarginsInCss(QString css) {
    css.replace(
        QRegularExpression(QStringLiteral("margin:\\s*0px\\s+0px\\s+(\\d+)px\\s+(\\d+)px\\s*;?")),
        QStringLiteral("margin-top:0;margin-bottom:\\1px;margin-left:\\2px;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin:\\s*0px\\s+0px\\s+(\\d+)px\\s+0px\\s*;?")),
        QStringLiteral("margin-top:0;margin-bottom:\\1px;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin:\\s*0px\\s+0px\\s+0px\\s+(\\d+)px\\s*;?")),
        QStringLiteral("margin-top:0;margin-left:\\1px;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin:\\s*0px\\s+0px\\s+0px\\s+0px\\s*;?")),
        QStringLiteral("margin:0;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin-top:\\s*\\d+px\\s*;?")),
        QStringLiteral("margin-top:0;"));
    css.replace(
        QRegularExpression(QStringLiteral("margin-bottom:\\s*11px\\s*;?")),
        QStringLiteral("margin-bottom:8px;"));
    return css;
}

QString simplifyHelpInlineStyle(QString declarations) {
    declarations = declarations.trimmed();
    if (declarations.isEmpty()) {
        return {};
    }

    static const QRegularExpression dropRule(
        QStringLiteral("(widows|orphans|text-justify|text-align-last)\\s*:[^;\"']*;?"),
        QRegularExpression::CaseInsensitiveOption);
    declarations.remove(dropRule);

    declarations = normalizeHelpMarginsInCss(declarations);
    declarations.replace(
        QRegularExpression(QStringLiteral("line-height:\\s*[^;\"']+\\s*;?")),
        QStringLiteral("line-height:100%;"));
    declarations.replace(
        QRegularExpression(QStringLiteral("(text-align|font-weight|font-style|color|text-decoration)\\s*:\\s*"),
                           QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("\\1:"));

    declarations.replace(QStringLiteral("'Calibri Light'"), QStringLiteral("'DejaVu Sans','Liberation Sans',sans-serif"));
    declarations.replace(QStringLiteral("'Calibri'"), QStringLiteral("'DejaVu Sans','Liberation Sans',sans-serif"));
    declarations.replace(QStringLiteral("'Arial', 'Helvetica', sans-serif"), QStringLiteral("sans-serif"));

    while (declarations.contains(QStringLiteral(";;"))) {
        declarations.replace(QStringLiteral(";;"), QStringLiteral(";"));
    }
    return declarations.trimmed();
}

QHash<QString, QString> parseHelpCssClassRules(const QString &stylesheet) {
    QHash<QString, QString> rules;
    if (stylesheet.isEmpty()) {
        return rules;
    }

    QRegularExpression blockRe(
        QStringLiteral("([^{]+)\\{([^}]*)\\}"),
        QRegularExpression::CaseInsensitiveOption);
    auto it = blockRe.globalMatch(stylesheet);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QStringList selectors = match.captured(1).split(QLatin1Char(','), Qt::SkipEmptyParts);
        const QString inlineStyle = simplifyHelpInlineStyle(match.captured(2));
        if (inlineStyle.isEmpty()) {
            continue;
        }
        for (QString selector : selectors) {
            selector = selector.trimmed();
            const QRegularExpression classRe(
                QStringLiteral("\\.(rv(?:ts|ps)\\d+)\\b"),
                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch classMatch = classRe.match(selector);
            if (!classMatch.hasMatch()) {
                continue;
            }
            const QString className = classMatch.captured(1);
            if (rules.contains(className)) {
                rules[className] = rules.value(className) + QLatin1Char(';') + inlineStyle;
            } else {
                rules.insert(className, inlineStyle);
            }
        }
    }
    return rules;
}

void appendStyleAttribute(QString *openTag, const QString &styleToAdd) {
    if (styleToAdd.isEmpty()) {
        return;
    }
    if (openTag->contains(QStringLiteral("style="), Qt::CaseInsensitive)) {
        static const QRegularExpression styleAttrRe(
            QStringLiteral(R"rx(style="([^"]*)")rx"),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch styleMatch = styleAttrRe.match(*openTag);
        if (styleMatch.hasMatch()) {
            QString merged = styleMatch.captured(1).trimmed();
            if (!merged.isEmpty() && !merged.endsWith(QLatin1Char(';'))) {
                merged += QLatin1Char(';');
            }
            merged += styleToAdd;
            openTag->replace(
                styleMatch.capturedStart(0),
                styleMatch.capturedLength(0),
                QStringLiteral("style=\"") + merged + QLatin1Char('"'));
        }
    } else {
        *openTag += QStringLiteral(" style=\"") + styleToAdd + QLatin1Char('"');
    }
}

void applyHelpClassStylesToHtml(QString &html, const QHash<QString, QString> &classRules) {
    if (classRules.isEmpty()) {
        return;
    }

    for (auto it = classRules.constBegin(); it != classRules.constEnd(); ++it) {
        const QString className = it.key();
        const QString inlineStyle = it.value();
        if (inlineStyle.isEmpty()) {
            continue;
        }

        const QRegularExpression tagRe(
            QString(
                R"re(<(?:(span|p|a|div|li|ul|ol|h[1-6]))([^>]*?\bclass=(?:%1|"%1"|'%1')\b)([^>]*?)(>))re")
                .arg(QRegularExpression::escape(className)),
            QRegularExpression::CaseInsensitiveOption);
        if (!tagRe.isValid()) {
            continue;
        }

        QString rebuilt;
        int offset = 0;
        auto tagIt = tagRe.globalMatch(html);
        while (tagIt.hasNext()) {
            const QRegularExpressionMatch match = tagIt.next();
            rebuilt += html.mid(offset, match.capturedStart(0) - offset);

            QString openTag = match.captured(1) + match.captured(2) + match.captured(3);
            appendStyleAttribute(&openTag, inlineStyle);

            if (match.captured(1).compare(QStringLiteral("p"), Qt::CaseInsensitive) == 0
                && inlineStyle.contains(QStringLiteral("text-align:center"), Qt::CaseInsensitive)
                && !openTag.contains(QStringLiteral("align="), Qt::CaseInsensitive)) {
                openTag += QStringLiteral(" align=\"center\"");
            }

            rebuilt += QLatin1Char('<') + openTag + match.captured(4);
            offset = match.capturedEnd(0);
        }
        if (offset > 0) {
            rebuilt += html.mid(offset);
            html = rebuilt;
        }
    }
}

void reinforceHelpRichTextTags(QString &html) {
    static const QRegularExpression styledInline(
        QStringLiteral(R"rx(<(span|a|p)([^>]*style="([^"]*)"[^>]*>([^<]+)</\1>)rx"),
        QRegularExpression::CaseInsensitiveOption);
    if (!styledInline.isValid()) {
        return;
    }

    auto styledIt = styledInline.globalMatch(html);
    if (!styledIt.hasNext()) {
        return;
    }

    int offset = 0;
    QString result;
    styledIt = styledInline.globalMatch(html);
    while (styledIt.hasNext()) {
        const QRegularExpressionMatch match = styledIt.next();
        result += html.mid(offset, match.capturedStart(0) - offset);

        const QString tag = match.captured(1);
        const QString attrs = match.captured(2);
        const QString style = match.captured(3);
        QString text = match.captured(4);

        static const QRegularExpression boldRe(
            QStringLiteral("font-weight\\s*:\\s*(?:bold|700)"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression italicRe(
            QStringLiteral("font-style\\s*:\\s*italic"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression underlineRe(
            QStringLiteral("text-decoration\\s*:\\s*underline"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression colorRe(
            QStringLiteral("color\\s*:\\s*(#[0-9a-fA-F]{3,6})"),
            QRegularExpression::CaseInsensitiveOption);

        if (boldRe.match(style).hasMatch()) {
            text = QStringLiteral("<b>") + text + QStringLiteral("</b>");
        }
        if (italicRe.match(style).hasMatch()) {
            text = QStringLiteral("<i>") + text + QStringLiteral("</i>");
        }
        if (underlineRe.match(style).hasMatch()) {
            text = QStringLiteral("<u>") + text + QStringLiteral("</u>");
        }
        const QRegularExpressionMatch colorMatch = colorRe.match(style);
        if (colorMatch.hasMatch()) {
            text = QStringLiteral("<font color=\"") + colorMatch.captured(1) + QStringLiteral("\">")
                + text + QStringLiteral("</font>");
        }

        result += QLatin1Char('<') + tag + attrs + QLatin1Char('>') + text + QStringLiteral("</") + tag
            + QLatin1Char('>');
        offset = match.capturedEnd(0);
    }
    result += html.mid(offset);
    html = result;
}

void compactHelpSpacingStyles(QString &html) {
    const int styleStart = html.indexOf(QStringLiteral("<style"), 0, Qt::CaseInsensitive);
    if (styleStart < 0) {
        return;
    }
    const int contentStart = html.indexOf(QLatin1Char('>'), styleStart);
    if (contentStart < 0) {
        return;
    }
    const int styleEnd = html.indexOf(QStringLiteral("</style>"), contentStart, Qt::CaseInsensitive);
    if (styleEnd < 0) {
        return;
    }

    QString css = html.mid(contentStart + 1, styleEnd - contentStart - 1);
    css.replace(
        QRegularExpression(QStringLiteral("line-height:\\s*[^;\"']+")),
        QStringLiteral("line-height:100%"));
    css = normalizeHelpMarginsInCss(css);
    css.replace(
        QStringLiteral("margin: 48px 48px 48px 48px"),
        QStringLiteral("margin: 8px"));

    html.replace(contentStart + 1, styleEnd - contentStart - 1, css);
}

QString buildHelpDefaultStylesheet(const QString &html) {
    QString css = extractHelpStylesheet(html);
    css.replace(QRegularExpression(QStringLiteral("<!--|-->")), QString());
    css.replace(
        QRegularExpression(
            QStringLiteral("(widows|orphans|text-justify|text-align-last)\\s*:[^;\"']*;?"),
            QRegularExpression::CaseInsensitiveOption),
        QString());
    css.replace(QStringLiteral("'Calibri Light'"), QStringLiteral("'DejaVu Sans','Liberation Sans',sans-serif"));
    css.replace(QStringLiteral("'Calibri'"), QStringLiteral("'DejaVu Sans','Liberation Sans',sans-serif"));
    css.replace(
        QRegularExpression(QStringLiteral("(span|a|p|div|li|body|table|ul|ol)\\.rv")),
        QStringLiteral(".rv"));
    css += QStringLiteral(
        "body { margin: 8px; line-height: 100%; }"
        "p, ul, ol { margin-top: 0; margin-bottom: 8px; line-height: 100%; }"
        "a { color: #0563c1; text-decoration: underline; }"
    );
    return css;
}

void compactHelpDocumentSpacing(QTextDocument *doc) {
    if (!doc || doc->isEmpty()) {
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

class HelpTextBrowser final : public QTextBrowser {
public:
    using LinkHandler = std::function<void(const QUrl &)>;

    explicit HelpTextBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {}

    void setLinkHandler(LinkHandler handler) {
        m_linkHandler = std::move(handler);
    }

protected:
    void setSource(const QUrl &name) override {
        if (m_linkHandler) {
            m_linkHandler(name);
            return;
        }
        QTextBrowser::setSource(name);
    }

private:
    LinkHandler m_linkHandler;
};

QString loadHelpHtmlFromFile(const QString &path, QString *rawSource = nullptr) {
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QString source = QString::fromUtf8(file.readAll());
    if (source.isEmpty()) {
        return {};
    }
    if (rawSource) {
        *rawSource = source;
    }
    return prepareHelpHtml(source);
}

class HelpWindowDragFilter final : public QObject {
public:
    explicit HelpWindowDragFilter(QDialog *dialog, int titleHeight, QObject *parent = nullptr)
        : QObject(parent), m_dialog(dialog), m_titleHeight(titleHeight) {
        dialog->installEventFilter(this);
    }

    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched != m_dialog || !m_dialog) {
            return QObject::eventFilter(watched, event);
        }

        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() != Qt::LeftButton || mouseEvent->pos().y() >= m_titleHeight) {
                break;
            }
            QWidget *child = m_dialog->childAt(mouseEvent->pos());
            while (child && child != m_dialog) {
                if (qobject_cast<QAbstractButton *>(child)) {
                    return QObject::eventFilter(watched, event);
                }
                child = child->parentWidget();
            }
            m_dragging = true;
            m_dragOffset = mouseEvent->globalPos() - m_dialog->frameGeometry().topLeft();
            return true;
        }
        case QEvent::MouseMove:
            if (m_dragging) {
                auto *mouseEvent = static_cast<QMouseEvent *>(event);
                m_dialog->move(mouseEvent->globalPos() - m_dragOffset);
                return true;
            }
            break;
        case QEvent::MouseButtonRelease:
            m_dragging = false;
            break;
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QDialog *m_dialog = nullptr;
    int m_titleHeight = 110;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

QString loadAnamnesisTemplateFromFile(const QString &path) {
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray raw = file.readAll();
    if (raw.isEmpty()) {
        return {};
    }

    const QString source = QString::fromUtf8(raw);
    if (path.endsWith(QStringLiteral("anamnez_clean.html"), Qt::CaseInsensitive)) {
        return prepareAnamnesisHtml(source);
    }

#ifndef Q_OS_WIN
    if (raw.size() > 50000 || isWordExportHtml(source)) {
        return {};
    }
#endif

    return prepareAnamnesisHtml(source);
}

QString resolveHtmlAssetPath(const QString &name) {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/assets/htmls",
        QCoreApplication::applicationDirPath() + "/../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../../assets/htmls",
        QDir::currentPath() + "/assets/htmls",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/htmls"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString readAnamnesisTemplateHtml() {
    const QString preparedClean = loadAnamnesisTemplateFromFile(
        resolveHtmlAssetPath(QStringLiteral("anamnez_clean.html"))
    );
    if (!preparedClean.trimmed().isEmpty()) {
        return preparedClean;
    }

#ifndef Q_OS_WIN
    return {};
#else
    return loadAnamnesisTemplateFromFile(resolveHtmlAssetPath(QStringLiteral("anamnez.html")));
#endif
}

} // namespace

class PatientsItemDelegate final : public QStyledItemDelegate {
public:
    explicit PatientsItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void setSearchText(const QString &search) { m_search = search.trimmed(); }
    void setHoverRow(int row) { m_hoverRow = row; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem itemOption = option;
        initStyleOption(&itemOption, index);

        if (index.row() == m_hoverRow) {
            painter->fillRect(option.rect, QColor(0xD3, 0xD3, 0xD3));
        } else if (itemOption.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, itemOption.palette.highlight());
        } else {
            painter->fillRect(option.rect, itemOption.palette.base());
        }

        if (index.column() == 3) {
            itemOption.displayAlignment = Qt::AlignCenter;
            QStyledItemDelegate::paint(painter, itemOption, index);
            return;
        }

        if (index.column() == 1 && !m_search.isEmpty()) {
            const QString text = index.data(Qt::DisplayRole).toString();
            const QStringList parts = text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            int x = option.rect.left() + 4;
            const int y = option.rect.center().y();
            QFont font = itemOption.font;
            font.setBold(false);
            font.setWeight(QFont::Normal);
            painter->setFont(font);
            const QFontMetrics fm(font);
            const QColor textColor = itemOption.state & QStyle::State_Selected
                ? itemOption.palette.color(QPalette::HighlightedText)
                : itemOption.palette.color(QPalette::Text);

            for (int partIndex = 0; partIndex < parts.size(); ++partIndex) {
                if (partIndex > 0) {
                    painter->setPen(textColor);
                    painter->drawText(x, y, QStringLiteral(" "));
                    x += fm.horizontalAdvance(QStringLiteral(" "));
                }

                const QString part = parts.at(partIndex);
                const bool prefixMatch = part.startsWith(m_search, Qt::CaseInsensitive);
                const int highlightLen = prefixMatch ? qMin(m_search.size(), part.size()) : 0;

                if (highlightLen > 0) {
                    const QString highlighted = part.left(highlightLen);
                    const QString rest = part.mid(highlightLen);
                    const int highlightWidth = fm.horizontalAdvance(highlighted);
                    if (!(itemOption.state & QStyle::State_Selected)) {
                        painter->fillRect(
                            x,
                            y - fm.ascent(),
                            highlightWidth,
                            fm.height(),
                            QColor(0xFF, 0xFF, 0x00)
                        );
                    }
                    painter->setPen(textColor);
                    painter->drawText(x, y, highlighted);
                    x += highlightWidth;
                    if (!rest.isEmpty()) {
                        painter->drawText(x, y, rest);
                        x += fm.horizontalAdvance(rest);
                    }
                } else {
                    painter->setPen(textColor);
                    painter->drawText(x, y, part);
                    x += fm.horizontalAdvance(part);
                }
            }
            return;
        }

        QStyledItemDelegate::paint(painter, itemOption, index);
    }

private:
    QString m_search;
    int m_hoverRow = -1;
};

InfantWindow::InfantWindow(const QString &licenseKey, bool openAdminOnStart, QWidget *parent)
    : QMainWindow(parent), m_repository(&m_api, this), m_licenseKey(licenseKey) {
    buildUi();
    applyLegacyStyle();
    bindSignals();
    if (openAdminOnStart) {
        m_mainId.clear();
        m_userRole->setCurrentText("Администратор");
        setScreen(ScreenMode::Admin);
    } else {
        setScreen(ScreenMode::Enter);
    }
}

void InfantWindow::buildUi() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    resize(kDesignWidth, kDesignHeight);
    setMinimumSize(1024, 768);
    setWindowTitle("Инфант");
    const QString iconFile = resourcePath("infant.ico");
    if (!iconFile.isEmpty()) {
        setWindowIcon(QIcon(iconFile));
    }

    m_root = new QWidget(this);
    setCentralWidget(m_root);

    m_bClose = new ImageButton(m_root);
    m_bClose->setGeometry(1878, 10, 36, 34);
    m_bLine = new ImageButton(m_root);
    m_bLine->setGeometry(1794, 10, 36, 34);
    m_bUp = new ImageButton(m_root);
    m_bUp->setGeometry(1836, 10, 36, 34);
    m_bBack = new ImageButton(m_root);
    m_bBack->setGeometry(8, 12, 36, 34);
    m_bList = new ImageButton(m_root);
    m_bList->setGeometry(50, 12, 166, 34);
    m_bExit = new ImageButton(m_root);
    m_bExit->setGeometry(1683, 10, 36, 34);
    m_bPicPrint = new ImageButton(m_root);
    m_bPicPrint->setGeometry(1440, 10, 36, 34);
    m_bUpload = new ImageButton(m_root);
    m_bUpload->setGeometry(1398, 10, 36, 34);
    m_bSave = new ImageButton(m_root);
    m_bSave->setGeometry(1490, 10, 36, 34);
    m_bPrint = new ImageButton(m_root);
    m_bPrint->setGeometry(1532, 10, 36, 34);
    m_bSettings = new ImageButton(m_root);
    m_bSettings->setGeometry(1574, 10, 36, 34);
    m_bInfo = new ImageButton(m_root);
    m_bInfo->setGeometry(1617, 10, 36, 34);

    m_pAna = new ImageButton(m_root);
    m_pAna->setGeometry(745, 24, 143, 34);
    m_pProto = new ImageButton(m_root);
    m_pProto->setGeometry(889, 24, 143, 34);
    m_pUpr = new ImageButton(m_root);
    m_pUpr->setGeometry(1033, 24, 143, 34);

    m_logo1 = new QLabel(m_root);
    m_logo1->setAttribute(Qt::WA_TranslucentBackground, true);
    m_logo1->setStyleSheet("background: transparent;");
    m_logo2 = new QLabel(m_root);
    m_logo2->setAttribute(Qt::WA_TranslucentBackground, true);
    m_logo2->setStyleSheet("background: transparent;");

    m_panelLogin = new QWidget(m_root);
    m_panelLogin->setGeometry((1920 - 500) / 2, (1080 - 176) / 2, 500, 176);
    m_panelLogin->setAttribute(Qt::WA_StyledBackground, true);
    m_panelLogin->setStyleSheet("background: transparent;");
    m_loginEdit = new QLineEdit(m_panelLogin);
    m_loginEdit->setGeometry(75, 22, 393, 29);
    m_passwordEdit = new QLineEdit(m_panelLogin);
    m_passwordEdit->setGeometry(75, 75, 258, 29);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_loginClear = new ImageButton(m_panelLogin);
    m_loginClear->setGeometry(440, 26, 23, 22);
    m_loginClear->setStyleSheet("background:white;");
    m_loginClear->hide();
    m_passwordClear = new ImageButton(m_panelLogin);
    m_passwordClear->setGeometry(305, 79, 23, 22);
    m_passwordClear->setStyleSheet("background:white;");
    m_passwordClear->hide();
    m_loginManIcon = new QLabel(m_panelLogin);
    m_loginManIcon->setGeometry(47, 23, 24, 26);
    m_loginManIcon->setScaledContents(true);
    m_loginKeyIcon = new QLabel(m_panelLogin);
    m_loginKeyIcon->setGeometry(47, 76, 24, 26);
    m_loginKeyIcon->setScaledContents(true);
    m_loginEye = new ImageButton(m_panelLogin);
    m_loginEye->setGeometry(342, 77, 35, 23);
    m_loginButton = new ImageButton(m_panelLogin);
    m_loginButton->setGeometry(387, 71, 80, 36);
    m_adminButton = new ImageButton(m_panelLogin);
    m_adminButton->setGeometry(125, 129, 247, 37);

    m_adminTitle = new QLabel("Администрирование", m_root);
    m_adminTitle->setGeometry(660, 58, 600, 36);
    m_adminTitle->setAlignment(Qt::AlignCenter);
    m_adminTitle->hide();

    m_panelAdmin = new QWidget(m_root);
    m_panelAdmin->setGeometry(80, 100, 1760, 880);
    m_panelAdmin->setAttribute(Qt::WA_StyledBackground, true);
    m_panelAdmin->setStyleSheet("background: transparent;");

    constexpr int kAdminTableW = 536;
    constexpr int kAdminFormW = 340;
    constexpr int kAdminFieldH = 32;
    constexpr int kAdminGap = 80;
    constexpr int kAdminBlockW = kAdminTableW + kAdminGap + kAdminFormW;
    constexpr int kAdminBlockX = (1760 - kAdminBlockW) / 2;
    constexpr int kAdminTableX = kAdminBlockX - 40;
    constexpr int kAdminFormX = kAdminBlockX + kAdminTableW + kAdminGap;
    constexpr int kAdminIconX = kAdminFormX - 44;
    constexpr int kAdminFirstFieldY = 102;
    constexpr int kAdminFieldStep = 44;
    constexpr int kAdminSaveY = 334;

    m_usersTable = new QTableWidget(m_panelAdmin);
    m_usersTable->setGeometry(kAdminTableX, 45, kAdminTableW, 120);
    m_usersTable->setColumnCount(4);
    m_usersTable->setHorizontalHeaderLabels({"id", "ФИО", "Логин", "Уровень доступа"});
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_usersTable->setFrameShape(QFrame::NoFrame);
    m_usersTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_usersTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_usersTable->hideColumn(0);
    m_usersTable->setItemDelegate(new AdminUsersItemDelegate(m_usersTable));

    m_adminLabel1 = new QLabel("Создать пользователя", m_panelAdmin);
    m_adminLabel1->setGeometry(kAdminFormX, 40, kAdminFormW, 24);
    m_adminLabel1->setAlignment(Qt::AlignCenter);
    m_adminLabel2 = new QLabel("Уровень доступа", m_panelAdmin);
    m_adminLabel2->setGeometry(kAdminFormX, kAdminFirstFieldY + kAdminFieldStep * 3 + 44, 154, 24);
    m_adminLabel2->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_adminManIcon = new QLabel(m_panelAdmin);
    m_adminManIcon->setGeometry(kAdminIconX, kAdminFirstFieldY - 6, 40, 44);
    m_adminManIcon->setScaledContents(true);
    m_adminLoginIcon = new QLabel(m_panelAdmin);
    m_adminLoginIcon->setGeometry(kAdminIconX + 4, kAdminFirstFieldY + kAdminFieldStep - 2, 32, 33);
    m_adminLoginIcon->setScaledContents(true);
    m_adminKeyIcon1 = new QLabel(m_panelAdmin);
    m_adminKeyIcon1->setGeometry(kAdminIconX + 4, kAdminFirstFieldY + kAdminFieldStep * 2 - 2, 32, 33);
    m_adminKeyIcon1->setScaledContents(true);
    m_adminKeyIcon2 = new QLabel(m_panelAdmin);
    m_adminKeyIcon2->setGeometry(kAdminIconX + 4, kAdminFirstFieldY + kAdminFieldStep * 3 - 2, 32, 33);
    m_adminKeyIcon2->setScaledContents(true);

    m_userFioPanel = new QWidget(m_panelAdmin);
    m_userFioPanel->setGeometry(kAdminFormX, kAdminFirstFieldY, kAdminFormW, kAdminFieldH);
    m_userFioPanel->setAttribute(Qt::WA_StyledBackground, true);
    m_userFio = new QLineEdit(m_userFioPanel);
    m_userFio->setGeometry(8, 5, kAdminFormW - 42, 22);
    m_userFio->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_userFioClear = new ImageButton(m_userFioPanel);
    m_userFioClear->setGeometry(kAdminFormW - 30, 5, 23, 22);
    m_userFioClear->hide();

    m_userLoginPanel = new QWidget(m_panelAdmin);
    m_userLoginPanel->setGeometry(kAdminFormX, kAdminFirstFieldY + kAdminFieldStep, kAdminFormW, kAdminFieldH);
    m_userLoginPanel->setAttribute(Qt::WA_StyledBackground, true);
    m_userLogin = new QLineEdit(m_userLoginPanel);
    m_userLogin->setGeometry(8, 5, kAdminFormW - 42, 22);
    m_userLogin->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_userLoginClear = new ImageButton(m_userLoginPanel);
    m_userLoginClear->setGeometry(kAdminFormW - 30, 5, 23, 22);
    m_userLoginClear->hide();

    m_userPassPanel = new QWidget(m_panelAdmin);
    m_userPassPanel->setGeometry(kAdminFormX, kAdminFirstFieldY + kAdminFieldStep * 2, kAdminFormW, kAdminFieldH);
    m_userPassPanel->setAttribute(Qt::WA_StyledBackground, true);
    m_userPass = new QLineEdit(m_userPassPanel);
    m_userPass->setGeometry(8, 5, kAdminFormW - 42, 22);
    m_userPass->setEchoMode(QLineEdit::Password);
    m_userPassClear = new ImageButton(m_userPassPanel);
    m_userPassClear->setGeometry(kAdminFormW - 30, 5, 23, 22);
    m_userPassClear->hide();
    m_adminEye1 = new ImageButton(m_panelAdmin);
    m_adminEye1->setGeometry(kAdminFormX + kAdminFormW + 8, kAdminFirstFieldY + kAdminFieldStep * 2 + 7, 35, 23);

    m_userPass2Panel = new QWidget(m_panelAdmin);
    m_userPass2Panel->setGeometry(kAdminFormX, kAdminFirstFieldY + kAdminFieldStep * 3, kAdminFormW, kAdminFieldH);
    m_userPass2Panel->setAttribute(Qt::WA_StyledBackground, true);
    m_userPass2 = new QLineEdit(m_userPass2Panel);
    m_userPass2->setGeometry(8, 5, kAdminFormW - 42, 22);
    m_userPass2->setEchoMode(QLineEdit::Password);
    m_userPass2Clear = new ImageButton(m_userPass2Panel);
    m_userPass2Clear->setGeometry(kAdminFormW - 30, 5, 23, 22);
    m_userPass2Clear->hide();
    m_adminEye2 = new ImageButton(m_panelAdmin);
    m_adminEye2->setGeometry(kAdminFormX + kAdminFormW + 8, kAdminFirstFieldY + kAdminFieldStep * 3 + 7, 35, 23);

    m_userRole = new QComboBox(m_panelAdmin);
    m_userRole->setGeometry(kAdminFormX + 164, kAdminFirstFieldY + kAdminFieldStep * 3 + 42, kAdminFormW - 164, 24);
    m_userRole->addItems({"Специалист", "Администратор"});

    m_userSaveButton = new ImageButton(m_panelAdmin);
    m_userSaveButton->setGeometry(kAdminFormX + (kAdminFormW - 101) / 2, kAdminSaveY, 101, 30);
    constexpr int kAdminEnterW = 100;
    m_userOpenPatients = new ImageButton(m_panelAdmin);
    m_userOpenPatients->setGeometry(kAdminFormX + kAdminFormW - kAdminEnterW, kAdminSaveY, kAdminEnterW, 30);

    m_dualScreenCheck = new QCheckBox(QStringLiteral("Два экрана"), m_panelAdmin);
    m_dualScreenCheck->setGeometry(80, 78, 220, 28);
    m_dualScreenCheck->setChecked(AppSettings::dualScreenEnabled());
    m_screenSettingsTitle = new QLabel(QStringLiteral("Настройка экранов"), m_panelAdmin);
    m_screenSettingsTitle->setGeometry(80, 48, 220, 24);
    m_screenSettingsTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_panelPatients = new QWidget(m_root);
    m_panelPatients->setGeometry((1920 - 749) / 2, 80, 749, 950);
    m_panelPatients->setAttribute(Qt::WA_StyledBackground, true);
    m_panelPatients->setStyleSheet("background: transparent;");
    m_patientSearch = new QLineEdit(m_panelPatients);
    m_patientSearch->setGeometry(14, 21, 290, 29);
    m_patientSearchClear = new ImageButton(m_panelPatients);
    m_patientSearchClear->setGeometry(281, 25, 23, 22);
    m_patientSearchClear->hide();
    m_addPatient = new ImageButton(m_panelPatients);
    m_addPatient->setGeometry(334, 21, 143, 29);
    m_dateFilter = new QCheckBox("Показать пациентов, добавленных", m_panelPatients);
    m_dateFilter->setGeometry(485, 31, 263, 19);
    m_dateFrom = new QDateEdit(QDate::currentDate().addMonths(-1), m_panelPatients);
    m_dateFrom->setGeometry(523, 57, 200, 20);
    m_dateTo = new QDateEdit(QDate::currentDate(), m_panelPatients);
    m_dateTo->setGeometry(523, 83, 200, 20);
    m_dateFrom->setCalendarPopup(true);
    m_dateTo->setCalendarPopup(true);
    m_dateFrom->setDisplayFormat("dd.MM.yyyy");
    m_dateTo->setDisplayFormat("dd.MM.yyyy");
    m_labelFrom = new QLabel("С", m_panelPatients);
    m_labelFrom->setGeometry(495, 63, 15, 13);
    m_labelTo = new QLabel("До", m_panelPatients);
    m_labelTo->setGeometry(495, 89, 24, 13);
    m_patientsTable = new QTableWidget(m_panelPatients);
    m_patientsTable->setGeometry(14, 63, 463, 800);
    m_patientsTable->setColumnCount(4);
    m_patientsTable->setHorizontalHeaderLabels({"id", "ФИО", "День рождения", "Уд."});
    m_patientsTable->hideColumn(0);
    m_patientsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_patientsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_patientsTable->setFrameShape(QFrame::StyledPanel);
    m_patientsTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_patientsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_patientsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_patientsTable->setFocusPolicy(Qt::StrongFocus);
    m_patientsTable->verticalHeader()->setVisible(false);
    m_patientsTable->verticalHeader()->setDefaultSectionSize(27);
    m_patientsDelegate = new PatientsItemDelegate(m_patientsTable);
    m_patientsTable->setItemDelegate(m_patientsDelegate);

    m_workStack = new QStackedWidget(m_root);
    m_workStack->setObjectName(QStringLiteral("workStack"));
    m_workStack->setGeometry((1920 - 965) / 2, 57, 965, 1000);
    m_workStack->setAutoFillBackground(true);
    m_workStack->setAttribute(Qt::WA_StyledBackground, true);
    m_workStack->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_workStack->setStyleSheet(QStringLiteral(
        "QStackedWidget#workStack { background-color: #ffffff; background-image: none; }"
    ));

    m_panelProtocols = new WorkPanelWidget(m_workStack);
    m_panelProtocols->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_panelProtocols->setGeometry(0, 0, 965, 1000);

    m_protocolsTitle = new QLabel(m_panelProtocols);
    m_protocolsTitle->setGeometry(100, 15, 760, 30);
    m_protocolsTitle->setText(QStringLiteral("Индивидуальная карта психологического развития ребенка"));
    m_protocolsTitle->setWordWrap(true);
    m_protocolsTitle->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:16pt; text-decoration: underline; background: transparent;"));

    m_protocolsPatient = new QLabel(m_panelProtocols);
    m_protocolsPatient->setGeometry(100, 50, 760, 20);
    m_protocolsPatient->setStyleSheet(QStringLiteral(
        "color:#000000; font-family:'Microsoft Sans Serif',sans-serif;"
        "font-size:9pt; background: transparent;"));

    m_protocolsView = new QTextEdit(m_panelProtocols);
    m_protocolsView->setReadOnly(false);
    m_protocolsView->setTextInteractionFlags(Qt::TextEditorInteraction);
    m_protocolsView->setFocusPolicy(Qt::StrongFocus);
    m_protocolsView->setGeometry(100, 99, 760, 889);
    m_protocolsView->setFrameShape(QFrame::NoFrame);
    m_protocolsView->setLineWidth(0);
    m_protocolsView->setAutoFillBackground(true);
    m_protocolsView->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_protocolsView->setAttribute(Qt::WA_StaticContents, false);
    m_protocolsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_protocolsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    auto *protocolsCorner = new QWidget(m_protocolsView);
    protocolsCorner->setAutoFillBackground(true);
    protocolsCorner->setStyleSheet(QStringLiteral("background-color:#ffffff; background-image:none;"));
    m_protocolsView->setCornerWidget(protocolsCorner);
    m_protocolsView->setStyleSheet(
        "QTextEdit { background-color: #ffffff; color: #000000; }"
    );
    if (m_protocolsView->viewport()) {
        m_protocolsView->viewport()->setAttribute(Qt::WA_StaticContents, false);
        m_protocolsView->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_protocolsView->viewport()->setAutoFillBackground(true);
        m_protocolsView->viewport()->setStyleSheet(
            QStringLiteral("background-color: #ffffff; background-image: none; color: #000000;")
        );
        m_protocolsView->viewport()->setCursor(Qt::ArrowCursor);
    }

    m_protocolsSaveTimer = new QTimer(this);
    m_protocolsSaveTimer->setSingleShot(true);
    m_protocolsSaveTimer->setInterval(700);
    connect(m_protocolsSaveTimer, &QTimer::timeout, this, [this]() { saveProtocolsEdits(false); });
    connect(m_protocolsView->document(), &QTextDocument::contentsChanged, this, [this]() {
        if (m_protocolsSuppressDirty) {
            return;
        }
        m_protocolsViewDirty = true;
        if (m_protocolsSaveTimer) {
            m_protocolsSaveTimer->start();
        }
    });
    ProtocolEditGuard::install(m_protocolsView, ProtocolEditGuard::Mode::LimitedEdit);

    m_panelExercises = new WhitePanelWidget(m_workStack);
    m_panelExercises->setObjectName(QStringLiteral("exercisesPanel"));
    m_panelExercises->setGeometry(0, 0, 965, 1000);
    m_authorsFilterHost = new WhiteComboHost(m_root);
    m_authorsFilterHost->setObjectName(QStringLiteral("authorsFilterHost"));
    m_authorsFilterHost->setGeometry(1202, 29, 232, 24);
    m_authorsFilterHost->hide();
    m_authorsFilter = new QComboBox(m_authorsFilterHost);
    m_authorsFilter->setObjectName(QStringLiteral("authorsFilter"));
    m_authorsFilter->setGeometry(1, 1, 230, 22);
    m_authorsFilter->setFrame(false);
    m_authorsFilter->addItems({
        QStringLiteral("Все авторы"),
        QStringLiteral("Забрамная С.Д., Боровик О.В."),
        QStringLiteral("Немов Р.С."),
        QStringLiteral("Фатихова Л.Ф."),
        QStringLiteral("Избранное")
    });
    m_exercisesTreeHost = new ExercisesTreeHost(m_panelExercises);
    m_exercisesTreeHost->setObjectName(QStringLiteral("exercisesTreeHost"));
    m_exercisesTreeHost->setGeometry(0, 0, 965, 1000);
    m_exercisesTree = new QTreeWidget(m_exercisesTreeHost);
    m_exercisesTree->setObjectName(QStringLiteral("exercisesTree"));
    m_exercisesTree->setGeometry(29, 55, 900, 945);
    m_exercisesTree->setHeaderHidden(true);
    m_exercisesTree->setRootIsDecorated(true);
    m_exercisesTree->setIndentation(20);
    m_exercisesTree->setUniformRowHeights(true);
    m_exercisesTree->setAnimated(false);
    m_exercisesTree->setAllColumnsShowFocus(false);
    m_exercisesTree->setFrameShape(QFrame::NoFrame);
    m_exercisesTree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_exercisesTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_exercisesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    if (QWidget *treeViewport = m_exercisesTree->viewport()) {
        treeViewport->setAttribute(Qt::WA_OpaquePaintEvent, true);
        treeViewport->setAutoFillBackground(true);
        treeViewport->installEventFilter(new ViewportWhitePaintFilter(m_exercisesTree));
    }

    m_panelWork = new WorkPanelWidget(m_workStack);
    m_panelWork->setGeometry(0, 0, 965, 1000);
    m_underlineButton = new ImageButton(m_panelWork);
    m_underlineButton->setGeometry(38, 15, 36, 34);
    m_boldButton = new ImageButton(m_panelWork);
    m_boldButton->setGeometry(80, 15, 36, 34);
    m_patientTitle = new GrayTitleLabel(m_root);
    m_patientTitle->setGeometry(478, 24, 237, 34);
    m_anamnesisEdit = new QTextEdit(m_panelWork);
    m_anamnesisEdit->setGeometry(29, 55, 900, 950);

    m_workStack->addWidget(m_panelWork);
    m_workStack->addWidget(m_panelProtocols);
    m_workStack->addWidget(m_panelExercises);
    m_workStack->setCurrentWidget(m_panelWork);
    m_workStack->hide();

    buildSlidePanels();

    const QWidgetList chromeWidgets = {
        m_bBack, m_bList, m_bExit, m_bSave, m_bPrint, m_bSettings, m_bInfo,
        m_bClose, m_bLine, m_bUp, m_pAna, m_pProto, m_pUpr,
        m_patientTitle, m_userOpenPatients, m_authorsFilterHost
    };
    for (QWidget *widget : chromeWidgets) {
        if (widget) {
            widget->raise();
        }
    }
}

void InfantWindow::buildSlidePanels() {
    const auto panelStyle = [this]() {
        const QString path = imagePath("popup3.png");
        if (!path.isEmpty()) {
            QString normalizedPath = path;
            normalizedPath.replace('\\', '/');
            return QStringLiteral(
                "QWidget#settingsPanel { background-image: url('%1'); background-repeat: no-repeat; }"
            ).arg(normalizedPath);
        }
        return QStringLiteral(
            "QWidget#settingsPanel { background-color: #ece9e9; border: 1px solid #808080; border-radius: 6px; }"
        );
    };
    const auto smallPanelStyle = [this](const char *objectName) {
        const QString path = imagePath("popup2.png");
        if (!path.isEmpty()) {
            QString normalizedPath = path;
            normalizedPath.replace('\\', '/');
            return QStringLiteral(
                "QWidget#%1 { background-image: url('%2'); background-repeat: no-repeat; }"
            ).arg(QString::fromLatin1(objectName), normalizedPath);
        }
        return QStringLiteral(
            "QWidget#%1 { background-color: #ece9e9; border: 1px solid #808080; border-radius: 6px; }"
        ).arg(QString::fromLatin1(objectName));
    };
    const QString transparentPanelStyle = QStringLiteral("background: transparent;");
    const QString panelCheckBoxStyle = panelCheckBoxStyleSheet();
    const QString boldTitleStyle = QStringLiteral(
        "QLabel { background: transparent; color: #000000; font-family: 'Microsoft Sans Serif'; font-size: 8.25pt; font-weight: bold; }"
    );
    const QString plainLabelStyle = QStringLiteral(
        "QLabel { background: transparent; color: #000000; font-family: 'Microsoft Sans Serif'; font-size: 8.25pt; }"
    );

    m_settingsPanel = new QWidget(m_root);
    m_settingsPanel->setObjectName(QStringLiteral("settingsPanel"));
    m_settingsPanel->setGeometry(1450, -200, 378, 190);
    m_settingsPanel->setStyleSheet(panelStyle());
    m_settingsPanel->hide();

    m_settingsMainView = new QWidget(m_settingsPanel);
    m_settingsMainView->setGeometry(0, 0, 378, 190);
    m_settingsMainView->setStyleSheet(transparentPanelStyle);
    m_settingsSaveView = new QWidget(m_settingsPanel);
    m_settingsSaveView->setGeometry(0, 0, 378, 190);
    m_settingsSaveView->setStyleSheet(transparentPanelStyle);
    m_settingsSaveView->hide();

    m_fontDownButton = new ImageButton(m_settingsMainView);
    m_fontDownButton->setGeometry(62, 13, 27, 23);
    setImage(m_fontDownButton, "fdown.png");
    m_fontUpButton = new ImageButton(m_settingsMainView);
    m_fontUpButton->setGeometry(275, 12, 27, 23);
    setImage(m_fontUpButton, "fup.png");
    m_fontSlider = new QSlider(Qt::Horizontal, m_settingsMainView);
    m_fontSlider->setGeometry(104, 12, 165, 30);
    m_fontSlider->setRange(21, 27);
    m_fontSlider->setValue(24);
    m_fontSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #ffffff; height: 8px; border: 1px solid #808080; }"
        "QSlider::handle:horizontal { background: #d4d0c8; width: 12px; margin: -4px 0; border: 1px solid #808080; }"
    );
    m_fontSizeLabel = new QLabel("24", m_settingsMainView);
    m_fontSizeLabel->setGeometry(178, 44, 30, 16);
    m_fontSizeLabel->setAlignment(Qt::AlignCenter);
    m_fontSizeLabel->setStyleSheet(
        "QLabel { background-color: #ffffff; color: #000000; font-family: 'Microsoft Sans Serif'; font-size: 8.25pt; }"
    );

    auto *templateLabel = new QLabel(QStringLiteral("Использовать как шаблон"), m_settingsMainView);
    templateLabel->setGeometry(104, 74, 180, 16);
    templateLabel->setStyleSheet(plainLabelStyle);
    m_templates = new QComboBox(m_settingsMainView);
    m_templates->setGeometry(82, 90, 197, 28);
    m_templates->setEditable(true);
    m_templates->setInsertPolicy(QComboBox::NoInsert);
    m_templateDeleteButton = new ImageButton(m_settingsMainView);
    m_templateDeleteButton->setGeometry(284, 91, 26, 26);
    {
        const QStringList candidates = {resourcePath("close.png"), imagePath("delete.png")};
        for (const QString &file : candidates) {
            if (!file.isEmpty()) {
                m_templateDeleteButton->setImagePath(file);
                break;
            }
        }
    }
    m_templateSaveAsButton = new ImageButton(m_settingsMainView);
    m_templateSaveAsButton->setGeometry(119, 124, 127, 31);
    setImage(m_templateSaveAsButton, "save.png");

    auto *saveTemplateTitle = new QLabel(QStringLiteral("Сохранить как шаблон"), m_settingsSaveView);
    saveTemplateTitle->setGeometry(95, 20, 200, 20);
    saveTemplateTitle->setStyleSheet(plainLabelStyle);
    m_templateNameEdit = new QLineEdit(m_settingsSaveView);
    m_templateNameEdit->setGeometry(82, 55, 220, 28);
    m_templateNameEdit->setPlaceholderText(QStringLiteral("Название шаблона"));
    m_templateNameEdit->setStyleSheet(
        "QLineEdit { background: #ffffff; color: #000000; font-family: 'Microsoft Sans Serif'; font-size: 10pt; border: 1px solid #808080; padding: 2px 6px; }"
    );
    auto *saveTemplateBtn = new ImageButton(m_settingsSaveView);
    saveTemplateBtn->setGeometry(119, 100, 101, 30);
    setImage(saveTemplateBtn, "save.png");
    auto *cancelTemplateBtn = new ImageButton(m_settingsSaveView);
    cancelTemplateBtn->setGeometry(119, 140, 101, 30);
    setImage(cancelTemplateBtn, "cancel.png");

    m_savePanel = new QWidget(m_root);
    m_savePanel->setObjectName(QStringLiteral("savePanel"));
    m_savePanel->setGeometry(1450, -220, 172, 201);
    m_savePanel->setStyleSheet(smallPanelStyle("savePanel"));
    m_savePanel->hide();
    auto *saveTitle = new QLabel(QStringLiteral("Сохранить:"), m_savePanel);
    saveTitle->setGeometry(33, 9, 120, 22);
    saveTitle->setStyleSheet(boldTitleStyle);
    auto *saveMainPanel = new QWidget(m_savePanel);
    saveMainPanel->setGeometry(12, 25, 102, 53);
    saveMainPanel->setStyleSheet(transparentPanelStyle);
    m_saveAnamnesisCb = new QRadioButton(QStringLiteral("Анамнез"), saveMainPanel);
    m_saveAnamnesisCb->setGeometry(10, 10, 90, 20);
    m_saveAnamnesisCb->setChecked(true);
    m_saveAnamnesisCb->setStyleSheet(panelCheckBoxStyle);
    m_saveProtocolsCb = new QRadioButton(QStringLiteral("Протоколы"), saveMainPanel);
    m_saveProtocolsCb->setGeometry(10, 33, 90, 20);
    m_saveProtocolsCb->setStyleSheet(panelCheckBoxStyle);
    auto *saveMainGroup = new QButtonGroup(saveMainPanel);
    saveMainGroup->addButton(m_saveAnamnesisCb);
    saveMainGroup->addButton(m_saveProtocolsCb);
    m_saveProtocolsSubPanel = new QWidget(m_savePanel);
    m_saveProtocolsSubPanel->setGeometry(25, 79, 123, 60);
    m_saveProtocolsSubPanel->setStyleSheet(transparentPanelStyle);
    m_saveProtocolsSubPanel->setEnabled(false);
    setPanelChildOpacity(m_saveProtocolsSubPanel, false);
    m_saveForPatientCb = new QRadioButton(QStringLiteral("Для пациента"), m_saveProtocolsSubPanel);
    m_saveForPatientCb->setGeometry(10, 10, 110, 20);
    m_saveForPatientCb->setChecked(true);
    m_saveForPatientCb->setStyleSheet(panelCheckBoxStyle);
    m_saveForSpecialistCb = new QRadioButton(QStringLiteral("Для специалиста"), m_saveProtocolsSubPanel);
    m_saveForSpecialistCb->setGeometry(10, 33, 120, 20);
    m_saveForSpecialistCb->setStyleSheet(panelCheckBoxStyle);
    auto *saveProtocolsGroup = new QButtonGroup(m_saveProtocolsSubPanel);
    saveProtocolsGroup->addButton(m_saveForPatientCb);
    saveProtocolsGroup->addButton(m_saveForSpecialistCb);
    auto *saveAsBtn = new ImageButton(m_savePanel);
    saveAsBtn->setGeometry(22, 145, 109, 33);
    setImage(saveAsBtn, "saveas.png");

    m_printPanel = new QWidget(m_root);
    m_printPanel->setObjectName(QStringLiteral("printPanel"));
    m_printPanel->setGeometry(1450, -220, 172, 201);
    m_printPanel->setStyleSheet(smallPanelStyle("printPanel"));
    m_printPanel->hide();
    auto *printTitle = new QLabel(QStringLiteral("Отправить на печать:"), m_printPanel);
    printTitle->setGeometry(12, 21, 150, 22);
    printTitle->setStyleSheet(plainLabelStyle);
    auto *printMainPanel = new QWidget(m_printPanel);
    printMainPanel->setGeometry(21, 37, 102, 53);
    printMainPanel->setStyleSheet(transparentPanelStyle);
    m_printAnamnesisCb = new QRadioButton(QStringLiteral("Анамнез"), printMainPanel);
    m_printAnamnesisCb->setGeometry(10, 10, 90, 20);
    m_printAnamnesisCb->setChecked(true);
    m_printAnamnesisCb->setStyleSheet(panelCheckBoxStyle);
    m_printProtocolsCb = new QRadioButton(QStringLiteral("Протоколы"), printMainPanel);
    m_printProtocolsCb->setGeometry(10, 33, 90, 20);
    m_printProtocolsCb->setStyleSheet(panelCheckBoxStyle);
    auto *printMainGroup = new QButtonGroup(printMainPanel);
    printMainGroup->addButton(m_printAnamnesisCb);
    printMainGroup->addButton(m_printProtocolsCb);
    m_printProtocolsSubPanel = new QWidget(m_printPanel);
    m_printProtocolsSubPanel->setGeometry(42, 93, 123, 60);
    m_printProtocolsSubPanel->setStyleSheet(transparentPanelStyle);
    m_printProtocolsSubPanel->setEnabled(false);
    setPanelChildOpacity(m_printProtocolsSubPanel, false);
    m_printForPatientCb = new QRadioButton(QStringLiteral("Для пациента"), m_printProtocolsSubPanel);
    m_printForPatientCb->setGeometry(10, 10, 110, 20);
    m_printForPatientCb->setChecked(true);
    m_printForPatientCb->setStyleSheet(panelCheckBoxStyle);
    m_printForSpecialistCb = new QRadioButton(QStringLiteral("Для специалиста"), m_printProtocolsSubPanel);
    m_printForSpecialistCb->setGeometry(10, 33, 120, 20);
    m_printForSpecialistCb->setStyleSheet(panelCheckBoxStyle);
    auto *printProtocolsGroup = new QButtonGroup(m_printProtocolsSubPanel);
    printProtocolsGroup->addButton(m_printForPatientCb);
    printProtocolsGroup->addButton(m_printForSpecialistCb);
    auto *printBtn = new ImageButton(m_printPanel);
    printBtn->setGeometry(42, 159, 81, 34);
    {
        const QStringList candidates = {
            imagePath("Печать_текст.png"),
            resourcePath("Печать_текст.png"),
            imagePath("Печать текст.png")
        };
        for (const QString &file : candidates) {
            if (!file.isEmpty()) {
                printBtn->setImagePath(file);
                break;
            }
        }
    }

    connect(m_fontDownButton, &ImageButton::clicked, this, [this]() {
        if (m_fontSlider->value() > m_fontSlider->minimum()) {
            m_fontSlider->setValue(m_fontSlider->value() - 1);
        }
    });
    connect(m_fontUpButton, &ImageButton::clicked, this, [this]() {
        if (m_fontSlider->value() < m_fontSlider->maximum()) {
            m_fontSlider->setValue(m_fontSlider->value() + 1);
        }
    });
    connect(m_fontSlider, &QSlider::valueChanged, this, [this](int value) {
        m_fontSizeLabel->setText(QString::number(value));
        changeDocumentFontSize(value);
    });
    connect(m_templateDeleteButton, &ImageButton::clicked, this, [this]() {
        const QString name = m_templates->currentText();
        if (name == QStringLiteral("Стандартный")) {
            return;
        }
        if (!CustomMessageBox::askConfirm(this, "Внимание! Вы собираетесь удалить данные\nЭто действие необратимо!")) {
            return;
        }
        QString err;
        if (!m_repository.deleteTemplate(name, &err)) {
            CustomMessageBox::showError(this, err);
            return;
        }
        refreshTemplateNames();
        m_templates->setCurrentText(QStringLiteral("Стандартный"));
        writeProfileConfig(QStringLiteral("Стандартный"), m_fontSlider->value());
        loadDefaultAnamnesisTemplate();
    });
    connect(m_templateSaveAsButton, &ImageButton::clicked, this, [this]() { saveCurrentAnamnesisTemplate(); });
    connect(saveTemplateBtn, &ImageButton::clicked, this, [this]() {
        const QString name = m_templateNameEdit->text().trimmed();
        if (name.isEmpty()) {
            CustomMessageBox::showWarning(this, "Введите название шаблона.");
            return;
        }
        {
            QSignalBlocker blocker(m_templates);
            m_templates->setEditText(name);
        }
        saveCurrentAnamnesisTemplate();
        showSettingsSaveTemplateView(false);
    });
    connect(cancelTemplateBtn, &ImageButton::clicked, this, [this]() { showSettingsSaveTemplateView(false); });
    connect(m_saveProtocolsCb, &QRadioButton::toggled, this, [this](bool checked) {
        m_saveProtocolsSubPanel->setEnabled(checked);
        setPanelChildOpacity(m_saveProtocolsSubPanel, checked);
        if (checked && m_saveForPatientCb && !m_saveForPatientCb->isChecked() && !m_saveForSpecialistCb->isChecked()) {
            m_saveForPatientCb->setChecked(true);
        }
    });
    connect(m_printProtocolsCb, &QRadioButton::toggled, this, [this](bool checked) {
        m_printProtocolsSubPanel->setEnabled(checked);
        setPanelChildOpacity(m_printProtocolsSubPanel, checked);
        if (checked && m_printForPatientCb && !m_printForPatientCb->isChecked() && !m_printForSpecialistCb->isChecked()) {
            m_printForPatientCb->setChecked(true);
        }
    });
    connect(saveAsBtn, &ImageButton::clicked, this, [this]() { exportDocument(); });
    connect(printBtn, &ImageButton::clicked, this, [this]() { printSelectedContent(); });

    m_root->installEventFilter(this);
    m_patientsTable->viewport()->installEventFilter(this);
}

void InfantWindow::applyLegacyStyle() {
    m_root->setObjectName(QStringLiteral("rootPanel"));
    // Только на сам root — иначе fone.jpg наследуется всеми дочерними виджетами (кнопки/радио/поля).
    m_root->setStyleSheet(
        QStringLiteral("QWidget#rootPanel { background-image: url('%1'); background-repeat: no-repeat; }")
            .arg(imagePath("fone.jpg")));
    QFont formFont("Microsoft Sans Serif", 10);
    setFont(formFont);

    auto setFromCandidates = [](ImageButton *button, const QStringList &candidates) {
        for (const QString &file : candidates) {
            if (!file.isEmpty()) {
                button->setImagePath(file);
                return;
            }
        }
    };

    setFromCandidates(m_bClose, {resourcePath("Закрыть.png"), resourcePath("close.png"), imagePath("close.png")});
    setFromCandidates(m_bLine, {resourcePath("Свернуть.png"), imagePath("build.png")});
    setImage(m_bUp, "up.png");
    setImage(m_bBack, "back.png");
    setImage(m_bList, "plist.png");
    setImage(m_bExit, "exit.png");
    setImage(m_bPicPrint, "saveas.png");
    setImage(m_bUpload, "showp.png");
    setFromCandidates(m_bSave, {
        imagePath("Сохранить.png"),
        resourcePath("Сохранить.png"),
        imagePath("save.png")
    });
    setFromCandidates(m_bPrint, {
        imagePath("Печать.png"),
        resourcePath("Печать.png"),
        imagePath("saveas.png")
    });
    setFromCandidates(m_bSettings, {
        imagePath("Настройки.png"),
        resourcePath("Настройки.png"),
        imagePath("toogl.png")
    });
    setFromCandidates(m_bInfo, {resourcePath("Информация.png"), imagePath("im1.png")});
    setImage(m_loginButton, "enter.png");
    setImage(m_adminButton, "admin.png");
    if (m_loginManIcon) {
        m_loginManIcon->setPixmap(QPixmap(imagePath("man.png")));
    }
    if (m_loginKeyIcon) {
        m_loginKeyIcon->setPixmap(QPixmap(imagePath("key.png")));
    }
    setImage(m_loginEye, "pon.png");
    setImage(m_adminEye1, "pon.png");
    setImage(m_adminEye2, "pon.png");
    setImage(m_userSaveButton, "save.png");
    setImage(m_userOpenPatients, "enter.png");
    if (m_adminManIcon) {
        const QString man2Path = imagePath("man2.png");
        if (!man2Path.isEmpty()) {
            m_adminManIcon->setPixmap(
                QPixmap(man2Path).scaled(40, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            );
        } else {
            const QString img2Path = htmlPath("spravka/администрирование/img2.png");
            const QPixmap source(img2Path);
            if (!source.isNull()) {
                const QPixmap icon = source.copy(0, 0, 52, source.height());
                m_adminManIcon->setPixmap(
                    icon.scaled(40, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                );
            } else {
                m_adminManIcon->setPixmap(
                    QPixmap(imagePath("man.png")).scaled(40, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                );
            }
        }
    }
    if (m_adminLoginIcon) {
        m_adminLoginIcon->setPixmap(
            QPixmap(imagePath("man.png")).scaled(32, 33, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        );
    }
    if (m_adminKeyIcon1) {
        m_adminKeyIcon1->setPixmap(QPixmap(imagePath("key.png")));
    }
    if (m_adminKeyIcon2) {
        m_adminKeyIcon2->setPixmap(QPixmap(imagePath("key.png")));
    }
    setImage(m_addPatient, "addp.png");
    setFromCandidates(m_underlineButton, {
        resourcePath("undeline.png"),
        imagePath("undeline.png")
    });
    setFromCandidates(m_boldButton, {
        resourcePath("Жирный шрифт.png"),
        imagePath("Жирный шрифт.png")
    });
    setImage(m_pAna, "anaoff.png");
    setImage(m_pProto, "protoff.png");
    setImage(m_pUpr, "uproff.png");

    applyEnterLogos();

    m_loginEdit->setPlaceholderText("Логин");
    m_passwordEdit->setPlaceholderText("Пароль");
    m_userFio->setPlaceholderText("ФИО");
    m_userLogin->setPlaceholderText("Логин");
    m_userPass->setPlaceholderText("Придумайте пароль");
    m_userPass2->setPlaceholderText("Подтвердите пароль");
    m_patientSearch->setPlaceholderText("Поиск");

    styleInputField(m_loginEdit);
    styleInputField(m_passwordEdit);
    styleAdminScreen();
    stylePatientsScreen();
    styleAnamnesisScreen();
    styleProtocolsView();
    styleExercisesScreen();
    styleTemplateComboBox();
    installToolbarTooltips();

    const QString crossImage = resourcePath("крестик.png");
    if (!crossImage.isEmpty()) {
        m_loginClear->setImagePath(crossImage);
        m_passwordClear->setImagePath(crossImage);
        m_userFioClear->setImagePath(crossImage);
        m_userLoginClear->setImagePath(crossImage);
        m_userPassClear->setImagePath(crossImage);
        m_userPass2Clear->setImagePath(crossImage);
        m_patientSearchClear->setImagePath(crossImage);
    }
    m_loginClear->raise();
    m_passwordClear->raise();
    m_userFioClear->raise();
    m_userLoginClear->raise();
    m_userPassClear->raise();
    m_userPass2Clear->raise();
    m_adminEye1->raise();
    m_adminEye2->raise();
    m_adminManIcon->raise();
    m_adminLoginIcon->raise();
    m_adminKeyIcon1->raise();
    m_adminKeyIcon2->raise();
    m_adminTitle->setStyleSheet(
        "color: white; font-family: 'Microsoft Sans Serif'; font-size: 16pt; font-weight: bold; background: transparent;"
    );
    m_patientTitle->setAlignment(Qt::AlignCenter);
    styleUsersTable();
}

void InfantWindow::bindSignals() {
    connect(m_bClose, &ImageButton::clicked, this, &InfantWindow::close);
    connect(m_bLine, &ImageButton::clicked, this, [this]() {
        if (!m_isCustomMaximized) {
            m_savedWindowGeometry = geometry();
        }
        showMinimized();
    });
    connect(m_bUp, &ImageButton::clicked, this, [this]() { toggleWindowMaximize(); });
    connect(m_bExit, &ImageButton::clicked, this, [this]() {
        if (m_currentScreen == ScreenMode::Anamnesis) {
            tryAutoSaveAnamnesis();
        }
        m_session.reset();
        m_mainId.clear();
        m_loginEdit->clear();
        m_passwordEdit->clear();
        m_loginClear->hide();
        m_passwordClear->hide();
        setScreen(ScreenMode::Enter);
    });
    connect(m_bBack, &ImageButton::clicked, this, [this]() { navigateBack(); });
    connect(m_bList, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Patients); });
    connect(m_pAna, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Anamnesis); });
    connect(m_pProto, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Protocols); });
    connect(m_pUpr, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Exercises); });
    connect(m_bInfo, &ImageButton::clicked, this, [this]() { showInfoPopup(); });

    bindClearableField(m_loginEdit, m_loginClear);
    bindClearableField(m_passwordEdit, m_passwordClear);
    bindClearableField(m_userFio, m_userFioClear);
    bindClearableField(m_userLogin, m_userLoginClear);
    bindClearableField(m_userPass, m_userPassClear);
    bindClearableField(m_userPass2, m_userPass2Clear);
    bindClearableField(m_patientSearch, m_patientSearchClear);

    connect(m_loginButton, &ImageButton::clicked, this, [this]() {
        const auto user = m_repository.login(m_loginEdit->text().trimmed(), m_passwordEdit->text());
        if (!user.has_value()) {
            CustomMessageBox::showError(this, "Неверный логин или пароль!");
            return;
        }
        if (user->role != "Администратор" && user->role != "Специалист") {
            CustomMessageBox::showError(this, "Недостаточный уровень доступа.");
            return;
        }
        m_session = user;
        m_mainId = user->id;
        rememberManagedUser(user->id, user->login, m_passwordEdit->text());
        refreshPatients();
        setScreen(ScreenMode::Patients);
    });
    connect(m_loginEye, &ImageButton::clicked, this, [this]() {
        if (m_passwordEdit->echoMode() == QLineEdit::Password) {
            m_passwordEdit->setEchoMode(QLineEdit::Normal);
            setImage(m_loginEye, "poff.png");
        } else {
            m_passwordEdit->setEchoMode(QLineEdit::Password);
            setImage(m_loginEye, "pon.png");
        }
    });

    connect(m_adminButton, &ImageButton::clicked, this, [this]() {
        const auto user = m_repository.login(m_loginEdit->text().trimmed(), m_passwordEdit->text());
        if (!user.has_value()) {
            CustomMessageBox::showError(this, "Неверный логин или пароль!");
            return;
        }
        if (user->role != "Администратор") {
            CustomMessageBox::showError(this, "Ваш уровень доступа не позволяет осуществлять администрирование!");
            return;
        }
        m_session = user;
        m_mainId = user->mainId.isEmpty() ? user->fio : user->mainId;
        rememberManagedUser(user->id, user->login, m_passwordEdit->text());
        refreshUsers();
        setScreen(ScreenMode::Admin);
    });

    connect(m_userSaveButton, &ImageButton::clicked, this, [this]() { saveUser(); });

    connect(m_userOpenPatients, &ImageButton::clicked, this, [this]() { enterAsManagedUser(); });
    connect(m_adminEye1, &ImageButton::clicked, this, [this]() {
        if (m_userPass->echoMode() == QLineEdit::Password) {
            m_userPass->setEchoMode(QLineEdit::Normal);
            setImage(m_adminEye1, "poff.png");
        } else {
            m_userPass->setEchoMode(QLineEdit::Password);
            setImage(m_adminEye1, "pon.png");
        }
    });
    connect(m_adminEye2, &ImageButton::clicked, this, [this]() {
        if (m_userPass2->echoMode() == QLineEdit::Password) {
            m_userPass2->setEchoMode(QLineEdit::Normal);
            setImage(m_adminEye2, "poff.png");
        } else {
            m_userPass2->setEchoMode(QLineEdit::Password);
            setImage(m_adminEye2, "pon.png");
        }
    });

    connect(m_patientSearch, &QLineEdit::textChanged, this, [this]() { refreshPatients(); });
    connect(m_dateFilter, &QCheckBox::stateChanged, this, [this]() { refreshPatients(); });
    connect(m_dateFrom, &QDateEdit::dateChanged, this, [this]() { refreshPatients(); });
    connect(m_dateTo, &QDateEdit::dateChanged, this, [this]() { refreshPatients(); });

    connect(m_addPatient, &ImageButton::clicked, this, [this]() {
        m_currentPatientId.clear();
        m_patientTitle->setText("Новая карта");
        loadDefaultAnamnesisTemplate();
        setScreen(ScreenMode::Anamnesis);
    });

    connect(m_patientsTable, &QTableWidget::cellClicked, this, &InfantWindow::handlePatientsTableClick);
    connect(m_patientsTable->horizontalHeader(), &QHeaderView::sectionClicked, this, &InfantWindow::handlePatientsHeaderClick);
    connect(m_patientsTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        if (column == 1) {
            m_patientsTable->selectRow(row);
            openPatientFromTable();
        }
    });
    auto *patientEnter = new QShortcut(QKeySequence(Qt::Key_Return), m_patientsTable);
    connect(patientEnter, &QShortcut::activated, this, [this]() {
        if (m_currentScreen == ScreenMode::Patients && m_patientsTable->currentRow() >= 0) {
            openPatientFromTable();
        }
    });
    auto *patientEnter2 = new QShortcut(QKeySequence(Qt::Key_Enter), m_patientsTable);
    connect(patientEnter2, &QShortcut::activated, this, [this]() {
        if (m_currentScreen == ScreenMode::Patients && m_patientsTable->currentRow() >= 0) {
            openPatientFromTable();
        }
    });

    connect(m_bSave, &ImageButton::clicked, this, [this]() { toggleSlidePanel(m_savePanel); });
    connect(m_bPrint, &ImageButton::clicked, this, [this]() { toggleSlidePanel(m_printPanel); });
    connect(m_bSettings, &ImageButton::clicked, this, [this]() {
        refreshTemplateNames();
        syncTemplateSelectorFromProfile();
        toggleSlidePanel(m_settingsPanel);
    });
    connect(m_templates, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        Q_UNUSED(index);
        if (m_settingsPanel == nullptr || !m_settingsPanel->isVisible() || m_suppressTemplateLoad) {
            return;
        }
        loadAnamnesisTemplateByName(m_templates->currentText().trimmed());
    });
    connect(m_underlineButton, &ImageButton::clicked, this, [this]() {
        m_underlineActive = !m_underlineActive;
        QTextCharFormat fmt;
        fmt.setFontUnderline(m_underlineActive);
        m_anamnesisEdit->mergeCurrentCharFormat(fmt);
        updateFormatButtonIcons();
    });
    connect(m_boldButton, &ImageButton::clicked, this, [this]() {
        m_boldActive = !m_boldActive;
        QTextCharFormat fmt;
        fmt.setFontWeight(m_boldActive ? QFont::Bold : QFont::Normal);
        m_anamnesisEdit->mergeCurrentCharFormat(fmt);
        updateFormatButtonIcons();
    });
    connect(m_anamnesisEdit, &QTextEdit::cursorPositionChanged, this, [this]() { updateFormatButtonIcons(); });
    connect(m_anamnesisEdit, &QTextEdit::selectionChanged, this, [this]() { updateFormatButtonIcons(); });
    m_anamnesisEdit->installEventFilter(this);
    auto *anamnesisAutoSaveTimer = new QTimer(this);
    anamnesisAutoSaveTimer->setSingleShot(true);
    anamnesisAutoSaveTimer->setInterval(1200);
    connect(m_anamnesisEdit, &QTextEdit::textChanged, this, [this, anamnesisAutoSaveTimer]() {
        updatePatientTitleFromDocument();
        anamnesisAutoSaveTimer->start();
    });
    connect(anamnesisAutoSaveTimer, &QTimer::timeout, this, [this]() { tryAutoSaveAnamnesis(); });
    connect(m_authorsFilter, &QComboBox::currentTextChanged, this, [this](const QString &) { refreshExercisesTree(); });
    connect(m_exercisesTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        showExercisesContextMenu(pos);
    });
    connect(m_exercisesTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        if (!item || item->childCount() > 0) {
            return;
        }
        const QString exerciseId = item->data(0, Qt::UserRole).toString();
        if (exerciseId.isEmpty()) {
            return;
        }
        openExercise(exerciseId);
    });
    connect(m_dualScreenCheck, &QCheckBox::toggled, this, [this](bool checked) {
        applyDualScreenSetting(checked);
    });
}

void InfantWindow::setScreen(ScreenMode mode, bool pushHistory) {
    if (m_exerciseOpen) {
        if (m_exerciseHost) {
            m_exerciseHost->saveProtocolEdits();
            m_exerciseHost->hide();
        }
        m_exerciseOpen = false;
    }

    const ScreenMode previousScreen = m_currentScreen;
    m_screenTransitionGuard = true;
    struct ScreenTransitionGuard {
        bool *flag = nullptr;
        ~ScreenTransitionGuard() {
            if (flag) {
                *flag = false;
            }
        }
    } transitionGuard{&m_screenTransitionGuard};

    if (previousScreen == ScreenMode::Anamnesis && mode != ScreenMode::Anamnesis) {
        tryAutoSaveAnamnesis(true);
        refreshPatients();
    }

    if (previousScreen == ScreenMode::Protocols && mode != ScreenMode::Protocols) {
        saveProtocolsEdits(true);
    }

    if (pushHistory && !m_navigatingBack) {
        m_navHistory.append(mode);
    }

    m_currentScreen = mode;
    hideSlidePanels();
    if (m_infoPopup && m_infoPopup->isVisible()) {
        m_infoPopup->hide();
    }

    const bool enter = mode == ScreenMode::Enter;
    const bool patients = mode == ScreenMode::Patients;
    const bool admin = mode == ScreenMode::Admin;
    const bool anamnesis = mode == ScreenMode::Anamnesis;
    const bool protocols = mode == ScreenMode::Protocols;
    const bool exercises = mode == ScreenMode::Exercises;
    const bool workScreen = anamnesis || protocols || exercises;

    m_logo1->setVisible(enter);
    m_logo2->setVisible(enter);
    m_panelLogin->setVisible(enter);

    m_panelPatients->setVisible(patients);
    m_panelAdmin->setVisible(admin);

    if (m_workStack) {
        m_workStack->setVisible(workScreen);
        if (anamnesis) {
            if (m_anamnesisEdit) {
                m_anamnesisEdit->clearFocus();
            }
            m_workStack->setCurrentWidget(m_panelWork);
        } else if (protocols) {
            if (m_anamnesisEdit) {
                m_anamnesisEdit->clearFocus();
            }
            m_workStack->setCurrentWidget(m_panelProtocols);
        } else if (exercises) {
            if (m_anamnesisEdit) {
                m_anamnesisEdit->clearFocus();
            }
            m_workStack->setCurrentWidget(m_panelExercises);
        }
        m_workStack->update();
    }
    if (m_authorsFilterHost) {
        m_authorsFilterHost->setVisible(exercises && !m_exerciseOpen);
    }
    if (m_authorsFilter) {
        m_authorsFilter->setVisible(exercises && !m_exerciseOpen);
    }
    m_adminTitle->setVisible(admin);
    m_userOpenPatients->setVisible(admin);
    if (m_dualScreenCheck) {
        m_dualScreenCheck->setVisible(admin);
    }
    m_userOpenPatients->raise();

    m_bBack->setVisible(patients || workScreen);
    m_bList->setVisible(workScreen);
    m_bExit->setVisible(!enter);
    m_bSave->setVisible(workScreen);
    m_bPrint->setVisible(workScreen);
    m_bSettings->setVisible(anamnesis);
    if (!anamnesis && m_settingsPanel && m_settingsPanel->isVisible()) {
        m_settingsPanel->hide();
    }
    m_bPicPrint->setVisible(false);
    m_bUpload->setVisible(false);
    m_bInfo->setVisible(true);
    m_pAna->setVisible(workScreen);
    m_pProto->setVisible(workScreen);
    m_pUpr->setVisible(workScreen);
    m_patientTitle->setVisible(workScreen);
    m_underlineButton->setVisible(anamnesis);
    m_boldButton->setVisible(anamnesis);

    if (workScreen) {
        if (m_workStack) {
            m_workStack->raise();
        }
        if (exercises && m_authorsFilterHost && !m_exerciseOpen) {
            m_authorsFilterHost->raise();
        }
        m_patientTitle->raise();
        m_pAna->raise();
        m_pProto->raise();
        m_pUpr->raise();
        m_bSave->raise();
        m_bPrint->raise();
        if (anamnesis) {
            m_bSettings->raise();
        }
    }

    if (patients) {
        m_helpIndex = "списокпациентов.html";
        refreshPatients();
    }
    if (admin) {
        m_helpIndex = "администрирование.html";
        if (m_dualScreenCheck) {
            m_dualScreenCheck->blockSignals(true);
            m_dualScreenCheck->setChecked(AppSettings::dualScreenEnabled());
            m_dualScreenCheck->blockSignals(false);
        }
        refreshUsers();
    }
    if (anamnesis) {
        m_helpIndex = "анамнез.html";
        QTimer::singleShot(0, this, [this]() {
            if (m_currentScreen != ScreenMode::Anamnesis) {
                return;
            }
            refreshTemplateNames();
            if (m_currentPatientId.isEmpty() && m_anamnesisEdit->toPlainText().trimmed().isEmpty()) {
                loadDefaultAnamnesisTemplate();
            } else {
                int fontSize = 24;
                const QString configPath = profileConfigPath();
                if (!configPath.isEmpty()) {
                    QFile configFile(configPath);
                    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
                        if (parts.size() > 1) {
                            fontSize = parts.at(1).toInt();
                        }
                    }
                }
                if (m_fontSlider) {
                    QSignalBlocker blocker(m_fontSlider);
                    m_fontSlider->setValue(fontSize);
                }
                if (m_fontSizeLabel) {
                    m_fontSizeLabel->setText(QString::number(fontSize));
                }
                changeDocumentFontSize(fontSize, false);
            }
        });
    }
    if (protocols) {
        m_helpIndex = "протоколы.html";
        QTimer::singleShot(0, this, [this]() {
            if (m_currentScreen == ScreenMode::Protocols) {
                refreshProtocolsView();
            }
        });
    }
    if (exercises) {
        m_helpIndex = "упражнения.html";
        QTimer::singleShot(0, this, [this]() {
            if (m_currentScreen == ScreenMode::Exercises) {
                styleExercisesScreen();
                refreshExercisesTree();
            }
        });
    }
    if (workScreen) {
        updatePatientTabIcons();
    }
    if (enter) {
        m_helpIndex = "окновходавпрограмму.html";
        applyEnterLogos();
    }
}

void InfantWindow::navigateBack() {
    if (m_exerciseOpen) {
        closeExerciseHost();
        return;
    }
    if (m_navHistory.size() <= 1) {
        setScreen(ScreenMode::Patients, false);
        return;
    }
    m_navigatingBack = true;
    m_navHistory.removeLast();
    setScreen(m_navHistory.last(), false);
    m_navigatingBack = false;
}

void InfantWindow::toggleWindowMaximize() {
    QRect frame = m_savedWindowGeometry.isValid() ? m_savedWindowGeometry : geometry();
    if (m_isCustomMaximized) {
        frame.setHeight(kDesignHeight);
        m_isCustomMaximized = false;
    } else {
        frame.setHeight(kDesignHeight - kTaskbarReserve);
        m_isCustomMaximized = true;
    }
    applyWindowGeometry(frame);
    updateMaximizeButtonIcon();
}

void InfantWindow::applyWindowGeometry(const QRect &rect) {
    m_programmaticGeometryChange = true;
    setGeometry(rect);
    m_savedWindowGeometry = rect;
    m_programmaticGeometryChange = false;
}

void InfantWindow::moveEvent(QMoveEvent *event) {
#ifndef Q_OS_WIN
    if (!m_programmaticGeometryChange && m_savedWindowGeometry.isValid()
        && pos() != m_savedWindowGeometry.topLeft()) {
        m_programmaticGeometryChange = true;
        move(m_savedWindowGeometry.topLeft());
        m_programmaticGeometryChange = false;
        return;
    }
#endif
    QMainWindow::moveEvent(event);
}

void InfantWindow::resizeEvent(QResizeEvent *event) {
#ifndef Q_OS_WIN
    if (!m_programmaticGeometryChange && m_savedWindowGeometry.isValid()
        && size() != m_savedWindowGeometry.size()) {
        m_programmaticGeometryChange = true;
        resize(m_savedWindowGeometry.size());
        m_programmaticGeometryChange = false;
        return;
    }
#endif
    QMainWindow::resizeEvent(event);
}

void InfantWindow::updateMaximizeButtonIcon() {
    setImage(m_bUp, m_isCustomMaximized ? "down.png" : "up.png");
}

void InfantWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        const auto *stateEvent = static_cast<QWindowStateChangeEvent *>(event);
        const Qt::WindowStates oldState = stateEvent->oldState();
        const Qt::WindowStates newState = windowState();

        if (!(oldState & Qt::WindowMinimized) && (newState & Qt::WindowMinimized)) {
            m_savedWindowGeometry = geometry();
        } else if ((oldState & Qt::WindowMinimized) && !(newState & Qt::WindowMinimized)) {
            QTimer::singleShot(0, this, [this]() {
                if (m_savedWindowGeometry.isValid()) {
                    applyWindowGeometry(m_savedWindowGeometry);
                }
            });
        }
    }
    QMainWindow::changeEvent(event);
}

void InfantWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (!m_geometryInitialized) {
        m_geometryInitialized = true;
        applyNormalWindowGeometry();
    }
}

QRect InfantWindow::calculateNormalWindowGeometry() const {
    QScreen *screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    if (!screen) {
        return QRect(0, 0, kDesignWidth, kDesignHeight);
    }

    const QRect screenGeometry = screen->geometry();
    const int width = qMin(kDesignWidth, screenGeometry.width());
    const int height = kDesignHeight;
    const int x = screenGeometry.x() + qMax(0, (screenGeometry.width() - width) / 2);
    const int y = screenGeometry.y();
    return QRect(x, y, width, height);
}

QRect InfantWindow::calculateMaximizedWindowGeometry() const {
    return calculateNormalWindowGeometry();
}

void InfantWindow::applyNormalWindowGeometry() {
    const QRect initialGeometry = calculateNormalWindowGeometry();
    applyWindowGeometry(initialGeometry);
    m_isCustomMaximized = false;
    updateMaximizeButtonIcon();
}

void InfantWindow::updatePatientTabIcons() {
    setImage(m_pAna, m_currentScreen == ScreenMode::Anamnesis ? "anaon.png" : "anaoff.png");
    setImage(m_pProto, m_currentScreen == ScreenMode::Protocols ? "proton.png" : "protoff.png");
    setImage(m_pUpr, m_currentScreen == ScreenMode::Exercises ? "upron.png" : "uproff.png");
}

void InfantWindow::applyDualScreenSetting(bool enabled) {
    AppSettings::setDualScreenEnabled(enabled);
    if (m_dualScreenCheck) {
        m_dualScreenCheck->blockSignals(true);
        m_dualScreenCheck->setChecked(enabled);
        m_dualScreenCheck->blockSignals(false);
    }
    if (m_exerciseHost) {
        m_exerciseHost->setDualScreenEnabled(enabled);
    }
}

void InfantWindow::styleInputField(QLineEdit *edit) const {
    edit->setStyleSheet("font-family:'Microsoft Sans Serif';font-size:14pt;color:black;background:white;");
    QPalette palette = edit->palette();
    palette.setColor(QPalette::PlaceholderText, QColor("#C0C0C0"));
    edit->setPalette(palette);
}

void InfantWindow::styleAdminInputField(QLineEdit *edit) const {
    edit->setFrame(false);
    edit->setMaxLength(35);
    edit->setStyleSheet(
        "QLineEdit {"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 12pt;"
        "  color: black;"
        "  background: white;"
        "  border: none;"
        "  padding: 0px 0px 0px 2px;"
        "}"
    );
    QPalette palette = edit->palette();
    palette.setColor(QPalette::PlaceholderText, QColor("#C0C0C0"));
    edit->setPalette(palette);
}

void InfantWindow::styleAdminScreen() {
    const QString fieldBoxStyle = QStringLiteral("background: white; border: none; border-radius: 0px;");
    m_userFioPanel->setStyleSheet(fieldBoxStyle);
    m_userLoginPanel->setStyleSheet(fieldBoxStyle);
    m_userPassPanel->setStyleSheet(fieldBoxStyle);
    m_userPass2Panel->setStyleSheet(fieldBoxStyle);

    styleAdminInputField(m_userFio);
    styleAdminInputField(m_userLogin);
    styleAdminInputField(m_userPass);
    styleAdminInputField(m_userPass2);

    m_userFioClear->setStyleSheet("background: white;");
    m_userLoginClear->setStyleSheet("background: white;");
    m_userPassClear->setStyleSheet("background: white;");
    m_userPass2Clear->setStyleSheet("background: white;");

    m_adminLabel1->setStyleSheet(
        "color: white; font-family: 'Microsoft Sans Serif'; font-size: 12pt; font-weight: bold; background: transparent;"
    );
    m_adminLabel2->setStyleSheet(
        "color: white; font-family: 'Microsoft Sans Serif'; font-size: 12pt; font-weight: bold; background: transparent;"
    );
    if (m_dualScreenCheck) {
        m_dualScreenCheck->setStyleSheet(
            "QCheckBox { color: white; font-family: 'Microsoft Sans Serif'; font-size: 12pt; background: transparent; spacing: 8px; }"
            + checkBoxIndicatorCss());
    }
    if (m_screenSettingsTitle) {
        m_screenSettingsTitle->setStyleSheet(
            "color: white; font-family: 'Microsoft Sans Serif'; font-size: 12pt; font-weight: bold; background: transparent;");
    }
    m_userRole->setStyleSheet(QString(
        "QComboBox {"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 11pt;"
        "  background: white;"
        "  color: black;"
        "  border: none;"
        "  border-radius: 0px;"
        "  padding: 1px 28px 1px 6px;"
        "  min-height: 20px;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 22px;"
        "  border: none;"
        "  border-left: 1px solid #b0b0b0;"
        "  background: #ececec;"
        "}"
        "QComboBox::down-arrow {"
        "%1"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: white;"
        "  color: black;"
        "  selection-background-color: #d0d0d0;"
        "}"
    ).arg(dropdownArrowCss()));
}

void InfantWindow::stylePatientsScreen() {
    m_patientSearch->setStyleSheet(
        "QLineEdit {"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 14pt;"
        "  color: black;"
        "  background: white;"
        "  border: 1px solid #7f9db9;"
        "  padding: 2px 4px;"
        "}"
    );
    QPalette searchPalette = m_patientSearch->palette();
    searchPalette.setColor(QPalette::PlaceholderText, QColor("#C0C0C0"));
    m_patientSearch->setPalette(searchPalette);

    m_dateFilter->setStyleSheet(
        QStringLiteral(
            "QCheckBox {"
            "  color: white;"
            "  font-family: 'Microsoft Sans Serif';"
            "  font-size: 9pt;"
            "  font-weight: bold;"
            "  spacing: 6px;"
            "  background: transparent;"
            "}"
        ) + checkBoxIndicatorCss()
    );

    const QString dateLabelStyle =
        "color: white; font-family: 'Microsoft Sans Serif'; font-size: 8pt; font-weight: bold; background: transparent;";
    m_labelFrom->setStyleSheet(dateLabelStyle);
    m_labelTo->setStyleSheet(dateLabelStyle);

    const QString dateEditStyle = QString(
        "QDateEdit {"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 8pt;"
        "  color: black;"
        "  background: white;"
        "  border: 1px solid #7f9db9;"
        "  padding: 1px 22px 1px 4px;"
        "  min-height: 18px;"
        "}"
        "QDateEdit::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 18px;"
        "  border: none;"
        "  border-left: 1px solid #7f9db9;"
        "  background: white;"
        "}"
        "QDateEdit::down-arrow {"
        "%1"
        "}"
    ).arg(dropdownArrowCss());
    m_dateFrom->setStyleSheet(dateEditStyle);
    m_dateTo->setStyleSheet(dateEditStyle);

    const QString calendarStyle = QStringLiteral(
        "QCalendarWidget { background-color: white; }"
        "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: white; }"
        "QCalendarWidget QToolButton {"
        "  color: black;"
        "  background-color: white;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 9pt;"
        "  border: none;"
        "}"
        "QCalendarWidget QToolButton#qt_calendar_monthbutton {"
        "  min-width: 110px;"
        "  padding-right: 18px;"
        "}"
        "QCalendarWidget QToolButton#qt_calendar_yearbutton {"
        "  min-width: 58px;"
        "}"
        "QCalendarWidget QToolButton::menu-indicator {"
        "  subcontrol-position: right center;"
        "  width: 12px;"
        "  right: 2px;"
        "}"
        "QCalendarWidget QMenu { background-color: white; color: black; }"
        "QCalendarWidget QAbstractItemView:enabled {"
        "  background-color: white;"
        "  color: black;"
        "  selection-background-color: #316ac5;"
        "  selection-color: white;"
        "}"
        "QCalendarWidget QAbstractItemView:disabled { color: #a0a0a0; }"
    );
    if (m_dateFrom->calendarWidget()) {
        m_dateFrom->calendarWidget()->setStyleSheet(calendarStyle);
    }
    if (m_dateTo->calendarWidget()) {
        m_dateTo->calendarWidget()->setStyleSheet(calendarStyle);
    }

    m_patientsTable->setStyleSheet(
        "QTableWidget {"
        "  background: white;"
        "  alternate-background-color: white;"
        "  color: black;"
        "  border: 1px solid #7f9db9;"
        "  gridline-color: #d4d0c8;"
        "  selection-background-color: #316ac5;"
        "  selection-color: white;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 8pt;"
        "  outline: none;"
        "}"
        "QTableWidget::item {"
        "  background: white;"
        "  color: black;"
        "  font-weight: normal;"
        "  padding: 2px 4px;"
        "}"
        "QTableWidget::item:selected {"
        "  background: #316ac5;"
        "  color: white;"
        "}"
        "QHeaderView {"
        "  background: #ebe9e9;"
        "}"
        "QHeaderView::section {"
        "  background-color: #ebe9e9;"
        "  color: black;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 8pt;"
        "  font-weight: bold;"
        "  border: none;"
        "  border-right: 1px solid #d4d0c8;"
        "  border-bottom: 1px solid #d4d0c8;"
        "  padding: 4px 6px;"
        "}"
        "QScrollBar:vertical {"
        "  background: #f0f0f0;"
        "  width: 17px;"
        "  border: 1px solid #aca899;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #c1c1c1;"
        "  min-height: 20px;"
        "}"
    );

    m_patientsTable->setShowGrid(true);
    m_patientsTable->setAlternatingRowColors(false);
    m_patientsTable->horizontalHeader()->setHighlightSections(false);
    m_patientsTable->horizontalHeader()->setStretchLastSection(false);
    m_patientsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_patientsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_patientsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_patientsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_patientsTable->setIconSize(QSize(16, 16));
}

void InfantWindow::styleAnamnesisScreen() {
    m_patientTitle->setStyleSheet(
        "QLabel {"
        "  background-color: #f2f0f0;"
        "  color: #000000;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 11pt;"
        "}"
    );
    m_anamnesisEdit->setFrameShape(QFrame::StyledPanel);
    m_anamnesisEdit->setFrameShadow(QFrame::Sunken);
    m_anamnesisEdit->setLineWidth(1);
    m_anamnesisEdit->setAutoFillBackground(true);
    m_anamnesisEdit->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_anamnesisEdit->setStyleSheet(
        whiteScrollBarCss(QStringLiteral("QTextEdit")) +
        QStringLiteral(
            "QTextEdit {"
            "  background-color: #ffffff;"
            "  background-image: none;"
            "  color: #000000;"
            "  selection-background-color: #316ac5;"
            "  selection-color: #ffffff;"
            "}"
        )
    );
    applyAnamnesisDocumentFontDefaults();
    m_anamnesisEdit->document()->setDocumentMargin(6);
    QPalette editorPalette = m_anamnesisEdit->palette();
    editorPalette.setColor(QPalette::Base, Qt::white);
    editorPalette.setColor(QPalette::Text, Qt::black);
    editorPalette.setColor(QPalette::Window, Qt::white);
    m_anamnesisEdit->setPalette(editorPalette);
    if (m_anamnesisEdit->viewport()) {
        m_anamnesisEdit->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_anamnesisEdit->viewport()->setAutoFillBackground(true);
        m_anamnesisEdit->viewport()->setStyleSheet(
            QStringLiteral("background-color: #ffffff; background-image: none;")
        );
    }
}

void InfantWindow::applyAnamnesisDocumentFontDefaults() {
    if (!m_anamnesisEdit) {
        return;
    }
    QFont font(QStringLiteral("Times New Roman"), 12);
    m_anamnesisEdit->setFont(font);
    m_anamnesisEdit->document()->setDefaultFont(font);
    m_anamnesisEdit->document()->setDefaultStyleSheet(
        "body { font-family: 'Times New Roman'; font-size: 12pt; color: #000000; background-color: #ffffff; }"
        "p { margin-top: 0pt; margin-bottom: 0pt; line-height: 85%; }"
    );
}

void InfantWindow::applyCompactAnamnesisLineSpacing() {
#ifndef Q_OS_WIN
    return;
#else
    if (!m_anamnesisEdit) {
        return;
    }
    QTextDocument *doc = m_anamnesisEdit->document();
    if (!doc || doc->isEmpty()) {
        return;
    }
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        fmt.setLineHeight(85, QTextBlockFormat::ProportionalHeight);
        fmt.setTopMargin(0);
        fmt.setBottomMargin(0);
        cursor.setPosition(block.position());
        cursor.mergeBlockFormat(fmt);
    }
    cursor.endEditBlock();
#endif
}

void InfantWindow::styleProtocolsView() {
    if (!m_protocolsView) {
        return;
    }

    m_protocolsView->setAutoFillBackground(true);
    m_protocolsView->setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_protocolsView->setStyleSheet(
        whiteScrollBarCss(QStringLiteral("QTextEdit")) +
        QStringLiteral(
            "QTextEdit {"
            "  background-color: #ffffff;"
            "  background-image: none;"
            "  color: #000000;"
            "}"
        )
    );
    QPalette browserPalette = m_protocolsView->palette();
    browserPalette.setColor(QPalette::Base, Qt::white);
    browserPalette.setColor(QPalette::Text, Qt::black);
    browserPalette.setColor(QPalette::Window, Qt::white);
    m_protocolsView->setPalette(browserPalette);
    if (m_protocolsView->viewport()) {
        m_protocolsView->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_protocolsView->viewport()->setAutoFillBackground(true);
        m_protocolsView->viewport()->setStyleSheet(
            QStringLiteral("background-color: #ffffff; background-image: none; color: #000000;")
        );
        QPalette viewportPalette = m_protocolsView->viewport()->palette();
        viewportPalette.setColor(QPalette::Base, Qt::white);
        viewportPalette.setColor(QPalette::Window, Qt::white);
        m_protocolsView->viewport()->setPalette(viewportPalette);
    }
    if (m_protocolsView->document()) {
        m_protocolsView->document()->setDefaultStyleSheet(
            QStringLiteral(
                "body { background-color: #ffffff; color: #000000; }"
                "table { table-layout:fixed; width:671px; max-width:671px; min-width:671px; "
                "border-collapse:collapse; border:1px solid #000000; }"
                "td, th { background-color: #ffffff; border:1px solid #000000; "
                "text-align:left; vertical-align:top; }"
                "td[align='center'], th[align='center'] { text-align:center; }"
                "div[contenteditable='true'] { min-height:1.2em; text-align:left; }")
        );
    }
    if (QWidget *corner = m_protocolsView->cornerWidget()) {
        corner->setAutoFillBackground(true);
        corner->setStyleSheet(QStringLiteral("background-color:#ffffff; background-image:none;"));
    }
}

void InfantWindow::styleExercisesScreen() {
    if (!m_authorsFilter || !m_exercisesTree) {
        return;
    }

    static QStyle *exercisesUiStyle = nullptr;
    if (!exercisesUiStyle) {
        exercisesUiStyle = QStyleFactory::create(QStringLiteral("Fusion"));
    }

    if (m_panelExercises) {
        m_panelExercises->setAttribute(Qt::WA_StyledBackground, true);
        m_panelExercises->setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_panelExercises->setAutoFillBackground(true);
        m_panelExercises->setStyleSheet(QStringLiteral(
            "QWidget#exercisesPanel {"
            "  background-color: #ffffff;"
            "  background-image: none;"
            "}"
        ));
        QPalette panelPalette = m_panelExercises->palette();
        panelPalette.setColor(QPalette::Window, Qt::white);
        panelPalette.setColor(QPalette::Base, Qt::white);
        m_panelExercises->setPalette(panelPalette);
        m_panelExercises->update();
    }

    if (m_exercisesTreeHost) {
        m_exercisesTreeHost->setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_exercisesTreeHost->setAutoFillBackground(true);
        m_exercisesTreeHost->setStyleSheet(QStringLiteral(
            "QWidget#exercisesTreeHost {"
            "  background-color: #ffffff;"
            "  background-image: none;"
            "}"
        ));
        m_exercisesTreeHost->update();
    }

    if (m_workStack) {
        m_workStack->setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_workStack->update();
    }

    if (m_authorsFilterHost) {
        m_authorsFilterHost->setAutoFillBackground(true);
        m_authorsFilterHost->update();
    }

    if (exercisesUiStyle) {
        m_authorsFilter->setStyle(exercisesUiStyle);
    }
    m_authorsFilter->setStyleSheet(QString());
    m_authorsFilter->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 9));
    m_authorsFilter->setAutoFillBackground(true);
    QPalette authorPalette = m_authorsFilter->palette();
    authorPalette.setColor(QPalette::Base, Qt::white);
    authorPalette.setColor(QPalette::Button, Qt::white);
    authorPalette.setColor(QPalette::Window, Qt::white);
    authorPalette.setColor(QPalette::Text, Qt::black);
    authorPalette.setColor(QPalette::ButtonText, Qt::black);
    m_authorsFilter->setPalette(authorPalette);

    auto *authorListView = qobject_cast<QListView *>(m_authorsFilter->view());
    if (!authorListView) {
        authorListView = new QListView(m_authorsFilter);
        m_authorsFilter->setView(authorListView);
    }
    if (exercisesUiStyle) {
        authorListView->setStyle(exercisesUiStyle);
    }
    authorListView->setAutoFillBackground(true);
    authorListView->setStyleSheet(QStringLiteral(
        "QListView { background-color: #ffffff; color: #000000; border: 1px solid #7f9db9; font-size: 9pt; }"
        "QListView::item:selected { background-color: #316ac5; color: #ffffff; }"
    ) + whiteScrollBarCss(QStringLiteral("QListView")));

    if (exercisesUiStyle) {
        m_exercisesTree->setStyle(exercisesUiStyle);
    }
    m_exercisesTree->setStyleSheet(
        QStringLiteral(
            "QTreeWidget#exercisesTree {"
            "  background: transparent;"
            "  background-image: none;"
            "  color: #000000;"
            "  border: none;"
            "}"
            "QTreeWidget#exercisesTree::item {"
            "  background-color: #ffffff;"
            "  background-image: none;"
            "  color: #000000;"
            "}"
            "QTreeWidget#exercisesTree::item:selected {"
            "  background-color: #316ac5;"
            "  color: #ffffff;"
            "}"
        ) + whiteScrollBarCss(QStringLiteral("QTreeWidget#exercisesTree"))
    );
    m_exercisesTree->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 9));
    m_exercisesTree->setAutoFillBackground(false);
    m_exercisesTree->setAttribute(Qt::WA_OpaquePaintEvent, false);
    QPalette treePalette = m_exercisesTree->palette();
    treePalette.setColor(QPalette::Base, Qt::white);
    treePalette.setColor(QPalette::Window, Qt::white);
    treePalette.setColor(QPalette::AlternateBase, Qt::white);
    treePalette.setColor(QPalette::Text, Qt::black);
    treePalette.setColor(QPalette::Highlight, QColor(0x31, 0x6a, 0xc5));
    treePalette.setColor(QPalette::HighlightedText, Qt::white);
    m_exercisesTree->setPalette(treePalette);
    if (QWidget *treeViewport = m_exercisesTree->viewport()) {
        treeViewport->setAttribute(Qt::WA_OpaquePaintEvent, true);
        treeViewport->setAutoFillBackground(true);
        QPalette viewportPalette = treeViewport->palette();
        viewportPalette.setColor(QPalette::Base, Qt::white);
        viewportPalette.setColor(QPalette::Window, Qt::white);
        treeViewport->setPalette(viewportPalette);
        treeViewport->setStyleSheet(
            QStringLiteral("background-color: #ffffff; background-image: none;")
        );
        treeViewport->update();
    }
    m_exercisesTree->update();
}

void InfantWindow::styleTemplateComboBox() {
    if (!m_templates) {
        return;
    }
    m_templates->setStyleSheet(QString(
        "QComboBox {"
        "  background: #ffffff;"
        "  color: #000000;"
        "  font-family: 'Microsoft Sans Serif';"
        "  font-size: 12pt;"
        "  border: 1px solid #808080;"
        "  padding: 2px 28px 2px 6px;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 22px;"
        "  border: none;"
        "  border-left: 1px solid #b0b0b0;"
        "  background: #ececec;"
        "}"
        "QComboBox::down-arrow {"
        "%1"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #ffffff;"
        "  color: #000000;"
        "  selection-background-color: #316ac5;"
        "}"
    ).arg(dropdownArrowCss()));
}

void InfantWindow::fitPatientsTableToContent() {
    if (!m_patientsTable) {
        return;
    }

    constexpr int kTableHeight = 800;
    m_patientsTable->setFixedHeight(kTableHeight);
}

void InfantWindow::styleUsersTable() {
    m_usersTable->setStyleSheet(
        "QTableWidget {"
        "  background: transparent;"
        "  alternate-background-color: transparent;"
        "  color: white;"
        "  border: 1px solid white;"
        "  gridline-color: white;"
        "  selection-background-color: rgba(255, 255, 255, 35);"
        "  selection-color: white;"
        "  font-family: 'Tahoma';"
        "  font-size: 12pt;"
        "  outline: none;"
        "}"
        "QTableWidget::item {"
        "  background: transparent;"
        "  color: white;"
        "  border: none;"
        "  padding: 4px 8px;"
        "}"
        "QHeaderView {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QHeaderView::section {"
        "  background-color: transparent;"
        "  color: white;"
        "  font-family: 'Tahoma';"
        "  font-size: 12pt;"
        "  font-style: italic;"
        "  font-weight: normal;"
        "  border: none;"
        "  border-right: 1px solid white;"
        "  border-bottom: 1px solid white;"
        "  padding: 6px 8px;"
        "}"
    );

    m_usersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_usersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_usersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);

    QPalette tablePalette = m_usersTable->palette();
    tablePalette.setColor(QPalette::Base, Qt::transparent);
    tablePalette.setColor(QPalette::AlternateBase, Qt::transparent);
    tablePalette.setColor(QPalette::Text, Qt::white);
    tablePalette.setColor(QPalette::WindowText, Qt::white);
    m_usersTable->setPalette(tablePalette);
    if (m_usersTable->viewport()) {
        m_usersTable->viewport()->setAutoFillBackground(false);
    }

    m_usersTable->setShowGrid(false);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_usersTable->setFocusPolicy(Qt::NoFocus);
    m_usersTable->verticalHeader()->setVisible(false);
    m_usersTable->verticalHeader()->setDefaultSectionSize(40);
    m_usersTable->horizontalHeader()->setVisible(true);
    m_usersTable->horizontalHeader()->setHighlightSections(false);
    m_usersTable->horizontalHeader()->setStretchLastSection(false);
    m_usersTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void InfantWindow::fitUsersTableToContent() {
    if (!m_usersTable) {
        return;
    }

    const int headerH = m_usersTable->horizontalHeader()->height();
    const int rowH = m_usersTable->verticalHeader()->defaultSectionSize();
    const int rows = m_usersTable->rowCount();
    const int border = 2;
    const int contentH = headerH + rows * rowH + border;

    m_usersTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_usersTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_usersTable->setFixedHeight(contentH);
}

void InfantWindow::bindClearableField(QLineEdit *edit, ImageButton *clearBtn) {
    connect(edit, &QLineEdit::textChanged, this, [clearBtn](const QString &text) {
        clearBtn->setVisible(!text.isEmpty());
    });
    connect(clearBtn, &ImageButton::clicked, this, [edit, clearBtn]() {
        edit->clear();
        clearBtn->hide();
    });
}

void InfantWindow::applyEnterLogos() {
    auto placeLogo = [](QLabel *label, const QString &path, int x, int y, bool centerHorizontally, int canvasWidth) {
        if (!label || path.isEmpty()) {
            return;
        }
        const QPixmap pixmap(path);
        if (pixmap.isNull()) {
            return;
        }
        label->setPixmap(pixmap);
        label->setScaledContents(false);
        const int posX = centerHorizontally ? (canvasWidth - pixmap.width()) / 2 : x;
        label->setGeometry(posX, y, pixmap.width(), pixmap.height());
    };

    // infant2.png — иконка слева в верхней панели (как в оригинале: Left=0, Top=7)
    placeLogo(m_logo2, imagePath("infant2.png"), 0, 7, false, width());
    // infant.png — текстовый логотип по центру экрана над формой входа (как в оригинале: Top=81)
    placeLogo(m_logo1, imagePath("infant.png"), 0, 81, true, width());
}

QString InfantWindow::imagePath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../../assets/sysImages",
        QDir::currentPath() + "/assets/sysImages",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/sysImages",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/maindata/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString InfantWindow::resourcePath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/assets/resources",
        QCoreApplication::applicationDirPath() + "/../assets/resources",
        QCoreApplication::applicationDirPath() + "/../../assets/resources",
        QCoreApplication::applicationDirPath() + "/../../../assets/resources",
        QDir::currentPath() + "/assets/resources",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/Resources",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString InfantWindow::htmlPath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/assets/htmls",
        QCoreApplication::applicationDirPath() + "/../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../../assets/htmls",
        QDir::currentPath() + "/assets/htmls",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/htmls"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString InfantWindow::profileConfigPath() const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/ex/names",
        QCoreApplication::applicationDirPath() + "/../../assets/ex/names",
        QCoreApplication::applicationDirPath() + "/../../../assets/ex/names",
        QDir::currentPath() + "/assets/ex/names",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/ex/names",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/maindata/ex/names"
    };
    const QStringList names = {QStringLiteral("шаблон.txt"), QStringLiteral("Шаблон.txt")};
    for (const QString &root : roots) {
        for (const QString &name : names) {
            const QString candidate = QDir(root).filePath(name);
            if (QFile::exists(candidate)) {
                return candidate;
            }
        }
    }
    return {};
}

QString InfantWindow::defaultAnamnesisHtml() const {
    const QString prepared = readAnamnesisTemplateHtml();
    if (!prepared.trimmed().isEmpty()) {
        return prepared;
    }
    return m_repository.defaultAnamnesisTemplate();
}

void InfantWindow::setImage(ImageButton *button, const QString &name) {
    const QString path = imagePath(name);
    if (!path.isEmpty()) {
        button->setImagePath(path);
    }
}

QString InfantWindow::decodeDocument(const QString &raw) const {
    if (raw.trimmed().startsWith("{\\rtf")) {
        return QString();
    }
    if (raw.trimmed().isEmpty()) {
        return defaultAnamnesisHtml();
    }
    return raw;
}

void InfantWindow::loadAnamnesisRtf(const QByteArray &rtf) {
#ifndef Q_OS_WIN
    Q_UNUSED(rtf);
    loadStandardAnamnesisHtml();
    return;
#else
    const QByteArray compactRtf = compactAnamnesisRtf(rtf);
    m_lastAnamnesisRtf = compactRtf;
    if (!m_anamnesisEdit || compactRtf.isEmpty()) {
        loadStandardAnamnesisHtml();
        return;
    }

    m_anamnesisEdit->clear();
    auto *mimeData = new QMimeData();
    mimeData->setData(QStringLiteral("application/rtf"), compactRtf);
    mimeData->setData(QStringLiteral("text/rtf"), compactRtf);
    mimeData->setData(QStringLiteral("Rich Text Format"), compactRtf);
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
    m_anamnesisEdit->setFocus();
    m_anamnesisEdit->paste();
    QApplication::processEvents();

    if (isAnamnesisTemplateLoaded(m_anamnesisEdit)) {
        applyAnamnesisDocumentFontDefaults();
        applyCompactAnamnesisLineSpacing();
        return;
    }

    loadStandardAnamnesisHtml();
#endif
}

void InfantWindow::loadStandardAnamnesisHtml() {
    if (!m_anamnesisEdit) {
        return;
    }
    m_lastAnamnesisRtf.clear();
    const QString prepared = readAnamnesisTemplateHtml();
    if (prepared.trimmed().isEmpty()) {
        m_anamnesisEdit->setHtml(m_repository.defaultAnamnesisTemplate());
        applyAnamnesisDocumentFontDefaults();
        return;
    }
    m_anamnesisEdit->setHtml(prepared);
    applyAnamnesisDocumentFontDefaults();
#ifndef Q_OS_WIN
    return;
#else
    applyCompactAnamnesisLineSpacing();
#endif
}

void InfantWindow::applyAnamnesisFont(int pointSize) {
    if (pointSize <= 0 || !m_anamnesisEdit) {
        return;
    }
    QTextCharFormat format;
    format.setFontFamily(QStringLiteral("Times New Roman"));
    format.setFontPointSize(pointSize / 2.0);
    m_anamnesisEdit->mergeCurrentCharFormat(format);
    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(QString::number(pointSize));
    }
}

void InfantWindow::applyAnamnesisFontToEntireDocument(int pointSize) {
    if (pointSize <= 0 || !m_anamnesisEdit) {
        return;
    }

    const double fontPt = pointSize / 2.0;
    QTextDocument *doc = m_anamnesisEdit->document();
    if (!doc) {
        return;
    }

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    format.setFontFamily(QStringLiteral("Times New Roman"));
    format.setFontPointSize(fontPt);
    cursor.mergeCharFormat(format);
    cursor.clearSelection();
    cursor.endEditBlock();

    QFont defaultFont(QStringLiteral("Times New Roman"));
    defaultFont.setPointSizeF(fontPt);
    doc->setDefaultFont(defaultFont);
    doc->setDefaultStyleSheet(
        QStringLiteral(
            "body { font-family: 'Times New Roman'; font-size: %1pt; color: #000000; background-color: #ffffff; }"
            "p { margin-top: 0pt; margin-bottom: 0pt; line-height: 85%%; }"
        ).arg(fontPt, 0, 'f', 1)
    );
    m_anamnesisEdit->setFont(defaultFont);

    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(QString::number(pointSize));
    }
}

void InfantWindow::applyAnamnesisDocument(const QString &raw) {
    if (raw.trimmed().startsWith("{\\rtf")) {
#ifndef Q_OS_WIN
        loadStandardAnamnesisHtml();
#else
        loadAnamnesisRtf(raw.toLocal8Bit());
#endif
        return;
    }
    m_lastAnamnesisRtf.clear();
    if (raw.trimmed().isEmpty()) {
        loadStandardAnamnesisHtml();
        return;
    }
    if (isRawRtfPlainText(raw)) {
        loadStandardAnamnesisHtml();
        return;
    }
    QString html = raw;
#ifndef Q_OS_WIN
    if (isWordExportHtml(html) || html.size() > 100000) {
        html = prepareAnamnesisHtml(html);
        if (html.trimmed().isEmpty()) {
            loadStandardAnamnesisHtml();
            return;
        }
    }
#endif
    m_anamnesisEdit->setHtml(html);
    applyAnamnesisDocumentFontDefaults();
#ifndef Q_OS_WIN
    return;
#else
    applyCompactAnamnesisLineSpacing();
#endif
}

void InfantWindow::loadDefaultAnamnesisTemplate() {
    QString profileName = QStringLiteral("Стандартный");
    const QString configPath = profileConfigPath();
    if (!configPath.isEmpty()) {
        QFile configFile(configPath);
        if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
            if (!parts.isEmpty() && !parts.first().trimmed().isEmpty()) {
                profileName = parts.first().trimmed();
            }
        }
    }

    if (profileName == QStringLiteral("Стандартный")) {
#ifndef Q_OS_WIN
        loadStandardAnamnesisHtml();
#else
        const QString rtfPath = htmlPath("anamnez.rtf");
        if (!rtfPath.isEmpty()) {
            QFile rtfFile(rtfPath);
            if (rtfFile.open(QIODevice::ReadOnly)) {
                loadAnamnesisRtf(rtfFile.readAll());
            } else {
                loadStandardAnamnesisHtml();
            }
        } else {
            loadStandardAnamnesisHtml();
        }
#endif
        int fontSize = 24;
        const QString configPath = profileConfigPath();
        if (!configPath.isEmpty()) {
            QFile configFile(configPath);
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
                if (parts.size() > 1) {
                    fontSize = parts.at(1).toInt();
                }
            }
        }
        if (m_fontSlider) {
            QSignalBlocker blocker(m_fontSlider);
            m_fontSlider->setValue(fontSize);
        }
        changeDocumentFontSize(fontSize);
        return;
    }

    const QString templateData = m_repository.loadTemplate(profileName);
    if (templateData.trimmed().isEmpty()) {
        loadStandardAnamnesisHtml();
        int fontSize = 24;
        const QString configPath = profileConfigPath();
        if (!configPath.isEmpty()) {
            QFile configFile(configPath);
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
                if (parts.size() > 1) {
                    fontSize = parts.at(1).toInt();
                }
            }
        }
        if (m_fontSlider) {
            QSignalBlocker blocker(m_fontSlider);
            m_fontSlider->setValue(fontSize);
        }
        changeDocumentFontSize(fontSize);
        return;
    }
    applyAnamnesisDocument(templateData);
}

void InfantWindow::saveUser() {
    if (m_userSaveInProgress) {
        return;
    }

    const bool creating = m_adminLabel1->text() == QStringLiteral("Создать пользователя");
    if (creating) {
        if (m_userFio->text().trimmed().isEmpty() || m_userLogin->text().trimmed().isEmpty()
            || m_userPass->text().isEmpty() || m_userPass2->text().isEmpty()) {
            CustomMessageBox::showWarning(this, "Заполните все поля.");
            return;
        }
    } else if (m_userFio->text().trimmed().isEmpty() || m_userLogin->text().trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Заполните все поля.");
        return;
    }
    if (m_userPass->text() != m_userPass2->text()) {
        CustomMessageBox::showWarning(this, "Символы в полях 'придумайте пароль' и 'подтвердите пароль' должны быть полностью идентичны.");
        return;
    }

    const QString fio = m_userFio->text().trimmed();
    const QString login = m_userLogin->text().trimmed();
    const QString password = m_userPass->text();
    const QString role = m_userRole->currentText();
    const QString mainId = m_mainId;
    const QString editUserId = m_editUserId;

    m_userSaveInProgress = true;
    m_userSaveButton->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    auto *watcher = new QFutureWatcher<UserSaveResult>(this);
    connect(watcher, &QFutureWatcher<UserSaveResult>::finished, this, [this, watcher]() {
        const UserSaveResult result = watcher->result();
        watcher->deleteLater();

        QApplication::restoreOverrideCursor();
        m_userSaveInProgress = false;
        m_userSaveButton->setEnabled(true);

        if (!result.ok) {
            CustomMessageBox::showError(this, result.error);
            return;
        }
        rememberManagedUser(result.userId, result.login, result.password);
        resetUserCreateForm();
        scheduleRefreshUsers();
    });

    watcher->setFuture(QtConcurrent::run([creating, fio, login, password, role, mainId, editUserId]() {
        ApiClient api;
        Repository repository(&api);
        QString error;
        UserSaveResult result;
        result.login = login;
        result.password = password;
        if (creating) {
            QString createdId;
            result.ok = repository.createUser(fio, login, password, role, mainId, &error, &createdId);
            result.userId = createdId;
        } else {
            result.ok = repository.updateUser(editUserId, fio, login, password, role, &error);
            result.userId = editUserId;
        }
        result.error = error;
        return result;
    }));
}

void InfantWindow::rememberManagedUser(const QString &userId, const QString &login, const QString &password) {
    if (!userId.isEmpty()) {
        m_lastManagedUserId = userId;
    }
    if (!login.isEmpty()) {
        m_lastManagedUserLogin = login;
    }
    if (!password.isEmpty()) {
        m_lastManagedUserPassword = password;
    }
}

void InfantWindow::enterAsManagedUser() {
    const QList<UserRecord> users = m_repository.fetchUsers();
    if (users.isEmpty()) {
        CustomMessageBox::showWarning(this, QStringLiteral("Вначале добавьте пользователя"));
        return;
    }

    if (m_session.has_value()) {
        refreshPatients();
        setScreen(ScreenMode::Patients);
        return;
    }

    QString login = m_loginEdit ? m_loginEdit->text().trimmed() : QString();
    QString password = m_passwordEdit ? m_passwordEdit->text() : QString();
    const UserRecord &lastUser = users.last();

    if (login.isEmpty()) {
        login = lastUser.login;
    }
    if (password.isEmpty()) {
        if (m_lastManagedUserLogin == login) {
            password = m_lastManagedUserPassword;
        } else if (m_lastManagedUserId == lastUser.id) {
            login = m_lastManagedUserLogin.isEmpty() ? lastUser.login : m_lastManagedUserLogin;
            password = m_lastManagedUserPassword;
        }
    }

    if (login.isEmpty()) {
        login = lastUser.login;
    }
    if (password.isEmpty()) {
        CustomMessageBox::showWarning(this, QStringLiteral("Введите пароль пользователя для входа."));
        return;
    }

    const auto user = m_repository.login(login, password);
    if (!user.has_value()) {
        CustomMessageBox::showError(this, QStringLiteral("Неверный логин или пароль!"));
        return;
    }
    if (user->role != QStringLiteral("Администратор") && user->role != QStringLiteral("Специалист")) {
        CustomMessageBox::showError(this, QStringLiteral("Недостаточный уровень доступа."));
        return;
    }

    m_session = user;
    m_mainId = user->id;
    rememberManagedUser(user->id, user->login, password);
    refreshPatients();
    setScreen(ScreenMode::Patients);
}

void InfantWindow::resetUserCreateForm() {
    m_editUserId.clear();
    m_adminLabel1->setText("Создать пользователя");
    m_userFio->clear();
    m_userLogin->clear();
    m_userPass->clear();
    m_userPass2->clear();
    m_userPass->setEchoMode(QLineEdit::Password);
    m_userPass2->setEchoMode(QLineEdit::Password);
    setImage(m_adminEye1, "pon.png");
    setImage(m_adminEye2, "pon.png");
    m_userFioClear->hide();
    m_userLoginClear->hide();
    m_userPassClear->hide();
    m_userPass2Clear->hide();
}

void InfantWindow::loadUserForEdit(const QString &id) {
    const auto user = m_repository.fetchUserById(id);
    if (!user.has_value()) {
        return;
    }
    m_editUserId = id;
    m_adminLabel1->setText("Изменить данные пользователя");
    if (m_lastManagedUserId != id) {
        m_lastManagedUserPassword.clear();
    }
    m_lastManagedUserId = id;
    m_lastManagedUserLogin = user->login;
    m_userFio->setText(user->fio);
    m_userLogin->setText(user->login);
    m_userRole->setCurrentText(user->role);
    m_userPass->clear();
    m_userPass2->clear();
    m_userPass->setEchoMode(QLineEdit::Password);
    m_userPass2->setEchoMode(QLineEdit::Password);
    setImage(m_adminEye1, "pon.png");
    setImage(m_adminEye2, "pon.png");
    m_userFioClear->hide();
    m_userLoginClear->hide();
    m_userPassClear->hide();
    m_userPass2Clear->hide();
}

void InfantWindow::deleteUserById(const QString &userId) {
    if (!CustomMessageBox::askConfirm(this, "Вы собираетесь удалить данные!\nЭто действие необратимо!")) {
        return;
    }
    QString err;
    if (!m_repository.deleteUser(userId, &err)) {
        CustomMessageBox::showError(this, err);
        return;
    }
    if (m_editUserId == userId) {
        resetUserCreateForm();
    }
    scheduleRefreshUsers();
}

void InfantWindow::handlePatientsTableClick(int row, int column) {
    if (row < 0) {
        return;
    }
    if (column == 1) {
        m_patientsTable->selectRow(row);
        openPatientFromTable();
        return;
    }
    if (column == 3) {
        const QString patientId = m_patientsTable->item(row, 0)->text();
        if (!CustomMessageBox::askConfirm(this, "Вы собираетесь удалить данные!\nЭто действие необратимо!")) {
            return;
        }
        QString err;
        if (!m_repository.deletePatient(patientId, &err)) {
            CustomMessageBox::showError(this, err);
            return;
        }
        if (m_currentPatientId == patientId) {
            m_currentPatientId.clear();
        }
        refreshPatients();
    }
}

void InfantWindow::scheduleRefreshUsers() {
    QTimer::singleShot(0, this, [this]() { refreshUsers(); });
}

void InfantWindow::refreshUsers() {
    if (!m_usersTable) {
        return;
    }

    const QList<UserRecord> users = m_repository.fetchUsers();
    const QString editPath = imagePath("edit.png");
    const QString deletePath = imagePath("delete.png");
    constexpr int kActionBtnSize = 30;

    const QSize actionIconSize(kActionBtnSize, kActionBtnSize);
    const QPixmap editPixmap = tintedPixmap(editPath, Qt::white, actionIconSize);
    const QPixmap deletePixmap = tintedPixmap(deletePath, QColor(255, 70, 70), actionIconSize);

    const int oldRows = m_usersTable->rowCount();
    for (int i = 0; i < oldRows; ++i) {
        if (QWidget *widget = m_usersTable->cellWidget(i, 3)) {
            m_usersTable->removeCellWidget(i, 3);
            widget->deleteLater();
        }
    }
    m_usersTable->setRowCount(0);
    m_usersTable->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        auto makeTextItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setForeground(QBrush(Qt::white));
            return item;
        };
        const QString userId = users.at(i).id;
        m_usersTable->setItem(i, 0, makeTextItem(userId));
        m_usersTable->setItem(i, 1, makeTextItem(users.at(i).fio));
        m_usersTable->setItem(i, 2, makeTextItem(users.at(i).login));

        auto *roleCell = new QWidget(m_usersTable);
        roleCell->setAttribute(Qt::WA_StyledBackground, true);
        roleCell->setStyleSheet("background: transparent;");
        auto *layout = new QHBoxLayout(roleCell);
        layout->setContentsMargins(8, 2, 8, 2);
        layout->setSpacing(6);

        auto *roleLabel = new QLabel(users.at(i).role, roleCell);
        roleLabel->setStyleSheet("color: white; background: transparent; font-family: 'Tahoma'; font-size: 12pt;");
        layout->addWidget(roleLabel, 1);

        auto *editBtn = new ImageButton(roleCell);
        editBtn->setFixedSize(kActionBtnSize, kActionBtnSize);
        editBtn->setCursor(Qt::PointingHandCursor);
        if (!editPixmap.isNull()) {
            editBtn->setPixmap(editPixmap);
        }
        connect(editBtn, &ImageButton::clicked, this, [this, userId]() {
            loadUserForEdit(userId);
        });
        layout->addWidget(editBtn, 0, Qt::AlignVCenter);

        auto *deleteBtn = new ImageButton(roleCell);
        deleteBtn->setFixedSize(kActionBtnSize, kActionBtnSize);
        deleteBtn->setCursor(Qt::PointingHandCursor);
        if (!deletePixmap.isNull()) {
            deleteBtn->setPixmap(deletePixmap);
        }
        connect(deleteBtn, &ImageButton::clicked, this, [this, userId]() {
            deleteUserById(userId);
        });
        layout->addWidget(deleteBtn, 0, Qt::AlignVCenter);

        auto *placeholder = new QTableWidgetItem;
        placeholder->setFlags(Qt::NoItemFlags);
        m_usersTable->setItem(i, 3, placeholder);
        m_usersTable->setCellWidget(i, 3, roleCell);
    }
    m_usersTable->setColumnWidth(1, 180);
    m_usersTable->setColumnWidth(2, 120);
    m_usersTable->setColumnWidth(3, 236);
    styleUsersTable();
    fitUsersTableToContent();
    m_usersTable->viewport()->update();
}

void InfantWindow::refreshPatients() {
    QList<PatientRecord> patients = m_repository.fetchPatients(
        m_patientSearch->text(),
        m_dateFilter->isChecked(),
        m_dateFrom->date(),
        m_dateTo->date()
    );

    const auto compareBirthDates = [](const PatientRecord &a, const PatientRecord &b) {
        const QDate da = QDate::fromString(a.birthDate.trimmed(), "dd.MM.yyyy");
        const QDate db = QDate::fromString(b.birthDate.trimmed(), "dd.MM.yyyy");
        if (da.isValid() && db.isValid()) {
            return da < db;
        }
        return a.birthDate.localeAwareCompare(b.birthDate) < 0;
    };

    if (m_patientSortColumn == 1) {
        std::sort(patients.begin(), patients.end(), [this](const PatientRecord &a, const PatientRecord &b) {
            const int cmp = a.fio.localeAwareCompare(b.fio);
            return m_patientSortAscending ? cmp < 0 : cmp > 0;
        });
    } else {
        std::sort(patients.begin(), patients.end(), [this, compareBirthDates](const PatientRecord &a, const PatientRecord &b) {
            const bool less = compareBirthDates(a, b);
            return m_patientSortAscending ? less : !less;
        });
    }

    if (auto *delegate = dynamic_cast<PatientsItemDelegate *>(m_patientsDelegate)) {
        delegate->setSearchText(m_patientSearch->text());
    }

    const QIcon deleteIcon(imagePath("delete.png"));
    int restoreRow = -1;
    m_patientsTable->setRowCount(patients.size());
    for (int i = 0; i < patients.size(); ++i) {
        const PatientRecord &p = patients.at(i);
        m_patientsTable->setItem(i, 0, new QTableWidgetItem(p.id));
        m_patientsTable->setItem(i, 1, new QTableWidgetItem(p.fio));
        m_patientsTable->setItem(i, 2, new QTableWidgetItem(p.birthDate));
        auto *deleteItem = new QTableWidgetItem;
        deleteItem->setIcon(deleteIcon);
        deleteItem->setTextAlignment(Qt::AlignCenter);
        m_patientsTable->setItem(i, 3, deleteItem);
        if (!m_selectedPatientRowId.isEmpty() && p.id == m_selectedPatientRowId) {
            restoreRow = i;
        }
    }
    m_patientsTable->setColumnWidth(1, 250);
    m_patientsTable->setColumnWidth(2, 135);
    m_patientsTable->setColumnWidth(3, 28);

    const QString fioHeader = QStringLiteral("ФИО") + (m_patientSortColumn == 1 ? (m_patientSortAscending ? QStringLiteral(" ▼") : QStringLiteral(" ▲")) : QString());
    const QString drHeader = QStringLiteral("День рождения") + (m_patientSortColumn == 2 ? (m_patientSortAscending ? QStringLiteral(" ▼") : QStringLiteral(" ▲")) : QString());
    m_patientsTable->setHorizontalHeaderLabels({QStringLiteral("id"), fioHeader, drHeader, QStringLiteral("Уд.")});

    if (restoreRow >= 0) {
        m_patientsTable->selectRow(restoreRow);
    }

    stylePatientsScreen();
    fitPatientsTableToContent();
    m_patientsTable->viewport()->update();
}

void InfantWindow::handlePatientsHeaderClick(int section) {
    if (section != 1 && section != 2) {
        return;
    }
    if (m_patientSortColumn == section) {
        m_patientSortAscending = !m_patientSortAscending;
    } else {
        m_patientSortColumn = section;
        m_patientSortAscending = true;
    }
    refreshPatients();
}

void InfantWindow::refreshTemplateNames() {
    if (!m_templates) {
        return;
    }
    const QString current = m_templates->currentText().trimmed();
    const QSignalBlocker blocker(m_templates);
    m_templates->clear();
    m_templates->addItems(m_repository.loadTemplateNames());
    const int idx = m_templates->findText(current, Qt::MatchExactly);
    if (idx >= 0) {
        m_templates->setCurrentIndex(idx);
    } else if (!current.isEmpty()) {
        m_templates->setEditText(current);
    }
}

QString InfantWindow::anamnesisTemplatePayload() const {
    if (!m_anamnesisEdit) {
        return {};
    }
    return m_anamnesisEdit->toHtml();
}

void InfantWindow::loadAnamnesisTemplateByName(const QString &name) {
    if (name.isEmpty() || m_suppressTemplateLoad) {
        return;
    }
    writeProfileConfig(name, m_fontSlider ? m_fontSlider->value() : 24);
    if (name == QStringLiteral("Стандартный")) {
        loadDefaultAnamnesisTemplate();
        return;
    }
    const QString templateData = m_repository.loadTemplate(name);
    if (templateData.trimmed().isEmpty()) {
        return;
    }
    applyAnamnesisDocument(templateData);
}

void InfantWindow::saveCurrentAnamnesisTemplate() {
    if (!m_templates || !m_anamnesisEdit) {
        return;
    }
    const QString name = m_templates->currentText().trimmed();
    if (name.isEmpty()) {
        CustomMessageBox::showWarning(this, QStringLiteral("Введите название шаблона."));
        return;
    }
    if (name == QStringLiteral("Стандартный")) {
        CustomMessageBox::showWarning(this, QStringLiteral("Шаблон «Стандартный» нельзя перезаписать."));
        return;
    }
    const int fontSize = m_fontSlider ? m_fontSlider->value() : 24;
    prepareAnamnesisDocumentForOutput();
    QString err;
    if (!m_repository.saveTemplate(name, fontSize, anamnesisTemplatePayload(), &err)) {
        CustomMessageBox::showError(this, err);
        return;
    }
    writeProfileConfig(name, fontSize);
    refreshTemplateNames();
    {
        QSignalBlocker blocker(m_templates);
        const int idx = m_templates->findText(name, Qt::MatchExactly);
        if (idx >= 0) {
            m_templates->setCurrentIndex(idx);
        } else {
            m_templates->addItem(name);
            m_templates->setCurrentIndex(m_templates->findText(name, Qt::MatchExactly));
        }
    }
    hideSlidePanels();
}

void InfantWindow::syncTemplateSelectorFromProfile() {
    if (!m_templates) {
        return;
    }
    QString profileName = QStringLiteral("Стандартный");
    int fontSize = 24;
    const QString configPath = profileConfigPath();
    if (!configPath.isEmpty()) {
        QFile configFile(configPath);
        if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QStringList parts = QString::fromUtf8(configFile.readAll()).trimmed().split(';');
            if (!parts.isEmpty() && !parts.first().trimmed().isEmpty()) {
                profileName = parts.first().trimmed();
            }
            if (parts.size() > 1) {
                fontSize = parts.at(1).toInt();
            }
        }
    }
    m_suppressTemplateLoad = true;
    const QSignalBlocker blocker(m_templates);
    const int idx = m_templates->findText(profileName, Qt::MatchExactly);
    if (idx >= 0) {
        m_templates->setCurrentIndex(idx);
    } else {
        m_templates->setEditText(profileName);
    }
    if (m_fontSlider) {
        QSignalBlocker sliderBlocker(m_fontSlider);
        m_fontSlider->setValue(fontSize);
    }
    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(QString::number(fontSize));
    }
    if (m_currentScreen == ScreenMode::Anamnesis && m_anamnesisEdit) {
        changeDocumentFontSize(fontSize, false);
    }
    m_suppressTemplateLoad = false;
}

void InfantWindow::openPatientFromTable() {
    const int row = m_patientsTable->currentRow();
    if (row < 0) {
        return;
    }
    const QString patientId = m_patientsTable->item(row, 0)->text();
    QString err;
    if (!m_repository.verifyPatientAccess(patientId, m_licenseKey, &err)) {
        CustomMessageBox::showError(this, err);
        return;
    }
    const QString fio = m_patientsTable->item(row, 1)->text();
    const QString dr = m_patientsTable->item(row, 2)->text();
    m_currentPatientId = patientId;
    m_selectedPatientRowId = patientId;
    m_patientTitle->setText(fio);
    clearProtocolsView();
    m_protocolViewRecordIds = m_repository.loadPatientProtocolRecordIds(patientId);
    applyAnamnesisDocument(m_repository.loadPatientAnamnesis(patientId));
    if (m_fontSlider) {
        changeDocumentFontSize(m_fontSlider->value(), false);
    }
    setScreen(ScreenMode::Anamnesis);
}

void InfantWindow::updatePatientTitleFromDocument() {
    if (!m_anamnesisEdit || !m_patientTitle || m_currentScreen != ScreenMode::Anamnesis) {
        return;
    }
    QString fio;
    QString dr;
    m_repository.extractPatientFields(m_anamnesisEdit->toPlainText(), &fio, &dr);
    Q_UNUSED(dr);
    const QString trimmedFio = fio.trimmed();
    if (trimmedFio.size() > 3) {
        m_patientTitle->setText(trimmedFio);
    } else if (m_currentPatientId.isEmpty()) {
        m_patientTitle->setText(QStringLiteral("Новая карта"));
    }
}

void InfantWindow::setAnamnesisDbControlsEnabled(bool enabled) {
    const bool allow = enabled && !m_anamnesisSaveInProgress;
    if (m_bList) {
        m_bList->setEnabled(allow);
    }
    if (m_bBack) {
        m_bBack->setEnabled(allow);
    }
    if (m_pAna) {
        m_pAna->setEnabled(allow);
    }
    if (m_pProto) {
        m_pProto->setEnabled(allow);
    }
    if (m_pUpr) {
        m_pUpr->setEnabled(allow);
    }
    if (m_addPatient) {
        m_addPatient->setEnabled(allow);
    }
    if (m_bExit) {
        m_bExit->setEnabled(allow);
    }
}

void InfantWindow::tryAutoSaveAnamnesis(bool forceRefreshPatients) {
    if (m_anamnesisSaveInProgress) {
        return;
    }
    if (m_currentScreen != ScreenMode::Anamnesis || !m_anamnesisEdit) {
        return;
    }

    m_anamnesisSaveInProgress = true;
    setAnamnesisDbControlsEnabled(false);

    prepareAnamnesisDocumentForOutput();
    QString patientId = m_currentPatientId;
    QString fio;
    QString dr;
    QString err;
    const QString plainText = m_anamnesisEdit->toPlainText();
    const bool saved = m_repository.savePatientAnamnesis(
        &patientId,
        m_licenseKey,
        plainText,
        m_anamnesisEdit->toHtml(),
        &fio,
        &dr,
        &err);

    if (saved) {
        m_currentPatientId = patientId;
        if (!fio.trimmed().isEmpty()) {
            m_patientTitle->setText(fio.trimmed());
            m_selectedPatientRowId = patientId;
        }
        if (forceRefreshPatients) {
            refreshPatients();
        }
    }

    m_anamnesisSaveInProgress = false;
    setAnamnesisDbControlsEnabled(true);
}

void InfantWindow::saveAnamnesisToDb() {
    if (m_anamnesisSaveInProgress) {
        return;
    }
    m_anamnesisSaveInProgress = true;
    setAnamnesisDbControlsEnabled(false);

    prepareAnamnesisDocumentForOutput();
    QString patientId = m_currentPatientId;
    QString fio;
    QString dr;
    QString err;
    if (!m_repository.savePatientAnamnesis(
            &patientId,
            m_licenseKey,
            m_anamnesisEdit->toPlainText(),
            m_anamnesisEdit->toHtml(),
            &fio,
            &dr,
            &err)) {
        m_anamnesisSaveInProgress = false;
        setAnamnesisDbControlsEnabled(true);
        CustomMessageBox::showError(this, err);
        return;
    }
    m_currentPatientId = patientId;
    m_patientTitle->setText(fio.trimmed());
    refreshPatients();
    m_anamnesisSaveInProgress = false;
    setAnamnesisDbControlsEnabled(true);
    CustomMessageBox::showInfo(this, "Сохранено.");
}

bool InfantWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_anamnesisEdit && event->type() == QEvent::FocusOut && !m_screenTransitionGuard) {
        tryAutoSaveAnamnesis();
    }

    if (watched == m_patientsTable->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            const QModelIndex index = m_patientsTable->indexAt(mouseEvent->pos());
            const int row = index.isValid() ? index.row() : -1;
            if (row != m_hoveredPatientRow) {
                m_hoveredPatientRow = row;
                if (auto *delegate = dynamic_cast<PatientsItemDelegate *>(m_patientsDelegate)) {
                    delegate->setHoverRow(m_hoveredPatientRow);
                }
                m_patientsTable->viewport()->update();
            }
        } else if (event->type() == QEvent::Leave) {
            m_hoveredPatientRow = -1;
            if (auto *delegate = dynamic_cast<PatientsItemDelegate *>(m_patientsDelegate)) {
                delegate->setHoverRow(-1);
            }
            m_patientsTable->viewport()->update();
        }
    }

    if (watched == m_bPrint) {
        if (event->type() == QEvent::Enter) {
            if (m_printTooltipTimer) {
                m_printTooltipTimer->start();
            }
        } else if (event->type() == QEvent::Leave) {
            if (m_printTooltipTimer) {
                m_printTooltipTimer->stop();
            }
            QToolTip::hideText();
        }
    }

    if (watched == m_root && event->type() == QEvent::MouseButtonPress) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint localPos = m_root->mapFromGlobal(mouseEvent->globalPos());
        auto outsidePanel = [&](QWidget *panel) {
            return panel && panel->isVisible() && !panel->geometry().contains(localPos);
        };
        if (outsidePanel(m_settingsPanel) && outsidePanel(m_savePanel) && outsidePanel(m_printPanel)) {
            hideSlidePanels();
        }
        if (m_infoPopup && m_infoPopup->isVisible()) {
            const QPoint local = m_infoPopup->mapFromGlobal(mouseEvent->globalPos());
            if (!m_infoPopup->rect().contains(local)) {
                m_infoPopup->hide();
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void InfantWindow::toggleSlidePanel(QWidget *panel) {
    if (!panel) {
        return;
    }
    if (panel->isVisible() && panel->y() >= 60) {
        animateSlidePanel(panel, false);
        m_activeSlidePanel = nullptr;
        return;
    }
    hideSlidePanels();
    animateSlidePanel(panel, true);
    m_activeSlidePanel = panel;
}

void InfantWindow::hideSlidePanels() {
    for (QWidget *panel : {m_settingsPanel, m_savePanel, m_printPanel}) {
        if (panel && panel->isVisible()) {
            panel->hide();
        }
    }
    m_activeSlidePanel = nullptr;
    showSettingsSaveTemplateView(false);
}

void InfantWindow::animateSlidePanel(QWidget *panel, bool showPanel) {
    if (!panel) {
        return;
    }
    if (!showPanel) {
        panel->hide();
        return;
    }
    panel->show();
    panel->raise();
    auto *animation = new QPropertyAnimation(panel, "pos", panel);
    animation->setDuration(150);
    animation->setStartValue(QPoint(panel->x(), -panel->height()));
    animation->setEndValue(QPoint(panel->x(), 70));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void InfantWindow::showSettingsSaveTemplateView(bool show) {
    if (!m_settingsMainView || !m_settingsSaveView) {
        return;
    }
    m_settingsTemplateMode = show;
    m_settingsMainView->setVisible(!show);
    m_settingsSaveView->setVisible(show);
    if (show) {
        m_templateNameEdit->clear();
    }
}

void InfantWindow::installToolbarTooltips() {
    if (!m_bPrint) {
        return;
    }
    m_bPrint->setAttribute(Qt::WA_Hover, true);
    m_bPrint->installEventFilter(this);
    if (!m_printTooltipTimer) {
        m_printTooltipTimer = new QTimer(this);
        m_printTooltipTimer->setSingleShot(true);
        m_printTooltipTimer->setInterval(1000);
        connect(m_printTooltipTimer, &QTimer::timeout, this, [this]() {
            if (m_bPrint && m_bPrint->underMouse()) {
                QToolTip::showText(QCursor::pos(), QStringLiteral("Печать"), m_bPrint);
            }
        });
    }
}

void InfantWindow::updateFormatButtonIcons() {
    if (!m_anamnesisEdit) {
        return;
    }
    const bool underline = m_anamnesisEdit->currentCharFormat().fontUnderline();
    const bool bold = m_anamnesisEdit->currentCharFormat().fontWeight() >= QFont::Bold;
    m_underlineActive = underline;
    m_boldActive = bold;

    auto applyImage = [](ImageButton *button, const QStringList &candidates) {
        for (const QString &file : candidates) {
            if (!file.isEmpty() && QFile::exists(file)) {
                button->setImagePath(file);
                return;
            }
        }
    };
    applyImage(m_underlineButton, {
        underline ? resourcePath("Подчеркивание нажата.png") : QString(),
        resourcePath("undeline.png"),
        imagePath("undeline.png")
    });
    applyImage(m_boldButton, {
        bold ? resourcePath("Жирный шрифт Нажата.png") : QString(),
        resourcePath("Жирный шрифт.png"),
        imagePath("Жирный шрифт.png")
    });
}

void InfantWindow::changeDocumentFontSize(int pointSize, bool persistProfile) {
    if (!m_anamnesisEdit || pointSize <= 0) {
        return;
    }
    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(QString::number(pointSize));
    }

    bool reloadedFromRtf = false;
#ifndef Q_OS_WIN
    m_lastAnamnesisRtf.clear();
    applyAnamnesisFontToEntireDocument(pointSize);
#else
    if (!m_lastAnamnesisRtf.isEmpty()) {
        QString rtf = QString::fromLatin1(m_lastAnamnesisRtf);
        for (int size = 21; size <= 28; ++size) {
            rtf.replace(QStringLiteral("fs%1").arg(size), QStringLiteral("fs%1").arg(pointSize));
        }
        m_lastAnamnesisRtf = rtf.toLatin1();
        loadAnamnesisRtf(m_lastAnamnesisRtf);
        reloadedFromRtf = true;
    }
    if (!reloadedFromRtf) {
        QString html = m_anamnesisEdit->toHtml();
        for (int size = 21; size <= 28; ++size) {
            html.replace(QStringLiteral("fs%1").arg(size), QStringLiteral("fs%1").arg(pointSize));
            const QString oldPt = QString::number(size / 2.0, 'f', 1);
            const QString newPt = QString::number(pointSize / 2.0, 'f', 1);
            html.replace(
                QStringLiteral("font-size:%1pt").arg(oldPt),
                QStringLiteral("font-size:%1pt").arg(newPt)
            );
        }
        m_anamnesisEdit->setHtml(html);
    }

    applyAnamnesisFont(pointSize);
    applyCompactAnamnesisLineSpacing();
#endif
    if (persistProfile && m_templates) {
        writeProfileConfig(m_templates->currentText(), pointSize);
    }
}

void InfantWindow::prepareAnamnesisDocumentForOutput() {
    if (!m_anamnesisEdit) {
        return;
    }
    const int fontSize = m_fontSlider ? m_fontSlider->value() : 24;
    changeDocumentFontSize(fontSize, false);
}

void InfantWindow::writeProfileConfig(const QString &profileName, int fontSize) {
    const QString path = profileConfigPath();
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QString("%1;%2\n").arg(profileName).arg(fontSize).toUtf8());
    }
}

QString InfantWindow::exerciseNamesPath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/ex/names",
        QCoreApplication::applicationDirPath() + "/../../assets/ex/names",
        QDir::currentPath() + "/assets/ex/names",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/ex/names",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/maindata/ex/names"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString InfantWindow::protocolsDocumentHtml(const QString &innerContent) const {
    const QString trimmed = innerContent.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    if (trimmed.startsWith(QStringLiteral("<!DOCTYPE"), Qt::CaseInsensitive)
        || trimmed.startsWith(QStringLiteral("<html"), Qt::CaseInsensitive)) {
        return ExerciseAssets::wrapProtocolDocumentHtml(trimmed);
    }
    return ExerciseAssets::wrapProtocolDocumentHtml(
        QStringLiteral(
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">%1</head>"
            "<body>%2</body></html>")
            .arg(ExerciseAssets::protocolTableStyleHtml(), trimmed));
}

void InfantWindow::saveProtocolsEdits(bool force) {
    if (!m_protocolsView || m_currentPatientId.isEmpty() || m_protocolViewRecordIds.isEmpty()) {
        return;
    }
    if (!force && !m_protocolsViewDirty) {
        return;
    }
    if (force && m_protocolsView->hasFocus()) {
        m_protocolsView->clearFocus();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    QTextDocument *document = m_protocolsView->document();
    if (!document || document->isEmpty()) {
        return;
    }
    QString error;
    if (m_repository.updateProtocolsFromEditedDocument(document, m_protocolViewRecordIds, &error)) {
        m_protocolsViewDirty = false;
    }
}

void InfantWindow::clearProtocolsView() {
    if (!m_protocolsView) {
        return;
    }
    m_protocolViewRecordIds.clear();
    m_protocolsViewDirty = false;
    m_protocolsView->clear();
}

void InfantWindow::refreshProtocolsView() {
    if (!m_protocolsView || m_currentScreen != ScreenMode::Protocols) {
        return;
    }

    if (!m_currentPatientId.isEmpty() && m_protocolsViewDirty) {
        saveProtocolsEdits();
    }

    const QString fio = m_patientTitle ? m_patientTitle->text().trimmed() : QString();
    const QString birthDate = currentPatientBirthDate();
    if (m_protocolsPatient) {
        m_protocolsPatient->setText(
            QStringLiteral("ФИО: %1       Дата рождения: %2").arg(fio, birthDate));
    }
    if (m_protocolsTitle) {
        m_protocolsTitle->show();
    }
    if (m_protocolsPatient) {
        m_protocolsPatient->show();
    }

    QString inner;
    if (m_currentPatientId.isEmpty()) {
        inner.clear();
        m_protocolViewRecordIds.clear();
    } else {
        m_protocolViewRecordIds = m_repository.loadPatientProtocolRecordIds(m_currentPatientId);
        inner = m_repository.loadPatientProtocols(m_currentPatientId);
    }

    if (m_protocolsSaveTimer) {
        m_protocolsSaveTimer->stop();
    }
    m_protocolsSuppressDirty = true;
    m_protocolsView->setUpdatesEnabled(false);
    if (inner.trimmed().isEmpty()) {
        const QString emptyHtml = ExerciseAssets::wrapProtocolDocumentHtml(QStringLiteral(
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">%1</head>"
            "<body style=\"background-color:#ffffff;color:#000000;margin:0;padding:8px;min-height:900px;\">"
            "<p>Пока ни одного протокола не сформировано</p></body></html>")
            .arg(ExerciseAssets::protocolTableStyleHtml()));
        m_protocolsView->setHtml(emptyHtml);
        if (QTextDocument *doc = m_protocolsView->document()) {
            doc->setDocumentMargin(0);
            doc->setTextWidth(671);
        }
        m_protocolsView->moveCursor(QTextCursor::Start);
    } else {
        const QString html = protocolsDocumentHtml(inner);
        m_protocolsView->setHtml(html);
        if (QTextDocument *doc = m_protocolsView->document()) {
            doc->setDocumentMargin(12);
            doc->setTextWidth(671);
        }
        m_protocolsView->moveCursor(QTextCursor::Start);
    }
    m_protocolsSuppressDirty = false;
    m_protocolsViewDirty = false;
    ProtocolEditGuard::setMode(m_protocolsView, ProtocolEditGuard::Mode::LimitedEdit);
    if (QScrollBar *scrollBar = m_protocolsView->verticalScrollBar()) {
        scrollBar->setValue(0);
        scrollBar->setStyleSheet(QStringLiteral(
            "QScrollBar:vertical { background-color:#ffffff; background-image:none; border:none; width:14px; }"
            "QScrollBar::handle:vertical { background-color:#c1c1c1; min-height:20px; border:none; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
            "  background-color:#ffffff; background-image:none; border:none; }"));
    }
    m_protocolsView->setUpdatesEnabled(true);
    if (m_protocolsView->viewport()) {
        m_protocolsView->viewport()->update();
    }
}

QString InfantWindow::exerciseFavoritesPath() const {
    return exerciseNamesPath(QStringLiteral("избранное.txt"));
}

bool InfantWindow::isExerciseInFavorites(const QString &exerciseId) const {
    const QString path = exerciseFavoritesPath();
    if (path.isEmpty() || exerciseId.isEmpty()) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    for (const QString &line : QString::fromUtf8(file.readAll()).split('\n')) {
        const QString id = line.trimmed().split(';').value(0);
        if (id == exerciseId) {
            return true;
        }
    }
    return false;
}

bool InfantWindow::addExerciseToFavorites(const QString &exerciseId, const QString &displayText) {
    if (exerciseId.isEmpty() || isExerciseInFavorites(exerciseId)) {
        return false;
    }
    const QString path = exerciseFavoritesPath();
    if (path.isEmpty()) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return false;
    }
    file.write(QStringLiteral("%1;%2\n").arg(exerciseId, displayText).toUtf8());
    return true;
}

bool InfantWindow::removeExerciseFromFavorites(const QString &exerciseId) {
    const QString path = exerciseFavoritesPath();
    if (path.isEmpty() || exerciseId.isEmpty()) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QStringList kept;
    for (const QString &line : QString::fromUtf8(file.readAll()).split('\n')) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (trimmed.split(';').value(0) != exerciseId) {
            kept << trimmed;
        }
    }
    file.close();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    for (const QString &line : kept) {
        file.write(line.toUtf8());
        file.write("\n");
    }
    return true;
}

void InfantWindow::showExercisesContextMenu(const QPoint &pos) {
    if (!m_exercisesTree) {
        return;
    }
    QTreeWidgetItem *item = m_exercisesTree->itemAt(pos);
    if (!item || item->childCount() > 0) {
        return;
    }
    const QString exerciseId = item->data(0, Qt::UserRole).toString().trimmed();
    if (exerciseId.isEmpty()) {
        return;
    }

    QMenu menu(this);
    QAction *addAction = menu.addAction(QStringLiteral("Добавить в избранное"));
    QAction *removeAction = menu.addAction(QStringLiteral("Удалить из избранного"));
    const bool inFavorites = isExerciseInFavorites(exerciseId);
    addAction->setEnabled(!inFavorites);
    removeAction->setEnabled(inFavorites);

    QAction *chosen = menu.exec(m_exercisesTree->viewport()->mapToGlobal(pos));
    if (!chosen) {
        return;
    }
    if (chosen == addAction) {
        if (addExerciseToFavorites(exerciseId, item->text(0))) {
            refreshExercisesTree();
        }
        return;
    }
    if (chosen == removeAction) {
        if (removeExerciseFromFavorites(exerciseId)) {
            refreshExercisesTree();
        }
    }
}

void InfantWindow::refreshExercisesTree() {
    if (!m_exercisesTree) {
        return;
    }
    m_exercisesTree->clear();

    const QString authorFilter = m_authorsFilter ? m_authorsFilter->currentText() : QStringLiteral("Все авторы");

    const auto loadLines = [this](const QString &fileName) {
        QList<QStringList> rows;
        const QString path = exerciseNamesPath(fileName);
        if (path.isEmpty()) {
            return rows;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return rows;
        }
        const QStringList lines = QString::fromUtf8(file.readAll()).split('\n');
        for (const QString &line : lines) {
            const QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) {
                continue;
            }
            const QStringList parts = trimmed.split(';');
            if (parts.size() >= 3) {
                rows.push_back(parts);
            }
        }
        return rows;
    };

    QStringList favorites;
    const QString favPath = exerciseNamesPath(QStringLiteral("избранное.txt"));
    if (!favPath.isEmpty()) {
        QFile favFile(favPath);
        if (favFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            for (const QString &line : QString::fromUtf8(favFile.readAll()).split('\n')) {
                const QString id = line.trimmed().split(';').value(0);
                if (!id.isEmpty()) {
                    favorites << id;
                }
            }
        }
    }

    const auto shouldInclude = [&](const QStringList &parts) -> bool {
        const QString author = parts.at(2).trimmed();
        if (authorFilter == QStringLiteral("Избранное")) {
            return favorites.contains(parts.at(0));
        }
        if (authorFilter == QStringLiteral("Все авторы")) {
            return true;
        }
        return author.contains(authorFilter, Qt::CaseInsensitive);
    };

    const auto addExercise = [&](QTreeWidgetItem *parent, const QStringList &parts) {
        if (!shouldInclude(parts)) {
            return;
        }
        const QString title = parts.at(1).trimmed() + QLatin1Char(' ') + parts.at(2).trimmed();
        auto *item = new QTreeWidgetItem(parent, {title});
        item->setData(0, Qt::UserRole, parts.at(0));
        if (parts.size() >= 4) {
            item->setData(0, Qt::UserRole + 1, parts.at(3));
        }
        QFont itemFont = item->font(0);
        itemFont.setPointSize(9);
        item->setFont(0, itemFont);
    };

    const auto populateFile = [&](QTreeWidgetItem *parent, const QString &fileName) {
        for (const QStringList &parts : loadLines(fileName)) {
            addExercise(parent, parts);
        }
    };

    const auto makeGroup = [&](const QString &title) -> QTreeWidgetItem * {
        auto *groupItem = new QTreeWidgetItem(m_exercisesTree, {title});
        QFont groupFont = groupItem->font(0);
        groupFont.setBold(true);
        groupFont.setPointSize(12);
        groupItem->setFont(0, groupFont);
        return groupItem;
    };

    const auto makeSubgroup = [&](QTreeWidgetItem *parent, const QString &title) -> QTreeWidgetItem * {
        auto *subItem = new QTreeWidgetItem(parent, {title});
        QFont subFont = subItem->font(0);
        subFont.setBold(true);
        subFont.setPointSize(12);
        subItem->setFont(0, subFont);
        return subItem;
    };

    populateFile(makeGroup(QStringLiteral("Восприятие")), QStringLiteral("1.txt"));
    populateFile(makeGroup(QStringLiteral("Внимание")), QStringLiteral("2.txt"));

    auto *thinking = makeGroup(QStringLiteral("Мышление"));
    populateFile(
        makeSubgroup(thinking, QStringLiteral("Образно – логическое мышление")),
        QStringLiteral("3.txt"));
    populateFile(
        makeSubgroup(thinking, QStringLiteral("Словесно (вербально) – логическое мышление")),
        QStringLiteral("4.txt"));
    populateFile(
        makeSubgroup(thinking, QStringLiteral("Наглядно – действенное мышление")),
        QStringLiteral("5.txt"));

    auto *memory = makeGroup(QStringLiteral("Память"));
    populateFile(
        makeSubgroup(memory, QStringLiteral("Кратковременная зрительная память")),
        QStringLiteral("6.txt"));
    populateFile(
        makeSubgroup(memory, QStringLiteral("Кратковременная слуховая память")),
        QStringLiteral("7.txt"));
    if (!loadLines(QStringLiteral("8.txt")).isEmpty()) {
        populateFile(
            makeSubgroup(memory, QStringLiteral("Опосредственное запоминание")),
            QStringLiteral("8.txt"));
    }

    auto *speech = makeGroup(QStringLiteral("Диагностика речи"));
    populateFile(speech, QStringLiteral("9.txt"));
    populateFile(speech, QStringLiteral("10.txt"));
    populateFile(speech, QStringLiteral("11.txt"));

    for (int i = 0; i < m_exercisesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *topItem = m_exercisesTree->topLevelItem(i);
        const auto hasVisibleDescendant = [&](const auto &self, QTreeWidgetItem *node) -> bool {
            if (node->childCount() == 0) {
                return node->data(0, Qt::UserRole).isValid();
            }
            for (int childIndex = 0; childIndex < node->childCount(); ++childIndex) {
                if (self(self, node->child(childIndex))) {
                    return true;
                }
            }
            return false;
        };
        if (authorFilter == QStringLiteral("Избранное")) {
            topItem->setExpanded(hasVisibleDescendant(hasVisibleDescendant, topItem));
        } else {
            topItem->setExpanded(false);
        }
        for (int j = 0; j < topItem->childCount(); ++j) {
            QTreeWidgetItem *subItem = topItem->child(j);
            if (subItem->childCount() > 0) {
                if (authorFilter == QStringLiteral("Избранное")) {
                    subItem->setExpanded(hasVisibleDescendant(hasVisibleDescendant, subItem));
                } else {
                    subItem->setExpanded(true);
                }
            }
        }
    }
}

void InfantWindow::openExercise(const QString &exerciseId) {
    if (m_currentPatientId.isEmpty()) {
        CustomMessageBox::showWarning(this, QStringLiteral("Сначала выберите пациента в списке."));
        return;
    }
    if (!ExerciseConfig::isRunnable(exerciseId)) {
        CustomMessageBox::showInfo(this, ExerciseConfig::unsupportedMessage(exerciseId));
        return;
    }

    if (!m_exerciseHost) {
        m_exerciseHost = new ExerciseHost(m_root);
        m_exerciseHost->setGeometry(0, kTitleBarHeight, kDesignWidth, kDesignHeight - kTitleBarHeight);
        m_exerciseHost->hide();
        connect(m_exerciseHost, &ExerciseHost::protocolSaved, this, [this]() { refreshProtocolsView(); });
        connect(m_exerciseHost, &ExerciseHost::exerciseOverlayChanged, this, [this](bool overlayActive) {
            setWorkChromeVisible(!overlayActive);
        });
    }

    if (m_workStack) {
        m_workStack->hide();
    }
    if (m_authorsFilterHost) {
        m_authorsFilterHost->hide();
    }

    QString patientFio = m_patientTitle ? m_patientTitle->text().trimmed() : QString();
    if (patientFio == QStringLiteral("Новая карта")) {
        patientFio.clear();
    }
    const QString specialistFio = m_session ? m_session->fio : QString();
    m_helpIndex = exerciseId + QStringLiteral(".html");
    m_exerciseHost->openExercise(
        exerciseId,
        m_currentPatientId,
        specialistFio,
        patientFio,
        currentPatientBirthDate(),
        &m_repository,
        AppSettings::dualScreenEnabled());
    m_exerciseOpen = true;
    m_exerciseHost->show();
    if (m_workStack) {
        m_workStack->lower();
    }
    m_exerciseHost->raise();
    if (m_bSettings) {
        m_bSettings->hide();
    }
    raiseChromeWidgets();
}

void InfantWindow::setWorkChromeVisible(bool visible) {
    const bool exercisesList = m_currentScreen == ScreenMode::Exercises && !m_exerciseOpen;
    const bool showSettings = visible && m_currentScreen == ScreenMode::Anamnesis;
    const QWidgetList chromeWidgets = {
        m_bBack, m_bList, m_bExit, m_bSave, m_bPrint, m_bInfo,
        m_pAna, m_pProto, m_pUpr, m_patientTitle, m_userOpenPatients
    };
    for (QWidget *widget : chromeWidgets) {
        if (widget) {
            widget->setVisible(visible);
        }
    }
    if (m_bSettings) {
        m_bSettings->setVisible(showSettings);
    }
    if (m_authorsFilterHost) {
        m_authorsFilterHost->setVisible(visible && exercisesList);
    }
    if (m_authorsFilter) {
        m_authorsFilter->setVisible(visible && exercisesList);
    }
    if (m_bClose) {
        m_bClose->setVisible(visible);
    }
    if (m_bLine) {
        m_bLine->setVisible(visible);
    }
    if (m_bUp) {
        m_bUp->setVisible(visible);
    }
    if (visible) {
        raiseChromeWidgets();
    }
}

void InfantWindow::raiseChromeWidgets() {
    const QWidgetList chromeWidgets = {
        m_bBack, m_bList, m_bExit, m_bSave, m_bPrint, m_bSettings, m_bInfo,
        m_bClose, m_bLine, m_bUp, m_pAna, m_pProto, m_pUpr,
        m_patientTitle, m_userOpenPatients
    };
    for (QWidget *widget : chromeWidgets) {
        if (widget && widget->isVisible()) {
            widget->raise();
        }
    }
    if (m_authorsFilterHost && m_authorsFilterHost->isVisible()) {
        m_authorsFilterHost->raise();
    }
}

void InfantWindow::closeExerciseHost() {
    if (m_exerciseHost) {
        m_exerciseHost->saveProtocolEdits();
        m_exerciseHost->hide();
    }
    m_exerciseOpen = false;

    if (m_currentScreen == ScreenMode::Exercises) {
        if (m_workStack) {
            m_workStack->setCurrentWidget(m_panelExercises);
            m_workStack->show();
            m_workStack->raise();
        }
        if (m_authorsFilterHost) {
            m_authorsFilterHost->show();
        }
        raiseChromeWidgets();
    }
    refreshProtocolsView();
}

QString InfantWindow::currentPatientBirthDate() const {
    if (!m_currentPatientId.isEmpty() && m_patientsTable) {
        for (int row = 0; row < m_patientsTable->rowCount(); ++row) {
            const QTableWidgetItem *idItem = m_patientsTable->item(row, 0);
            const QTableWidgetItem *birthItem = m_patientsTable->item(row, 2);
            if (idItem && birthItem && idItem->text() == m_currentPatientId) {
                return birthItem->text().trimmed();
            }
        }
    }
    QString birthDate;
    if (m_anamnesisEdit) {
        m_repository.extractPatientFields(m_anamnesisEdit->toPlainText(), nullptr, &birthDate);
    }
    return birthDate.trimmed();
}

QString InfantWindow::protocolsExportHeader() const {
    QString fio = m_patientTitle ? m_patientTitle->text().trimmed() : QString();
    if (fio == QStringLiteral("Новая карта")) {
        fio.clear();
    }
    const QString birthDate = currentPatientBirthDate();
    return QStringLiteral("ФИО: %1 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;    Дата рождения: %2")
        .arg(fio, birthDate);
}

QString InfantWindow::assembleExportHtml(const ExportSelection &selection) {
    if (!selection.anamnesis && !selection.protocols) {
        return {};
    }

    QStringList parts;
    if (selection.anamnesis && m_anamnesisEdit) {
        parts << m_anamnesisEdit->toHtml();
    }

    if (selection.protocols) {
        if (m_currentPatientId.isEmpty()) {
            return {};
        }
        if (!selection.forPatient && !selection.forSpecialist) {
            return {};
        }

        if (m_protocolsViewDirty) {
            saveProtocolsEdits(true);
        }

        const QString patientData = protocolsExportHeader();
        if (selection.forPatient) {
            const QString html = m_repository.loadPatientProtocolsForExport(
                m_currentPatientId, QStringLiteral("p"), patientData);
            if (!html.isEmpty()) {
                parts << html;
            }
        }
        if (selection.forSpecialist) {
            const QString html = m_repository.loadPatientProtocolsForExport(
                m_currentPatientId, QStringLiteral("s"), patientData);
            if (!html.isEmpty()) {
                parts << html;
            }
        }
    }

    if (parts.isEmpty()) {
        return {};
    }
    if (parts.size() == 1) {
        return parts.first();
    }

    QString combined;
    for (int i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            combined += QStringLiteral("<div style='page-break-before:always'></div><hr>");
        }
        const QString &part = parts.at(i);
        const int bodyStart = part.indexOf(QStringLiteral("<body"), 0, Qt::CaseInsensitive);
        if (bodyStart >= 0) {
            const int tagEnd = part.indexOf(QLatin1Char('>'), bodyStart);
            const int bodyEnd = part.lastIndexOf(QStringLiteral("</body>"), -1, Qt::CaseInsensitive);
            if (tagEnd >= 0 && bodyEnd > tagEnd) {
                combined += part.mid(tagEnd + 1, bodyEnd - tagEnd - 1);
                continue;
            }
        }
        combined += part;
    }
    return ExerciseAssets::buildProtocolDocumentHtml(combined);
}

void InfantWindow::renderExportToPrinter(
    QPrinter &printer, const ExportSelection &selection, const QString &assembledHtml) {
    const bool onlyAnamnesis = selection.anamnesis && !selection.protocols;
    if (onlyAnamnesis && m_anamnesisEdit) {
        std::unique_ptr<QTextDocument> doc(m_anamnesisEdit->document()->clone());
        doc->print(&printer);
        return;
    }

    QTextDocument doc;
    doc.setHtml(assembledHtml);
    doc.print(&printer);
}

InfantWindow::ExportSelection InfantWindow::saveExportSelection() const {
    ExportSelection selection;
    selection.anamnesis = m_saveAnamnesisCb && m_saveAnamnesisCb->isChecked();
    selection.protocols = m_saveProtocolsCb && m_saveProtocolsCb->isChecked();
    selection.forPatient = m_saveForPatientCb && m_saveForPatientCb->isChecked();
    selection.forSpecialist = m_saveForSpecialistCb && m_saveForSpecialistCb->isChecked();
    return selection;
}

InfantWindow::ExportSelection InfantWindow::printExportSelection() const {
    ExportSelection selection;
    selection.anamnesis = m_printAnamnesisCb && m_printAnamnesisCb->isChecked();
    selection.protocols = m_printProtocolsCb && m_printProtocolsCb->isChecked();
    selection.forPatient = m_printForPatientCb && m_printForPatientCb->isChecked();
    selection.forSpecialist = m_printForSpecialistCb && m_printForSpecialistCb->isChecked();
    return selection;
}

bool InfantWindow::validateExportSelection(
    const ExportSelection &selection,
    const QString &emptyMessage,
    const QString &protocolsMessage) {
    if (!selection.anamnesis && !selection.protocols) {
        CustomMessageBox::showWarning(this, emptyMessage);
        return false;
    }
    if (selection.protocols && !selection.forPatient && !selection.forSpecialist) {
        CustomMessageBox::showWarning(this, protocolsMessage);
        return false;
    }
    if (selection.protocols && m_currentPatientId.isEmpty()) {
        CustomMessageBox::showWarning(this, "Сначала выберите пациента.");
        return false;
    }
    return true;
}

void InfantWindow::exportDocument() {
    hideSlidePanels();
    QString suggested = m_patientTitle->text().trimmed();
    if (suggested.isEmpty() || suggested == QStringLiteral("Новая карта")) {
        suggested = QStringLiteral("Документ");
    }

    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString suggestedPath = QDir(homeDir).filePath(suggested + QStringLiteral(".pdf"));

    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Сохранить как"),
        suggestedPath,
        QStringLiteral("PDF (*.pdf);;Word (*.doc)")
    );
    if (path.isEmpty()) {
        return;
    }

    const ExportSelection selection = saveExportSelection();
    if (!validateExportSelection(
            selection,
            QStringLiteral("Выберите содержимое для сохранения."),
            QStringLiteral("Выберите часть протоколов для сохранения."))) {
        return;
    }

    if (selection.anamnesis) {
        prepareAnamnesisDocumentForOutput();
    }

    const QString content = assembleExportHtml(selection);
    if (content.trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Нет данных для сохранения.");
        return;
    }

    const bool onlyAnamnesis = selection.anamnesis && !selection.protocols;

    if (path.endsWith(".pdf", Qt::CaseInsensitive)) {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(path);
        renderExportToPrinter(printer, selection, content);
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        CustomMessageBox::showError(this, "Не удалось сохранить файл.");
        return;
    }
    if (onlyAnamnesis && !m_lastAnamnesisRtf.isEmpty()) {
        file.write(m_lastAnamnesisRtf);
        return;
    }
    QString exportContent = content;
    if (path.endsWith(QStringLiteral(".doc"), Qt::CaseInsensitive)) {
        exportContent = ExerciseProtocol::stripMethodologyFillForDocExport(exportContent);
    }
    file.write(exportContent.toUtf8());
}

void InfantWindow::printSelectedContent() {
    hideSlidePanels();
    const ExportSelection selection = printExportSelection();
    if (!validateExportSelection(
            selection,
            QStringLiteral("Выберите содержимое для печати."),
            QStringLiteral("Выберите часть протоколов для печати."))) {
        return;
    }

    if (selection.anamnesis) {
        prepareAnamnesisDocumentForOutput();
    }

    const QString content = assembleExportHtml(selection);
    if (content.trimmed().isEmpty()) {
        CustomMessageBox::showWarning(this, "Нет данных для печати.");
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(QStringLiteral("Печать"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    renderExportToPrinter(printer, selection, content);
}

void InfantWindow::showInfoPopup() {
    if (!m_infoPopup) {
        m_infoPopup = new QDialog(this, Qt::FramelessWindowHint);
        m_infoPopup->setFixedSize(447, 133);
        m_infoPopup->setAttribute(Qt::WA_TranslucentBackground, false);
        const QString bg = imagePath("popup.png");
        m_infoPopup->setStyleSheet("QDialog { background-image: url('" + bg + "'); }");

        auto createLabel = [&](const QString &text, const QRect &rect, auto callback) {
            QLabel *label = new QLabel(text, m_infoPopup);
            label->setGeometry(rect);
            label->setStyleSheet("color:black;background:transparent;font: 11.25pt 'Microsoft Sans Serif';");
            label->setCursor(Qt::PointingHandCursor);
            class ClickLabel final : public QLabel {
            public:
                using QLabel::QLabel;
                std::function<void()> onClick;
            protected:
                void mousePressEvent(QMouseEvent *event) override {
                    if (onClick) {
                        onClick();
                    }
                    QLabel::mousePressEvent(event);
                }
            };
            ClickLabel *clickLabel = new ClickLabel(m_infoPopup);
            clickLabel->setText(text);
            clickLabel->setGeometry(rect);
            clickLabel->setStyleSheet(label->styleSheet());
            clickLabel->setCursor(label->cursor());
            clickLabel->onClick = callback;
            label->deleteLater();
            return clickLabel;
        };

        createLabel("Руководство пользователя к данной странице", QRect(23, 23, 380, 18), [this]() {
            m_infoPopup->hide();
            const QString path = htmlPath("spravka/" + m_helpIndex);
            if (!path.isEmpty()) {
                showHelpWindow(path);
            }
        });
        createLabel("Общее руководство пользователя", QRect(23, 56, 300, 18), [this]() {
            m_infoPopup->hide();
            QString path = htmlPath("spravka/руководствопользователя.html");
            if (path.isEmpty()) {
                path = htmlPath("spravka/основныеэлементыуправления.html");
            }
            if (!path.isEmpty()) {
                showHelpWindow(path);
            }
        });
        createLabel("О программе", QRect(23, 89, 140, 18), [this]() {
            m_infoPopup->hide();
            showAboutWindow();
        });
    }

    QPoint p = mapToGlobal(QPoint(1400, -150));
    m_infoPopup->move(p);
    m_infoPopup->show();
    m_infoPopup->raise();
    auto *animation = new QPropertyAnimation(m_infoPopup, "pos", m_infoPopup);
    animation->setDuration(150);
    animation->setStartValue(QPoint(p.x(), 20));
    animation->setEndValue(QPoint(p.x(), 70));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void InfantWindow::showHelpWindow(const QString &address) {
    const auto resolveHelpLinkTarget = [this](const QUrl &link) -> QString {
        if (m_currentHelpFilePath.isEmpty()) {
            return {};
        }
        const QUrl baseUrl = QUrl::fromLocalFile(m_currentHelpFilePath);
        const QString path = baseUrl.resolved(link).toLocalFile();
        if (path.isEmpty()) {
            return {};
        }
        if (QFile::exists(path)) {
            return path;
        }
        const QDir dir(QFileInfo(path).absolutePath());
        const QString fileName = QFileInfo(path).fileName();
        for (const QString &entry : dir.entryList(QDir::Files)) {
            if (entry.compare(fileName, Qt::CaseInsensitive) == 0) {
                return dir.filePath(entry);
            }
        }
        return htmlPath(QStringLiteral("spravka/") + fileName);
    };

    const auto loadHelpPage = [this, resolveHelpLinkTarget](const QString &path) {
        if (!m_helpBrowser || path.isEmpty()) {
            return;
        }
        QString rawSource;
        const QString html = loadHelpHtmlFromFile(path, &rawSource);
        if (html.isEmpty()) {
            return;
        }
        m_currentHelpFilePath = path;
        const QFileInfo info(path);
        m_helpBrowser->document()->setBaseUrl(QUrl::fromLocalFile(info.absolutePath() + QStringLiteral("/")));
        m_helpBrowser->document()->setDocumentMargin(8);
        m_helpBrowser->document()->setDefaultStyleSheet(buildHelpDefaultStylesheet(rawSource));
        m_helpBrowser->setHtml(html);
        compactHelpDocumentSpacing(m_helpBrowser->document());
    };

    const auto navigateHelpLink = [this, loadHelpPage, resolveHelpLinkTarget](const QUrl &url) {
        if (!m_helpBrowser) {
            return;
        }

        const QString fragment = url.fragment();
        const QUrl pageUrl = url.adjusted(QUrl::RemoveFragment);
        if (pageUrl.path().isEmpty() && !fragment.isEmpty()) {
            m_helpBrowser->scrollToAnchor(fragment);
            return;
        }

        const QString target = resolveHelpLinkTarget(pageUrl.isEmpty() ? url : pageUrl);
        if (target.isEmpty()) {
            return;
        }

        if (target != m_currentHelpFilePath) {
            loadHelpPage(target);
        }
        if (!fragment.isEmpty()) {
            m_helpBrowser->scrollToAnchor(fragment);
        }
    };

    if (!m_helpWindow) {
        m_helpWindow = new QDialog(this, Qt::FramelessWindowHint);
        m_helpWindow->setFixedSize(873, 900);
        m_helpWindow->setStyleSheet("QDialog { background-image: url('" + imagePath("spravka.png") + "'); }");

        auto *helpBrowser = new HelpTextBrowser(m_helpWindow);
        m_helpBrowser = helpBrowser;
        helpBrowser->setLinkHandler(navigateHelpLink);
        m_helpBrowser->setGeometry(10, 110, 851, 767);
        m_helpBrowser->setOpenExternalLinks(false);

        new HelpWindowDragFilter(m_helpWindow, 110, m_helpWindow);

        ImageButton *closeBtn = new ImageButton(m_helpWindow);
        closeBtn->setGeometry(817, 12, 44, 38);
        const QString closeRes = resourcePath("Закрыть.png");
        if (!closeRes.isEmpty()) {
            closeBtn->setImagePath(closeRes);
        }
        connect(closeBtn, &ImageButton::clicked, m_helpWindow, &QDialog::hide);

        ImageButton *manualBtn = new ImageButton(m_helpWindow);
        manualBtn->setGeometry(733, 74, 100, 30);
        manualBtn->setImagePath(imagePath("toogl.png"));
        connect(manualBtn, &ImageButton::clicked, this, [this, loadHelpPage]() {
            QString path = htmlPath("spravka/руководствопользователя.html");
            if (path.isEmpty()) {
                path = htmlPath("spravka/основныеэлементыуправления.html");
            }
            loadHelpPage(path);
        });
    }

    loadHelpPage(address);
    m_helpWindow->show();
    m_helpWindow->raise();
    m_helpWindow->activateWindow();
}

void InfantWindow::showAboutWindow() {
    if (!m_aboutWindow) {
        m_aboutWindow = new QDialog(this, Qt::FramelessWindowHint);
        m_aboutWindow->setFixedSize(447, 133);
        m_aboutWindow->setStyleSheet("QDialog { background-image: url('" + imagePath("popup.png") + "'); }");

        auto addText = [&](const QString &text, const QRect &r, int size) {
            QLabel *l = new QLabel(text, m_aboutWindow);
            l->setGeometry(r);
            l->setStyleSheet("color:black;background:transparent;font:" + QString::number(size) + "pt 'Microsoft Sans Serif';");
        };
        addText("О программе", QRect(170, 29, 150, 18), 11);
        addText("Программа для ЭВМ «Пособие для оценки, мониторинга и ", QRect(20, 57, 410, 16), 9);
        addText("скрининга психофизического развития ребенка «Инфант» ", QRect(20, 73, 410, 16), 9);
        addText("(интерактивная программа)» по ТУ 58.29.32-001-42446431-2019", QRect(12, 89, 430, 16), 9);
        addText("Версия 1.0 от 12.12.2019.", QRect(143, 105, 180, 16), 9);

        ImageButton *closeBtn = new ImageButton(m_aboutWindow);
        closeBtn->setGeometry(404, 11, 31, 29);
        QString closeRes = resourcePath("Закрыть.png");
        if (!closeRes.isEmpty()) {
            closeBtn->setImagePath(closeRes);
        }
        connect(closeBtn, &ImageButton::clicked, m_aboutWindow, &QDialog::hide);
    }
    m_aboutWindow->show();
    m_aboutWindow->raise();
    m_aboutWindow->activateWindow();
}
