/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include <QTextTable>
#include <QStringListModel>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>

#include "invitelistdialog.h"

#ifndef Q_MOC_RUN
#include <ndn-cpp/data.hpp>
#include <ndn-cpp/face.hpp>
#include <ndn-cpp/security/key-chain.hpp>
#include "sec-policy-chrono-chat-invitation.h"
#include "contact-item.h"

#include <sync-socket.h>
#include <sync-seq-no.h>
#include "chatbuf.pb.h"
#include "digesttreescene.h"

#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#endif

typedef ndn::func_lib::function<void()> OnEventualTimeout;

#define MAX_HISTORY_ENTRY   20

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ChatDialog(ndn::ptr_lib::shared_ptr<ContactManager> contactManager,
                      const ndn::Name& chatroomPrefix,
                      const ndn::Name& localPrefix,
                      const ndn::Name& defaultIdentity,
                      const std::string& nick,
                      bool trial = false,
                      QWidget *parent = 0);

  // explicit ChatDialog(const ndn::Name& chatroomPrefix,
  //                     const ndn::Name& localPrefix,
  //                     const ndn::Name& defaultIdentity,
  //                     const ndn::security::IdentityCertificate& identityCertificate,
  //                     QWidget *parent = 0);

  ~ChatDialog();

  const ndn::Name&
  getChatroomPrefix() const
  { return m_chatroomPrefix; }

  const ndn::Name&
  getLocalPrefix() const
  { return m_localPrefix; }

  void
  sendInvitation(ndn::ptr_lib::shared_ptr<ContactItem> contact, bool isIntroducer);

  void
  addTrustAnchor(const EndorseCertificate& selfEndorseCertificate);

  void
  addChatDataRule(const ndn::Name& prefix, 
                  const ndn::IdentityCertificate& identityCertificate,
                  bool isIntroducer);

  void 
  appendMessage(const SyncDemo::ChatMessage msg, bool isHistory = false);

  void 
  processTreeUpdateWrapper(const std::vector<Sync::MissingDataInfo>, Sync::SyncSocket *);

  void 
  processDataWrapper(const ndn::ptr_lib::shared_ptr<ndn::Data>& data);

  void 
  processDataNoShowWrapper(const ndn::ptr_lib::shared_ptr<ndn::Data>& data);

  void 
  processRemoveWrapper(std::string);

  void
  publishIntroCert(const ndn::IdentityCertificate& dskCertificate, bool isIntroducer);

protected:
  void 
  closeEvent(QCloseEvent *e);

  void
  changeEvent(QEvent *e);

private:

  void 
  startFace();

  void
  shutdownFace();

  void
  eventLoop();

  void
  connectToDaemon();

  void
  onConnectionData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
                   const ndn::ptr_lib::shared_ptr<ndn::Data>& data);
 
  void
  onConnectionDataTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest);

  void
  initializeSetting();

  QString 
  getRandomString();

  void 
  updateLabels();

  void
  initializeSync();

  void
  onTargetData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
               const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
               int stepCount,
               const ndn::OnVerified& onVerified,
               const ndn::OnVerifyFailed& onVerifyFailed,
               const OnEventualTimeout& timeoutNotify,
               const ndn::ptr_lib::shared_ptr<ndn::SecPolicy>& policy);

  void
  onTargetTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
                  int retry,
                  int stepCount,
                  const ndn::OnVerified& onVerified,
                  const ndn::OnVerifyFailed& onVerifyFailed,
                  const OnEventualTimeout& timeoutNotify,
                  const ndn::ptr_lib::shared_ptr<ndn::SecPolicy>& policy);
  
  void
  onCertData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
             const ndn::ptr_lib::shared_ptr<ndn::Data>& cert,
             ndn::ptr_lib::shared_ptr<ndn::ValidationRequest> previousStep,
             const ndn::ptr_lib::shared_ptr<ndn::SecPolicy>& policy);

  void
  onCertTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
                const ndn::OnVerifyFailed& onVerifyFailed,
                const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
                ndn::ptr_lib::shared_ptr<ndn::ValidationRequest> nextStep,
                const ndn::ptr_lib::shared_ptr<ndn::SecPolicy>& policy);

  void
  sendInterest(const ndn::Interest& interest,
               const ndn::OnVerified& onVerified,
               const ndn::OnVerifyFailed& onVerifyFailed,
               const OnEventualTimeout& timeoutNotify,
               const ndn::ptr_lib::shared_ptr<ndn::SecPolicy>& policy,
               int retry = 1,
               int stepCount = 0);
  
  void 
  onInviteReplyVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, 
                        const ndn::Name& identity, 
                        bool isIntroduce);

  void
  onInviteReplyVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
                            const ndn::Name& identity);

  void 
  onInviteReplyTimeout(const ndn::Name& identity);


  void
  invitationRejected(const ndn::Name& identity);
  
  void 
  invitationAccepted(const ndn::Name& identity, 
                     ndn::ptr_lib::shared_ptr<ndn::Data> data, 
                     const std::string& inviteePrefix, 
                     bool isIntroducer);

  void
  onLocalPrefix(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
                const ndn::ptr_lib::shared_ptr<ndn::Data>& data);

  void
  onLocalPrefixTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest);

  // void 
  // fetchHistory(std::string name);

  void 
  formChatMessage(const QString &text, SyncDemo::ChatMessage &msg);

  void 
  formControlMessage(SyncDemo::ChatMessage &msg, SyncDemo::ChatMessage::ChatMessageType type);

  void 
  sendMsg(SyncDemo::ChatMessage &msg);

  void 
  resizeEvent(QResizeEvent *);
  
  void 
  showEvent(QShowEvent *);

  void 
  fitView();

  void 
  createActions();

  void
  createTrayIcon();

  QString 
  formatTime(time_t);

  void 
  printTimeInCell(QTextTable *, time_t);

  void 
  disableTreeDisplay();

signals:
  void 
  dataReceived(QString name, const char *buf, size_t len, bool show, bool isHistory);
  
  void 
  treeUpdated(const std::vector<Sync::MissingDataInfo>);
  
  void 
  removeReceived(QString prefix);

  void
  closeChatDialog(const ndn::Name& chatroomPrefix);

  void
  noNdnConnection(const QString& msg);
                                     
  void
  inivationRejection(const QString& msg);

public slots:
  void 
  processTreeUpdate(const std::vector<Sync::MissingDataInfo>);

  void 
  processData(QString name, const char *buf, size_t len, bool show, bool isHistory);

  void 
  processRemove(QString prefix);

private slots:
  void
  returnPressed();

  void 
  treeButtonPressed();

  void
  settingUpdated(QString, QString, QString);

  void 
  sendJoin();

  void
  sendHello();

  void
  sendLeave();

  void 
  replot();

  void 
  updateRosterList(QStringList);

  void 
  enableTreeDisplay();

  void
  updateLocalPrefix();

  void 
  summonReaper();

  void
  reap();

  void 
  iconActivated(QSystemTrayIcon::ActivationReason reason);

  void
  messageClicked();

  void 
  showMessage(QString, QString);
  
  void
  openInviteListDialog();
  
  void
  sendInvitationWrapper(QString, bool);

  void
  quit();
    
private:
  Ui::ChatDialog *ui;
  ndn::ptr_lib::shared_ptr<ContactManager> m_contactManager;
  ndn::Name m_chatroomPrefix;
  ndn::Name m_localPrefix;
  ndn::Name m_localChatPrefix;
  ndn::Name m_defaultIdentity;
  ndn::ptr_lib::shared_ptr<SecPolicyChronoChatInvitation> m_invitationPolicy;
  ndn::ptr_lib::shared_ptr<SecPolicySync> m_syncPolicy; 
  ndn::ptr_lib::shared_ptr<ndn::KeyChain> m_keyChain;
  ndn::ptr_lib::shared_ptr<ndn::Face> m_face;

  boost::recursive_mutex m_mutex;
  boost::thread m_thread;
  bool m_running;

  ndn::Name m_newLocalPrefix;
  bool m_newLocalPrefixReady;

  User m_user; 
  std::string m_nick;
  Sync::SyncSocket *m_sock;
  uint32_t m_session;
  DigestTreeScene *m_scene;
  boost::recursive_mutex m_msgMutex;
  boost::recursive_mutex m_sceneMutex;
  time_t m_lastMsgTime;
  int m_randomizedInterval;
  QTimer *m_timer;
  QStringListModel *m_rosterModel;
  QSystemTrayIcon *trayIcon;

  QAction *minimizeAction;
  QAction *maximizeAction;
  QAction *restoreAction;
  QAction *updateLocalPrefixAction;
  QAction *quitAction;
  QMenu *trayIconMenu;

  // QQueue<SyncDemo::ChatMessage> m_history;
  // bool m_historyInitialized;
  bool m_joined;

  QList<QString> m_zombieList;
  int m_zombieIndex;

  InviteListDialog* m_inviteListDialog;
};

#endif // ChatDIALOG_H
