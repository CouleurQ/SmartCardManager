#include "commandviewapdu.h"
#include "ui_commandviewapdu.h"
#include "commands/commandapdu.h"

#include <QTextDocument>
#include <QTimer>


CommandViewApdu::CommandViewApdu(QSharedPointer<CommandApdu> obj, QWidget *parent) :
    CommandView(parent),
    ui(new Ui::CommandViewApdu)
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

    connect(ui->lineEditCommandName, &QLineEdit::textChanged, this, &CommandViewApdu::sendUpdateCommand);
    connect(ui->hexEditCLA, &HexEdit::textChanged, this, &CommandViewApdu::sendUpdateCommand);
    connect(ui->hexEditINS, &HexEdit::textChanged, this, &CommandViewApdu::sendUpdateCommand);
    connect(ui->hexEditP1, &HexEdit::textChanged, this, &CommandViewApdu::sendUpdateCommand);
    connect(ui->hexEditP2, &HexEdit::textChanged, this, &CommandViewApdu::sendUpdateCommand);
    connect(ui->hexEditData, &HexEdit::textChanged, this, &CommandViewApdu::sendUpdateCommand);
    connect(ui->hexEditLe, &HexEdit::textChanged, this, &CommandViewApdu::sendUpdateCommand);
    connect(ui->hexEditExpectedResponse, &HexEdit::textChanged, this, &CommandViewApdu::sendUpdateCommand);

    // single line fields
    ui->hexEditData->setMaxBytes(261);
    ui->hexEditCLA->setMaxBytes(1);
    ui->hexEditCLA->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditCLA->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditCLA->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditINS->setMaxBytes(1);
    ui->hexEditINS->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditINS->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditINS->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditP1->setMaxBytes(1);
    ui->hexEditP1->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditP1->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditP1->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditP2->setMaxBytes(1);
    ui->hexEditP2->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditP2->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditP2->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditLe->setMaxBytes(1);
    ui->hexEditLe->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditLe->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditLe->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditExpectedResponse->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditExpectedResponse->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->hexEditExpectedResponse->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->lineEditLc->setFocusPolicy(Qt::NoFocus);

    // size the single byte fields to exactly two characters (no scrollbar, nearly square)
    HexEdit *byteEdits[] = {ui->hexEditCLA, ui->hexEditINS, ui->hexEditP1, ui->hexEditP2, ui->hexEditLe};
    const QFontMetrics metrics(ui->hexEditCLA->font());
    const int margin = 2 * (ui->hexEditCLA->frameWidth() + static_cast<int>(ui->hexEditCLA->document()->documentMargin())) + 2;
    const int byteWidth = metrics.horizontalAdvance(QStringLiteral("00")) + margin;
    const int byteHeight = metrics.lineSpacing() + margin;
    for (HexEdit *edit : byteEdits)
        edit->setFixedSize(byteWidth, byteHeight);
    ui->lineEditLc->setFixedWidth(byteWidth);
    ui->hexEditExpectedResponse->setFixedHeight(byteHeight);

    // set the focus (delayed)
    QTimer::singleShot(0, this, [this]() {
        ui->lineEditCommandName->setFocus();
    });
}


CommandViewApdu::~CommandViewApdu()
{
    delete ui;
}


void CommandViewApdu::sendUpdateCommand()
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
