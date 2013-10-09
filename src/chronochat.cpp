#include "chronochat.h"
#include "ui_chronochat.h"

ChronoChat::ChronoChat(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChronoChat)
{
    ui->setupUi(this);
}

ChronoChat::~ChronoChat()
{
    delete ui;
}
