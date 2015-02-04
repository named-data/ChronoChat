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
#include "invitation.hpp"

#include "digest-tree-scene.hpp"
#include "trust-tree-scene.hpp"
#include "trust-tree-node.hpp"
#include "chat-dialog-backend.hpp"

#include "chatroom-info.hpp"
#endif

namespace Ui {
class ChatDialog;
}

namespace chronochat {

class ChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit
  ChatDialog(const Name& chatroomPrefix,
             const Name& userChatPrefix,
             const Name& routingPrefix,
             const std::string& chatroomName,
             const std::string& nick,
             bool isSecured = false,
             const Name& signingId = Name(),
             QWidget* parent = 0);

  ~ChatDialog();

  void
  closeEvent(QCloseEvent* e);

  void
  changeEvent(QEvent* e);

  void
  resizeEvent(QResizeEvent* e);

  void
  showEvent(QShowEvent* e);

  ChatDialogBackend*
  getBackend()
  {
    return &m_backend;
  }

  void
  addSyncAnchor(const Invitation& invitation)
  {
  }

  ChatroomInfo
  getChatroomInfo();

private:
  void
  disableSyncTreeDisplay();

  void
  appendChatMessage(const QString& nick, const QString& text, time_t timestamp);

  void
  appendControlMessage(const QString& nick, const QString& action, time_t timestamp);

  QString
  formatTime(time_t timestamp);

  void
  printTimeInCell(QTextTable* table, time_t timestamp);

  void
  showMessage(const QString&, const QString&);

  void
  fitView();

signals:
  void
  shutdownBackend();

  void
  msgToSent(QString text, time_t timestamp);

  void
  closeChatDialog(const QString& chatroomName);

  void
  showChatMessage(const QString& chatroomName, const QString& from, const QString& data);

  void
  resetIcon();

public slots:
  void
  onShow();

  void
  shutdown();

private slots:
  void
  updateSyncTree(std::vector<chronochat::NodeInfo> updates, QString rootDigest);

  void
  receiveChatMessage(QString nick, QString text, time_t timestamp);

  void
  removeSession(QString sessionPrefix, QString nick, time_t timestamp);

  void
  receiveMessage(QString sessionPrefix, QString nick, uint64_t seqNo, time_t timestamp,
                 bool addSession);

  void
  updateLabels(ndn::Name newChatPrefix);

  void
  onReturnPressed();

  void
  onSyncTreeButtonPressed();

  void
  onTrustTreeButtonPressed();

  void
  enableSyncTreeDisplay();

private:
  Ui::ChatDialog* ui;

  ChatDialogBackend m_backend;

  std::string m_chatroomName;
  Name m_chatroomPrefix;
  QString m_nick;
  bool m_isSecured;


  DigestTreeScene* m_scene;
  TrustTreeScene* m_trustScene;
  QStringListModel* m_rosterModel;
};

} // namespace chronochat

#endif // CHRONOCHAT_CHAT_DIALOG_HPP
