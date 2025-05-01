#include "viewapdu.h"
#include "ui_viewapdu.h"
#include "commandapdu.h"
#include "commands/commandapdu.h"

#include <QTimer>


ViewApdu::ViewApdu(QSharedPointer<CommandApdu> obj, QWidget *parent) :
    View(parent),
    ui(new Ui::ViewApdu)
{
    ui->setupUi(this);

    if (obj != nullptr) {
        ui->lineEditCommandName->setText(obj->commandName);
        ui->hexEditCLA->setData(obj->cla);
        ui->hexEditINS->setData(obj->ins);
        ui->hexEditP1->setData(obj->p1);
        ui->hexEditP2->setData(obj->p2);
        ui->lineEditLc->setText(QString::number(obj->commandData.length(), 16));
        ui->hexEditData->setData(obj->commandData);
        ui->hexEditLe->setData(obj->le);
        ui->hexEditExpectedResponse->setData(obj->expectedResponse);
    }

    connect(ui->lineEditCommandName, &QLineEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditCLA, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditINS, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditP1, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditP2, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditData, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditLe, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditExpectedResponse, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);

    // single line fields
    ui->hexEditData->setMaxBytes(261);
    ui->hexEditCLA->setMaxBytes(1);
    ui->hexEditCLA->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditCLA->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditINS->setMaxBytes(1);
    ui->hexEditINS->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditINS->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditP1->setMaxBytes(1);
    ui->hexEditP1->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditP1->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditP2->setMaxBytes(1);
    ui->hexEditP2->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditP2->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditLe->setMaxBytes(1);
    ui->hexEditLe->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditLe->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditExpectedResponse->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditExpectedResponse->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->lineEditLc->setFocusPolicy(Qt::NoFocus);

    // set the focus (delayed)
    QTimer::singleShot(0, this, [this]() {
        ui->lineEditCommandName->setFocus();
    });
}


ViewApdu::~ViewApdu()
{
    delete ui;
}


void ViewApdu::sendUpdateCommand()
{
    QSharedPointer<CommandApdu> obj = QSharedPointer<CommandApdu>::create();

    obj->commandName = ui->lineEditCommandName->text();
    obj->cla = ui->hexEditCLA->data();
    obj->ins = ui->hexEditINS->data();
    obj->p1 = ui->hexEditP1->data();
    obj->p2 = ui->hexEditP2->data();
    obj->commandData = ui->hexEditData->data();
    obj->le = ui->hexEditLe->data();
    obj->expectedResponse = ui->hexEditExpectedResponse->data();

    emit notifyUpdateCommand(obj);

    QByteArray lc = ui->hexEditData->data();
    ui->lineEditLc->setText(QString::number(lc.length(), 16));
}
