#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QObject>
#include <QStandardItemModel>
#include <QTreeView>


class LogView : public QObject
{
    Q_OBJECT

public:
    enum LogType {
        Info,
        Warning,
        Error,
        Terminal,
        CardCmd,
        CardRsp
    };
    Q_ENUM(LogType)

public:
    explicit LogView(QTreeView *view, QObject *parent = nullptr);

    void addLogEntry(const LogType &type, const QString &message);

private:
    QTreeView *treeView;
    QStandardItemModel *model;

    void showContextMenu(const QPoint &pos);

    QIcon iconForType(LogType type);
    QString nameForType(LogType type);
    QIcon colorizeSvg(const QString &svgPath, const QColor &color, const QSize &size = QSize(16, 16));
};

#endif
