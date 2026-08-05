#include <QApplication>
#include <QTextEdit>
#include <QTextTable>
#include <QTextFrame>
#include <QAbstractTextDocumentLayout>
#include <iostream>
#include <QTextBlock>

// current collectTables (no recurse into tables)
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
static void collectTablesDeep(QTextFrame *frame, QList<QTextTable*> &out) {
    if (!frame) return;
    for (auto it = frame->begin(); !(it.atEnd()); ++it) {
        QTextFrame *child = it.currentFrame();
        if (child) {
            if (QTextTable *t = qobject_cast<QTextTable*>(child)) {
                out.append(t);
                collectTablesDeep(t, out); // also look inside
            } else {
                collectTablesDeep(child, out);
            }
        }
    }
}
void dump(QTextEdit &edit, QList<QTextTable*> tables, const char *label) {
    auto *layout = edit.document()->documentLayout();
    std::cout << label << " count=" << tables.size() << std::endl;
    for (int ti=0; ti<(int)tables.size(); ++ti) {
        QTextTable *t = tables[ti];
        std::cout << " t" << ti << " cols=" << t->columns() << " visual:";
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
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    // BAD assembly: header+process without closing summary table (causes nesting)
    QString bad =
      "<table border=1 width=671>"
      "<tr><td width=200>Методика</td><td width=471>Test</td></tr>"
      "<tr><td colspan=2>Процесс выполнения диагностической методики</td></tr>"
      // missing </table> — process appended inside
      "<table border=1 width=671>"
      "<tr><td width=308>Характер деятельности ребенка</td>"
      "<td width=298>Виды помощи</td><td width=65>Баллы</td></tr>"
      "<tr><td>a</td><td>b</td><td>1</td></tr>"
      "</table></table>";
    QString html = "<html><body>" + bad + "</body></html>";
    QTextEdit edit; edit.setFixedWidth(720); edit.setHtml(html);
    edit.document()->setTextWidth(671); edit.show(); app.processEvents();
    QList<QTextTable*> oldL, deepL;
    collectTablesOld(edit.document()->rootFrame(), oldL);
    collectTablesDeep(edit.document()->rootFrame(), deepL);
    dump(edit, oldL, "OLD collect");
    dump(edit, deepL, "DEEP collect");
    return 0;
}
