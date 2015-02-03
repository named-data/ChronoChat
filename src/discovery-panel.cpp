/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "discovery-panel.hpp"
#include "ui_discovery-panel.h"

#include <QItemSelectionModel>
#include <QModelIndex>
#include <QMessageBox>

#ifndef Q_MOC_RUN
#endif


namespace chronochat {

static const time::seconds REFRESH_INTERVAL(60);
static const ndn::Name::Component ROUTING_HINT_SEPARATOR =
  ndn::name::Component::fromEscapedString("%F0%2E");

DiscoveryPanel::DiscoveryPanel(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::DiscoveryPanel)
  , m_chatroomListModel(new QStringListModel)
  , m_rosterListModel(new QStringListModel)
{
  ui->setupUi(this);
  ui->ChatroomList->setModel(m_chatroomListModel);
  ui->RosterList->setModel(m_rosterListModel);

  connect(ui->ChatroomList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this,
          SLOT(onSelectedChatroomChanged(const QItemSelection &, const QItemSelection &)));
  connect(ui->RosterList->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this,
          SLOT(onSelectedParticipantChanged(const QItemSelection &, const QItemSelection &)));
  connect(ui->join, SIGNAL(clicked()),
          this, SLOT(onJoinClicked()));
  connect(ui->requestInvitation, SIGNAL(clicked()),
          this, SLOT(onRequestInvitation()));

  ui->join->setEnabled(false);
  ui->requestInvitation->setEnabled(false);
  ui->InChatroomWarning->clear();
  m_isParticipant = false;
}

DiscoveryPanel::~DiscoveryPanel()
{
  if (m_chatroomListModel)
    delete m_chatroomListModel;
  if (m_rosterListModel)
    delete m_rosterListModel;
  delete ui;
}

//private methods
void
DiscoveryPanel::resetPanel()
{
  // Clean up General tag.
  ui->NameData->clear();
  ui->NameSpaceData->clear();
  ui->TrustModelData->clear();

  // Clean up Roster tag.
  m_rosterList.clear();
  m_participant.clear();
  m_rosterListModel->setStringList(m_rosterList);

  // Clean up chatroom list.
  m_chatroomList.clear();
  m_chatroom.clear();
  m_chatroomListModel->setStringList(m_chatroomList);

  ui->join->setEnabled(false);
  ui->requestInvitation->setEnabled(false);
  ui->InChatroomWarning->clear();
}

// public slots
void
DiscoveryPanel::onIdentityUpdated(const QString& identity)
{
  resetPanel();
}

void
DiscoveryPanel::onChatroomListReady(const QStringList& list)
{
  m_chatroomList = list;
  m_chatroomListModel->setStringList(m_chatroomList);
}

void
DiscoveryPanel::onChatroomInfoReady(const ChatroomInfo& info, bool isParticipant)
{
  ui->NameData->setText(QString::fromStdString(info.getName().toUri()));
  ui->NameSpaceData->setText(QString::fromStdString(info.getSyncPrefix().toUri()));

  switch(info.getTrustModel()) {
  case 2:
    {
      ui->TrustModelData->setText(QString("Hierarchical"));
      ui->join->setEnabled(false);
      ui->requestInvitation->setEnabled(false);
      break;
    }
  case 1:
    {
      ui->TrustModelData->setText(QString("Web Of Trust"));
      ui->join->setEnabled(false);
      ui->requestInvitation->setEnabled(false);
      break;
    }
  case 0:
    {
      ui->TrustModelData->setText(QString("None"));
      ui->join->setEnabled(true);
      ui->requestInvitation->setEnabled(false);
      break;
    }
  default:
    {
      ui->TrustModelData->setText(QString("Unrecognized"));
      ui->join->setEnabled(false);
      ui->requestInvitation->setEnabled(false);
    }
  }
  ui->InChatroomWarning->clear();
  m_isParticipant = isParticipant;
  if (m_isParticipant) {
    ui->join->setEnabled(false);
    ui->requestInvitation->setEnabled(false);
    ui->InChatroomWarning->setText(QString("You are already in this chatroom"));
  }

  std::list<Name>roster = info.getParticipants();
  m_rosterList.clear();
  Name::Component routingHint = Name::Component(ROUTING_HINT_SEPARATOR);
  for (const auto& participant : roster) {
    size_t i;
    for (i = 0; i < participant.size(); ++i) {
      if (routingHint == participant.at(i))
        break;
    }
    if (i == participant.size())
      m_rosterList << QString::fromStdString(participant.toUri());
    else
      m_rosterList << QString::fromStdString(participant.getSubName(i + 1).toUri());
  }
  m_rosterListModel->setStringList(m_rosterList);
  ui->RosterList->setModel(m_rosterListModel);
}

// private slots
void
DiscoveryPanel::onSelectedChatroomChanged(const QItemSelection &selected,
                                          const QItemSelection &deselected)
{
  QModelIndexList items = selected.indexes();
  QString chatroomName = m_chatroomListModel->data(items.first(), Qt::DisplayRole).toString();

  bool chatroomFound = false;
  for (int i = 0; i < m_chatroomList.size(); i++) {
    if (chatroomName == m_chatroomList[i]) {
      chatroomFound = true;
      m_chatroom = m_chatroomList[i];
      m_participant.clear();
      break;
    }
  }

  if (!chatroomFound) {
    emit warning("This should not happen: DiscoveryPanel::onSelectedChatroomChanged");
    return;
  }

  emit waitForChatroomInfo(m_chatroom);
}

void
DiscoveryPanel::onSelectedParticipantChanged(const QItemSelection &selected,
                                             const QItemSelection &deselected)
{
  if (m_isParticipant)
    return;

  QModelIndexList items = selected.indexes();
  QString participant = m_rosterListModel->data(items.first(), Qt::DisplayRole).toString();

  bool participantFound = false;
  for (int i = 0; i < m_rosterList.size(); i++) {
    if (participant == m_rosterList[i]) {
      participantFound = true;
      m_participant = m_rosterList[i];
      break;
    }
  }
  ui->requestInvitation->setEnabled(true);
  BOOST_ASSERT(participantFound);
}

void
DiscoveryPanel::onJoinClicked()
{
  emit startChatroom(m_chatroom, false);
}

void
DiscoveryPanel::onRequestInvitation()
{
  emit sendInvitationRequest(m_chatroom, m_participant);
}

void
DiscoveryPanel::onInvitationRequestResult(const std::string& message)
{
  QMessageBox::information(this, tr("Chatroom Discovery"),
                           tr(message.c_str()));
}

} // namespace chronochat

#if WAF
#include "discovery-panel.moc"
// #include "discovery-panel.cpp.moc"
#endif
