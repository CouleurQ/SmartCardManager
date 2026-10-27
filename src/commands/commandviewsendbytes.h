#ifndef VIEW_SEND_BYTES_H
#define VIEW_SEND_BYTES_H

#include <QWidget>

#include "commands/commandview.h"
#include "commands/commandsendbytes.h"


namespace Ui {
class CommandViewSendBytes;
}


class CommandViewSendBytes : public CommandView {
    Q_OBJECT

public:
    explicit CommandViewSendBytes(QSharedPointer<CommandSendBytes> obj = nullptr, QWidget *parent = nullptr);
    ~CommandViewSendBytes();

    void sendUpdateCommand();

private:
    Ui::CommandViewSendBytes *ui;
};

#endif
