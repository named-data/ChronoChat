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
#include "chronos-invitation.h"
#include "panel-policy-manager.h"
#endif


namespace Ui {
class ContactPanel;
}

class ContactPanel : public QDialog
{
  Q_OBJECT

public:
  explicit ContactPanel(QWidget *parent = 0);

  ~ContactPanel();

private:
  void 
  connectToDaemon();

  void
  onConnectionData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
                   const ndn::ptr_lib::shared_ptr<ndn::Data>& data);
 
  void
  onConnectionDataTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest);

  void
  createAction();

  void
  openDB();

  void
  loadTrustAnchor();

  void
  setLocalPrefix(int retry = 10);

  void
  onLocalPrefix(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
                const ndn::ptr_lib::shared_ptr<ndn::Data>& data);
  
  void
  onLocalPrefixTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
                       int retry);

  void
  setInvitationListener();

  void
  onInvitation(const ndn::ptr_lib::shared_ptr<const ndn::Name>& prefix, 
               const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
               ndn::Transport& transport, 
               uint64_t registeredPrefixI);

  void
  onInvitationRegisterFailed(const ndn::ptr_lib::shared_ptr<const ndn::Name>& prefix);

  void
  sendInterest(const ndn::Interest& interest,
               const ndn::OnVerified& onVerified,
               const ndn::OnVerifyFailed& onVerifyFailed,
               const TimeoutNotify& timeoutNotify,
               int retry = 1,
               int stepCount = 0);

  void
  onTargetData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
               const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
               int stepCount,
               const ndn::OnVerified& onVerified,
               const ndn::OnVerifyFailed& onVerifyFailed,
               const TimeoutNotify& timeoutNotify);

  void
  onTargetTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
                  int retry,
                  int stepCount,
                  const ndn::OnVerified& onVerified,
                  const ndn::OnVerifyFailed& onVerifyFailed,
                  const TimeoutNotify& timeoutNotify);


  void
  onCertData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
             const ndn::ptr_lib::shared_ptr<ndn::Data>& cert,
             ndn::ptr_lib::shared_ptr<ndn::ValidationRequest> previousStep);

  void
  onCertTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
                const ndn::OnVerifyFailed& onVerifyFailed,
                const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
                ndn::ptr_lib::shared_ptr<ndn::ValidationRequest> nextStep);
    
  void
  onInvitationCertVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, 
                           ndn::ptr_lib::shared_ptr<ChronosInvitation> invitation);

  void
  onInvitationCertVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& data);

  void
  onInvitationCertTimeoutNotify();

  void
  popChatInvitation(ndn::ptr_lib::shared_ptr<ChronosInvitation> invitation,
                    const ndn::Name& inviterNameSpace,
                    ndn::ptr_lib::shared_ptr<ndn::IdentityCertificate> certificate);

  void
  collectEndorsement();

  void
  onDnsEndorseeVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, int count);

  void
  onDnsEndorseeVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, int count);

  void
  onDnsEndorseeTimeoutNotify(int count);

  void 
  updateCollectStatus(int count);

  std::string
  getRandomString();

signals:
  void
  newInvitationReady();

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
  openInvitationDialog();

  void
  refreshContactList();

  void
  showContextMenu(const QPoint& pos);

  void
  startChatroom(const QString& chatroom, 
                const QString& invitee, 
                bool isIntroducer);

  void 
  startChatroom2(const ChronosInvitation& invitation, 
                 const ndn::IdentityCertificate& identityCertificate);

  void
  acceptInvitation(const ChronosInvitation& invitation, 
                   const ndn::IdentityCertificate& identityCertificate);

  void
  rejectInvitation(const ChronosInvitation& invitation);

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
  addContactIntoPanelPolicy(const ndn::Name& nameSpace);

  void 
  removeContactFromPanelPolicy(const ndn::Name& keyName);
  

private:
  Ui::ContactPanel *ui;
  WarningDialog* m_warningDialog;
  ndn::ptr_lib::shared_ptr<ContactManager> m_contactManager;
  QStringListModel* m_contactListModel;
  ProfileEditor* m_profileEditor;
  AddContactPanel* m_addContactPanel;
  BrowseContactDialog* m_browseContactDialog;
  SetAliasDialog* m_setAliasDialog;
  StartChatDialog* m_startChatDialog;
  InvitationDialog* m_invitationDialog;
  SettingDialog* m_settingDialog;
  std::map<ndn::Name, ChatDialog*, ndn::Name::BreadthFirstLess> m_chatDialogs;
  QAction* m_menuInvite;
  QAction* m_menuAlias;
  std::vector<ndn::ptr_lib::shared_ptr<ContactItem> > m_contactList;
  ndn::ptr_lib::shared_ptr<std::vector<bool> > m_collectStatus;

  ndn::ptr_lib::shared_ptr<PanelPolicyManager> m_policyManager;
  ndn::ptr_lib::shared_ptr<ndn::IdentityManager> m_identityManager;
  ndn::ptr_lib::shared_ptr<ndn::Transport> m_transport;
  ndn::ptr_lib::shared_ptr<ndn::Face> m_face;
  uint64_t m_invitationListenerId;

  ndn::Name m_defaultIdentity;
  std::string m_nickName;
  ndn::Name m_localPrefix;
  ndn::Name m_inviteListenPrefix;

  ndn::ptr_lib::shared_ptr<ContactItem> m_currentSelectedContact;
  QSqlTableModel* m_trustScopeModel;
  QSqlTableModel* m_endorseDataModel;
  EndorseComboBoxDelegate* m_endorseComboBoxDelegate;
};

#endif // CONTACTPANEL_H
