#include "viewsendbytes.h"
#include "ui_viewsendbytes.h"

#include <QDebug>
#include <QResizeEvent>


ViewSendBytes::ViewSendBytes(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ViewSendBytes)
{
    ui->setupUi(this);

    ui->commandLineEditCommandName->setText("Send bytes command");

    connect(ui->commandLineEditCommandName, &QLineEdit::textChanged, this, &ViewSendBytes::updateCommand);
    connect(ui->commandHexEditCommandApdu, &HexEdit::textChanged, this, &ViewSendBytes::updateCommand);
    connect(ui->commandLineEditExpectedResponse, &HexEdit::textChanged, this, &ViewSendBytes::updateCommand);

    ui->commandLineEditCommandName->setFocus();
}


ViewSendBytes::~ViewSendBytes()
{
    delete ui;
}


void ViewSendBytes::updateCommand()
{
    qDebug() << "update Command";
}
