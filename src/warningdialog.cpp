#include "warningdialog.h"
#include "ui_warningdialog.h"

WarningDialog::WarningDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WarningDialog)
{
    ui->setupUi(this);
}

WarningDialog::~WarningDialog()
{
    delete ui;
}

#if WAF
#include "warningdialog.moc"
#include "warningdialog.cpp.moc"
#endif
