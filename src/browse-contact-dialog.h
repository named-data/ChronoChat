/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef BROWSE_CONTACT_DIALOG_H
#define BROWSE_CONTACT_DIALOG_H

#include <QDialog>
#include <QStringListModel>
#include <QCloseEvent>
#include <QTableWidgetItem>

#ifndef Q_MOC_RUN
#include <ndn-cxx/security/identity-certificate.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#endif

namespace Ui {
class BrowseContactDialog;
}

class BrowseContactDialog : public QDialog
{
  Q_OBJECT

public:
  explicit
  BrowseContactDialog(QWidget *parent = 0);

  ~BrowseContactDialog();

protected:
  void
  closeEvent(QCloseEvent *e);

private slots:
  void
  onSelectionChanged(const QItemSelection &selected,
                     const QItemSelection &deselected);

  void
  onAddClicked();

  void
  onDirectAddClicked();

public slots:
  void
  onIdCertNameListReady(const QStringList& certNameList);

  void
  onNameListReady(const QStringList& nameList);

  void
  onIdCertReady(const ndn::IdentityCertificate& idCert);

signals:
  void
  directAddClicked();

  void
  fetchIdCert(const QString& certName);

  void
  addContact(const QString& qCertName);

private:

  typedef boost::recursive_mutex RecLock;
  typedef boost::unique_lock<RecLock> UniqueRecLock;

  Ui::BrowseContactDialog *ui;

  QTableWidgetItem* m_typeHeader;
  QTableWidgetItem* m_valueHeader;

  QStringListModel* m_contactListModel;

  QStringList m_contactNameList;
  QStringList m_contactCertNameList;
};

#endif // BROWSE_CONTACT_DIALOG_H
