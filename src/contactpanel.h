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

#include "profileeditor.h"
#include "addcontactpanel.h"

#ifndef Q_MOC_RUN
#include "contact-storage.h"
#endif

namespace Ui {
class ContactPanel;
}

class ContactPanel : public QDialog
{
    Q_OBJECT

public:
  explicit ContactPanel(ndn::Ptr<ContactStorage> contactStorage, QWidget *parent = 0);
  ~ContactPanel();

private slots:
  void
  updateSelection(const QItemSelection &selected,
                  const QItemSelection &deselected);

  void
  openProfileEditor();

  void
  openAddContactPanel();

private:
  Ui::ContactPanel *ui;
  ndn::Ptr<ContactStorage> m_contactStorage;
  QStringListModel* m_contactListModel;
  ProfileEditor* m_profileEditor;
  AddContactPanel* m_addContactPanel;
};

#endif // CONTACTPANEL_H
