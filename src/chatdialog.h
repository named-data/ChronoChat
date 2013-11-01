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

#include "invitelistdialog.h"

#ifndef Q_MOC_RUN
#include <ndn.cxx/data.h>
#include <ndn.cxx/security/keychain.h>
#include <ndn.cxx/wrapper/wrapper.h>
#include "invitation-policy-manager.h"
#include "contact-item.h"

#include <ccnx/sync-socket.h>
#include <sync-seq-no.h>
#include "chatbuf.pb.h"
#include "digesttreescene.h"
#endif

#define MAX_HISTORY_ENTRY   20

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ChatDialog(ndn::Ptr<ContactManager> contactManager,
                      const ndn::Name& chatroomPrefix,
                      const ndn::Name& localPrefix,
                      const ndn::Name& defaultIdentity,
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
  sendInvitation(ndn::Ptr<ContactItem> contact, bool isIntroducer);

  void
  addTrustAnchor(const EndorseCertificate& selfEndorseCertificate);

  void
  addChatDataRule(const ndn::Name& prefix, 
                  const ndn::security::IdentityCertificate& identityCertificate,
                  bool isIntroducer);

  void 
  appendMessage(const SyncDemo::ChatMessage msg, bool isHistory = false);

  void 
  processTreeUpdateWrapper(const std::vector<Sync::MissingDataInfo>, Sync::SyncSocket *);

  void 
  processDataWrapper(ndn::Ptr<ndn::Data> data);

  void 
  processDataNoShowWrapper(ndn::Ptr<ndn::Data> data);

  void 
  processRemoveWrapper(std::string);

private:

  void
  initializeSetting();

  void 
  updateLabels();

  void
  setWrapper();

  void
  initializeSync();

  void
  publishIntroCert(ndn::Ptr<ndn::security::IdentityCertificate> dskCertificate, bool isIntroducer);
  
  void 
  onInviteReplyVerified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity, bool isIntroducer);

  void 
  onInviteTimeout(ndn::Ptr<ndn::Closure> closure, 
                  ndn::Ptr<ndn::Interest> interest, 
                  const ndn::Name& identity, 
                  int retry);

  void
  invitationRejected(const ndn::Name& identity);
  
  void 
  invitationAccepted(const ndn::Name& identity,
                     ndn::Ptr<ndn::Data> data, 
                     const std::string& inviteePrefix,
                     bool isIntroducer);

  void
  onUnverified(ndn::Ptr<ndn::Data> data);



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
  summonReaper();

  void
  reap();

  void 
  showMessage(QString, QString);
  
  void
  openInviteListDialog();
  
  void
  sendInvitationWrapper(QString, bool);
    
private:
  Ui::ChatDialog *ui;
  ndn::Ptr<ContactManager> m_contactManager;
  ndn::Name m_chatroomPrefix;
  ndn::Name m_localPrefix;
  ndn::Name m_localChatPrefix;
  ndn::Name m_defaultIdentity;
  ndn::Ptr<InvitationPolicyManager> m_invitationPolicyManager;
  ndn::Ptr<SyncPolicyManager> m_syncPolicyManager; 
  ndn::Ptr<ndn::security::IdentityManager> m_identityManager;
  ndn::Ptr<ndn::security::Keychain> m_keychain;
  ndn::Ptr<ndn::Wrapper> m_handler;

  User m_user; 
  Sync::SyncSocket *m_sock;
  uint32_t m_session;
  DigestTreeScene *m_scene;
  boost::recursive_mutex m_msgMutex;
  boost::recursive_mutex m_sceneMutex;
  time_t m_lastMsgTime;
  int m_randomizedInterval;
  QTimer *m_timer;
  QStringListModel *m_rosterModel;
  

  // QQueue<SyncDemo::ChatMessage> m_history;
  // bool m_historyInitialized;
  bool m_joined;

  QList<QString> m_zombieList;
  int m_zombieIndex;

  InviteListDialog* m_inviteListDialog;
};

#endif // ChatDIALOG_H
