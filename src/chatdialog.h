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
#include "contact-item.h"
#include "chatbuf.pb.h"
#include "digesttreescene.h"
#include <sync-socket.h>
#include <sync-seq-no.h>
#include <ndn-cpp-dev/security/key-chain.hpp>
#ifdef WITH_SECURITY
#include "validator-invitation.h"
#include <validator-sync.h>
#else
#include <ndn-cpp-dev/security/validator-null.hpp>
#endif

#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#endif

typedef ndn::function<void()> OnEventualTimeout;

#define MAX_HISTORY_ENTRY   20

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ChatDialog(ndn::shared_ptr<chronos::ContactManager> contactManager,
                      ndn::shared_ptr<ndn::Face> face,
                      const ndn::Name& chatroomPrefix,
                      const ndn::Name& localPrefix,
                      const ndn::Name& defaultIdentity,
                      const std::string& nick,
                      QWidget *parent = 0);

  ~ChatDialog();

  const ndn::Name&
  getChatroomPrefix() const
  { return m_chatroomPrefix; }

  const ndn::Name&
  getLocalPrefix() const
  { return m_localPrefix; }

  void
  sendInvitation(ndn::shared_ptr<chronos::ContactItem> contact, bool isIntroducer);

  void
  addChatDataRule(const ndn::Name& prefix, 
                  const ndn::IdentityCertificate& identityCertificate,
                  bool isIntroducer);

  void
  addTrustAnchor(const chronos::EndorseCertificate& selfEndorseCertificate);

  void 
  appendMessage(const SyncDemo::ChatMessage msg, bool isHistory = false);

  void 
  processTreeUpdateWrapper(const std::vector<Sync::MissingDataInfo>&, Sync::SyncSocket *);

  void 
  processDataWrapper(const ndn::shared_ptr<const ndn::Data>& data);

  void 
  processDataNoShowWrapper(const ndn::shared_ptr<const ndn::Data>& data);

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
  initializeSetting();

  QString 
  getRandomString();

  void 
  updateLabels();

  void
  initializeSync();

  void
  onTargetData(const ndn::Interest& interest, 
               const ndn::Data& data,
               const ndn::OnDataValidated& onValidated,
               const ndn::OnDataValidationFailed& onValidationFailed)
  { m_invitationValidator->validate(data, onValidated, onValidationFailed); }

  void
  onTargetTimeout(const ndn::Interest& interest, 
                  int retry,
                  const ndn::OnDataValidated& onValidated,
                  const ndn::OnDataValidationFailed& onValidationFailed,
                  const OnEventualTimeout& timeoutNotify)
  {
    if(retry > 0)
      sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, retry-1);
    else
      timeoutNotify();
  }

  void
  sendInterest(const ndn::Interest& interest,
               const ndn::OnDataValidated& onValidated,
               const ndn::OnDataValidationFailed& onValidationFailed,
               const OnEventualTimeout& timeoutNotify,
               int retry = 1);
  
  void 
  onInviteReplyValidated(const ndn::shared_ptr<const ndn::Data>& data, 
                         const ndn::Name& identity, 
                         bool isIntroduce);

  void
  onInviteReplyValidationFailed(const ndn::shared_ptr<const ndn::Data>& data,
                            const ndn::Name& identity);

  void 
  onInviteReplyTimeout(const ndn::Name& identity);


  void
  invitationRejected(const ndn::Name& identity);
  
  void 
  invitationAccepted(const ndn::Name& identity, 
                     ndn::shared_ptr<const ndn::Data> data, 
                     const std::string& inviteePrefix, 
                     bool isIntroducer);

  void
  onLocalPrefix(const ndn::Interest& interest, 
                ndn::Data& data);

  void
  onLocalPrefixTimeout(const ndn::Interest& interest);

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
  dataReceived(ndn::shared_ptr<const ndn::Data> data, bool show, bool isHistory);

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
  processTreeUpdate(const std::vector<Sync::MissingDataInfo>&);

  void 
  processData(ndn::shared_ptr<const ndn::Data> data, bool show, bool isHistory);

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
  ndn::shared_ptr<chronos::ContactManager> m_contactManager;
  ndn::shared_ptr<ndn::Face> m_face;
  ndn::shared_ptr<boost::asio::io_service> m_ioService;

  ndn::Name m_chatroomPrefix;
  ndn::Name m_localPrefix;
  ndn::Name m_localChatPrefix;
  ndn::Name m_defaultIdentity;
  User m_user; 
  std::string m_nick;

  ndn::Scheduler m_scheduler;
  ndn::EventId m_replotEventId;

#ifndef WITH_SECURITY
  ndn::shared_ptr<ndn::Validator> m_invitationValidator;
  ndn::shared_ptr<ndn::Validator> m_syncValidator; 
#else
  ndn::shared_ptr<chronos::ValidatorInvitation> m_invitationValidator;
  ndn::shared_ptr<Sync::ValidatorSync> m_syncValidator;
#endif
  ndn::shared_ptr<ndn::KeyChain> m_keyChain;

  ndn::Name m_newLocalPrefix;
  bool m_newLocalPrefixReady;


  Sync::SyncSocket *m_sock;
  uint64_t m_session;
  DigestTreeScene *m_scene;
  boost::recursive_mutex m_msgMutex;
  boost::recursive_mutex m_sceneMutex;
  int64_t m_lastMsgTime;
  int m_randomizedInterval;


  QStringListModel *m_rosterModel;
  QSystemTrayIcon *trayIcon;

  QAction *minimizeAction;
  QAction *maximizeAction;
  QAction *restoreAction;
  QAction *updateLocalPrefixAction;
  QAction *quitAction;
  QMenu *trayIconMenu;

  bool m_joined;

  QList<QString> m_zombieList;
  int m_zombieIndex;

  InviteListDialog* m_inviteListDialog;
};

#endif // ChatDIALOG_H
