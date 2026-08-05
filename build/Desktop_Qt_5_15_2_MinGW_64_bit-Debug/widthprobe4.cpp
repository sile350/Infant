#include <QApplication>
#include <QTextEdit>
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
void dump(QTextEdit &edit, const char *label) {
    QList<QTextTable*> tables;
    collectTables(edit.document()->rootFrame(), tables);
    auto *layout = edit.document()->documentLayout();
    std::cout << label << std::endl;
    for (int ti = 0; ti < tables.size(); ++ti) {
        QTextTable *t = tables[ti];
        std::cout << " table " << ti << " cols=" << t->columns();
        auto cs = t->format().columnWidthConstraints();
        std::cout << " constr:";
        for (int j=0;j<cs.size();++j) std::cout << " " << cs[j].rawValue();
        std::cout << " visual:";
        for (int c = 0; c < t->columns(); ++c) {
            QTextTableCell cell = t->cellAt(0, c);
            QRectF cellRect; bool first=true;
            for (auto it = cell.begin(); !(it.atEnd()); ++it) {
                QTextBlock b = it.currentBlock();
                if (b.isValid()) {
                    QRectF r = layout->blockBoundingRect(b);
                    if (first) { cellRect=r; first=false; } else cellRect |= r;
                }
            }
            std::cout << " " << cellRect.width();
        }
        std::cout << std::endl;
    }
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QFile f("d:/projects/DokitLab/infant/assets/ex/1.12/template.html");
    f.open(QIODevice::ReadOnly);
    QString html = QString::fromUtf8(f.readAll());
    // like prepareTemplateHtml - inject style WITHOUT col widths in CSS for td
    html.replace("</head>", QString::fromUtf8(
        "<style>table { table-layout:fixed; width:671px; max-width:671px; min-width:671px; "
        "border-collapse:collapse; border:1px solid #000; }"
        "td,th { border:1px solid #000; }</style></head>").toUtf8());

    QTextEdit edit;
    edit.setFixedWidth(720);
    edit.setHtml(html);
    edit.document()->setDocumentMargin(0);
    edit.document()->setTextWidth(671);
    edit.show();
    app.processEvents();
    dump(edit, "BEFORE any force (with CSS like app)");

    // Simulate buggy force: keep existing if sum near 671
    QList<QTextTable*> tables;
    collectTables(edit.document()->rootFrame(), tables);
    for (QTextTable *table : tables) {
        QTextTableFormat fmt = table->format();
        fmt.setWidth(QTextLength(QTextLength::FixedLength, 671));
        // DON'T change constraints - like KEPT existing
        table->setFormat(fmt);
    }
    app.processEvents();
    dump(edit, "AFTER setWidth only");
    return 0;
}
