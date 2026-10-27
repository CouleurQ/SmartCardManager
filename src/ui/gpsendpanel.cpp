#include "gpsendpanel.h"

#include "commands/commandapdu.h"
#include "hexedit.h"
#include "utility.h"

#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

HexEdit *singleByte(const QString &name)
{
    auto *edit = new HexEdit;
    edit->setObjectName(name);
    edit->setMaxBytes(1);
    edit->setLineWrapMode(QTextEdit::NoWrap);
    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setFixedHeight(edit->fontMetrics().lineSpacing() + 12);
    edit->setMaximumWidth(70);
    return edit;
}

} // namespace


GpSendPanel::GpSendPanel(Mode m, QWidget *parent)
    : QWidget(parent)
    , mode(m)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    if (mode == Mode::Apdu) {
        cla = singleByte("hexCla");
        ins = singleByte("hexIns");
        p1 = singleByte("hexP1");
        p2 = singleByte("hexP2");
        le = singleByte("hexLe");
        data = new HexEdit;
        data->setObjectName("hexData");
        data->setMaxBytes(261);
        lc = new QLineEdit;
        lc->setObjectName("lineLc");
        lc->setReadOnly(true);
        lc->setFont(mono);
        lc->setMaximumWidth(70);

        auto *grid = new QGridLayout;
        const QStringList names = {"CLA", "INS", "P1", "P2", "Lc", "Le"};
        QWidget *fields[] = {cla, ins, p1, p2, lc, le};
        for (int i = 0; i < 6; ++i) {
            grid->addWidget(new QLabel(names[i]), 0, i);
            grid->addWidget(fields[i], 1, i);
        }
        grid->setColumnStretch(6, 1);
        layout->addLayout(grid);
        layout->addWidget(new QLabel("Data"));
        layout->addWidget(data, 1);

        for (HexEdit *edit : {cla, ins, p1, p2, data, le})
            connect(edit, &HexEdit::textChanged, this, &GpSendPanel::updatePreview);
    } else {
        layout->addWidget(new QLabel("Bytes"));
        rawBytes = new HexEdit;
        rawBytes->setObjectName("hexBytes");
        rawBytes->setMaxBytes(261);
        layout->addWidget(rawBytes, 1);
        connect(rawBytes, &HexEdit::textChanged, this, &GpSendPanel::updatePreview);
    }

    buttonSend = new QPushButton("Send");
    buttonSend->setObjectName("buttonSend");
    buttonSend->setDefault(true);
    connect(buttonSend, &QPushButton::clicked, this, &GpSendPanel::send);

    layout->addWidget(buttonSend, 0, Qt::AlignRight);

    layout->addWidget(new QLabel("Status word"));
    sw = new QLineEdit;
    sw->setObjectName("lineSw");
    sw->setReadOnly(true);
    sw->setFont(mono);
    sw->setMaximumWidth(70);
    swText = new QLineEdit;
    swText->setObjectName("lineSwText");
    swText->setReadOnly(true);
    auto *swRow = new QHBoxLayout;
    swRow->addWidget(sw);
    swRow->addWidget(swText, 1);
    layout->addLayout(swRow);

    layout->addWidget(new QLabel("Response"));
    response = new QTextEdit;
    response->setObjectName("textResponse");
    response->setReadOnly(true);
    response->setFont(mono);
    layout->addWidget(response, 1);

    updatePreview();
}


QByteArray GpSendPanel::command() const
{
    if (mode == Mode::Bytes)
        return rawBytes->data();

    CommandApdu apdu;
    apdu.cla = cla->data();
    apdu.ins = ins->data();
    apdu.p1 = p1->data();
    apdu.p2 = p2->data();
    apdu.commandData = data->data();
    apdu.le = le->data();
    return apdu.buildApdu();
}


void GpSendPanel::setSendEnabled(bool enabled)
{
    buttonSend->setProperty("allowed", enabled);
    updatePreview();
}


void GpSendPanel::updatePreview()
{
    if (lc)
        lc->setText(data->length() ? QString("%1").arg(data->length(), 2, 16, QChar('0')).toUpper() : QString());
    buttonSend->setEnabled(buttonSend->property("allowed").toBool() && !command().isEmpty());
}


void GpSendPanel::send()
{
    const QByteArray cmd = command();
    if (!cmd.isEmpty())
        emit sendRequested(cmd);
}


void GpSendPanel::clearResponse()
{
    response->clear();
    sw->clear();
    swText->clear();
}


void GpSendPanel::showResponse(const QByteArray &answer)
{
    response->setPlainText(Utility::byteArrayToHexString(answer));
    if (answer.size() < 2) {
        sw->clear();
        swText->clear();
        return;
    }
    sw->setText(Utility::byteArrayToHexString(answer.right(2)));
    swText->setText(Utility::statusWordText(quint8(answer[answer.size() - 2]), quint8(answer[answer.size() - 1])));
}
