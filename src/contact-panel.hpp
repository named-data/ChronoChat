/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CONTACT_PANEL_HPP
#define CHRONOCHAT_CONTACT_PANEL_HPP

#include <QDialog>
#include <QStringListModel>
#include <QSqlTableModel>

#include "set-alias-dialog.hpp"
#include "endorse-combobox-delegate.hpp"

#ifndef Q_MOC_RUN
#endif

namespace Ui {
class ContactPanel;
}

namespace chronochat {

class ContactPanel : public QDialog
{
  Q_OBJECT

public:
  explicit
  ContactPanel(QWidget* parent = 0);

  virtual
  ~ContactPanel();

private:
  void
  resetPanel();

signals:
  void
  waitForContactList();

  void
  waitForContactInfo(const QString& identity);

  void
  removeContact(const QString& identity);

  void
  updateAlias(const QString& identity, const QString& alias);

  void
  updateIsIntroducer(const QString& identity, bool isIntro);

  void
  updateEndorseCertificate(const QString& identity);

  void
  warning(const QString& msg);

public slots:
  void
  onCloseDBModule();

  void
  onIdentityUpdated(const QString& identity);

  void
  onContactAliasListReady(const QStringList& aliasList);

  void
  onContactIdListReady(const QStringList& idList);

  void
  onContactInfoReady(const QString& identity,
                     const QString& name,
                     const QString& institute,
                     bool isIntro);

private slots:
  void
  onSelectionChanged(const QItemSelection& selected,
                     const QItemSelection& deselected);

  void
  onContextMenuRequested(const QPoint& pos);

  void
  onSetAliasDialogRequested();

  void
  onContactDeletionRequested();

  void
  onIsIntroducerChanged(int state);

  void
  onAddScopeClicked();

  void
  onDeleteScopeClicked();

  void
  onSaveScopeClicked();

  void
  onEndorseButtonClicked();

  void
  onAliasChanged(const QString& identity, const QString& alias);

private:
  Ui::ContactPanel* ui;

  // Dialogs.
  SetAliasDialog* m_setAliasDialog;

  // Models.
  QStringListModel* m_contactListModel;
  QSqlTableModel*   m_trustScopeModel;
  QSqlTableModel*   m_endorseDataModel;

  // Delegates.
  EndorseComboBoxDelegate* m_endorseComboBoxDelegate;

  // Actions.
  QAction* m_menuAlias;
  QAction* m_menuDelete;

  // Internal data structure.
  QStringList m_contactAliasList;
  QStringList m_contactIdList;
  QString     m_currentSelectedContact;
};

} // namespace chronochat

#endif // CHRONOCHAT_CONTACT_PANEL_HPP
