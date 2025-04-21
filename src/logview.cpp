#include "logview.h"

#include <QHeaderView>
#include <QMetaObject>
#include <QMetaEnum>
#include <QSvgRenderer>
#include <QPainter>
#include <QTimer>
#include <QMenu>
#include <QGuiApplication>
#include <QClipboard>


LogView::LogView(QTreeView *view, QObject *parent)
    : QObject(parent), treeView(view)
{
    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels(QStringList() << "Type" << "Command" << "ASCII");

    treeView->setModel(model);
    treeView->setRootIsDecorated(false);
    treeView->setItemsExpandable(false);
    treeView->setAlternatingRowColors(true);
    treeView->setUniformRowHeights(true);
    //treeView->header()->setStretchLastSection(true);
    treeView->setIconSize(QSize(16, 16));
    treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    treeView->header()->setSectionResizeMode(QHeaderView::Interactive);
    treeView->header()->resizeSection(0, 38);
    treeView->header()->resizeSection(1, 500);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, &QTreeView::customContextMenuRequested, this, &LogView::showContextMenu);

}


void LogView::addLogEntry(LogType type, const QString &message, const QString &ascii)
{
    QList<QStandardItem *> items;

    QStandardItem *typeItem = new QStandardItem(iconForType(type), nameForType(type));
    typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
    QStandardItem *messageItem = new QStandardItem(message);
    messageItem->setFlags(messageItem->flags() & ~Qt::ItemIsEditable);
    QStandardItem *asciiItem = nullptr;

    if (type == LogType::CardCmd || type == LogType::CardRsp) {
        asciiItem = new QStandardItem(ascii);
        asciiItem->setFlags(asciiItem->flags() & ~Qt::ItemIsEditable);
    }

    items << typeItem << messageItem << asciiItem;
    model->appendRow(items);

    // scroll to last entry
    QModelIndex lastIndex = model->index(model->rowCount() - 1, 0, QModelIndex());
    QTimer::singleShot(0, this, [this, lastIndex]() {
        treeView->scrollTo(lastIndex, QAbstractItemView::PositionAtBottom);
    });
}


void LogView::addLogEntry(LogType type, const QByteArray &message)
{
    QString ascii;
    for (char c : message) {
        if (c >= 0x20 && c <= 0x7E)
            ascii += QChar(c);
        else
            ascii += '.';
    }

    addLogEntry(type, message.toHex(' ').toUpper(), ascii);
}


void LogView::showContextMenu(const QPoint &pos)
{
    QModelIndex index = treeView->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu contextMenu;
    contextMenu.addAction("Copy Message", this, [this, index]() {
        QString message = model->item(index.row(), 1)->text(); // copy 2nd column
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(message);
    });

    contextMenu.addAction("Save Log", this, [this]() {
        // TODO
    });

    contextMenu.addAction("Clear Log", this, [this]() {
        model->removeRows(0, model->rowCount());
    });

    contextMenu.exec(treeView->viewport()->mapToGlobal(pos));
}


QIcon LogView::iconForType(LogType type)
{
    QColor color = Qt::black;
    QString path = "";

    switch (type) {
    case LogType::Info:
        path = ":/res/icons/log_info.svg";
        color = QColor(0x0077cc);
        break;
    case LogType::Warning:
        path = ":/res/icons/log_warning.svg";
        color = QColor(0xffaa00);
        break;
    case LogType::Error:
        path = ":/res/icons/log_error.svg";
        color = QColor(0xcc0000);
        break;
    case LogType::CardCmd:
        path = ":/res/icons/log_card_command.svg";
        break;
    case LogType::CardRsp:
        path = ":/res/icons/log_card_response.svg";
        break;
    case LogType::Terminal:
        path = ":/res/icons/log_terminal.svg";
        break;
    }

    return colorizeSvg(path, color, QSize(16, 16));
}


QString LogView::nameForType(LogType type)
{
    const QMetaObject *mo = metaObject();
    if (mo) {
        int index = mo->indexOfEnumerator("LogType");
        QMetaEnum metaEnum = mo->enumerator(index);
        return metaEnum.valueToKey(static_cast<int>(type));
    }

    return QString();
}


QIcon LogView::colorizeSvg(const QString &svgPath, const QColor &color, const QSize &size)
{
    QSvgRenderer svg(svgPath);
    QPixmap basePixmap(size);
    basePixmap.fill(Qt::transparent);

    QPainter painter(&basePixmap);
    svg.render(&painter);

    // Tint layer
    QPixmap tintedPixmap(size);
    tintedPixmap.fill(Qt::transparent);

    QPainter tintPainter(&tintedPixmap);
    tintPainter.setCompositionMode(QPainter::CompositionMode_Source);
    tintPainter.fillRect(tintedPixmap.rect(), color);
    tintPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    tintPainter.drawPixmap(0, 0, basePixmap);
    tintPainter.end();

    return QIcon(tintedPixmap);
}
