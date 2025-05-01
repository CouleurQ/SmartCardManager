#ifndef VIEW_APDU_H
#define VIEW_APDU_H

#include <QWidget>

#include "commands/view.h"
#include "commands/commandapdu.h"


namespace Ui {
class ViewApdu;
}


class ViewApdu : public View {
    Q_OBJECT

public:
    explicit ViewApdu(QSharedPointer<CommandApdu> obj = nullptr, QWidget *parent = nullptr);
    ~ViewApdu();

    void sendUpdateCommand();

private:
    Ui::ViewApdu *ui;
};

#endif
