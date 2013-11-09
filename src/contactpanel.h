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

#include "profileeditor.h"
#include "addcontactpanel.h"
#include "setaliasdialog.h"
#include "startchatdialog.h"
#include "invitationdialog.h"
#include "settingdialog.h"
#include "chatdialog.h"
#include "endorse-combobox-delegate.h"
#include "browsecontactdialog.h"

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
  explicit ContactPanel(ndn::Ptr<ContactManager> contactManager, 
                        QWidget *parent = 0);

  ~ContactPanel();

private:
  void
  createAction();

  void
  openDB();

  void
  setKeychain();

  void
  setLocalPrefix();

  void
  onLocalPrefixVerified(ndn::Ptr<ndn::Data> data);
  
  void
  onLocalPrefixTimeout(ndn::Ptr<ndn::Closure> closure, 
                       ndn::Ptr<ndn::Interest> interest);

  void
  setInvitationListener();

  void
  onInvitation(ndn::Ptr<ndn::Interest> interest);

  void
  onUnverified(ndn::Ptr<ndn::Data> data);
  
  void
  onTimeout(ndn::Ptr<ndn::Closure> closure, 
            ndn::Ptr<ndn::Interest> interest);
    
  void
  onInvitationCertVerified(ndn::Ptr<ndn::Data> data,
                           ndn::Ptr<ChronosInvitation> invitation);

  void
  popChatInvitation(ndn::Ptr<ChronosInvitation> invitation,
                    const ndn::Name& inviterNameSpace,
                    ndn::Ptr<ndn::security::IdentityCertificate> certificate);

  void
  collectEndorsement();

  void
  onDnsEndorseeVerified(ndn::Ptr<ndn::Data> data, int count);

  void
  onDnsEndorseeTimeout(ndn::Ptr<ndn::Closure> closure, 
                       ndn::Ptr<ndn::Interest> interest, 
                       int count);
  
  void
  onDnsEndorseeUnverified(ndn::Ptr<ndn::Data> data, int count);

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
                 const ndn::security::IdentityCertificate& identityCertificate);

  void
  acceptInvitation(const ChronosInvitation& invitation, 
                   const ndn::security::IdentityCertificate& identityCertificate);

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

  

private:
  Ui::ContactPanel *ui;
  ndn::Ptr<ContactManager> m_contactManager;
  QStringListModel* m_contactListModel;
  ProfileEditor* m_profileEditor;
  AddContactPanel* m_addContactPanel;
  BrowseContactDialog* m_browseContactDialog;
  SetAliasDialog* m_setAliasDialog;
  StartChatDialog* m_startChatDialog;
  InvitationDialog* m_invitationDialog;
  SettingDialog* m_settingDialog;
  std::map<ndn::Name, ChatDialog*> m_chatDialogs;
  QAction* m_menuInvite;
  QAction* m_menuAlias;
  std::vector<ndn::Ptr<ContactItem> > m_contactList;
  ndn::Ptr<std::vector<bool> > m_collectStatus;

  ndn::Ptr<PanelPolicyManager> m_panelPolicyManager;
  ndn::Ptr<ndn::security::Keychain> m_keychain;
  ndn::Ptr<ndn::Wrapper> m_handler;

  ndn::Name m_defaultIdentity;
  std::string m_nickName;
  ndn::Name m_localPrefix;
  ndn::Name m_inviteListenPrefix;

  ndn::Ptr<ContactItem> m_currentSelectedContact;
  QSqlTableModel* m_trustScopeModel;
  QSqlTableModel* m_endorseDataModel;
  EndorseComboBoxDelegate* m_endorseComboBoxDelegate;
};

#endif // CONTACTPANEL_H
