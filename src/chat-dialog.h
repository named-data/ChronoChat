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

#ifndef CHAT_DIALOG_H
#define CHAT_DIALOG_H

#include <QDialog>
#include <QTextTable>
#include <QStringListModel>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>

#include "invite-list-dialog.h"

#ifndef Q_MOC_RUN
#include "contact-manager.h"
#include "invitation.h"
#include "contact.h"
#include "chatbuf.pb.h"
#include "intro-cert-list.pb.h"
#include "digest-tree-scene.h"
#include "trust-tree-scene.h"
#include "trust-tree-node.h"
#include <sync-socket.h>
#include <sync-seq-no.h>
#include <ndn-cxx/security/key-chain.hpp>
#include "validator-invitation.h"
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#endif

#define MAX_HISTORY_ENTRY   20

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit
  ChatDialog(chronos::ContactManager* contactManager,
             ndn::shared_ptr<ndn::Face> face,
             const ndn::IdentityCertificate& myCertificate,
             const ndn::Name& chatroomPrefix,
             const ndn::Name& localPrefix,
             const std::string& nick,
             bool witSecurity,
             QWidget* parent = 0);

  ~ChatDialog();

  void
  addSyncAnchor(const chronos::Invitation& invitation);

  void
  processTreeUpdateWrapper(const std::vector<Sync::MissingDataInfo>&, Sync::SyncSocket *);

  void
  processDataWrapper(const ndn::shared_ptr<const ndn::Data>& data);

  void
  processDataNoShowWrapper(const ndn::shared_ptr<const ndn::Data>& data);

  void
  processRemoveWrapper(std::string);

protected:
  void
  closeEvent(QCloseEvent *e);

  void
  changeEvent(QEvent *e);

  void
  resizeEvent(QResizeEvent *);

  void
  showEvent(QShowEvent *);

private:
  void
  updatePrefix();

  void
  updateLabels();

  void
  initializeSync();

  void
  sendInvitation(ndn::shared_ptr<chronos::Contact> contact, bool isIntroducer);

  void
  replyWrapper(const ndn::Interest& interest,
               ndn::Data& data,
               size_t routablePrefixOffset,
               bool isIntroducer);

  void
  replyTimeoutWrapper(const ndn::Interest& interest,
                      size_t routablePrefixOffset);

  void
  onReplyValidated(const ndn::shared_ptr<const ndn::Data>& data,
                   size_t inviteeRoutablePrefixOffset,
                   bool isIntroduce);

  void
  onReplyValidationFailed(const ndn::shared_ptr<const ndn::Data>& data,
                          const std::string& failureInfo);

  void
  invitationRejected(const ndn::Name& identity);

  void
  invitationAccepted(const ndn::IdentityCertificate& inviteeCert,
                     const ndn::Name& inviteePrefix,
                     bool isIntroducer);

  void
  fetchIntroCert(const ndn::Name& identity, const ndn::Name& prefix);

  void
  onIntroCertList(const ndn::Interest& interest, const ndn::Data& data);

  void
  onIntroCertListTimeout(const ndn::Interest& interest, int retry, const std::string& msg);

  void
  introCertWrapper(const ndn::Interest& interest, ndn::Data& data);

  void
  introCertTimeoutWrapper(const ndn::Interest& interest, int retry, const QString& msg);

  void
  onCertListInterest(const ndn::Name& prefix, const ndn::Interest& interest);

  void
  onCertListRegisterFailed(const ndn::Name& prefix, const std::string& msg);

  void
  onCertSingleInterest(const ndn::Name& prefix, const ndn::Interest& interest);

  void
  onCertSingleRegisterFailed(const ndn::Name& prefix, const std::string& msg);


  void
  sendMsg(SyncDemo::ChatMessage &msg);

  void
  disableSyncTreeDisplay();

  void
  appendMessage(const SyncDemo::ChatMessage msg, bool isHistory = false);

  void
  processRemove(QString prefix);

  ndn::Name
  getInviteeRoutablePrefix(const ndn::Name& invitee);

  void
  formChatMessage(const QString &text, SyncDemo::ChatMessage &msg);

  void
  formControlMessage(SyncDemo::ChatMessage &msg, SyncDemo::ChatMessage::ChatMessageType type);

  QString
  formatTime(time_t);

  void
  printTimeInCell(QTextTable *, time_t);

  std::string
  getRandomString();

  void
  showMessage(const QString&, const QString&);

  void
  fitView();

  void
  summonReaper();

  void
  getTree(TrustTreeNodeList& nodeList);

  void
  plotTrustTree();

signals:
  void
  processData(const ndn::shared_ptr<const ndn::Data>& data, bool show, bool isHistory);

  void
  processTreeUpdate(const std::vector<Sync::MissingDataInfo>);

  void
  closeChatDialog(const QString& chatroomName);

  void
  inivationRejection(const QString& msg);

  void
  showChatMessage(const QString& chatroomName, const QString& from, const QString& data);

  void
  resetIcon();

  void
  reply(const ndn::Interest& interest,
        const ndn::shared_ptr<const ndn::Data>& data,
        size_t routablePrefixOffset, bool isIntroducer);

  void
  replyTimeout(const ndn::Interest& interest,
               size_t routablePrefixOffset);

  void
  introCert(const ndn::Interest& interest,
            const ndn::shared_ptr<const ndn::Data>& data);

  void
  introCertTimeout(const ndn::Interest& interest,
                   int retry, const QString& msg);

  void
  waitForContactList();

public slots:
  void
  onLocalPrefixUpdated(const QString& localPrefix);

  void
  onClose();

private slots:
  void
  onReturnPressed();

  void
  onSyncTreeButtonPressed();

  void
  onTrustTreeButtonPressed();

  void
  onProcessData(const ndn::shared_ptr<const ndn::Data>& data,
                bool show, bool isHistory);

  void
  onProcessTreeUpdate(const std::vector<Sync::MissingDataInfo>&);

  void
  onReplot();

  void
  onRosterChanged(QStringList);

  void
  onInviteListDialogRequested();

  void
  sendJoin();

  void
  sendHello();

  void
  sendLeave();

  void
  enableSyncTreeDisplay();

  void
  reap();

  void
  onSendInvitation(QString);

  void
  onReply(const ndn::Interest& interest,
          const ndn::shared_ptr<const ndn::Data>& data,
          size_t routablePrefixOffset, bool isIntroducer);

  void
  onReplyTimeout(const ndn::Interest& interest,
                 size_t routablePrefixOffset);

  void
  onIntroCert(const ndn::Interest& interest,
              const ndn::shared_ptr<const ndn::Data>& data);

  void
  onIntroCertTimeout(const ndn::Interest& interest,
                     int retry, const QString& msg);

private:
  Ui::ChatDialog *ui;
  ndn::KeyChain m_keyChain;

  chronos::ContactManager* m_contactManager;
  ndn::shared_ptr<ndn::Face> m_face;

  ndn::IdentityCertificate m_myCertificate;
  ndn::Name m_identity;

  ndn::Name m_certListPrefix;
  const ndn::RegisteredPrefixId* m_certListPrefixId;
  ndn::Name m_certSinglePrefix;
  const ndn::RegisteredPrefixId* m_certSinglePrefixId;

  std::string m_chatroomName;
  ndn::Name m_chatroomPrefix;
  ndn::Name m_localPrefix;
  bool m_useRoutablePrefix;
  ndn::Name m_chatPrefix;
  ndn::Name m_localChatPrefix;
  std::string m_nick;
  DigestTreeScene *m_scene;
  TrustTreeScene *m_trustScene;
  QStringListModel *m_rosterModel;
  QTimer* m_timer;


  int64_t m_lastMsgTime;
  int m_randomizedInterval;
  bool m_joined;

  Sync::SyncSocket *m_sock;
  uint64_t m_session;
  ndn::shared_ptr<ndn::SecRuleRelative> m_dataRule;

  InviteListDialog* m_inviteListDialog;
  ndn::shared_ptr<chronos::ValidatorInvitation> m_invitationValidator;

  boost::recursive_mutex m_msgMutex;
  boost::recursive_mutex m_sceneMutex;
  QList<QString> m_zombieList;
  int m_zombieIndex;
};

#endif // CHAT_DIALOG_H
