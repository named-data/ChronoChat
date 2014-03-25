/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chat-dialog.h"
#include "ui_chat-dialog.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QCloseEvent>

#ifndef Q_MOC_RUN
#include <sync-intro-certificate.h>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <ndn-cpp-dev/util/random.hpp>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#include <queue>
#include "logging.h"
#endif

using namespace std;
using namespace ndn;
using namespace chronos;

INIT_LOGGER("ChatDialog");

static const int HELLO_INTERVAL = FRESHNESS * 3 / 4;
static const uint8_t CHRONOS_RP_SEPARATOR[2] = {0xF0, 0x2E}; // %F0.

Q_DECLARE_METATYPE(std::vector<Sync::MissingDataInfo> )
Q_DECLARE_METATYPE(ndn::shared_ptr<const ndn::Data>)
Q_DECLARE_METATYPE(ndn::Interest)
Q_DECLARE_METATYPE(size_t)

ChatDialog::ChatDialog(ContactManager* contactManager,
                       shared_ptr<Face> face,
                       const IdentityCertificate& myCertificate,
                       const Name& chatroomPrefix,
		       const Name& localPrefix,
                       const string& nick,
                       bool withSecurity,
		       QWidget* parent) 
  : QDialog(parent)
  , ui(new Ui::ChatDialog)
  , m_contactManager(contactManager)
  , m_face(face)
  , m_myCertificate(myCertificate)
  , m_chatroomName(chatroomPrefix.get(-1).toEscapedString())
  , m_chatroomPrefix(chatroomPrefix)
  , m_localPrefix(localPrefix)
  , m_useRoutablePrefix(false)
  , m_nick(nick)
  , m_lastMsgTime(time::toUnixTimestamp(time::system_clock::now()).count())
  , m_joined(false)
  , m_sock(NULL)
  , m_session(static_cast<uint64_t>(time::toUnixTimestamp(time::system_clock::now()).count()))
  , m_inviteListDialog(new InviteListDialog)
{
  qRegisterMetaType<std::vector<Sync::MissingDataInfo> >("std::vector<Sync::MissingDataInfo>");
  qRegisterMetaType<ndn::shared_ptr<const ndn::Data> >("ndn.DataPtr");
  qRegisterMetaType<ndn::Interest>("ndn.Interest");
  qRegisterMetaType<size_t>("size_t");

  m_scene = new DigestTreeScene(this);
  m_trustScene = new TrustTreeScene(this);
  m_rosterModel = new QStringListModel(this);
  m_timer = new QTimer(this);

  ui->setupUi(this);
  ui->syncTreeViewer->setScene(m_scene);
  m_scene->setSceneRect(m_scene->itemsBoundingRect());
  ui->syncTreeViewer->hide();
  ui->trustTreeViewer->setScene(m_trustScene);
  m_trustScene->setSceneRect(m_trustScene->itemsBoundingRect());
  ui->trustTreeViewer->hide();
  ui->listView->setModel(m_rosterModel);

  m_identity = IdentityCertificate::certificateNameToPublicKeyName(m_myCertificate.getName()).getPrefix(-1);
  updatePrefix();
  updateLabels();

  m_scene->setCurrentPrefix(QString(m_localChatPrefix.toUri().c_str()));
  m_scene->plot("Empty");

  connect(ui->lineEdit, SIGNAL(returnPressed()), 
          this, SLOT(onReturnPressed()));
  connect(ui->syncTreeButton, SIGNAL(pressed()), 
          this, SLOT(onSyncTreeButtonPressed()));
  connect(ui->trustTreeButton, SIGNAL(pressed()), 
          this, SLOT(onTrustTreeButtonPressed()));
  connect(m_scene, SIGNAL(replot()),
          this, SLOT(onReplot()));
  connect(m_scene, SIGNAL(rosterChanged(QStringList)), 
          this, SLOT(onRosterChanged(QStringList)));
  connect(m_timer, SIGNAL(timeout()), 
          this, SLOT(onReplot()));

  connect(this, SIGNAL(processData(const ndn::shared_ptr<const ndn::Data>&, bool, bool)), 
          this, SLOT(onProcessData(const ndn::shared_ptr<const ndn::Data>&, bool, bool)));
  connect(this, SIGNAL(processTreeUpdate(const std::vector<Sync::MissingDataInfo>)), 
          this, SLOT(onProcessTreeUpdate(const std::vector<Sync::MissingDataInfo>)));
  connect(this, SIGNAL(reply(const ndn::Interest&, const ndn::shared_ptr<const ndn::Data>&, size_t, bool)),
          this, SLOT(onReply(const ndn::Interest&, const ndn::shared_ptr<const ndn::Data>&, size_t, bool)));
  connect(this, SIGNAL(replyTimeout(const ndn::Interest&, size_t)),
          this, SLOT(onReplyTimeout(const ndn::Interest&, size_t)));
  connect(this, SIGNAL(introCert(const ndn::Interest&, const ndn::shared_ptr<const ndn::Data>&)),
          this, SLOT(onIntroCert(const ndn::Interest&, const ndn::shared_ptr<const ndn::Data>&)));
  connect(this, SIGNAL(introCertTimeout(const ndn::Interest&, int, const QString&)),
          this, SLOT(onIntroCertTimeout(const ndn::Interest&, int, const QString&)));

  if(withSecurity)
    {

      m_invitationValidator = make_shared<chronos::ValidatorInvitation>();
      m_dataRule = make_shared<SecRuleRelative>("([^<CHRONOCHAT-DATA>]*)<CHRONOCHAT-DATA><>",
                                                "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT>$",
                                                "==", "\\1", "\\1", true);

      ui->inviteButton->setEnabled(true);
      ui->trustTreeButton->setEnabled(true);

      connect(ui->inviteButton, SIGNAL(clicked()),
              this, SLOT(onInviteListDialogRequested()));
      connect(m_inviteListDialog, SIGNAL(sendInvitation(const QString&)),
              this, SLOT(onSendInvitation(const QString&)));
      connect(this, SIGNAL(waitForContactList()),
              m_contactManager, SLOT(onWaitForContactList()));
      connect(m_contactManager, SIGNAL(contactAliasListReady(const QStringList&)),
              m_inviteListDialog, SLOT(onContactAliasListReady(const QStringList&)));
      connect(m_contactManager, SIGNAL(contactIdListReady(const QStringList&)),
              m_inviteListDialog, SLOT(onContactIdListReady(const QStringList&)));
    }

  initializeSync();
}


ChatDialog::~ChatDialog()
{
  if(m_certListPrefixId)
    m_face->unsetInterestFilter(m_certListPrefixId);

  if(m_certSinglePrefixId)
    m_face->unsetInterestFilter(m_certSinglePrefixId);

  if(m_sock != NULL)
    {
      sendLeave();
      delete m_sock;
      m_sock = NULL;
    }
}

// public methods:
void
ChatDialog::addSyncAnchor(const Invitation& invitation)
{
  _LOG_DEBUG("Add sync anchor from invation");
  // Add inviter certificate as trust anchor.
  m_sock->addParticipant(invitation.getInviterCertificate());
  plotTrustTree();

  // Ask inviter for IntroCertificate
  Name inviterNameSpace = IdentityCertificate::certificateNameToPublicKeyName(invitation.getInviterCertificate().getName()).getPrefix(-1);
  fetchIntroCert(inviterNameSpace, invitation.getInviterRoutingPrefix());
}

void
ChatDialog::processTreeUpdateWrapper(const vector<Sync::MissingDataInfo>& v, Sync::SyncSocket *sock)
{
  emit processTreeUpdate(v);
  _LOG_DEBUG("<<< Tree update signal emitted");
}

void
ChatDialog::processDataWrapper(const shared_ptr<const Data>& data)
{
  emit processData(data, true, false);
  _LOG_DEBUG("<<< " << data->getName() << " fetched");
}

void
ChatDialog::processDataNoShowWrapper(const shared_ptr<const Data>& data)
{
  emit processData(data, false, false);
}

void
ChatDialog::processRemoveWrapper(string prefix)
{
  _LOG_DEBUG("Sync REMOVE signal received for prefix: " << prefix);
}

// protected methods:
void
ChatDialog::closeEvent(QCloseEvent *e)
{
  QMessageBox::information(this, tr("ChronoChat"),
                           tr("The chatroom will keep running in the "
                              "system tray. To close the chatroom, "
                              "choose <b>Close chatroom</b> in the "
                              "context memu of the system tray entry."));
  hide();
  e->ignore();
}

void
ChatDialog::changeEvent(QEvent *e)
{
  switch(e->type())
  {
  case QEvent::ActivationChange:
    if (isActiveWindow())
    {
      emit resetIcon();
    }
    break;
  default:
    break;
  }
}

void
ChatDialog::resizeEvent(QResizeEvent *e)
{
  fitView();
}

void
ChatDialog::showEvent(QShowEvent *e)
{
  fitView();
}

// private methods:
void
ChatDialog::updatePrefix()
{
  m_certListPrefix.clear();
  m_certSinglePrefix.clear();
  m_localChatPrefix.clear();
  m_chatPrefix.clear();
  m_chatPrefix.append(m_identity).append("CHRONOCHAT-DATA").append(m_chatroomName).append(getRandomString());
  if(!m_localPrefix.isPrefixOf(m_identity))
    {
      m_useRoutablePrefix = true;
      m_certListPrefix.append(m_localPrefix).append(CHRONOS_RP_SEPARATOR, 2);
      m_certSinglePrefix.append(m_localPrefix).append(CHRONOS_RP_SEPARATOR, 2);
      m_localChatPrefix.append(m_localPrefix).append(CHRONOS_RP_SEPARATOR, 2);
    }
  m_certListPrefix.append(m_identity).append("CHRONOCHAT-CERT-LIST").append(m_chatroomName);
  m_certSinglePrefix.append(m_identity).append("CHRONOCHAT-CERT-SINGLE").append(m_chatroomName);
  m_localChatPrefix.append(m_chatPrefix);

  if(m_certListPrefixId)
    m_face->unsetInterestFilter(m_certListPrefixId);
  m_certListPrefixId = m_face->setInterestFilter (m_certListPrefix, 
                                                  bind(&ChatDialog::onCertListInterest, this, _1, _2), 
                                                  bind(&ChatDialog::onCertListRegisterFailed, this, _1, _2));

  if(m_certSinglePrefixId)
    m_face->unsetInterestFilter(m_certSinglePrefixId);
  m_certSinglePrefixId = m_face->setInterestFilter (m_certSinglePrefix, 
                                                    bind(&ChatDialog::onCertSingleInterest, this, _1, _2), 
                                                    bind(&ChatDialog::onCertSingleRegisterFailed, this, _1, _2));
}

void
ChatDialog::updateLabels()
{
  QString settingDisp = QString("Chatroom: %1").arg(QString::fromStdString(m_chatroomName));
  ui->infoLabel->setStyleSheet("QLabel {color: #630; font-size: 16px; font: bold \"Verdana\";}");
  ui->infoLabel->setText(settingDisp);
  QString prefixDisp;
  Name privatePrefix("/private/local");
  if(privatePrefix.isPrefixOf(m_localChatPrefix))
    {
      prefixDisp = 
        QString("<Warning: no connection to hub or hub does not support prefix autoconfig.>\n <Prefix = %1>")
        .arg(QString::fromStdString(m_localChatPrefix.toUri()));
      ui->prefixLabel->setStyleSheet("QLabel {color: red; font-size: 12px; font: bold \"Verdana\";}");
    }
  else
    {
      prefixDisp = QString("<Prefix = %1>")
        .arg(QString::fromStdString(m_localChatPrefix.toUri()));
      ui->prefixLabel->setStyleSheet("QLabel {color: Green; font-size: 12px; font: bold \"Verdana\";}");
    }
  ui->prefixLabel->setText(prefixDisp);
}

void
ChatDialog::initializeSync()
{
  
  m_sock = new Sync::SyncSocket(m_chatroomPrefix,
                                m_chatPrefix,
                                m_session,
                                m_useRoutablePrefix,
                                m_localPrefix,
                                m_face,
                                m_myCertificate,
                                m_dataRule,
                                bind(&ChatDialog::processTreeUpdateWrapper, this, _1, _2),
                                bind(&ChatDialog::processRemoveWrapper, this, _1));
  
  usleep(100000);

  QTimer::singleShot(600, this, SLOT(sendJoin()));
  m_timer->start(FRESHNESS * 1000);
  disableSyncTreeDisplay();
  QTimer::singleShot(2200, this, SLOT(enableSyncTreeDisplay()));
}

void
ChatDialog::sendInvitation(shared_ptr<Contact> contact, bool isIntroducer)
{
  // Add invitee as a trust anchor.
  m_invitationValidator->addTrustAnchor(contact->getPublicKeyName(),
                                        contact->getPublicKey());

  // Prepared an invitation interest without routable prefix.
  Invitation invitation(contact->getNameSpace(),
                        m_chatroomName,
                        m_localPrefix,
                        m_myCertificate);
  Interest tmpInterest(invitation.getUnsignedInterestName());
  m_keyChain.sign(tmpInterest, m_myCertificate.getName());

  // Get invitee's routable prefix (ideally it will do some DNS lookup, but we assume everyone use /ndn/broadcast
  Name routablePrefix = getInviteeRoutablePrefix(contact->getNameSpace());

  // Check if we need to prepend the routable prefix to the interest name.
  bool requireRoutablePrefix = false;
  Name interestName;
  size_t routablePrefixOffset = 0;
  if(!routablePrefix.isPrefixOf(tmpInterest.getName()))
    {
      interestName.append(routablePrefix).append(CHRONOS_RP_SEPARATOR, 2);
      requireRoutablePrefix = true;
      routablePrefixOffset = routablePrefix.size() + 1;
    }
  interestName.append(tmpInterest.getName());

  // Send the invitation out
  Interest interest(interestName);
  interest.setMustBeFresh(true);
  _LOG_DEBUG("sendInvitation: " << interest.getName());
  m_face->expressInterest(interest,
                          bind(&ChatDialog::replyWrapper, this, _1, _2, routablePrefixOffset, isIntroducer),
                          bind(&ChatDialog::replyTimeoutWrapper, this, _1, routablePrefixOffset));
}

void
ChatDialog::replyWrapper(const Interest& interest, 
                         Data& data,
                         size_t routablePrefixOffset,
                         bool isIntroducer)
{
  _LOG_DEBUG("ChatDialog::replyWrapper");
  emit reply(interest, data.shared_from_this(), routablePrefixOffset, isIntroducer);
  _LOG_DEBUG("OK?");
}

void
ChatDialog::replyTimeoutWrapper(const Interest& interest, 
                                size_t routablePrefixOffset)
{
  _LOG_DEBUG("ChatDialog::replyTimeoutWrapper");
  emit replyTimeout(interest, routablePrefixOffset);
}

void 
ChatDialog::onReplyValidated(const shared_ptr<const Data>& data,
                             size_t inviteeRoutablePrefixOffset,
                             bool isIntroducer)
{
  if(data->getName().size() <= inviteeRoutablePrefixOffset)
    {
      Invitation invitation(data->getName());
      invitationRejected(invitation.getInviteeNameSpace());
    }
  else
    {
      Name inviteePrefix;
      inviteePrefix.wireDecode(data->getName().get(inviteeRoutablePrefixOffset).blockFromValue());
      IdentityCertificate inviteeCert;
      inviteeCert.wireDecode(data->getContent().blockFromValue());
      invitationAccepted(inviteeCert, inviteePrefix, isIntroducer);
    }
}

void
ChatDialog::onReplyValidationFailed(const shared_ptr<const Data>& data,
                                    const string& failureInfo)
{
  _LOG_DEBUG("Invitation reply cannot be validated: " + failureInfo + " ==> " + data->getName().toUri());
}

void
ChatDialog::invitationRejected(const Name& identity)
{
  QString msg = QString::fromStdString(identity.toUri()) + " rejected your invitation!";
  emit inivationRejection(msg);
}

void
ChatDialog::invitationAccepted(const IdentityCertificate& inviteeCert, 
                               const Name& inviteePrefix, 
                               bool isIntroducer)
{
  // Add invitee certificate as trust anchor.
  m_sock->addParticipant(inviteeCert);
  plotTrustTree();

  // Ask invitee for IntroCertificate.
  Name inviteeNameSpace = IdentityCertificate::certificateNameToPublicKeyName(inviteeCert.getName()).getPrefix(-1);
  fetchIntroCert(inviteeNameSpace, inviteePrefix);
}

void
ChatDialog::fetchIntroCert(const Name& identity, const Name& prefix)
{
  Name interestName;

  if(!prefix.isPrefixOf(identity))
    interestName.append(prefix).append(CHRONOS_RP_SEPARATOR, 2);
  
  interestName.append(identity)
    .append("CHRONOCHAT-CERT-LIST")
    .append(m_chatroomName)
    .appendVersion();

  Interest interest(interestName);
  interest.setMustBeFresh(true);

  m_face->expressInterest(interest,
                          bind(&ChatDialog::onIntroCertList, this, _1, _2),
                          bind(&ChatDialog::onIntroCertListTimeout, this, _1, 1, 
                               "IntroCertList: " + interestName.toUri()));
}

void
ChatDialog::onIntroCertList(const Interest& interest, const Data& data)
{
  Chronos::IntroCertListMsg introCertList;
  if(!introCertList.ParseFromArray(data.getContent().value(), data.getContent().value_size()))
    return;

  for(int i = 0; i < introCertList.certname_size(); i++)
    {
      Name certName(introCertList.certname(i));
      Interest interest(certName);
      interest.setMustBeFresh(true);

      _LOG_DEBUG("onIntroCertList: to fetch " << certName);

      m_face->expressInterest(interest,
                              bind(&ChatDialog::introCertWrapper, this, _1, _2),
                              bind(&ChatDialog::introCertTimeoutWrapper, this, _1, 0, 
                                   QString("IntroCert: %1").arg(introCertList.certname(i).c_str())));
    }  
}

void
ChatDialog::onIntroCertListTimeout(const Interest& interest, int retry, const string& msg)
{
  if(retry > 0)
    m_face->expressInterest(interest,
                            bind(&ChatDialog::onIntroCertList, this, _1, _2),
                            bind(&ChatDialog::onIntroCertListTimeout, this, _1, retry - 1, msg));
  else
    _LOG_DEBUG(msg << " TIMEOUT!");
}

void
ChatDialog::introCertWrapper(const Interest& interest, Data& data)
{
  emit introCert(interest, data.shared_from_this());
}

void
ChatDialog::introCertTimeoutWrapper(const Interest& interest, int retry, const QString& msg)
{
  emit introCertTimeout(interest, retry, msg);
}

void
ChatDialog::onCertListInterest(const Name& prefix, const ndn::Interest& interest)
{
  vector<Name> certNameList;
  m_sock->getIntroCertNames(certNameList);

  Chronos::IntroCertListMsg msg;

  vector<Name>::const_iterator it  = certNameList.begin();
  vector<Name>::const_iterator end = certNameList.end();
  for(; it != end; it++)
    {
      Name certName;
      certName.append(m_certSinglePrefix).append(*it);
      msg.add_certname(certName.toUri());
    }
  OBufferStream os;
  msg.SerializeToOstream(&os);

  Data data(interest.getName());
  data.setContent(os.buf());
  m_keyChain.sign(data, m_myCertificate.getName());

  m_face->put(data);
}

void
ChatDialog::onCertListRegisterFailed(const Name& prefix, const string& msg)
{
  _LOG_DEBUG("ChatDialog::onCertListRegisterFailed failed: " + msg);
}

void
ChatDialog::onCertSingleInterest(const Name& prefix, const ndn::Interest& interest)
{
  try
    {
      Name certName = interest.getName().getSubName(prefix.size());
      const Sync::IntroCertificate& introCert = m_sock->getIntroCertificate(certName);
      Data data(interest.getName());
      data.setContent(introCert.wireEncode());
      m_keyChain.sign(data,  m_myCertificate.getName());
      m_face->put(data);
    }
  catch(Sync::SyncSocket::Error& e)
    {
      return;
    }
}

void
ChatDialog::onCertSingleRegisterFailed(const Name& prefix, const string& msg)
{
  _LOG_DEBUG("ChatDialog::onCertListRegisterFailed failed: " + msg);
}

void
ChatDialog::sendMsg(SyncDemo::ChatMessage &msg)
{
  // send msg
  OBufferStream os;
  msg.SerializeToOstream(&os);
  
  if (!msg.IsInitialized())
  {
    _LOG_DEBUG("Errrrr.. msg was not probally initialized "<<__FILE__ <<":"<<__LINE__<<". what is happening?");
    abort();
  }
  uint64_t nextSequence = m_sock->getNextSeq();
  m_sock->publishData(os.buf()->buf(), os.buf()->size(), FRESHNESS);

  m_lastMsgTime = time::toUnixTimestamp(time::system_clock::now()).count();

  Sync::MissingDataInfo mdi = {m_localChatPrefix.toUri(), Sync::SeqNo(0), Sync::SeqNo(nextSequence)};
  std::vector<Sync::MissingDataInfo> v;
  v.push_back(mdi);
  {
    boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
    m_scene->processUpdate(v, m_sock->getRootDigest().c_str());
    m_scene->msgReceived(QString::fromStdString(m_localChatPrefix.toUri()),
                         QString::fromStdString(m_nick));
  }
}

void ChatDialog::disableSyncTreeDisplay()
{
  ui->syncTreeButton->setEnabled(false);
  ui->syncTreeViewer->hide();
  fitView();
}

void
ChatDialog::appendMessage(const SyncDemo::ChatMessage msg, bool isHistory)
{
  boost::recursive_mutex::scoped_lock lock(m_msgMutex);

  if (msg.type() == SyncDemo::ChatMessage::CHAT)
  {

    if (!msg.has_data())
    {
      return;
    }

    if (msg.from().empty() || msg.data().empty())
    {
      return;
    }

    if (!msg.has_timestamp())
    {
      return;
    }

    // if (m_history.size() == MAX_HISTORY_ENTRY)
    // {
    //   m_history.dequeue();
    // }

    // m_history.enqueue(msg);

    QTextCharFormat nickFormat;
    nickFormat.setForeground(Qt::darkGreen);
    nickFormat.setFontWeight(QFont::Bold);
    nickFormat.setFontUnderline(true);
    nickFormat.setUnderlineColor(Qt::gray);

    QTextCursor cursor(ui->textEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    QTextTable *table = cursor.insertTable(1, 2, tableFormat);
    QString from = QString("%1 ").arg(msg.from().c_str());
    QTextTableCell fromCell = table->cellAt(0, 0);
    fromCell.setFormat(nickFormat);
    fromCell.firstCursorPosition().insertText(from);

    time_t timestamp = msg.timestamp();
    printTimeInCell(table, timestamp);

    QTextCursor nextCursor(ui->textEdit->textCursor());
    nextCursor.movePosition(QTextCursor::End);
    table = nextCursor.insertTable(1, 1, tableFormat);
    table->cellAt(0, 0).firstCursorPosition().insertText(QString::fromUtf8(msg.data().c_str()));
    if (!isHistory)
    {
      showMessage(from, QString::fromUtf8(msg.data().c_str()));
    }
  }

  if (msg.type() == SyncDemo::ChatMessage::JOIN || msg.type() == SyncDemo::ChatMessage::LEAVE)
  {
    QTextCharFormat nickFormat;
    nickFormat.setForeground(Qt::gray);
    nickFormat.setFontWeight(QFont::Bold);
    nickFormat.setFontUnderline(true);
    nickFormat.setUnderlineColor(Qt::gray);

    QTextCursor cursor(ui->textEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    QTextTable *table = cursor.insertTable(1, 2, tableFormat);
    QString action;
    if (msg.type() == SyncDemo::ChatMessage::JOIN)
    {
      action = "enters room";
    }
    else
    {
      action = "leaves room";
    }

    QString from = QString("%1 %2  ").arg(msg.from().c_str()).arg(action);
    QTextTableCell fromCell = table->cellAt(0, 0);
    fromCell.setFormat(nickFormat);
    fromCell.firstCursorPosition().insertText(from);

    time_t timestamp = msg.timestamp();
    printTimeInCell(table, timestamp);
  }

  QScrollBar *bar = ui->textEdit->verticalScrollBar();
  bar->setValue(bar->maximum());
}

void
ChatDialog::processRemove(QString prefix)
{
  _LOG_DEBUG("<<< remove node for prefix" << prefix.toStdString());

  bool removed = m_scene->removeNode(prefix);
  if (removed)
  {
    boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
    m_scene->plot(m_sock->getRootDigest().c_str());
  }
}

Name
ChatDialog::getInviteeRoutablePrefix(const Name& invitee)
{
  return ndn::Name("/ndn/broadcast");
}

void
ChatDialog::formChatMessage(const QString &text, SyncDemo::ChatMessage &msg) {
  msg.set_from(m_nick);
  msg.set_to(m_chatroomName);
  msg.set_data(text.toStdString());
  int32_t seconds = static_cast<int32_t>(time::toUnixTimestamp(time::system_clock::now()).count()/1000000000);
  msg.set_timestamp(seconds);
  msg.set_type(SyncDemo::ChatMessage::CHAT);
}

void
ChatDialog::formControlMessage(SyncDemo::ChatMessage &msg, SyncDemo::ChatMessage::ChatMessageType type)
{
  msg.set_from(m_nick);
  msg.set_to(m_chatroomName);
  int32_t seconds = static_cast<int32_t>(time::toUnixTimestamp(time::system_clock::now()).count()/1000000000);
  msg.set_timestamp(seconds);
  msg.set_type(type);
}

QString
ChatDialog::formatTime(time_t timestamp)
{
  struct tm *tm_time = localtime(&timestamp);
  int hour = tm_time->tm_hour;
  QString amOrPM;
  if (hour > 12)
  {
    hour -= 12;
    amOrPM = "PM";
  }
  else
  {
    amOrPM = "AM";
    if (hour == 0)
    {
      hour = 12;
    }
  }

  char textTime[12];
  sprintf(textTime, "%d:%02d:%02d %s", hour, tm_time->tm_min, tm_time->tm_sec, amOrPM.toStdString().c_str());
  return QString(textTime);
}

void
ChatDialog::printTimeInCell(QTextTable *table, time_t timestamp)
{
  QTextCharFormat timeFormat;
  timeFormat.setForeground(Qt::gray);
  timeFormat.setFontUnderline(true);
  timeFormat.setUnderlineColor(Qt::gray);
  QTextTableCell timeCell = table->cellAt(0, 1);
  timeCell.setFormat(timeFormat);
  timeCell.firstCursorPosition().insertText(formatTime(timestamp));
}

string
ChatDialog::getRandomString()
{
  uint32_t r = random::generateWord32();
  stringstream ss;
  {
    using namespace CryptoPP;
    StringSource(reinterpret_cast<uint8_t*>(&r), 4, true,
                 new HexEncoder(new FileSink(ss), false));
    
  }

  return ss.str();
}

void
ChatDialog::showMessage(const QString& from, const QString& data)
{
  if (!isActiveWindow())
    emit showChatMessage(QString::fromStdString(m_chatroomName),
                         from, data);
}

void
ChatDialog::fitView()
{
  boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
  QRectF rect = m_scene->itemsBoundingRect();
  m_scene->setSceneRect(rect);
  ui->syncTreeViewer->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);

  QRectF trustRect = m_trustScene->itemsBoundingRect();
  m_trustScene->setSceneRect(trustRect);
  ui->trustTreeViewer->fitInView(m_trustScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void
ChatDialog::summonReaper()
{
  Sync::SyncLogic &logic = m_sock->getLogic ();
  map<string, bool> branches = logic.getBranchPrefixes();
  QMap<QString, DisplayUserPtr> roster = m_scene->getRosterFull();

  m_zombieList.clear();

  QMapIterator<QString, DisplayUserPtr> it(roster);
  map<string, bool>::iterator mapIt;
  while(it.hasNext())
  {
    it.next();
    DisplayUserPtr p = it.value();
    if (p != DisplayUserNullPtr)
    {
      mapIt = branches.find(p->getPrefix().toStdString());
      if (mapIt != branches.end())
      {
        mapIt->second = true;
      }
    }
  }

  for (mapIt = branches.begin(); mapIt != branches.end(); ++mapIt)
  {
    // this is zombie. all active users should have been marked true
    if (! mapIt->second)
    {
      m_zombieList.append(mapIt->first.c_str());
    }
  }

  m_zombieIndex = 0;

  // start reaping
  reap();
}

void
ChatDialog::getTree(TrustTreeNodeList& nodeList)
{
  typedef map<Name, shared_ptr<TrustTreeNode> > NodeMap;

  vector<Name> certNameList;
  NodeMap nodeMap;

  m_sock->getIntroCertNames(certNameList);

  vector<Name>::const_iterator it  = certNameList.begin();
  vector<Name>::const_iterator end = certNameList.end();
  for(; it != end; it++)
    {
      Name introducerCertName;
      Name introduceeCertName;

      introducerCertName.wireDecode(it->get(-2).blockFromValue());
      introduceeCertName.wireDecode(it->get(-3).blockFromValue());

      Name introducerName = IdentityCertificate::certificateNameToPublicKeyName(introducerCertName).getPrefix(-1);
      Name introduceeName = IdentityCertificate::certificateNameToPublicKeyName(introduceeCertName).getPrefix(-1);

      NodeMap::iterator introducerIt = nodeMap.find(introducerName);
      if(introducerIt == nodeMap.end())
        {
          shared_ptr<TrustTreeNode> introducerNode(new TrustTreeNode(introducerName));
          nodeMap[introducerName] = introducerNode;
        }
      shared_ptr<TrustTreeNode> erNode = nodeMap[introducerName];

      NodeMap::iterator introduceeIt = nodeMap.find(introduceeName);
      if(introduceeIt == nodeMap.end())
        {
          shared_ptr<TrustTreeNode> introduceeNode(new TrustTreeNode(introduceeName));
          nodeMap[introduceeName] = introduceeNode;
        }
      shared_ptr<TrustTreeNode> eeNode = nodeMap[introduceeName];

      erNode->addIntroducee(eeNode);
      eeNode->addIntroducer(erNode);
    }
  
  nodeList.clear();
  queue<shared_ptr<TrustTreeNode> > nodeQueue;

  NodeMap::iterator nodeIt = nodeMap.find(m_identity);
  if(nodeIt == nodeMap.end())
    return;

  nodeQueue.push(nodeIt->second);
  nodeIt->second->setLevel(0);
  while(!nodeQueue.empty())
    {
      shared_ptr<TrustTreeNode>& node = nodeQueue.front();
      node->setVisited();

      TrustTreeNodeList& introducees = node->getIntroducees();
      TrustTreeNodeList::iterator eeIt  = introducees.begin();
      TrustTreeNodeList::iterator eeEnd = introducees.end();

      for(; eeIt != eeEnd; eeIt++)
        {
          _LOG_DEBUG("introducee: " << (*eeIt)->name() << " visited: " << boolalpha << (*eeIt)->visited());
          if(!(*eeIt)->visited())
            {
              nodeQueue.push(*eeIt);
              (*eeIt)->setLevel(node->level()+1);
            }
        }

      nodeList.push_back(node);
      nodeQueue.pop();
    }
}

void
ChatDialog::plotTrustTree()
{
  TrustTreeNodeList nodeList;

  getTree(nodeList);
  {
    boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
    m_trustScene->plotTrustTree(nodeList);
    fitView();
  }
}

// public slots:
void
ChatDialog::onLocalPrefixUpdated(const QString& localPrefix)
{
  Name newLocalPrefix(localPrefix.toStdString());
  if(!newLocalPrefix.empty() && newLocalPrefix != m_localPrefix) 
    {
      // Update localPrefix
      m_localPrefix = newLocalPrefix;

      updatePrefix();
      updateLabels();
      m_scene->setCurrentPrefix(QString(m_localChatPrefix.toUri().c_str()));

      if(m_sock != NULL)
        {
          {
            boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
            m_scene->clearAll();
            m_scene->plot("Empty");
          }

          ui->textEdit->clear();
          
          if (m_joined)
            {
              sendLeave();
            }

          delete m_sock;
          m_sock = NULL;
          
          usleep(100000);
          m_sock = new Sync::SyncSocket(m_chatroomPrefix,
                                        m_chatPrefix,
                                        m_session,
                                        m_useRoutablePrefix,
                                        m_localPrefix,
                                        m_face,
                                        m_myCertificate,
                                        m_dataRule,
                                        bind(&ChatDialog::processTreeUpdateWrapper, this, _1, _2),
                                        bind(&ChatDialog::processRemoveWrapper, this, _1));
          usleep(100000);
          QTimer::singleShot(600, this, SLOT(sendJoin()));
          m_timer->start(FRESHNESS * 1000);
          disableSyncTreeDisplay();
          QTimer::singleShot(2200, this, SLOT(enableSyncTreeDisplay()));
        }
      else
        initializeSync();
    }
  else 
    if (m_sock == NULL)
      initializeSync();

  fitView();
}

void
ChatDialog::onClose()
{
  hide();
  emit closeChatDialog(QString::fromStdString(m_chatroomName));
}


// private slots:
void
ChatDialog::onReturnPressed()
{
  QString text = ui->lineEdit->text();
  if (text.isEmpty())
    return;

  ui->lineEdit->clear();

  if (text.startsWith("boruoboluomi"))
  {
    summonReaper ();
    // reapButton->show();
    fitView();
    return;
  }

  if (text.startsWith("minimanihong"))
  {
    // reapButton->hide();
    fitView();
    return;
  }

  SyncDemo::ChatMessage msg;
  formChatMessage(text, msg);

  appendMessage(msg);

  sendMsg(msg);

  fitView();
}

void 
ChatDialog::onSyncTreeButtonPressed()
{
  if (ui->syncTreeViewer->isVisible())
  {
    ui->syncTreeViewer->hide();
    ui->syncTreeButton->setText("Show ChronoSync Tree");
  }
  else
  {
    ui->syncTreeViewer->show();
    ui->syncTreeButton->setText("Hide ChronoSync Tree");
  }

  fitView();
}

void
ChatDialog::onTrustTreeButtonPressed()
{
  if (ui->trustTreeViewer->isVisible())
  {
    ui->trustTreeViewer->hide();
    ui->trustTreeButton->setText("Show Trust Tree");
  }
  else
  {
    ui->trustTreeViewer->show();
    ui->trustTreeButton->setText("Hide Trust Tree");
  }

  fitView();
}

void
ChatDialog::onProcessData(const shared_ptr<const Data>& data, bool show, bool isHistory)
{
  SyncDemo::ChatMessage msg;
  bool corrupted = false;
  if (!msg.ParseFromArray(data->getContent().value(), data->getContent().value_size()))
  {
    _LOG_DEBUG("Errrrr.. Can not parse msg with name: " << data->getName() << ". what is happening?");
    // nasty stuff: as a remedy, we'll form some standard msg for inparsable msgs
    msg.set_from("inconnu");
    msg.set_type(SyncDemo::ChatMessage::OTHER);
    corrupted = true;
  }

  // display msg received from network
  // we have to do so; this function is called by ccnd thread
  // so if we call appendMsg directly
  // Qt crash as "QObject: Cannot create children for a parent that is in a different thread"
  // the "cannonical" way to is use signal-slot
  if (show && !corrupted)
  {
    appendMessage(msg, isHistory);
  }

  if (!isHistory)
  {
    // update the tree view
    std::string prefix = data->getName().getPrefix(-2).toUri();
    _LOG_DEBUG("<<< updating scene for" << prefix << ": " << msg.from());
    if (msg.type() == SyncDemo::ChatMessage::LEAVE)
    {
      processRemove(prefix.c_str());
    }
    else
    {
      boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
      m_scene->msgReceived(prefix.c_str(), msg.from().c_str());
    }
  }
  fitView();
}

void
ChatDialog::onProcessTreeUpdate(const vector<Sync::MissingDataInfo>& v)
{
  _LOG_DEBUG("<<< processing Tree Update");

  if (v.empty())
  {
    return;
  }

  // reflect the changes on digest tree
  {
    boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
    m_scene->processUpdate(v, m_sock->getRootDigest().c_str());
  }

  int n = v.size();
  int totalMissingPackets = 0;
  for (int i = 0; i < n; i++)
  {
    totalMissingPackets += v[i].high.getSeq() - v[i].low.getSeq() + 1;
  }

  for (int i = 0; i < n; i++)
  {
    if (totalMissingPackets < 4)
    {
      for (Sync::SeqNo seq = v[i].low; seq <= v[i].high; ++seq)
      {
        m_sock->fetchData(v[i].prefix, seq, bind(&ChatDialog::processDataWrapper, this, _1), 2);
        _LOG_DEBUG("<<< Fetching " << v[i].prefix << "/" <<seq.getSession() <<"/" << seq.getSeq());
      }
    }
    else
    {
        m_sock->fetchData(v[i].prefix, v[i].high, bind(&ChatDialog::processDataNoShowWrapper, this, _1), 2);
    }
  }
  // adjust the view
  fitView();
}

void
ChatDialog::onReplot()
{
  boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
  m_scene->plot(m_sock->getRootDigest().c_str());
  fitView();
}

void
ChatDialog::onRosterChanged(QStringList staleUserList)
{
  boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
  QStringList rosterList = m_scene->getRosterList();
  m_rosterModel->setStringList(rosterList);
  QString user;
  QStringListIterator it(staleUserList);
  while(it.hasNext())
  {
    std::string nick = it.next().toStdString();
    if (nick.empty())
      continue;

    SyncDemo::ChatMessage msg;
    formControlMessage(msg, SyncDemo::ChatMessage::LEAVE);
    msg.set_from(nick);
    appendMessage(msg);
  }
  plotTrustTree();
}

void
ChatDialog::onInviteListDialogRequested()
{
  emit waitForContactList();
  m_inviteListDialog->setInviteLabel(m_chatroomPrefix.toUri());
  m_inviteListDialog->show();
}

void
ChatDialog::sendJoin()
{
  m_joined = true;
  SyncDemo::ChatMessage msg;
  formControlMessage(msg, SyncDemo::ChatMessage::JOIN);
  sendMsg(msg);
  boost::random::random_device rng;
  boost::random::uniform_int_distribution<> uniform(1, FRESHNESS / 5 * 1000);
  m_randomizedInterval = HELLO_INTERVAL * 1000 + uniform(rng);
  QTimer::singleShot(m_randomizedInterval, this, SLOT(sendHello()));
}

void
ChatDialog::sendHello()
{
  int64_t now = time::toUnixTimestamp(time::system_clock::now()).count();
  int elapsed = (now - m_lastMsgTime) / 1000000000;
  if (elapsed >= m_randomizedInterval / 1000)
  {
    SyncDemo::ChatMessage msg;
    formControlMessage(msg, SyncDemo::ChatMessage::HELLO);
    sendMsg(msg);
    boost::random::random_device rng;
    boost::random::uniform_int_distribution<> uniform(1, FRESHNESS / 5 * 1000);
    m_randomizedInterval = HELLO_INTERVAL * 1000 + uniform(rng);
    QTimer::singleShot(m_randomizedInterval, this, SLOT(sendHello()));
  }
  else
  {
    QTimer::singleShot((m_randomizedInterval - elapsed * 1000), this, SLOT(sendHello()));
  }
}

void
ChatDialog::sendLeave()
{
  SyncDemo::ChatMessage msg;
  formControlMessage(msg, SyncDemo::ChatMessage::LEAVE);
  sendMsg(msg);
  usleep(500000);
  m_sock->leave();
  usleep(5000);
  m_joined = false;
  _LOG_DEBUG("Sync REMOVE signal sent");
}

void ChatDialog::enableSyncTreeDisplay()
{
  ui->syncTreeButton->setEnabled(true);
  // treeViewer->show();
  // fitView();
}

void
ChatDialog::reap()
{
  if (m_zombieIndex < m_zombieList.size())
  {
    string prefix = m_zombieList.at(m_zombieIndex).toStdString();
    m_sock->remove(prefix);
    _LOG_DEBUG("Reaped: prefix = " << prefix);
    m_zombieIndex++;
    // reap again in 10 seconds
    QTimer::singleShot(10000, this, SLOT(reap()));
  }
}

void
ChatDialog::onSendInvitation(QString invitee)
{
  Name inviteeNamespace(invitee.toStdString());
  shared_ptr<Contact> inviteeItem = m_contactManager->getContact(inviteeNamespace);
  sendInvitation(inviteeItem, true);
}

void 
ChatDialog::onReply(const Interest& interest, 
                    const shared_ptr<const Data>& data,
                    size_t routablePrefixOffset,
                    bool isIntroducer)
{
  OnDataValidated onValidated = bind(&ChatDialog::onReplyValidated,
                                     this, _1, 
                                     interest.getName().size()-routablePrefixOffset, //RoutablePrefix will be removed before data is passed to the validator
                                     isIntroducer);

  OnDataValidationFailed onFailed = bind(&ChatDialog::onReplyValidationFailed,
                                         this, _1, _2);

  if(routablePrefixOffset > 0)
    {
      // It is an encapsulated packet, we only validate the inner packet.
      shared_ptr<Data> innerData = make_shared<Data>();
      innerData->wireDecode(data->getContent().blockFromValue());
      m_invitationValidator->validate(*innerData, onValidated, onFailed);
    }
  else
    m_invitationValidator->validate(*data, onValidated, onFailed);
}

void
ChatDialog::onReplyTimeout(const Interest& interest, 
                           size_t routablePrefixOffset)
{
  Name interestName;
  if(routablePrefixOffset > 0)
    interestName = interest.getName().getSubName(routablePrefixOffset);
  else
    interestName = interest.getName();

  Invitation invitation(interestName);

  QString msg = QString::fromUtf8("Your invitation to ") + QString::fromStdString(invitation.getInviteeNameSpace().toUri()) + " times out!";
  emit inivationRejection(msg);
}

void
ChatDialog::onIntroCert(const Interest& interest, const shared_ptr<const Data>& data)
{
  Data innerData;
  innerData.wireDecode(data->getContent().blockFromValue());
  Sync::IntroCertificate introCert(innerData);
  m_sock->addParticipant(introCert);
  plotTrustTree();
}

void
ChatDialog::onIntroCertTimeout(const Interest& interest, int retry, const QString& msg)
{
  _LOG_DEBUG("onIntroCertTimeout: " << msg.toStdString());
}


#if WAF
#include "chat-dialog.moc"
#include "chat-dialog.cpp.moc"
#endif
