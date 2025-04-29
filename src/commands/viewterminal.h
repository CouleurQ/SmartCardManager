#ifndef COMMAND_TERMINAL_H
#define COMMAND_TERMINAL_H

#include <QWidget>


namespace Ui {
class ViewTerminal;
}


class ViewTerminal : public QWidget {
    Q_OBJECT

public:
    explicit ViewTerminal(QWidget *parent = nullptr);
    ~ViewTerminal();

private:
    Ui::ViewTerminal *ui;
};

#endif
