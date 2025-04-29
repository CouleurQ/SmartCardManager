#include "viewapdu.h"
#include "ui_viewapdu.h"


ViewApdu::ViewApdu(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ViewApdu)
{
    ui->setupUi(this);
}

ViewApdu::~ViewApdu()
{
    delete ui;
}
