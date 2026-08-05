#include <QApplication>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextTable>
#include <QTextFrame>
#include <QAbstractTextDocumentLayout>
#include <QFile>
#include <iostream>

static void collectTables(QTextFrame *frame, QList<QTextTable*> &out) {
    if (!frame) return;
    for (auto it = frame->begin(); !(it.atEnd()); ++it) {
        QTextFrame *child = it.currentFrame();
        if (child) {
            if (QTextTable *t = qobject_cast<QTextTable*>(child)) out.append(t);
            collectTables(child, out);
        }
    }
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QFile f("d:/projects/DokitLab/infant/assets/ex/1.12/template.html");
    f.open(QIODevice::ReadOnly);
    QString html = QString::fromUtf8(f.readAll());

    QTextEdit edit;
    edit.setHtml(html);
    edit.resize(800, 700);
    edit.document()->setDocumentMargin(0);
    edit.document()->setTextWidth(671);

    QList<QTextTable*> tables;
    collectTables(edit.document()->rootFrame(), tables);
    for (QTextTable *table : tables) {
        QTextTableFormat fmt = table->format();
        fmt.setWidth(QTextLength(QTextLength::FixedLength, 671));
        fmt.setBorder(1);
        fmt.setCellPadding(0);
        fmt.setCellSpacing(0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        fmt.setBorderCollapse(true);
#endif
        if (table->columns() == 3) {
            fmt.clearColumnWidthConstraints();
            fmt.setColumnWidthConstraints({
                QTextLength(QTextLength::FixedLength, 308),
                QTextLength(QTextLength::FixedLength, 298),
                QTextLength(QTextLength::FixedLength, 65)});
        }
        table->setFormat(fmt);
    }
    edit.show();
    app.processEvents();

    auto *layout = edit.document()->documentLayout();
    for (int ti = 0; ti < tables.size(); ++ti) {
        QTextTable *t = tables[ti];
        QRectF fr = layout->frameBoundingRect(t);
        std::cout << "table " << ti << " frameW=" << fr.width() << " cols=" << t->columns() << std::endl;
        qreal prevRight = fr.left();
        for (int c = 0; c < t->columns(); ++c) {
            QTextTableCell cell = t->cellAt(0, c);
            // Walk blocks in cell to get bounding rect
            QRectF cellRect;
            bool first = true;
            for (auto it = cell.begin(); !(it.atEnd()); ++it) {
                QTextFrame *cf = it.currentFrame();
                if (cf) {
                    QRectF r = layout->frameBoundingRect(cf);
                    if (first) { cellRect = r; first = false; }
                    else cellRect |= r;
                } else {
                    QTextBlock b = it.currentBlock();
                    if (b.isValid()) {
                        QRectF r = layout->blockBoundingRect(b);
                        if (first) { cellRect = r; first = false; }
                        else cellRect |= r;
                    }
                }
            }
            std::cout << "  col " << c << " x=" << cellRect.x() << " w=" << cellRect.width()
                      << " constraint=" << t->format().columnWidthConstraints().value(c).rawValue()
                      << std::endl;
        }
    }
    return 0;
}
