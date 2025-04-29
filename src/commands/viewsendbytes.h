#ifndef COMMAND_SEND_BYTES_H
#define COMMAND_SEND_BYTES_H

#include <QWidget>


namespace Ui {
class ViewSendBytes;
}


class ViewSendBytes : public QWidget {
    Q_OBJECT

public:
    explicit ViewSendBytes(QWidget *parent = nullptr);
    ~ViewSendBytes();

private:
    void updateCommand();

private:
    Ui::ViewSendBytes *ui;
};

#endif
