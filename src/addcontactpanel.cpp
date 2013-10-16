#include "addcontactpanel.h"
#include "ui_addcontactpanel.h"

AddContactPanel::AddContactPanel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddContactPanel)
{
    ui->setupUi(this);
}

AddContactPanel::~AddContactPanel()
{
    delete ui;
}
