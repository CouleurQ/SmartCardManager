#include "viewterminal.h"
#include "ui_viewterminal.h"


ViewTerminal::ViewTerminal(QWidget *parent) :
    View(parent),
    ui(new Ui::ViewTerminal)
{
    ui->setupUi(this);
}

ViewTerminal::~ViewTerminal()
{
    delete ui;
}
