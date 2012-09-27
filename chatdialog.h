#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>
#include "digesttreescene.h"
#include "ui_chatdialog.h"
#include "chatbuf.pb.h"
#include <sync-app-socket.h>
#include <sync-logic.h>
#include <sync-seq-no.h>
#include <QSystemTrayIcon>

#define ORGANIZATION "IRL@UCLA"
#define APPLICATION "SYNC-DEMO"

class QAction;
class QMenu;

class ChatDialog : public QDialog,  private Ui::ChatDialog 
{
	Q_OBJECT

public:
  ChatDialog(QWidget *parent = 0);
  ~ChatDialog();
  void setVisible(bool visible);
  void appendMessage(const SyncDemo::ChatMessage msg);
  void processTreeUpdateWrapper(const std::vector<Sync::MissingDataInfo>, Sync::SyncAppSocket *);
  void processDataWrapper(std::string, const char *buf, size_t len);
  void processDataNoShowWrapper(std::string, const char *buf, size_t len);
  void processRemoveWrapper(std::string);

protected:
  void closeEvent(QCloseEvent *e);
  void changeEvent(QEvent *e);

public slots:
  void processTreeUpdate(const std::vector<Sync::MissingDataInfo>);
  void processData(QString name, const char *buf, size_t len, bool show);
  void processRemove(QString);

private:
  QString getRandomString();
  void formChatMessage(const QString &text, SyncDemo::ChatMessage &msg);
  void formHelloMessage(SyncDemo::ChatMessage &msg);
  void sendMsg(SyncDemo::ChatMessage &msg);
  bool readSettings();
  void writeSettings();
  void updateLabels();
  void resizeEvent(QResizeEvent *);
  void showEvent(QShowEvent *);
  void fitView();
  void testDraw();
  void createTrayIcon();
  void createActions();

private slots:
  void returnPressed();
  void buttonPressed();
  void checkSetting();
  void settingUpdated(QString, QString, QString);
  void sendHello();

  // icon related
  void iconActivated(QSystemTrayIcon::ActivationReason reason);
  void showMessage(QString, QString);
  void messageClicked();

signals:
  void dataReceived(QString name, const char *buf, size_t len, bool show);
  void treeUpdated(const std::vector<Sync::MissingDataInfo>);
  void removeReceived(QString prefix);

private:
  User m_user; 
  Sync::SyncAppSocket *m_sock;
  uint32_t m_session;
  DigestTreeScene *m_scene;
  boost::mutex m_msgMutex;
  boost::mutex m_sceneMutex;
  time_t m_lastMsgTime;

  // icon related
  QAction *minimizeAction;
  QAction *maximizeAction;
  QAction *restoreAction;
  QAction *quitAction;
  QSystemTrayIcon *trayIcon;
  QMenu *trayIconMenu;
};
#endif
