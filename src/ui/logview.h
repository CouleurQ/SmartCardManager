#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QObject>
#include <QStandardItemModel>
#include <QTreeView>

#include "global.h"


class LogView : public QObject
{
    Q_OBJECT

public:
    explicit LogView(QTreeView *view, QObject *parent = nullptr);

    void addLogEntry(LogType type, const QString &message, const QString &ascii = nullptr);
    void addLogEntry(LogType type, const QByteArray &message);

private:
    QTreeView *treeView;
    QStandardItemModel *model;

    void showContextMenu(const QPoint &pos);

    QIcon iconForType(LogType type);
    QString nameForType(LogType type);
    QIcon colorizeSvg(const QString &svgPath, const QColor &color, const QSize &size = QSize(16, 16));
};

#endif
