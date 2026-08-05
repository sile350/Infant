#include <QCoreApplication>
#include <QTextDocument>
#include <QTextTable>
#include <QTextFrame>
#include <iostream>
#include <QTextBlock>
#include <QFile>

// paste force logic simplified - link against infant? just duplicate key part

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

void force(QTextDocument *document, int widthPx) {
    QList<QTextTable*> tables;
    collectTables(document->rootFrame(), tables);
    for (QTextTable *table : tables) {
        if (!table || table->columns() <= 0) continue;
        QTextTableFormat fmt = table->format();
        fmt.setWidth(QTextLength(QTextLength::FixedLength, widthPx));
        fmt.setBorder(1);
        fmt.setCellPadding(0);
        fmt.setCellSpacing(0);
        const int cols = table->columns();
        if (cols == 2) {
            fmt.setColumnWidthConstraints({
                QTextLength(QTextLength::FixedLength, 200),
                QTextLength(QTextLength::FixedLength, widthPx - 200),
            });
        } else {
            QVector<QTextLength> constraints = fmt.columnWidthConstraints();
            bool applied = false;
            if (constraints.size() == cols) {
                qreal sum = 0;
                bool allFixed = true;
                for (const QTextLength &c : constraints) {
                    if (c.type() != QTextLength::FixedLength) { allFixed = false; break; }
                    sum += c.rawValue();
                }
                if (allFixed && sum > 1.0 && qAbs(sum - widthPx) > 0.5) {
                    QVector<QTextLength> scaled;
                    qreal used = 0;
                    for (int i = 0; i < cols; ++i) {
                        if (i == cols - 1) scaled.append(QTextLength(QTextLength::FixedLength, widthPx - used));
                        else {
                            const qreal w = qRound(constraints.at(i).rawValue() * widthPx / sum);
                            scaled.append(QTextLength(QTextLength::FixedLength, w));
                            used += w;
                        }
                    }
                    fmt.setColumnWidthConstraints(scaled);
                    applied = true;
                    std::cout << "SCALED existing" << std::endl;
                } else if (allFixed && qAbs(sum - widthPx) <= 0.5) {
                    applied = true;
                    std::cout << "KEPT existing (sum ok)" << std::endl;
                }
            }
            if (!applied) {
                int headerRow = 0;
                for (int r = 0; r < qMin(3, table->rows()); ++r) {
                    const QString c0 = cellText(table, r, 0);
                    if (c0.contains(QStringLiteral("Процесс выполнения"), Qt::CaseInsensitive)) continue;
                    int nonEmpty = 0;
                    for (int c = 0; c < cols; ++c)
                        if (!cellText(table, r, c).trimmed().isEmpty()) ++nonEmpty;
                    if (nonEmpty >= qMin(2, cols)) { headerRow = r; break; }
                }
                QString headerJoin;
                for (int c = 0; c < cols; ++c) headerJoin += cellText(table, headerRow, c) + " ";
                std::cout << "headerJoin=[" << headerJoin.toStdString() << "] cols=" << cols << std::endl;
                if (cols == 3 && headerJoin.contains(QStringLiteral("Характер"), Qt::CaseInsensitive)
                    && headerJoin.contains(QStringLiteral("Баллы"), Qt::CaseInsensitive)) {
                    fmt.setColumnWidthConstraints({
                        QTextLength(QTextLength::FixedLength, 308),
                        QTextLength(QTextLength::FixedLength, 298),
                        QTextLength(QTextLength::FixedLength, 65)});
                    applied = true;
                    std::cout << "APPLIED 308/298/65" << std::endl;
                }
            }
        }
        table->setFormat(fmt);
        auto c = table->format().columnWidthConstraints();
        std::cout << "RESULT:";
        for (int j = 0; j < c.size(); ++j) std::cout << " " << c[j].rawValue();
        std::cout << std::endl;
    }
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    // Simulate stored protocol without widths (Qt roundtrip often drops them)
    QString html =
      "<html><body>"
      "<table border='1' width='671'><tr>"
      "<td>Характер деятельности ребенка</td>"
      "<td>Виды помощи</td>"
      "<td>Баллы</td></tr>"
      "<tr><td>text1</td><td>text2</td><td>3</td></tr>"
      "</table></body></html>";
    QTextDocument doc;
    doc.setHtml(html);
    QList<QTextTable*> tables;
    collectTables(doc.rootFrame(), tables);
    std::cout << "BEFORE force:" << std::endl;
    for (auto *t : tables) {
        auto c = t->format().columnWidthConstraints();
        std::cout << "cols=" << t->columns() << " n=" << c.size();
        for (int j = 0; j < c.size(); ++j) std::cout << " " << c[j].type() << ":" << c[j].rawValue();
        std::cout << std::endl;
    }
    force(&doc, 671);
    // Roundtrip: toHtml and back
    QString out = doc.toHtml();
    std::cout << "toHtml snippet col widths:" << std::endl;
    // find width in cols
    int idx = 0;
    while ((idx = out.indexOf("width:", idx)) >= 0) {
        std::cout << out.mid(idx, 40).toStdString() << std::endl;
        idx += 6;
        if (idx > 5000) break;
    }
    QTextDocument doc2;
    doc2.setHtml(out);
    force(&doc2, 671);
    return 0;
}
