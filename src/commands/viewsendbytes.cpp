#include "viewsendbytes.h"
#include "ui_viewsendbytes.h"
#include "commandsendbytes.h"
#include "utility.h"

#include <QTimer>


ViewSendBytes::ViewSendBytes(QSharedPointer<CommandSendBytes> obj, QWidget *parent) :
    View(parent),
    ui(new Ui::ViewSendBytes)
{
    ui->setupUi(this);

    if (obj != nullptr) {
        ui->lineEditCommandName->setText(obj->commandName);
        ui->hexEditCommandApdu->setData(obj->commandData);
        ui->hexEditExpectedResponse->setData(obj->expectedResponse);
    }

    connect(ui->lineEditCommandName, &QLineEdit::textChanged, this, &ViewSendBytes::sendUpdateCommand);
    connect(ui->hexEditCommandApdu, &HexEdit::textChanged, this, &ViewSendBytes::sendUpdateCommand);
    connect(ui->hexEditExpectedResponse, &HexEdit::textChanged, this, &ViewSendBytes::sendUpdateCommand);

    // expected response is single line
    ui->hexEditCommandApdu->setMaxBytes(261);
    ui->hexEditExpectedResponse->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditExpectedResponse->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    // set the focus (delayed) and update the listview
    QTimer::singleShot(0, this, [this]() {
        ui->lineEditCommandName->setFocus();
    });
}


ViewSendBytes::~ViewSendBytes()
{
    delete ui;
}


void ViewSendBytes::sendUpdateCommand()
{
    QSharedPointer<CommandSendBytes> obj = QSharedPointer<CommandSendBytes>::create();

    obj->commandName = ui->lineEditCommandName->text();
    obj->commandData = ui->hexEditCommandApdu->data();
    obj->expectedResponse = ui->hexEditExpectedResponse->data();

    emit notifyUpdateCommand(obj);
}
