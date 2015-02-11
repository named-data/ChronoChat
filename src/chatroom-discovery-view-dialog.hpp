/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Mengjin Yan <jane.yan0129@gmail.com>
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CHATROOM_DISCOVERY_VIEW_DIALOG_HPP
#define CHRONOCHAT_CHATROOM_DISCOVERY_VIEW_DIALOG_HPP

#include <QDialog>

#ifndef Q_MOC_RUN
#include "common.hpp"
#endif

namespace Ui {
class ChatroomDiscoveryViewDialog;
}

namespace chronochat {

class ChatroomDiscoveryViewDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ChatroomDiscoveryViewDialog(QWidget* parent = 0);
  ~ChatroomDiscoveryViewDialog();

  void
  setChatroomName(QString chatroomName);

  void
  setChatroomTrustModel(QString chatroomTrustModel);

  void
  setChatroomParticipants(const std::list<ndn::Name>& chatroomParticipants);


private slots:
  void
  onCloseButtonClicked();

private:
  Ui::ChatroomDiscoveryViewDialog* ui;
};

} //namespace chronochat

#endif // CHRONOCHAT_CHATROOM_DISCOVERY_VIEW_DIALOG_HPP
