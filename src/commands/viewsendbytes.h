#ifndef VIEW_SEND_BYTES_H
#define VIEW_SEND_BYTES_H

#include <QWidget>

#include "commands/view.h"


namespace Ui {
class ViewSendBytes;
}


class ViewSendBytes : public View {
    Q_OBJECT

public:
    explicit ViewSendBytes(QWidget *parent = nullptr);
    ~ViewSendBytes();

    void sendUpdateCommand();

private:
    Ui::ViewSendBytes *ui;
};

#endif
