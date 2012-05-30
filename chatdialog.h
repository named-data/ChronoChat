#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include "ui_chatdialog.h"

class ChatDialog : public QDialog,  private Ui::ChatDialog 
{
	Q_OBJECT

public:
	ChatDialog(QWidget *parent = 0);
};
#endif
