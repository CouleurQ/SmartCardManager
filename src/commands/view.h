#ifndef VIEW_H
#define VIEW_H

#include <QWidget>

#include "commands/command.h"


class View : public QWidget {
    Q_OBJECT

public:
    explicit View(QWidget *parent = nullptr);
    ~View();

    virtual void sendUpdateCommand() {};

signals:
    void notifyUpdateCommand(QSharedPointer<Command> obj);
};

#endif
