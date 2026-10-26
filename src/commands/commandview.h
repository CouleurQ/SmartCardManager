#ifndef COMMAND_VIEW_H
#define COMMAND_VIEW_H

#include <QWidget>

#include "commands/command.h"


class CommandView : public QWidget {
    Q_OBJECT

public:
    explicit CommandView(QWidget *parent = nullptr);
    ~CommandView();

    virtual void sendUpdateCommand() {};

signals:
    void notifyUpdateCommand(QSharedPointer<Command> obj);
};

#endif
