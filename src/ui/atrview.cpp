#include "atrview.h"

#include "card/atrparser.h"
#include "utility.h"

#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

const int RangeFirstRole = Qt::UserRole;
const int RangeCountRole = Qt::UserRole + 1;

QString bytesText(const QByteArray &atr, int first, int count)
{
    return Utility::byteArrayToHexString(atr.mid(first, count));
}


void addRows(QTreeWidgetItem *parent, QTreeWidget *tree, const QByteArray &atr, const QList<AtrParser::Row> &rows)
{
    for (const AtrParser::Row &row : rows) {
        auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree);
        item->setText(0, row.first >= 0 ? bytesText(atr, row.first, row.count) : QString());
        item->setText(1, row.field);
        item->setText(2, row.meaning);
        item->setData(0, RangeFirstRole, row.first);
        item->setData(0, RangeCountRole, row.count);
        item->setData(0, Qt::FontRole, QFontDatabase::systemFont(QFontDatabase::FixedFont));
        addRows(item, tree, atr, row.children);
    }
}

} // namespace


AtrView::AtrView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel("<b>Answer To Reset (ATR)</b>");
    layout->addWidget(title);

    atrLabel = new QLabel;
    atrLabel->setObjectName("atrLabel");
    atrLabel->setTextFormat(Qt::RichText);
    atrLabel->setWordWrap(true);
    atrLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(mono.pointSize() + 3);
    atrLabel->setFont(mono);
    layout->addWidget(atrLabel);

    summaryLabel = new QLabel;
    summaryLabel->setObjectName("atrSummary");
    summaryLabel->setTextFormat(Qt::RichText);
    summaryLabel->setWordWrap(true);
    summaryLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    layout->addWidget(summaryLabel);

    warningLabel = new QLabel;
    warningLabel->setObjectName("atrWarnings");
    warningLabel->setTextFormat(Qt::RichText);
    warningLabel->setWordWrap(true);
    layout->addWidget(warningLabel);

    tree = new QTreeWidget;
    tree->setObjectName("atrTree");
    tree->setColumnCount(3);
    tree->setHeaderLabels({"Bytes", "Field", "Interpretation"});
    tree->setRootIsDecorated(true);
    tree->setAlternatingRowColors(true);
    tree->setUniformRowHeights(true);
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setStretchLastSection(true);
    layout->addWidget(tree, 1);

    connect(tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *item) {
        if (item && item->data(0, RangeFirstRole).toInt() >= 0)
            highlight(item->data(0, RangeFirstRole).toInt(), item->data(0, RangeCountRole).toInt());
        else
            highlight(-1, 0);
    });

    setAtr(QByteArray());
}


void AtrView::setAtr(const QByteArray &atr)
{
    currentAtr = atr;
    tree->clear();

    if (atr.isEmpty()) {
        atrLabel->setText("<i>No ATR available</i>");
        summaryLabel->clear();
        warningLabel->clear();
        return;
    }

    const AtrParser::Info info = AtrParser::parse(atr);

    highlight(-1, 0);

    QString summary = "<ul style='margin-left:-20px'>";
    for (const QString &line : info.summary)
        summary += QString("<li>%1</li>").arg(line.toHtmlEscaped());
    summary += QString("<li>Length: %1 byte(s)</li></ul>").arg(atr.size());
    summaryLabel->setText(summary);

    QString warnings;
    for (const QString &line : info.warnings)
        warnings += QString("&#9888; %1<br>").arg(line.toHtmlEscaped());
    warningLabel->setText(warnings.isEmpty() ? QString() : QString("<span style='color:#c62828'>%1</span>").arg(warnings));

    addRows(nullptr, tree, atr, info.rows);
    tree->expandAll();
}


void AtrView::highlight(int first, int count)
{
    QStringList spans;
    for (int i = 0; i < currentAtr.size(); ++i) {
        const QString byte = bytesText(currentAtr, i, 1);
        if (i >= first && i < first + count)
            spans << QString("<span style='background-color:#ffd54f; color:#000000; font-weight:bold'>%1</span>").arg(byte);
        else
            spans << byte;
    }
    atrLabel->setText(spans.join(' '));
}
