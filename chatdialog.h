#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include <boost/function.hpp>
#include <vector>
#include "digesttreescene.h"
#include "ui_chatdialog.h"
#include "chatbuf.pb.h"
#include <sync-app-socket.h>
#include <sync-logic.h>
#include <sync-seq-no.h>

#define ORGANIZATION "IRL@UCLA"
#define APPLICATION "SYNC-DEMO"

class ChatDialog : public QDialog,  private Ui::ChatDialog 
{
	Q_OBJECT

public:
	ChatDialog(QWidget *parent = 0);
  void appendMessage(const SyncDemo::ChatMessage &msg);
  void processData(const std::vector<Sync::MissingDataInfo> &, Sync::SyncAppSocket *);

private:
  void formChatMessage(const QString &text, SyncDemo::ChatMessage &msg);
  void readSettings();
  void writeSettings();
  void updateLabels();

private slots:
  void returnPressed();
  void buttonPressed();
  void settingUpdated(QString, QString, QString);

private:
  User m_user; 
  Sync::SyncAppSocket *m_sock;
};
#endif
