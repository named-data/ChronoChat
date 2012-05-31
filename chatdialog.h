#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include "ui_chatdialog.h"
#include "chatbuf.pb.h"

class ChatDialog : public QDialog,  private Ui::ChatDialog 
{
	Q_OBJECT

public:
	ChatDialog(QWidget *parent = 0);

public slots:
  void appendMessage(const SyncDemo::ChatMessage &msg);
  void updateTreeView();

private slots:
  void returnPressed();
  void formChatMessage(const QString &text, SyncDemo::ChatMessage &msg);

private:
  QString m_nick;
  QString m_chatroom;
};
#endif
