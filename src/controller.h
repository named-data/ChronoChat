/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_CONTROLLER_H
#define CHRONOS_CONTROLLER_H

#include <QDialog>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QtSql/QSqlDatabase>

#include "setting-dialog.h"
#include "start-chat-dialog.h"
#include "profile-editor.h"
#include "invitation-dialog.h"
#include "contact-panel.h"
#include "browse-contact-dialog.h"
#include "add-contact-panel.h"
#include "chat-dialog.h"

#ifndef Q_MOC_RUN
#include "contact-manager.h"
#include "validator-invitation.h"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#endif

namespace chronos {

class Controller : public QDialog
{
  Q_OBJECT

public: // public methods
  Controller(ndn::shared_ptr<ndn::Face> face,
             QWidget* parent = 0);

  virtual
  ~Controller();

private: // private methods
  std::string
  getDBName();

  void
  openDB();

  void
  initialize();

  void
  setInvitationListener();

  void
  loadConf();

  void
  saveConf();

  void
  createActions();

  void
  createTrayIcon();

  void
  updateMenu();

  void
  onLocalPrefix(const ndn::Interest& interest, ndn::Data& data);

  void
  onLocalPrefixTimeout(const ndn::Interest& interest);

  void
  onInvitationInterestWrapper(const ndn::Name& prefix, const ndn::Interest& interest, size_t routingPrefixOffset);

  void
  onInvitationRegisterFailed(const ndn::Name& prefix, const std::string& failInfo);

  void
  onInvitationValidated(const ndn::shared_ptr<const ndn::Interest>& interest);

  void
  onInvitationValidationFailed(const ndn::shared_ptr<const ndn::Interest>& interest, std::string failureInfo);

  std::string
  getRandomString();

  ndn::Name
  getInvitationRoutingPrefix();

  void
  addChatDialog(const QString& chatroomName, ChatDialog* chatDialog);

signals:
  void
  closeDBModule();

  void
  localPrefixUpdated(const QString& localPrefix);

  void
  identityUpdated(const QString& identity);

  void
  refreshBrowseContact();

  void
  invitationInterest(const ndn::Name& prefix, const ndn::Interest& interest, size_t routingPrefixOffset);

private slots:
  void
  onIdentityUpdated(const QString& identity);

  void
  onIdentityUpdatedContinued();

  void
  onContactIdListReady(const QStringList& list);

  void
  onNickUpdated(const QString& nick);

  void
  onLocalPrefixUpdated(const QString& localPrefix);

  void
  onStartChatAction();

  void
  onSettingsAction();

  void
  onProfileEditorAction();

  void
  onAddContactAction();

  void
  onContactListAction();

  void
  onDirectAdd();

  void
  onUpdateLocalPrefixAction();

  void
  onMinimizeAction();

  void
  onQuitAction();

  void
  onStartChatroom(const QString& chatroom, bool secured);

  void
  onInvitationResponded(const ndn::Name& invitationName, bool accepted);

  void
  onShowChatMessage(const QString& chatroomName, const QString& from, const QString& data);

  void
  onResetIcon();

  void
  onRemoveChatDialog(const QString& chatroom);

  void
  onWarning(const QString& msg);

  void
  onError(const QString& msg);

  void
  onInvitationInterest(const ndn::Name& prefix, const ndn::Interest& interest, size_t routingPrefixOffset);

private: // private member
  typedef std::map<std::string, QAction*> ChatActionList;
  typedef std::map<std::string, ChatDialog*> ChatDialogList;

  // Communication
  ndn::shared_ptr<ndn::Face>     m_face;
  ndn::Name                      m_localPrefix;
  const ndn::RegisteredPrefixId* m_invitationListenerId;

  // Contact Manager
  ContactManager m_contactManager;

  // Tray
  QAction*         m_startChatroom;
  QAction*         m_minimizeAction;
  QAction*         m_settingsAction;
  QAction*         m_editProfileAction;
  QAction*         m_contactListAction;
  QAction*         m_addContactAction;
  QAction*         m_updateLocalPrefixAction;
  QAction*         m_quitAction;
  QMenu*           m_trayIconMenu;
  QMenu*           m_closeMenu;
  QSystemTrayIcon* m_trayIcon;
  ChatActionList   m_chatActionList;
  ChatActionList   m_closeActionList;

  // Dialogs
  SettingDialog*       m_settingDialog;
  StartChatDialog*     m_startChatDialog;
  ProfileEditor*       m_profileEditor;
  InvitationDialog*    m_invitationDialog;
  ContactPanel*        m_contactPanel;
  BrowseContactDialog* m_browseContactDialog;
  AddContactPanel*     m_addContactPanel;
  ChatDialogList       m_chatDialogList;

  // Conf
  ndn::Name   m_identity;
  std::string m_nick;
  QSqlDatabase m_db;

  // Security related;
  ndn::KeyChain                m_keyChain;
  chronos::ValidatorInvitation m_validator;
};

} // namespace chronos

#endif //CHRONOS_CONTROLLER_H
