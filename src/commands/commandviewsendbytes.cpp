#include "commandviewsendbytes.h"
#include "ui_commandviewsendbytes.h"
#include "commandsendbytes.h"
#include "utility.h"

#include <QTimer>


CommandViewSendBytes::CommandViewSendBytes(QSharedPointer<CommandSendBytes> obj, QWidget *parent) :
    CommandView(parent),
    ui(new Ui::CommandViewSendBytes)
{
    ui->setupUi(this);

    if (obj != nullptr) {
        ui->lineEditCommandName->setText(obj->commandName);
        ui->hexEditCommandApdu->setData(obj->commandData);
        ui->hexEditExpectedResponse->setData(obj->expectedResponse);
    }

    connect(ui->lineEditCommandName, &QLineEdit::textChanged, this, &CommandViewSendBytes::sendUpdateCommand);
    connect(ui->hexEditCommandApdu, &HexEdit::textChanged, this, &CommandViewSendBytes::sendUpdateCommand);
    connect(ui->hexEditExpectedResponse, &HexEdit::textChanged, this, &CommandViewSendBytes::sendUpdateCommand);

    // expected response is single line
    ui->hexEditCommandApdu->setMaxBytes(261);
    ui->hexEditExpectedResponse->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditExpectedResponse->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    // set the focus (delayed) and update the listview
    QTimer::singleShot(0, this, [this]() {
        ui->lineEditCommandName->setFocus();
    });
}


CommandViewSendBytes::~CommandViewSendBytes()
{
    delete ui;
}


void CommandViewSendBytes::sendUpdateCommand()
{
    QSharedPointer<CommandSendBytes> obj = QSharedPointer<CommandSendBytes>::create();

    obj->commandName = ui->lineEditCommandName->text();
    obj->commandData = ui->hexEditCommandApdu->data();
    obj->expectedResponse = ui->hexEditExpectedResponse->data();

    emit notifyUpdateCommand(obj);
}
