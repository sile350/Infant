#include "protocoleditguard.h"

#include <QAbstractTextDocumentLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextTable>
#include <QVariant>
#include <QWidget>

namespace {

constexpr char kGuardProperty[] = "protocolEditGuard";

QString readProtocolTableCellText(QTextTable *table, int row, int column) {
    if (!table || row < 0 || column < 0 || row >= table->rows() || column >= table->columns()) {
        return {};
    }
    const QTextTableCell cell = table->cellAt(row, column);
    if (!cell.isValid()) {
        return {};
    }
    QTextCursor cursor = cell.firstCursorPosition();
    cursor.setPosition(cell.lastCursorPosition().position(), QTextCursor::KeepAnchor);
    QString text = cursor.selectedText();
    text.replace(QChar(0x2029), QLatin1Char(' '));
    text.replace(QChar::ParagraphSeparator, QLatin1Char(' '));
    return text.trimmed();
}

bool isLockedHeaderCell(const QString &cellText) {
    return cellText.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
        || cellText.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
        || cellText.contains(QStringLiteral("Виды возможной помощи"), Qt::CaseInsensitive);
}

bool isEditableProtocolCursor(const QTextCursor &cursor) {
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
    const QString cellText = readProtocolTableCellText(table, row, col);
    if (isLockedHeaderCell(cellText)) {
        return false;
    }
    const QString firstCell = readProtocolTableCellText(table, row, 0);
    if (firstCell.contains(QStringLiteral("Результат"), Qt::CaseInsensitive) && col == 1) {
        return true;
    }
    if (firstCell.contains(QStringLiteral("Примечание"), Qt::CaseInsensitive) && col == 1) {
        return true;
    }
    // OR / HLP после формирования (как contenteditable в оригинале).
    if ((firstCell.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
         || firstCell.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive))
        && col >= 1) {
        return true;
    }
    // Колонка «Баллы» + ячейки итоговых сумм (1.26 и др.).
    int ballsCol = -1;
    int ballsHeaderRow = -1;
    int selectedPicCol = -1;
    int explanationCol = -1;
    int activityCol = -1;
    int helpCol = -1;
    for (int r = 0; r < table->rows(); ++r) {
        for (int c = 0; c < table->columns(); ++c) {
            const QString header = readProtocolTableCellText(table, r, c);
            if (ballsCol < 0
                && (header.compare(QStringLiteral("Баллы"), Qt::CaseInsensitive) == 0
                    || (header.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)
                        && header.length() <= 12))) {
                ballsCol = c;
                ballsHeaderRow = r;
            }
            if (selectedPicCol < 0
                && header.contains(QStringLiteral("Выбранная картинка"), Qt::CaseInsensitive)) {
                selectedPicCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
            if (explanationCol < 0
                && header.contains(QStringLiteral("Объяснение выбора"), Qt::CaseInsensitive)) {
                explanationCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
            if (activityCol < 0
                && header.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                activityCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
            if (helpCol < 0 && header.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                && !header.contains(QStringLiteral("возможной"), Qt::CaseInsensitive)) {
                helpCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
        }
    }
    if (ballsHeaderRow >= 0 && row > ballsHeaderRow) {
        if (selectedPicCol >= 0 && col == selectedPicCol) {
            return true;
        }
        if (explanationCol >= 0 && col == explanationCol) {
            return true;
        }
        if (activityCol >= 0 && col == activityCol) {
            return true;
        }
        if (helpCol >= 0 && col == helpCol) {
            return true;
        }
    }
    if (ballsCol >= 0 && col == ballsCol && row > ballsHeaderRow) {
        return true;
    }
    if (firstCell.contains(QStringLiteral("Итоговая оценка"), Qt::CaseInsensitive) && col >= 1) {
        return true;
    }
    if (firstCell.contains(QStringLiteral("Индекс успешности"), Qt::CaseInsensitive) && col >= 1) {
        return true;
    }
    return false;
}

class ProtocolEditGuardImpl final : public QObject {
public:
    explicit ProtocolEditGuardImpl(QTextEdit *editor, ProtocolEditGuard::Mode mode)
        : QObject(editor)
        , m_editor(editor)
        , m_mode(mode) {
        if (!m_editor) {
            return;
        }
        m_editor->setProperty(kGuardProperty, QVariant::fromValue(static_cast<QObject *>(this)));
        applyModeToEditor();
        connect(m_editor, &QTextEdit::cursorPositionChanged, this, &ProtocolEditGuardImpl::enforceCursor);
        m_editor->installEventFilter(this);
        if (QWidget *viewport = m_editor->viewport()) {
            viewport->setCursor(Qt::ArrowCursor);
            viewport->installEventFilter(this);
        }
    }

    void setMode(ProtocolEditGuard::Mode mode) {
        if (m_mode == mode) {
            return;
        }
        m_mode = mode;
        m_lastEditablePos = -1;
        applyModeToEditor();
        enforceCursor();
    }

    ProtocolEditGuard::Mode mode() const { return m_mode; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        Q_UNUSED(watched);
        if (!m_editor) {
            return QObject::eventFilter(watched, event);
        }

        if (m_mode == ProtocolEditGuard::Mode::ReadOnly) {
            switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            case QEvent::KeyPress:
            case QEvent::InputMethod:
                // Прокрутка/выделение ссылок не нужны — блокируем ввод и установку курсора.
                if (event->type() == QEvent::MouseButtonPress
                    || event->type() == QEvent::MouseButtonDblClick) {
                    m_editor->setCursorWidth(0);
                    m_editor->clearFocus();
                    return true;
                }
                if (event->type() == QEvent::KeyPress || event->type() == QEvent::InputMethod) {
                    return true;
                }
                break;
            case QEvent::FocusIn:
                m_editor->setCursorWidth(0);
                break;
            default:
                break;
            }
            return QObject::eventFilter(watched, event);
        }

        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            return handleMouseButton(watched, static_cast<QMouseEvent *>(event));
        case QEvent::MouseMove:
            updateHoverCursor(watched, static_cast<QMouseEvent *>(event));
            return false;
        case QEvent::KeyPress:
        case QEvent::InputMethod:
            if (!isEditableProtocolCursor(m_editor->textCursor())) {
                return true;
            }
            break;
        case QEvent::FocusIn:
            enforceCursor();
            break;
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void applyModeToEditor() {
        if (!m_editor) {
            return;
        }
        if (m_mode == ProtocolEditGuard::Mode::ReadOnly) {
            m_editor->setReadOnly(true);
            m_editor->setTextInteractionFlags(Qt::TextBrowserInteraction);
            m_editor->setCursorWidth(0);
            if (m_editor->viewport()) {
                m_editor->viewport()->setCursor(Qt::ArrowCursor);
            }
        } else {
            m_editor->setReadOnly(false);
            m_editor->setTextInteractionFlags(Qt::TextEditorInteraction);
            m_editor->setCursorWidth(0);
        }
    }

    QTextCursor cursorAtViewportPos(const QPoint &viewportPos) const {
        QTextCursor cursor(m_editor->document());
        if (!m_editor->viewport()) {
            return cursor;
        }
        const int position = m_editor->document()->documentLayout()->hitTest(viewportPos, Qt::FuzzyHit);
        if (position >= 0) {
            cursor.setPosition(position);
        }
        return cursor;
    }

    QPoint viewportPosForMouse(QObject *watched, QMouseEvent *event) const {
        if (!m_editor || !m_editor->viewport()) {
            return {};
        }
        QWidget *viewport = m_editor->viewport();
        if (watched == viewport) {
            return event->pos();
        }
        if (watched == m_editor) {
            return viewport->mapFrom(m_editor, event->pos());
        }
        return viewport->mapFromGlobal(event->globalPos());
    }

    bool handleMouseButton(QObject *watched, QMouseEvent *event) {
        const QTextCursor probe = cursorAtViewportPos(viewportPosForMouse(watched, event));
        if (probe.position() < 0 || !isEditableProtocolCursor(probe)) {
            restoreLastEditableCursor();
            return true;
        }
        m_editor->setCursorWidth(1);
        return false;
    }

    void updateHoverCursor(QObject *watched, QMouseEvent *event) {
        if (!m_editor->viewport()) {
            return;
        }
        const QTextCursor probe = cursorAtViewportPos(viewportPosForMouse(watched, event));
        if (probe.position() >= 0 && isEditableProtocolCursor(probe)) {
            m_editor->viewport()->setCursor(Qt::IBeamCursor);
        } else {
            m_editor->viewport()->setCursor(Qt::ArrowCursor);
        }
    }

    void restoreLastEditableCursor() {
        m_guarding = true;
        if (m_lastEditablePos >= 0) {
            QTextCursor cursor = m_editor->textCursor();
            cursor.setPosition(m_lastEditablePos);
            m_editor->setTextCursor(cursor);
            m_editor->setCursorWidth(1);
        } else {
            QTextCursor cursor = m_editor->textCursor();
            cursor.clearSelection();
            m_editor->setTextCursor(cursor);
            m_editor->setCursorWidth(0);
            m_editor->clearFocus();
        }
        m_guarding = false;
    }

    void enforceCursor() {
        if (m_guarding || !m_editor) {
            return;
        }
        if (m_mode == ProtocolEditGuard::Mode::ReadOnly) {
            m_editor->setCursorWidth(0);
            return;
        }
        const QTextCursor current = m_editor->textCursor();
        if (isEditableProtocolCursor(current)) {
            m_lastEditablePos = current.position();
            m_editor->setCursorWidth(1);
            return;
        }
        restoreLastEditableCursor();
    }

    QTextEdit *m_editor = nullptr;
    ProtocolEditGuard::Mode m_mode = ProtocolEditGuard::Mode::LimitedEdit;
    int m_lastEditablePos = -1;
    bool m_guarding = false;
};

ProtocolEditGuardImpl *guardFor(QTextEdit *editor) {
    if (!editor) {
        return nullptr;
    }
    return static_cast<ProtocolEditGuardImpl *>(
        editor->property(kGuardProperty).value<QObject *>());
}

} // namespace

void ProtocolEditGuard::install(QTextEdit *editor, Mode mode) {
    if (!editor) {
        return;
    }
    if (ProtocolEditGuardImpl *existing = guardFor(editor)) {
        existing->setMode(mode);
        return;
    }
    new ProtocolEditGuardImpl(editor, mode);
}

void ProtocolEditGuard::setMode(QTextEdit *editor, Mode mode) {
    if (!editor) {
        return;
    }
    if (ProtocolEditGuardImpl *existing = guardFor(editor)) {
        existing->setMode(mode);
        return;
    }
    install(editor, mode);
}
