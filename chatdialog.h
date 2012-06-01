#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include <boost/function.hpp>
#include <vector>
#include "digesttreescene.h"
#include "ui_chatdialog.h"
#include "chatbuf.pb.h"
#include <sync-app-socket.h>

class ChatDialog : public QDialog,  private Ui::ChatDialog 
{
	Q_OBJECT

public:
	ChatDialog(QWidget *parent = 0);
  void appendMessage(const SyncDemo::ChatMessage &msg);
  void processData(const std::vector<MissingDataInfo> &, SyncAppSocket *);

private:
  void formChatMessage(const QString &text, SyncDemo::ChatMessage &msg);

private slots:
  void returnPressed();

private:
  QString m_nick;
  QString m_chatroom;
  QString m_prefix;
  Sync::SyncAppSocket *m_sock;
};
#endif
