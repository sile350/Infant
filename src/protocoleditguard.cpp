#include "protocoleditguard.h"

#include <QAbstractTextDocumentLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextTable>
#include <QWidget>

namespace {

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

bool tableHasActivityHelpHeader(QTextTable *table) {
    if (!table) {
        return false;
    }
    const int rows = qMin(3, table->rows());
    const int columns = table->columns();
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            const QString text = readProtocolTableCellText(table, row, col);
            if (text.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
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
    const QString firstCell = readProtocolTableCellText(table, row, 0);
    if (firstCell.contains(QStringLiteral("Результат"), Qt::CaseInsensitive) && col == 1) {
        return true;
    }
    if (firstCell.contains(QStringLiteral("Примечание"), Qt::CaseInsensitive) && col == 1) {
        return true;
    }
    if ((col == 2 || col == 3) && tableHasActivityHelpHeader(table)) {
        return true;
    }
    return false;
}

class ProtocolEditGuardImpl final : public QObject {
public:
    explicit ProtocolEditGuardImpl(QTextEdit *editor)
        : QObject(editor), m_editor(editor) {
        if (!m_editor) {
            return;
        }
        m_editor->setCursorWidth(0);
        connect(m_editor, &QTextEdit::cursorPositionChanged, this, &ProtocolEditGuardImpl::enforceCursor);
        m_editor->installEventFilter(this);
        if (QWidget *viewport = m_editor->viewport()) {
            viewport->setCursor(Qt::ArrowCursor);
            viewport->installEventFilter(this);
        }
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        Q_UNUSED(watched);
        if (!m_editor) {
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
        }
        m_guarding = false;
    }

    void enforceCursor() {
        if (m_guarding || !m_editor) {
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
    int m_lastEditablePos = -1;
    bool m_guarding = false;
};

} // namespace

void ProtocolEditGuard::install(QTextEdit *editor) {
    if (!editor) {
        return;
    }
    new ProtocolEditGuardImpl(editor);
}
