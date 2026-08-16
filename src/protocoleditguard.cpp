#include "protocoleditguard.h"

#include <QAbstractTextDocumentLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QString>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextTable>
#include <QVariant>
#include <QWidget>
#include <utility>

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

bool isProtocolFieldAnchorName(const QString &raw) {
    QString name = raw.trimmed();
    if (name.startsWith(QLatin1Char('#'))) {
        name = name.mid(1);
    }
    if (name.isEmpty()) {
        return false;
    }
    // Ячейки протокола (idb1, idsum, idvivod, idprod…) — не ссылки «Показать изображение».
    static const QRegularExpression fieldRe(
        QStringLiteral(
            "^(idballs|idsum|idvivod|idprod|idstab|idbls|idnote|idspc|cidd|"
            "idb\\d+|ids\\d+|idd\\d+|idp\\d+|idv\\d+|ide\\d+|dokit-pid)"),
        QRegularExpression::CaseInsensitiveOption);
    return fieldRe.match(name).hasMatch();
}

bool looksLikeProtocolScanIdKey(const QString &raw) {
    QString key = raw.trimmed();
    if (key.startsWith(QLatin1Char('#'))) {
        key = key.mid(1);
    }
    // Скан-якоря: id{protocolId} или id{protocolId}-{slot} (только цифры).
    static const QRegularExpression scanIdRe(
        QStringLiteral("^id\\d+(-\\d+)?$"),
        QRegularExpression::CaseInsensitiveOption);
    return scanIdRe.match(key).hasMatch();
}

bool looksLikeProtocolScanAnchor(const QString &anchor) {
    if (anchor.trimmed().isEmpty()) {
        return false;
    }
    if (isProtocolFieldAnchorName(anchor)) {
        return false;
    }
    if (looksLikeProtocolScanIdKey(anchor)) {
        return true;
    }
    // Прочие id* (idprod, idstab, …) — поля протокола, не сканы.
    QString key = anchor.trimmed();
    if (key.startsWith(QLatin1Char('#'))) {
        key = key.mid(1);
    }
    if (key.startsWith(QStringLiteral("id"), Qt::CaseInsensitive)) {
        return false;
    }
    return anchor.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)
        || anchor.contains(QStringLiteral("/scans/"), Qt::CaseInsensitive)
        || anchor.contains(QStringLiteral("\\scans\\"), Qt::CaseInsensitive)
        || anchor.contains(QStringLiteral(".JPG"), Qt::CaseInsensitive)
        || anchor.contains(QStringLiteral(".jpg"), Qt::CaseInsensitive)
        || anchor.contains(QStringLiteral(".png"), Qt::CaseInsensitive)
        || anchor.contains(QStringLiteral(".jpeg"), Qt::CaseInsensitive)
        || anchor.contains(QStringLiteral(".JPEG"), Qt::CaseInsensitive);
}

QString hrefFromCharFormat(const QTextCharFormat &fmt) {
    const QString href = fmt.anchorHref().trimmed();
    if (looksLikeProtocolScanAnchor(href)) {
        return href;
    }
    const QStringList names = fmt.anchorNames();
    for (const QString &name : names) {
        const QString trimmed = name.trimmed();
        if (isProtocolFieldAnchorName(trimmed)) {
            continue;
        }
        if (looksLikeProtocolScanIdKey(trimmed)) {
            return trimmed.startsWith(QLatin1Char('#')) ? trimmed : (QLatin1Char('#') + trimmed);
        }
        if (looksLikeProtocolScanAnchor(trimmed)) {
            return trimmed.startsWith(QLatin1Char('#')) ? trimmed : (QLatin1Char('#') + trimmed);
        }
    }
    return {};
}

bool isLockedHeaderCell(const QString &cellText) {
    const QString trimmed = cellText.trimmed();
    // Длинный текст — ячейка данных (в т.ч. после чекбоксов), не заголовок.
    if (trimmed.length() > 48) {
        return false;
    }
    return trimmed.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
        || trimmed.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
        || trimmed.contains(QStringLiteral("Виды возможной помощи"), Qt::CaseInsensitive)
        || trimmed.contains(QStringLiteral("Правильный ответ"), Qt::CaseInsensitive)
        || trimmed.compare(QStringLiteral("Баллы"), Qt::CaseInsensitive) == 0
        || trimmed.contains(QStringLiteral("Ответ ребенка"), Qt::CaseInsensitive)
        || trimmed.contains(QStringLiteral("Ответы ребенка"), Qt::CaseInsensitive)
        || trimmed.compare(QStringLiteral("Вопросы"), Qt::CaseInsensitive) == 0
        || trimmed.contains(QStringLiteral("Портретная"), Qt::CaseInsensitive)
        || (trimmed.contains(QStringLiteral("№"), Qt::CaseInsensitive)
            && trimmed.contains(QStringLiteral("рассказ"), Qt::CaseInsensitive));
}

bool isEditableProtocolCursor(const QTextCursor &cursor, QTextEdit *editor = nullptr) {
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
    // «Результат» редактируется по умолчанию.
    // Блокировка только там, где баллы вносятся программно (1.1 / 1.4 / 1.8).
    if (firstCell.contains(QStringLiteral("Результат"), Qt::CaseInsensitive)) {
        if (editor && editor->property("protocolLockResultEdit").toBool()) {
            return false;
        }
        return col >= 1;
    }
    if (firstCell.contains(QStringLiteral("Примечание"), Qt::CaseInsensitive) && col >= 1) {
        return true;
    }
    // OR / HLP после формирования (как contenteditable в оригинале).
    if ((firstCell.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
         || firstCell.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive))
        && col >= 1) {
        return true;
    }
    const bool strict126 = editor && editor->property("protocolStrict126Edit").toBool();

    // Ближайший заголовок колонки выше текущей строки (важно для 1.26: два блока
    // «Ответ ребенка» в одной таблице — у задания 1 и 2 разные индексы колонок).
    auto headerColAbove = [&](const QString &needle) -> int {
        for (int r = row - 1; r >= 0; --r) {
            for (int c = 0; c < table->columns(); ++c) {
                const QString header = readProtocolTableCellText(table, r, c);
                if (header.contains(needle, Qt::CaseInsensitive)) {
                    return c;
                }
            }
        }
        return -1;
    };

    if (strict126) {
        if (firstCell.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)
            || firstCell.contains(QStringLiteral("Индекс"), Qt::CaseInsensitive)
            || firstCell.contains(QStringLiteral("Задание"), Qt::CaseInsensitive)) {
            return false;
        }
        const int correctCol = headerColAbove(QStringLiteral("Правильный ответ"));
        if (correctCol >= 0 && col == correctCol) {
            return false;
        }
        const int answerCol = headerColAbove(QStringLiteral("Ответ ребенка"));
        if (answerCol >= 0 && col == answerCol) {
            return true;
        }
        const int ballsCol = headerColAbove(QStringLiteral("Баллы"));
        if (ballsCol >= 0 && col == ballsCol) {
            return true;
        }
        return false;
    }

    // OR/HLP/Баллы + «Ответ ребенка» + ячейки итоговых сумм (прочие методики).
    // protocolLockBallsEdit: запрет курсора в колонке «Баллы» (3.1.17 и т.п.).
    const bool lockBalls = editor && editor->property("protocolLockBallsEdit").toBool();
    QList<int> ballsCols;
    int answerCol = -1;
    int correctCol = -1;
    int ballsHeaderRow = -1;
    int selectedPicCol = -1;
    int explanationCol = -1;
    int activityCol = -1;
    int helpCol = -1;
    int reproducedBeforeCol = -1;
    int reproducedAfterCol = -1;
    auto isBallsHeaderText = [](const QString &header) {
        return header.compare(QStringLiteral("Баллы"), Qt::CaseInsensitive) == 0
            || (header.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)
                && (header.length() <= 12
                    || header.contains(QStringLiteral("продуктив"), Qt::CaseInsensitive)
                    || header.contains(QStringLiteral("устойчив"), Qt::CaseInsensitive)));
    };
    for (int r = 0; r < table->rows(); ++r) {
        for (int c = 0; c < table->columns(); ++c) {
            const QString header = readProtocolTableCellText(table, r, c);
            if (isBallsHeaderText(header)) {
                // 5.4.2: «Баллы» в col0 последней строки — подпись строки, не заголовок колонки.
                if (c == 0
                    && header.compare(QStringLiteral("Баллы"), Qt::CaseInsensitive) == 0) {
                    bool rowHasOtherHeaders = false;
                    for (int c2 = 1; c2 < table->columns(); ++c2) {
                        const QString h2 = readProtocolTableCellText(table, r, c2);
                        if (h2.contains(QStringLiteral("Ответ"), Qt::CaseInsensitive)
                            || h2.contains(QStringLiteral("помощи"), Qt::CaseInsensitive)
                            || h2.contains(QStringLiteral("деятельности"), Qt::CaseInsensitive)
                            || h2.contains(QStringLiteral("Вопрос"), Qt::CaseInsensitive)) {
                            rowHasOtherHeaders = true;
                            break;
                        }
                    }
                    if (!rowHasOtherHeaders) {
                        continue;
                    }
                }
                if (!ballsCols.contains(c)) {
                    ballsCols.append(c);
                }
                // Не затирать уже найденную строку заголовков (Ответы/Виды помощи).
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
            if (answerCol < 0
                && (header.contains(QStringLiteral("Ответ ребенка"), Qt::CaseInsensitive)
                    || header.contains(QStringLiteral("Ответы ребенка"), Qt::CaseInsensitive))) {
                answerCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
            if (correctCol < 0
                && header.contains(QStringLiteral("Правильный ответ"), Qt::CaseInsensitive)) {
                correctCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
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
                && (header.contains(QStringLiteral("Характер деятельности"), Qt::CaseInsensitive)
                    || header.contains(QStringLiteral("Характер выполнения"), Qt::CaseInsensitive))) {
                activityCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
            if (helpCol < 0
                && (header.contains(QStringLiteral("Виды помощи"), Qt::CaseInsensitive)
                    || header.contains(QStringLiteral("Виды и количество"), Qt::CaseInsensitive))
                && !header.contains(QStringLiteral("возможной"), Qt::CaseInsensitive)) {
                helpCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
            if (reproducedBeforeCol < 0
                && (header.contains(QStringLiteral("до предъявления"), Qt::CaseInsensitive)
                    || (header.contains(QStringLiteral("Воспроиз"), Qt::CaseInsensitive)
                        && header.contains(QStringLiteral("до"), Qt::CaseInsensitive)))) {
                reproducedBeforeCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
            if (reproducedAfterCol < 0
                && (header.contains(QStringLiteral("после предъявления"), Qt::CaseInsensitive)
                    || (header.contains(QStringLiteral("Воспроиз"), Qt::CaseInsensitive)
                        && header.contains(QStringLiteral("после"), Qt::CaseInsensitive)))) {
                reproducedAfterCol = c;
                if (ballsHeaderRow < 0) {
                    ballsHeaderRow = r;
                }
            }
        }
    }
    if (correctCol >= 0 && col == correctCol && row > ballsHeaderRow) {
        return false;
    }
    if (ballsHeaderRow >= 0 && row > ballsHeaderRow) {
        if (answerCol >= 0 && col == answerCol) {
            return true;
        }
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
        if (reproducedBeforeCol >= 0 && col == reproducedBeforeCol) {
            return true;
        }
        if (reproducedAfterCol >= 0 && col == reproducedAfterCol) {
            return true;
        }
        if (ballsCols.contains(col)) {
            return !lockBalls;
        }
    }
    if (ballsCols.contains(col) && row > ballsHeaderRow) {
        // 4.1.8: ячейка idsum в строке «Итоговая оценка» — не редактировать.
        if (firstCell.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)) {
            return false;
        }
        return !lockBalls;
    }
    if (firstCell.contains(QStringLiteral("Итоговая"), Qt::CaseInsensitive)) {
        // Подпись и итоговые баллы заполняются «Подвести итог».
        return false;
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
        // После setHtml документ новый — режим мог не смениться, но флаги редактора
        // нужно применить снова (иначе после «Подвести итог» ячейки недоступны).
        if (m_mode != mode) {
            m_mode = mode;
            m_lastEditablePos = -1;
        }
        applyModeToEditor();
        enforceCursor();
    }

    ProtocolEditGuard::Mode mode() const { return m_mode; }

    void setScanAnchorHandler(ProtocolEditGuard::ScanAnchorHandler handler) {
        m_scanAnchorHandler = std::move(handler);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        Q_UNUSED(watched);
        if (!m_editor) {
            return QObject::eventFilter(watched, event);
        }

        // Ссылки «Показать изображение» — до блокировки редактирования.
        if (m_editor->viewport() && watched == m_editor->viewport()
            && event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton
                && tryHandleScanAnchorClick(watched, mouseEvent)) {
                return true;
            }
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
            case QEvent::MouseMove:
                if (m_editor->viewport() && watched == m_editor->viewport()) {
                    updateHoverCursor(watched, static_cast<QMouseEvent *>(event));
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
        case QEvent::MouseButtonDblClick: {
            // Только viewport: иначе Press обрабатывается дважды (viewport + editor)
            // и второй hitTest часто «промахивается» по соседней/заголовочной ячейке.
            if (m_editor->viewport() && watched != m_editor->viewport()) {
                return false;
            }
            return handleMouseButton(watched, static_cast<QMouseEvent *>(event));
        }
        case QEvent::MouseButtonRelease:
            // Не сбрасывать курсор на Release — Press уже выставил позицию.
            return false;
        case QEvent::MouseMove:
            if (m_editor->viewport() && watched != m_editor->viewport()) {
                return false;
            }
            updateHoverCursor(watched, static_cast<QMouseEvent *>(event));
            return false;
        case QEvent::KeyPress:
        case QEvent::InputMethod:
            if (!isEditableProtocolCursor(m_editor->textCursor(), m_editor)) {
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
            // Только TextEditorInteraction: LinksAccessibleByMouse даёт Qt самой
            // ставить курсор по ссылкам и обходить LimitedEdit.
            m_editor->setTextInteractionFlags(Qt::TextEditorInteraction);
            m_editor->setCursorWidth(0);
        }
    }

    QString resolveScanAnchorAt(const QPoint &viewportPos) const {
        if (!m_editor) {
            return {};
        }
        const QString fromAnchorAt = m_editor->anchorAt(viewportPos).trimmed();
        if (looksLikeProtocolScanAnchor(fromAnchorAt)) {
            return fromAnchorAt;
        }

        // cursorForPosition учитывает scroll/margin — надёжнее raw hitTest.
        QTextCursor cursor = m_editor->cursorForPosition(viewportPos);
        const int base = cursor.position();
        for (int delta = 0; delta >= -3; --delta) {
            const int pos = base + delta;
            if (pos < 0) {
                continue;
            }
            QTextCursor probe(m_editor->document());
            probe.setPosition(pos);
            const QString href = hrefFromCharFormat(probe.charFormat());
            if (!href.isEmpty()) {
                return href;
            }
        }
        for (int delta = 1; delta <= 3; ++delta) {
            QTextCursor probe(m_editor->document());
            probe.setPosition(base + delta);
            const QString href = hrefFromCharFormat(probe.charFormat());
            if (!href.isEmpty()) {
                return href;
            }
        }

        // Клик рядом с «Показать изображение» в той же ячейке/блоке.
        QTextCursor blockCursor = cursor;
        blockCursor.movePosition(QTextCursor::StartOfBlock);
        const int blockStart = blockCursor.position();
        blockCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        const QString blockText = blockCursor.selectedText();
        if (!blockText.contains(QStringLiteral("Показать изображение"), Qt::CaseInsensitive)) {
            return {};
        }
        const int blockEnd = blockCursor.position();
        for (int pos = blockStart; pos < blockEnd; ++pos) {
            QTextCursor probe(m_editor->document());
            probe.setPosition(pos);
            const QString href = hrefFromCharFormat(probe.charFormat());
            if (!href.isEmpty()) {
                return href;
            }
        }
        return {};
    }

    bool tryHandleScanAnchorClick(QObject *watched, QMouseEvent *event) {
        if (!m_scanAnchorHandler || !event) {
            return false;
        }
        const QPoint vp = viewportPosForMouse(watched, event);
        const QString anchor = resolveScanAnchorAt(vp);
        if (!looksLikeProtocolScanAnchor(anchor)) {
            return false;
        }
        // Всегда съедаем клик: и открытие файла, и «ещё не загружено».
        m_scanAnchorHandler(anchor);
        m_guarding = true;
        m_editor->setCursorWidth(0);
        if (m_mode == ProtocolEditGuard::Mode::LimitedEdit) {
            if (m_lastEditablePos >= 0) {
                QTextCursor cursor = m_editor->textCursor();
                cursor.setPosition(m_lastEditablePos);
                m_editor->setTextCursor(cursor);
            } else {
                QTextCursor cursor = m_editor->textCursor();
                cursor.clearSelection();
                m_editor->setTextCursor(cursor);
                m_editor->clearFocus();
            }
        } else {
            m_editor->clearFocus();
        }
        m_guarding = false;
        return true;
    }

    QTextCursor cursorAtViewportPos(const QPoint &viewportPos) const {
        if (!m_editor) {
            return {};
        }
        // Как у Qt: позиция с учётом прокрутки/полей документа.
        return m_editor->cursorForPosition(viewportPos);
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
        const QPoint vp = viewportPosForMouse(watched, event);
        // Клик по ссылке скана уже обработан в tryHandleScanAnchorClick.
        // На всякий случай не отдаём Press в QTextEdit — иначе курсор встаёт в ячейку.
        if (looksLikeProtocolScanAnchor(resolveScanAnchorAt(vp))) {
            restoreLastEditableCursor();
            return true;
        }
        QTextCursor probe = cursorAtViewportPos(vp);
        if (probe.position() < 0 || !isEditableProtocolCursor(probe, m_editor)) {
            // ExactHit по viewport без mapToContents врёт при скролле — пробуем соседние точки.
            static const QPoint kNudge[] = {
                {0, 0}, {0, -2}, {0, 2}, {-2, 0}, {2, 0}, {0, -6}, {0, 6}};
            for (const QPoint &d : kNudge) {
                QTextCursor nudged = m_editor->cursorForPosition(vp + d);
                if (nudged.position() >= 0 && isEditableProtocolCursor(nudged, m_editor)) {
                    probe = nudged;
                    break;
                }
            }
        }
        if (probe.position() < 0 || !isEditableProtocolCursor(probe, m_editor)) {
            // Клик по заголовку OR/HLP → первая строка данных в той же колонке.
            if (QTextTable *table = probe.currentTable()) {
                const QTextTableCell headerCell = table->cellAt(probe.position());
                if (headerCell.isValid()) {
                    const QString headerText =
                        readProtocolTableCellText(table, headerCell.row(), headerCell.column());
                    if (isLockedHeaderCell(headerText)) {
                        for (int r = headerCell.row() + 1; r < table->rows(); ++r) {
                            QTextTableCell dataCell = table->cellAt(r, headerCell.column());
                            if (!dataCell.isValid()) {
                                continue;
                            }
                            QTextCursor dataCursor = dataCell.firstCursorPosition();
                            if (isEditableProtocolCursor(dataCursor, m_editor)) {
                                probe = dataCursor;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (probe.position() < 0 || !isEditableProtocolCursor(probe, m_editor)) {
            // Клик по заголовку OR/HLP → первая строка данных в той же колонке.
            if (QTextTable *table = probe.currentTable()) {
                const QTextTableCell headerCell = table->cellAt(probe.position());
                if (headerCell.isValid()) {
                    const QString headerText =
                        readProtocolTableCellText(table, headerCell.row(), headerCell.column());
                    if (isLockedHeaderCell(headerText)) {
                        for (int r = headerCell.row() + 1; r < table->rows(); ++r) {
                            const QTextTableCell dataCell = table->cellAt(r, headerCell.column());
                            if (!dataCell.isValid()) {
                                continue;
                            }
                            const QTextCursor dataCursor = dataCell.firstCursorPosition();
                            if (isEditableProtocolCursor(dataCursor, m_editor)) {
                                probe = dataCursor;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (probe.position() < 0 || !isEditableProtocolCursor(probe, m_editor)) {
            restoreLastEditableCursor();
            return true;
        }
        m_guarding = true;
        m_editor->setTextCursor(probe);
        m_lastEditablePos = probe.position();
        m_editor->setCursorWidth(1);
        m_editor->setFocus(Qt::MouseFocusReason);
        m_guarding = false;
        return false;
    }

    void updateHoverCursor(QObject *watched, QMouseEvent *event) {
        if (!m_editor->viewport()) {
            return;
        }
        const QPoint vp = viewportPosForMouse(watched, event);
        if (looksLikeProtocolScanAnchor(resolveScanAnchorAt(vp))) {
            m_editor->viewport()->setCursor(Qt::PointingHandCursor);
            return;
        }
        const QTextCursor probe = cursorAtViewportPos(vp);
        if (m_mode != ProtocolEditGuard::Mode::ReadOnly
            && probe.position() >= 0
            && isEditableProtocolCursor(probe, m_editor)) {
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
        if (isEditableProtocolCursor(current, m_editor)) {
            m_lastEditablePos = current.position();
            m_editor->setCursorWidth(1);
            return;
        }
        restoreLastEditableCursor();
    }

    QTextEdit *m_editor = nullptr;
    ProtocolEditGuard::Mode m_mode = ProtocolEditGuard::Mode::LimitedEdit;
    ProtocolEditGuard::ScanAnchorHandler m_scanAnchorHandler;
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

void ProtocolEditGuard::setScanAnchorHandler(QTextEdit *editor, ScanAnchorHandler handler) {
    if (!editor) {
        return;
    }
    if (ProtocolEditGuardImpl *existing = guardFor(editor)) {
        existing->setScanAnchorHandler(std::move(handler));
        return;
    }
    install(editor, Mode::LimitedEdit);
    if (ProtocolEditGuardImpl *created = guardFor(editor)) {
        created->setScanAnchorHandler(std::move(handler));
    }
}
