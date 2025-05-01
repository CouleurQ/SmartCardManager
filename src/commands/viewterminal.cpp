#include "viewterminal.h"
#include "ui_viewterminal.h"
#include "commandterminal.h"

#include <QTimer>
#include <QMenu>


ViewTerminal::ViewTerminal(QSharedPointer<CommandTerminal> obj, QWidget *parent) :
    View(parent),
    ui(new Ui::ViewTerminal)
{
    ui->setupUi(this);

    // Add actions to the menu
    ui->comboBoxAction->addItem("Warm Reset", QVariant::fromValue(CommandTerminal::ActionType::WarmReset));
    ui->comboBoxAction->addItem("Cold Reset", QVariant::fromValue(CommandTerminal::ActionType::ColdReset));
    ui->comboBoxAction->addItem("Wait for card", QVariant::fromValue(CommandTerminal::ActionType::WaitForCard));
    ui->comboBoxAction->addItem("Wait for removal", QVariant::fromValue(CommandTerminal::ActionType::WaitForRemoval));
    ui->comboBoxAction->addItem("Connect", QVariant::fromValue(CommandTerminal::ActionType::Connect));
    ui->comboBoxAction->addItem("Disconnect", QVariant::fromValue(CommandTerminal::ActionType::Disconnect));

    // fill data from obj
    if (obj != nullptr) {
        ui->lineEditCommandName->setText(obj->commandName);
        ui->comboBoxAction->setCurrentIndex(ui->comboBoxAction->findData(QVariant::fromValue(obj->action)));
    } else
        ui->comboBoxAction->setCurrentIndex(-1);

    connect(ui->lineEditCommandName, &QLineEdit::textChanged, this, &ViewTerminal::sendUpdateCommand);
    connect(ui->comboBoxAction, &QComboBox::currentIndexChanged, this, &ViewTerminal::sendUpdateCommand);

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
    obj->action = ui->comboBoxAction->currentData().value<CommandTerminal::ActionType>();

    emit notifyUpdateCommand(obj);
}
