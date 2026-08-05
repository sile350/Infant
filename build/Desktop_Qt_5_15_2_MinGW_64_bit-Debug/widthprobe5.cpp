#include <QApplication>
#include <QTextEdit>
#include <QTextTable>
#include <QTextFrame>
#include <QAbstractTextDocumentLayout>
#include <QFile>
#include <iostream>
#include <QTextBlock>

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
QString cellText(QTextTable *table, int row, int column) {
    QTextTableCell cell = table->cellAt(row, column);
    QString text;
    for (auto bit = cell.begin(); !(bit.atEnd()); ++bit) {
        QTextBlock b = bit.currentBlock();
        if (b.isValid()) text += b.text();
    }
    return text;
}
void dumpVisual(QTextEdit &edit, const char *label) {
    QList<QTextTable*> tables; collectTables(edit.document()->rootFrame(), tables);
    auto *layout = edit.document()->documentLayout();
    std::cout << label << std::endl;
    for (int ti=0; ti<tables.size(); ++ti) {
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
        auto cs=t->format().columnWidthConstraints();
        std::cout << " constr:";
        for (int j=0;j<cs.size();++j) std::cout << " " << cs[j].type() << ":" << cs[j].rawValue();
        std::cout << std::endl;
    }
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    // Simulate buildProtocol118 + normalize result for 1.12
    QString body =
      "<table border='1' style='table-layout:fixed;width:671px' width='671'>"
      "<colgroup><col width='200'><col width='471'></colgroup>"
      "<tr><td width='200'>Методика</td><td width='471'>Раскрась предметы</td></tr>"
      "<tr><td width='200'>Дата/специалист</td><td width='471'>01.01.2026</td></tr>"
      "<tr><td align='center' colspan='2'>Процесс выполнения диагностической методики</td></tr>"
      "</table>"
      "<table border='1' style='table-layout:fixed;width:671px' width='671'>"
      "<colgroup><col width='308'><col width='298'><col width='65'></colgroup>"
      "<tr><td width='308' align='center'>Характер деятельности ребенка</td>"
      "<td width='298' align='center'>Виды помощи</td>"
      "<td width='65' align='center'>Баллы</td></tr>"
      "<tr><td><div>OR text long enough to wrap around</div></td>"
      "<td><div>help text</div></td><td align='center'><div>2.5</div></td></tr>"
      "</table>";
    QString html = QString::fromUtf8(
        "<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<style>table{table-layout:fixed;width:671px;border-collapse:collapse;}"
        "td{border:1px solid #000;}</style></head><body>") + body + "</body></html>";

    QTextEdit edit;
    edit.setFixedWidth(720);
    edit.setHtml(html);
    edit.document()->setTextWidth(671);
    edit.show(); app.processEvents();
    dumpVisual(edit, "with colgroup");

    // Roundtrip via toHtml like autosave
    QString round = edit.toHtml();
    edit.setHtml(round);
    edit.document()->setTextWidth(671);
    app.processEvents();
    dumpVisual(edit, "after toHtml roundtrip NO force");

    // Check if roundtrip lost colgroup
    std::cout << "roundtrip has colgroup? " << (round.contains("colgroup")?"yes":"no") << std::endl;
    std::cout << "roundtrip has width=308? " << (round.contains("308")?"yes":"no") << std::endl;
    // print column-related snippets
    int p = 0; int n=0;
    while ((p = round.indexOf("width", p))>=0 && n<20) {
        std::cout << "  " << round.mid(p, 50).toStdString() << std::endl;
        p+=5; ++n;
    }
    return 0;
}
