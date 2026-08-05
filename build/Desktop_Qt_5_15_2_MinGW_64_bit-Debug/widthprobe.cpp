#include <QCoreApplication>
#include <QTextDocument>
#include <QTextTable>
#include <QTextFrame>
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

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QFile f("d:/projects/DokitLab/infant/assets/ex/1.12/template.html");
    f.open(QIODevice::ReadOnly);
    QString html = QString::fromUtf8(f.readAll());
    // with style like prepareTemplateHtml
    html.replace("</head>",
      "<style>table { table-layout:fixed; width:671px; }</style></head>");
    QTextDocument doc;
    doc.setHtml(html);
    QList<QTextTable*> tables;
    collectTables(doc.rootFrame(), tables);
    std::cout << "tables: " << tables.size() << std::endl;
    for (int i = 0; i < tables.size(); ++i) {
        QTextTable *t = tables[i];
        auto fmt = t->format();
        auto c = fmt.columnWidthConstraints();
        std::cout << "table " << i << " cols=" << t->columns() << " rows=" << t->rows()
                  << " constraints=" << c.size() << " widthType=" << fmt.width().type()
                  << " widthVal=" << fmt.width().rawValue() << std::endl;
        for (int j = 0; j < c.size(); ++j)
            std::cout << "  c" << j << " type=" << c[j].type() << " val=" << c[j].rawValue() << std::endl;
        for (int r = 0; r < qMin(2, t->rows()); ++r) {
            std::cout << "  row " << r << ": ";
            for (int col = 0; col < t->columns(); ++col) {
                QTextTableCell cell = t->cellAt(r, col);
                QString text;
                for (auto bit = cell.begin(); !(bit.atEnd()); ++bit) {
                    QTextBlock b = bit.currentBlock();
                    if (b.isValid()) text += b.text() + " ";
                }
                std::cout << "[" << text.trimmed().left(35).toStdString() << "] ";
            }
            std::cout << std::endl;
        }
        // apply force like OR/HLP
        if (t->columns() == 3) {
            QTextTableFormat nf = t->format();
            nf.setWidth(QTextLength(QTextLength::FixedLength, 671));
            nf.setColumnWidthConstraints({
                QTextLength(QTextLength::FixedLength, 308),
                QTextLength(QTextLength::FixedLength, 298),
                QTextLength(QTextLength::FixedLength, 65)
            });
            t->setFormat(nf);
            auto c2 = t->format().columnWidthConstraints();
            std::cout << "  AFTER force constraints=" << c2.size() << std::endl;
            for (int j = 0; j < c2.size(); ++j)
                std::cout << "  c" << j << " type=" << c2[j].type() << " val=" << c2[j].rawValue() << std::endl;
        }
    }
    // also test 1.12.json fragment
    QString frag =
      "<table border='1' style='table-layout:fixed' width='671' cellspacing='0' cellpadding='0'>"
      "<tr><td width='304' align='center'>Характер деятельности ребенка</td>"
      "<td width='304' align='center'>Виды помощи</td>"
      "<td width='61' align='center'>Баллы</td></tr>"
      "<tr><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td></tr></table>";
    QTextDocument doc2;
    doc2.setHtml(frag);
    QList<QTextTable*> tables2;
    collectTables(doc2.rootFrame(), tables2);
    std::cout << "json-like tables: " << tables2.size() << std::endl;
    if (!tables2.isEmpty()) {
        auto c = tables2[0]->format().columnWidthConstraints();
        std::cout << "cols=" << tables2[0]->columns() << " constraints=" << c.size() << std::endl;
        for (int j = 0; j < c.size(); ++j)
            std::cout << "  c" << j << " type=" << c[j].type() << " val=" << c[j].rawValue() << std::endl;
    }
    return 0;
}
