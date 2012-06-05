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
  ~ChatDialog();
  void appendMessage(const SyncDemo::ChatMessage &msg);
  void processTreeUpdate(const std::vector<Sync::MissingDataInfo> &, Sync::SyncAppSocket *);
  void processData(std::string, const char *buf, size_t len);
  void processRemove(const std::string);

private:
  void formChatMessage(const QString &text, SyncDemo::ChatMessage &msg);
  bool readSettings();
  void writeSettings();
  void updateLabels();
  void resizeEvent(QResizeEvent *);
  void showEvent(QShowEvent *);
  void fitView();
  void testDraw();

private slots:
  void returnPressed();
  void buttonPressed();
  void settingUpdated(QString, QString, QString);

private:
  User m_user; 
  Sync::SyncAppSocket *m_sock;
  uint32_t m_session;
  DigestTreeScene *m_scene;
};
#endif
