#ifndef VIEW_TERMINAL_H
#define VIEW_TERMINAL_H

#include <QWidget>

#include "commands/view.h"


namespace Ui {
class ViewTerminal;
}


class ViewTerminal : public View {
    Q_OBJECT

public:
    explicit ViewTerminal(QWidget *parent = nullptr);
    ~ViewTerminal();

    void sendUpdateCommand() {};

private:
    Ui::ViewTerminal *ui;
};

#endif
