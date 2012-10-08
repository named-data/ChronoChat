#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include <boost/function.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <vector>
#include "digesttreescene.h"
#include "ui_chatdialog.h"
#include "chatbuf.pb.h"
#include <sync-app-socket.h>
#include <sync-logic.h>
#include <sync-seq-no.h>
#include <QSystemTrayIcon>
#include <QQueue>

#define ORGANIZATION "IRL@UCLA"
#define APPLICATION "CHRONOS"
#define MAX_HISTORY_ENTRY   20

class QAction;
class QMenu;
class QStringListModel;
class QTextTable;

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
  void respondHistoryRequest(std::string interest);

protected:
  void closeEvent(QCloseEvent *e);
  void changeEvent(QEvent *e);

public slots:
  void processTreeUpdate(const std::vector<Sync::MissingDataInfo>);
  void processData(QString name, const char *buf, size_t len, bool show);
  void processRemove(QString);

private:
  void fetchHistory(std::string name);
  QString getRandomString();
  void formChatMessage(const QString &text, SyncDemo::ChatMessage &msg);
  void formControlMessage(SyncDemo::ChatMessage &msg, SyncDemo::ChatMessage::ChatMessageType type);
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
  QString formatTime(time_t);
  void printTimeInCell(QTextTable *, time_t);
  void disableTreeDisplay();
  QString getLocalPrefix();

private slots:
  void returnPressed();
  void buttonPressed();
  void treeButtonPressed();
  void checkSetting();
  void settingUpdated(QString, QString, QString);
  void sendHello();
  void sendJoin();
  void sendLeave();
  void replot();
  void updateRosterList(QStringList);
  void enableTreeDisplay();

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
  boost::recursive_mutex m_msgMutex;
  boost::recursive_mutex m_sceneMutex;
  time_t m_lastMsgTime;
  int m_randomizedInterval;
  QTimer *m_timer;
  QStringListModel *m_rosterModel;
  bool m_minimaniho;

  QQueue<SyncDemo::ChatMessage> m_history;
  bool m_historyInitialized;
  
  // icon related
  QAction *minimizeAction;
  QAction *maximizeAction;
  QAction *restoreAction;
  QAction *quitAction;
  QSystemTrayIcon *trayIcon;
  QMenu *trayIconMenu;
};
#endif
