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

#ifndef Q_MOC_RUN
#include "contact-manager.h"
#endif


namespace Ui {
class ContactPanel;
}

class ContactPanel : public QDialog
{
  Q_OBJECT

public:
  explicit ContactPanel(ndn::Ptr<ContactManager> contactManager, QWidget *parent = 0);

  ~ContactPanel();

private:
  void
  openDB();

  void
  setKeychain();

  void
  setLocalPrefix();

  void
  onLocalPrefixVerified(ndn::Ptr<ndn::Data> data);
  
  void
  onLocalPrefixTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest);

  void
  onUnverified(ndn::Ptr<ndn::Data> data);
  
  void
  onTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest);
    
  void
  setInvitationListener();

  void
  onInvitation(ndn::Ptr<ndn::Interest> interest);

  void
  onInvitationCertVerified(ndn::Ptr<ndn::Data> data,
                           const ndn::Name& interestName,
                           int inviterIndex);

  std::string
  getRandomString();

  void
  popChatInvitation(const ndn::Name& interestName,
                    int inviterIndex,
                    const ndn::Name& inviterNameSpace,
                    ndn::Ptr<ndn::security::IdentityCertificate> certificate);

signals:
  void
  newInvitationReady();

private slots:
  void
  updateSelection(const QItemSelection &selected,
                  const QItemSelection &deselected);

  void
  updateDefaultIdentity(const QString& identity);

  void
  openProfileEditor();

  void
  openAddContactPanel();

  void
  openSetAliasDialog();
  
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
  startChatroom(const QString& chatroom, const QString& invitee, bool isIntroducer);

  void 
  startChatroom2(const QString& chatroom, const QString& inviter);

  void
  acceptInvitation(const ndn::Name& interestName, 
                   const ndn::security::IdentityCertificate& identityCertificate, 
                   QString inviter, 
                   QString chatroom);

  void
  rejectInvitation(const ndn::Name& interestName);

private:
  Ui::ContactPanel *ui;
  ndn::Ptr<ContactManager> m_contactManager;
  QStringListModel* m_contactListModel;
  ProfileEditor* m_profileEditor;
  AddContactPanel* m_addContactPanel;
  SetAliasDialog* m_setAliasDialog;
  StartChatDialog* m_startChatDialog;
  InvitationDialog* m_invitationDialog;
  SettingDialog* m_settingDialog;
  std::map<ndn::Name, ChatDialog*> m_chatDialogs;
  QAction* m_menuInvite;
  QAction* m_menuAlias;
  std::vector<ndn::Ptr<ContactItem> > m_contactList;

  ndn::Ptr<ndn::security::Keychain> m_keychain;
  ndn::Ptr<ndn::Wrapper> m_handler;

  ndn::Name m_defaultIdentity;
  ndn::Name m_localPrefix;
  ndn::Name m_inviteListenPrefix;

  std::string m_currentSelectedContactAlias;
  std::string m_currentSelectedContactNamespace;


};

#endif // CONTACTPANEL_H
