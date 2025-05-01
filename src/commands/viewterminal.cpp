#include "viewterminal.h"
#include "ui_viewterminal.h"
#include "commandsendbytes.h"

#include <QTimer>


ViewTerminal::ViewTerminal(QSharedPointer<CommandTerminal> obj, QWidget *parent) :
    View(parent),
    ui(new Ui::ViewTerminal)
{
    ui->setupUi(this);

    if (obj != nullptr) {
        ui->lineEditCommandName->setText(obj->commandName);
    }

    connect(ui->lineEditCommandName, &QLineEdit::textChanged, this, &ViewTerminal::sendUpdateCommand);

    // set the focus (delayed) and update the listview
    QTimer::singleShot(0, this, [this]() {
        ui->lineEditCommandName->setFocus();
    });
}


ViewTerminal::~ViewTerminal()
{
    delete ui;
}


void ViewTerminal::sendUpdateCommand()
{
    QSharedPointer<CommandTerminal> obj = QSharedPointer<CommandTerminal>::create();

    obj->commandName = ui->lineEditCommandName->text();

    emit notifyUpdateCommand(obj);
}
