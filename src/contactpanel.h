/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CONTACTPANEL_H
#define CONTACTPANEL_H

#include <QDialog>
#include <QStringListModel>
#include <QtSql/QSqlDatabase>
#include <QMenu>
#include <QMessageBox>

#include "profileeditor.h"
#include "addcontactpanel.h"
#include "setaliasdialog.h"
#include "startchatdialog.h"
#include "invitationdialog.h"
#include "settingdialog.h"
#include "chatdialog.h"
#include "endorse-combobox-delegate.h"
#include "browsecontactdialog.h"
#include "warningdialog.h"

#ifndef Q_MOC_RUN
#include "contact-manager.h"
#include "invitation.h"

#ifdef WITH_SECURITY
#include "validator-panel.h"
#include "validator-invitation.h"
#else
#include <ndn-cpp-dev/security/validator-null.hpp>
#endif

#endif


namespace Ui {
class ContactPanel;
}

class ContactPanel : public QDialog
{
  Q_OBJECT

public:
  explicit ContactPanel(ndn::shared_ptr<ndn::Face> face,
                        QWidget *parent = 0);

  ~ContactPanel();

private:  
  void
  createAction();

  void
  openDB();

  void
  loadTrustAnchor();

  void
  setLocalPrefix(int retry = 10);

  void
  onLocalPrefix(const ndn::Interest& interest, ndn::Data& data);
  
  void
  onLocalPrefixTimeout(const ndn::Interest& interest, int retry);

  void
  setInvitationListener();

  void
  onInvitation(const ndn::Name& prefix, const ndn::Interest& interest);

  void
  onInvitationRegisterFailed(const ndn::Name& prefix, const std::string& msg);

  inline void
  onInvitationValidated(const ndn::shared_ptr<const ndn::Interest>& interest);
  
  inline void
  onInvitationValidationFailed(const ndn::shared_ptr<const ndn::Interest>& interest);

  void
  popChatInvitation(const ndn::Name& interestName);

  void
  sendInterest(const ndn::Interest& interest,
               const ndn::OnDataValidated& onValidated,
               const ndn::OnDataValidationFailed& onValidationFailed,
               const chronos::TimeoutNotify& timeoutNotify,
               int retry = 1);

  void
  onTargetData(const ndn::Interest& interest, 
               ndn::Data& data,
               const ndn::OnDataValidated& onValidated,
               const ndn::OnDataValidationFailed& onValidationFailed);

  void
  onTargetTimeout(const ndn::Interest& interest, 
                  int retry,
                  const ndn::OnDataValidated& onValidated,
                  const ndn::OnDataValidationFailed& onValidationFailed,
                  const chronos::TimeoutNotify& timeoutNotify);

  void
  collectEndorsement();

  void
  onDnsEndorseeValidated(const ndn::shared_ptr<const ndn::Data>& data, int count);

  void
  onDnsEndorseeValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, int count);

  void
  onDnsEndorseeTimeoutNotify(int count);

  void 
  updateCollectStatus(int count);

  std::string
  getRandomString();

signals:
  void
  refreshCertDirectory();

private slots:
  void
  showError(const QString& msg);

  void
  showWarning(const QString& msg);

  void
  updateSelection(const QItemSelection &selected,
                  const QItemSelection &deselected);

  void
  updateDefaultIdentity(const QString& identity,
                        const QString& nickName);

  void
  openProfileEditor();

  void
  openAddContactPanel();
  
  void
  openBrowseContactDialog();

  void
  openSetAliasDialog();

  void
  removeContactButton();
  
  void
  openStartChatDialog();

  void
  openSettingDialog();

  void
  refreshContactList();

  void
  showContextMenu(const QPoint& pos);

  void
  startChatroom(const QString& chatroom);

  void 
  startChatroom2(const ndn::Name& invitationInterest);

  inline void
  acceptInvitation(const ndn::Name& invitationInterest);

  inline void
  rejectInvitation(const ndn::Name& invitationInterest);

  void
  prepareInvitationReply(const ndn::Name& invitationInterest, const std::string& content);

  void
  isIntroducerChanged(int state);

  void
  addScopeClicked();

  void
  deleteScopeClicked();

  void
  saveScopeClicked();

  void
  endorseButtonClicked();
  
  void
  removeChatDialog(const ndn::Name& chatroomName);

  void 
  addContactIntoValidator(const ndn::Name& nameSpace);

  void 
  removeContactFromValidator(const ndn::Name& keyName);
  

private:

  Ui::ContactPanel *ui;
  WarningDialog* m_warningDialog;
  QStringListModel* m_contactListModel;
  ProfileEditor* m_profileEditor;
  AddContactPanel* m_addContactPanel;
  BrowseContactDialog* m_browseContactDialog;
  SetAliasDialog* m_setAliasDialog;
  StartChatDialog* m_startChatDialog;
  InvitationDialog* m_invitationDialog;
  SettingDialog* m_settingDialog;
  std::map<ndn::Name, ChatDialog*> m_chatDialogs;
  QAction* m_menuAlias;

#ifdef WITH_SECURITY
  ndn::shared_ptr<chronos::ValidatorPanel> m_panelValidator;
  ndn::shared_ptr<chronos::ValidatorInvitation> m_invitationValidator;
#else
  ndn::shared_ptr<ndn::Validator> m_panelValidator;
  ndn::shared_ptr<ndn::Validator> m_invitationValidator;
#endif

  ndn::shared_ptr<ndn::KeyChain> m_keyChain;
  ndn::shared_ptr<ndn::Face> m_face;
  ndn::shared_ptr<boost::asio::io_service> m_ioService;

  ndn::shared_ptr<chronos::ContactManager> m_contactManager;

  std::vector<ndn::shared_ptr<chronos::ContactItem> > m_contactList;
  ndn::shared_ptr<std::vector<bool> > m_collectStatus;

  const ndn::RegisteredPrefixId* m_invitationListenerId;

  ndn::Name m_defaultIdentity;
  std::string m_nickName;
  ndn::Name m_localPrefix;
  ndn::Name m_inviteListenPrefix;

  ndn::shared_ptr<chronos::ContactItem> m_currentSelectedContact;
  QSqlTableModel* m_trustScopeModel;
  QSqlTableModel* m_endorseDataModel;
  EndorseComboBoxDelegate* m_endorseComboBoxDelegate;
};

void
ContactPanel::onInvitationValidated(const ndn::shared_ptr<const ndn::Interest>& interest)
{ popChatInvitation(interest->getName()); }

void
ContactPanel::onInvitationValidationFailed(const ndn::shared_ptr<const ndn::Interest>& interest)
{}

void
ContactPanel::acceptInvitation(const ndn::Name& invitationInterest)
{ 
  prepareInvitationReply(invitationInterest, m_localPrefix.toUri()); 
  startChatroom2(invitationInterest);
}

void
ContactPanel::rejectInvitation(const ndn::Name& invitationInterest)
{ prepareInvitationReply(invitationInterest, "nack"); }


#endif // CONTACTPANEL_H
