#ifndef COMMAND_VIEW_APDU_H
#define COMMAND_VIEW_APDU_H

#include <QWidget>

#include "commands/commandview.h"
#include "commands/commandapdu.h"


namespace Ui {
class CommandViewApdu;
}


class CommandViewApdu : public CommandView {
    Q_OBJECT

public:
    explicit CommandViewApdu(QSharedPointer<CommandApdu> obj = nullptr, QWidget *parent = nullptr);
    ~CommandViewApdu();

    void sendUpdateCommand();

private:
    Ui::CommandViewApdu *ui;
};

#endif
