/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>

#ifndef Q_MOC_RUN
#include <ndn.cxx/data.h>
#endif

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ChatDialog(const ndn::Name& chatroomPrefix,
                      const ndn::Name& localPrefix,
                      QWidget *parent = 0);
  ~ChatDialog();

  const ndn::Name&
  getChatroomPrefix() const
  { return m_chatroomPrefix; }

  const ndn::Name&
  getLocalPrefix() const
  { return m_localPrefix; }

  void
  sendInvitation();

private:
  Ui::ChatDialog *ui;
  ndn::Name m_chatroomPrefix;
  ndn::Name m_localPrefix;
};

#endif // ChatDIALOG_H
