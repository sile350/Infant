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
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QString bad =
      "<table border=1 width=671>"
      "<tr><td width=200>Методика</td><td width=471>Test</td></tr>"
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
    for (QTextTable *table : deep) {
        QTextTableFormat fmt = table->format();
        QRectF fr = layout->frameBoundingRect(table);
        int w = qMax(100, (int)fr.width());
        // if almost full doc width use 671
        if (w > 600) w = 671;
        fmt.setWidth(QTextLength(QTextLength::FixedLength, w));
        fmt.setBorder(1); fmt.setCellPadding(0); fmt.setCellSpacing(0);
        if (table->columns()==2) {
            int a = qRound(200.0 * w / 671); fmt.setColumnWidthConstraints({
                QTextLength(QTextLength::FixedLength,a),
                QTextLength(QTextLength::FixedLength,w-a)});
        } else if (table->columns()==3) {
            int a=qRound(308.0*w/671), b=qRound(298.0*w/671);
            fmt.setColumnWidthConstraints({
                QTextLength(QTextLength::FixedLength,a),
                QTextLength(QTextLength::FixedLength,b),
                QTextLength(QTextLength::FixedLength,w-a-b)});
        }
        table->setFormat(fmt);
    }
    app.processEvents();
    deep.clear(); collectTablesDeep(edit.document()->rootFrame(), deep);
    for (int ti=0; ti<deep.size(); ++ti) {
        QTextTable *t = deep[ti];
        QRectF fr = layout->frameBoundingRect(t);
        std::cout << "t" << ti << " frameW=" << fr.width() << " cols=" << t->columns() << " visual:";
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
