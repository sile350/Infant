#include <QApplication>
#include <QTextEdit>
#include <QTextTable>
#include <QTextFrame>
#include <QAbstractTextDocumentLayout>
#include <iostream>
#include <QTextBlock>

static void collectTablesDeep(QTextFrame *frame, QList<QTextTable*> &out) {
    if (!frame) return;
    for (auto it = frame->begin(); !(it.atEnd()); ++it) {
        QTextFrame *child = it.currentFrame();
        if (child) {
            if (QTextTable *t = qobject_cast<QTextTable*>(child)) {
                out.append(t);
                collectTablesDeep(t, out);
            } else collectTablesDeep(child, out);
        }
    }
}
static void collectTablesOld(QTextFrame *frame, QList<QTextTable*> &out) {
    if (!frame) return;
    for (auto it = frame->begin(); !(it.atEnd()); ++it) {
        QTextFrame *child = it.currentFrame();
        if (child) {
            if (QTextTable *t = qobject_cast<QTextTable*>(child)) { out.append(t); continue; }
            collectTablesOld(child, out);
        }
    }
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QString bad =
      "<table border=1 width=671>"
      "<tr><td>Методика</td><td>Test</td></tr>"
      "<tr><td colspan=2>Процесс выполнения</td></tr>"
      "<table border=1 width=671>"
      "<tr><td>Характер деятельности ребенка</td>"
      "<td>Виды помощи</td><td>Баллы</td></tr>"
      "<tr><td>aaaaaaaaaaaaaaaaaaaa</td><td>bbbbbbbb</td><td>1</td></tr>"
      "</table></table>";
    QTextEdit edit; edit.setFixedWidth(720); edit.setHtml("<html><body>"+bad+"</body></html>");
    edit.document()->setTextWidth(671); edit.show(); app.processEvents();
    QList<QTextTable*> deep; collectTablesDeep(edit.document()->rootFrame(), deep);
    auto *layout = edit.document()->documentLayout();
    for (int ti=0; ti<deep.size(); ++ti) {
        QTextTable *t = deep[ti];
        std::cout << "t" << ti << " cols=" << t->columns() << " visual:";
        for (int c=0;c<t->columns();++c) {
            QTextTableCell cell = t->cellAt(0,c);
            QRectF cellRect; bool first=true;
            for (auto it=cell.begin(); !(it.atEnd()); ++it) {
                QTextBlock b = it.currentBlock();
                if (b.isValid()) {
                    QRectF r = layout->blockBoundingRect(b);
                    if (first){cellRect=r;first=false;} else cellRect|=r;
                }
            }
            std::cout << " " << (int)cellRect.width();
        }
        auto cs=t->format().columnWidthConstraints();
        std::cout << " constr:";
        for (int j=0;j<cs.size();++j) std::cout << " t" << cs[j].type() << ":" << cs[j].rawValue();
        std::cout << std::endl;
    }
    // Apply force ONLY to old-collected (top-level only) - simulates bug
    QList<QTextTable*> oldL; collectTablesOld(edit.document()->rootFrame(), oldL);
    for (QTextTable *table : oldL) {
        QTextTableFormat fmt = table->format();
        fmt.setWidth(QTextLength(QTextLength::FixedLength, 671));
        if (table->columns()==2) {
            fmt.setColumnWidthConstraints({
                QTextLength(QTextLength::FixedLength,200),
                QTextLength(QTextLength::FixedLength,471)});
        }
        table->setFormat(fmt);
    }
    app.processEvents();
    std::cout << "After force top-level only:" << std::endl;
    deep.clear(); collectTablesDeep(edit.document()->rootFrame(), deep);
    for (int ti=0; ti<deep.size(); ++ti) {
        QTextTable *t = deep[ti];
        std::cout << "t" << ti << " cols=" << t->columns() << " visual:";
        for (int c=0;c<t->columns();++c) {
            QTextTableCell cell = t->cellAt(0,c);
            QRectF cellRect; bool first=true;
            for (auto it=cell.begin(); !(it.atEnd()); ++it) {
                QTextBlock b = it.currentBlock();
                if (b.isValid()) {
                    QRectF r = layout->blockBoundingRect(b);
                    if (first){cellRect=r;first=false;} else cellRect|=r;
                }
            }
            std::cout << " " << (int)cellRect.width();
        }
        std::cout << std::endl;
    }
    return 0;
}
