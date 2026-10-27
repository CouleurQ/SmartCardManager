#ifndef VIEW_TERMINAL_H
#define VIEW_TERMINAL_H

#include <QWidget>

#include "commands/commandview.h"
#include "commands/commandterminal.h"


namespace Ui {
class CommandViewTerminal;
}


class CommandViewTerminal : public CommandView {
    Q_OBJECT

public:
    explicit CommandViewTerminal(QSharedPointer<CommandTerminal> obj = nullptr, QWidget *parent = nullptr);
    ~CommandViewTerminal();

    void sendUpdateCommand();

private:
    Ui::CommandViewTerminal *ui;
};

#endif
