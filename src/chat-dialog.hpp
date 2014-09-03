/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CHAT_DIALOG_HPP
#define CHRONOCHAT_CHAT_DIALOG_HPP

#include <QDialog>
#include <QTextTable>
#include <QStringListModel>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>

#ifndef Q_MOC_RUN
#include "common.hpp"
#include "contact-manager.hpp"
#include "invitation.hpp"
#include "contact.hpp"
#include "chatbuf.pb.h"
#include "intro-cert-list.pb.h"
#include "digest-tree-scene.hpp"
#include "trust-tree-scene.hpp"
#include "trust-tree-node.hpp"
#include "validator-invitation.hpp"
#include <sync-socket.h>
#include <sync-seq-no.h>
#include <ndn-cxx/security/key-chain.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include "chatroom-info.hpp"
#endif

#include "invite-list-dialog.hpp"


#define MAX_HISTORY_ENTRY   20

namespace Ui {
class ChatDialog;
}

namespace chronos {

class ChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit
  ChatDialog(ContactManager* contactManager,
             shared_ptr<ndn::Face> face,
             const ndn::IdentityCertificate& myCertificate,
             const Name& chatroomPrefix,
             const Name& localPrefix,
             const std::string& nick,
             bool witSecurity,
             QWidget* parent = 0);

  ~ChatDialog();

  void
  addSyncAnchor(const Invitation& invitation);

  void
  processTreeUpdateWrapper(const std::vector<Sync::MissingDataInfo>&, Sync::SyncSocket *);

  void
  processDataWrapper(const shared_ptr<const Data>& data);

  void
  processDataNoShowWrapper(const shared_ptr<const Data>& data);

  void
  processRemoveWrapper(const std::string& prefix);

  //ymj
  shared_ptr<ChatroomInfo>
  getChatroomInfo() const;

  void
  closeEvent(QCloseEvent* e);

  void
  changeEvent(QEvent* e);

  void
  resizeEvent(QResizeEvent* e);

  void
  showEvent(QShowEvent* e);

private:
  void
  updatePrefix();

  void
  updateLabels();

  void
  initializeSync();

  void
  sendInvitation(shared_ptr<Contact> contact, bool isIntroducer);

  void
  replyWrapper(const Interest& interest, Data& data,
               size_t routablePrefixOffset, bool isIntroducer);

  void
  replyTimeoutWrapper(const Interest& interest,
                      size_t routablePrefixOffset);

  void
  onReplyValidated(const ndn::shared_ptr<const ndn::Data>& data,
                   size_t inviteeRoutablePrefixOffset,
                   bool isIntroduce);

  void
  onReplyValidationFailed(const ndn::shared_ptr<const ndn::Data>& data,
                          const std::string& failureInfo);

  void
  invitationRejected(const Name& identity);

  void
  invitationAccepted(const ndn::IdentityCertificate& inviteeCert,
                     const Name& inviteePrefix, bool isIntroducer);

  void
  fetchIntroCert(const Name& identity, const Name& prefix);

  void
  onIntroCertList(const Interest& interest, const Data& data);

  void
  onIntroCertListTimeout(const Interest& interest, int retry, const std::string& msg);

  void
  introCertWrapper(const Interest& interest, Data& data);

  void
  introCertTimeoutWrapper(const Interest& interest, int retry, const QString& msg);

  void
  onCertListInterest(const ndn::Name& prefix, const ndn::Interest& interest);

  void
  onCertListRegisterFailed(const ndn::Name& prefix, const std::string& msg);

  void
  onCertSingleInterest(const Name& prefix, const Interest& interest);

  void
  onCertSingleRegisterFailed(const Name& prefix, const std::string& msg);


  void
  sendMsg(SyncDemo::ChatMessage& msg);

  void
  disableSyncTreeDisplay();

  void
  appendMessage(const SyncDemo::ChatMessage msg, bool isHistory = false);

  void
  processRemove(QString prefix);

  ndn::Name
  getInviteeRoutablePrefix(const Name& invitee);

  void
  formChatMessage(const QString& text, SyncDemo::ChatMessage& msg);

  void
  formControlMessage(SyncDemo::ChatMessage &msg, SyncDemo::ChatMessage::ChatMessageType type);

  QString
  formatTime(time_t timestamp);

  void
  printTimeInCell(QTextTable* table, time_t timestamp);

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
  processData(const ndn::shared_ptr<const ndn::Data>& data,
              bool show, bool isHistory);

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
  reply(const ndn::Interest& interest, const ndn::shared_ptr<const ndn::Data>& data,
        size_t routablePrefixOffset, bool isIntroducer);

  void
  replyTimeout(const ndn::Interest& interest, size_t routablePrefixOffset);

  void
  introCert(const ndn::Interest& interest, const ndn::shared_ptr<const ndn::Data>& data);

  void
  introCertTimeout(const ndn::Interest& interest, int retry, const QString& msg);

  void
  waitForContactList();

  void
  rosterChanged(const chronos::ChatroomInfo& info);

public slots:
  void
  onShow();

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
  onProcessData(const ndn::shared_ptr<const ndn::Data>& data, bool show, bool isHistory);

  void
  onProcessTreeUpdate(const std::vector<Sync::MissingDataInfo>&);

  void
  onReplot();

  void
  onRosterChanged(QStringList staleUserList);

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
  onSendInvitation(QString invitee);

  void
  onReply(const ndn::Interest& interest, const ndn::shared_ptr<const ndn::Data>& data,
          size_t routablePrefixOffset, bool isIntroducer);

  void
  onReplyTimeout(const ndn::Interest& interest, size_t routablePrefixOffset);

  void
  onIntroCert(const ndn::Interest& interest, const ndn::shared_ptr<const ndn::Data>& data);

  void
  onIntroCertTimeout(const ndn::Interest& interest, int retry, const QString& msg);

private:
  Ui::ChatDialog *ui;
  ndn::KeyChain m_keyChain;

  ContactManager* m_contactManager;
  shared_ptr<ndn::Face> m_face;

  ndn::IdentityCertificate m_myCertificate;
  Name m_identity;

  Name m_certListPrefix;
  const ndn::RegisteredPrefixId* m_certListPrefixId;
  Name m_certSinglePrefix;
  const ndn::RegisteredPrefixId* m_certSinglePrefixId;

  std::string m_chatroomName;
  Name m_chatroomPrefix;
  Name m_localPrefix;
  bool m_useRoutablePrefix;
  Name m_chatPrefix;
  Name m_localChatPrefix;
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
  shared_ptr<ndn::SecRuleRelative> m_dataRule;

  InviteListDialog* m_inviteListDialog;
  shared_ptr<ValidatorInvitation> m_invitationValidator;

  boost::recursive_mutex m_msgMutex;
  boost::recursive_mutex m_sceneMutex;
  QList<QString> m_zombieList;
  int m_zombieIndex;

  //ymj
  ChatroomInfo::TrustModel m_trustModel;
};

} // namespace chronos

#endif // CHRONOCHAT_CHAT_DIALOG_HPP
