#include "contactpanel.h"
#include "ui_contactpanel.h"

ContactPanel::ContactPanel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContactPanel)
{
    ui->setupUi(this);
}

ContactPanel::~ContactPanel()
{
    delete ui;
}
