#ifndef COMMAND_APDU_H
#define COMMAND_APDU_H

#include <QWidget>


namespace Ui {
class ViewApdu;
}


class ViewApdu : public QWidget {
    Q_OBJECT

public:
    explicit ViewApdu(QWidget *parent = nullptr);
    ~ViewApdu();

private:
    Ui::ViewApdu *ui;
};

#endif
