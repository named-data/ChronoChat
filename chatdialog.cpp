#include <QtGui>
#include "chatdialog.h"
#include "settingdialog.h"
#include <ctime>
#include <iostream>
#include <QTimer>

ChatDialog::ChatDialog(QWidget *parent)
  : QDialog(parent)
{
  setupUi(this);

  readSettings();
  updateLabels();

  lineEdit->setFocusPolicy(Qt::StrongFocus);
  DigestTreeScene *scene = new DigestTreeScene();

  treeViewer->setScene(scene);
  QRectF rect = scene->itemsBoundingRect();
  scene->setSceneRect(rect);
  //scene->plot();

  connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
  connect(setButton, SIGNAL(pressed()), this, SLOT(buttonPressed()));
}

void
ChatDialog::appendMessage(const SyncDemo::ChatMessage &msg) 
{

  if (msg.type() != SyncDemo::ChatMessage::CHAT) {
    return;
  }

  if (!msg.has_data()) {
    return;
  }

  if (msg.from().empty() || msg.data().empty()) {
    return;
  }

  QTextCursor cursor(textEdit->textCursor());
  cursor.movePosition(QTextCursor::End);
  QTextTableFormat tableFormat;
  tableFormat.setBorder(0);
  QTextTable *table = cursor.insertTable(1, 2, tableFormat);
  QString from = QString("<%1>: ").arg(msg.from().c_str());
  table->cellAt(0, 0).firstCursorPosition().insertText(from);
  table->cellAt(0, 1).firstCursorPosition().insertText(msg.data().c_str());
  QScrollBar *bar = textEdit->verticalScrollBar();
  bar->setValue(bar->maximum());
}

void
ChatDialog::formChatMessage(const QString &text, SyncDemo::ChatMessage &msg) {
  msg.set_from(m_user.getNick().toStdString());
  msg.set_to(m_user.getChatroom().toStdString());
  msg.set_data(text.toStdString());
  time_t seconds = time(NULL);
  msg.set_timestamp(seconds);
}

void
ChatDialog::readSettings()
{
  QSettings s(ORGANIZATION, APPLICATION);
  QString nick = s.value("nick", "").toString();
  QString chatroom = s.value("chatroom", "").toString();
  QString prefix = s.value("prefix", "").toString();
  if (nick == "" || chatroom == "" || prefix == "") {
    QTimer::singleShot(500, this, SLOT(buttonPressed()));
  }
  else {
    m_user.setNick(nick);
    m_user.setChatroom(chatroom);
    m_user.setPrefix(prefix);
  }
}

void 
ChatDialog::writeSettings()
{
  QSettings s(ORGANIZATION, APPLICATION);
  s.setValue("nick", m_user.getNick());
  s.setValue("chatroom", m_user.getChatroom());
  s.setValue("prefix", m_user.getPrefix());
}

void
ChatDialog::updateLabels()
{
  QString settingDisp = QString("<User: %1>, <Chatroom: %2>").arg(m_user.getNick()).arg(m_user.getChatroom());
  infoLabel->setText(settingDisp);
  QString prefixDisp = QString("<Prefix: %1>").arg(m_user.getPrefix());
  prefixLabel->setText(prefixDisp);
}

void 
ChatDialog::returnPressed()
{
  QString text = lineEdit->text();
  if (text.isEmpty())
    return;
 
  lineEdit->clear();

  SyncDemo::ChatMessage msg;
  formChatMessage(text, msg);
  
  // TODO:
  // send message
  appendMessage(msg);
  
}

void
ChatDialog::buttonPressed()
{
  SettingDialog dialog(this, m_user.getNick(), m_user.getChatroom(), m_user.getPrefix());
  connect(&dialog, SIGNAL(updated(QString, QString, QString)), this, SLOT(settingUpdated(QString, QString, QString)));
  dialog.exec();
}

void
ChatDialog::settingUpdated(QString nick, QString chatroom, QString prefix)
{
  bool needWrite = false;
  if (!nick.isEmpty() && nick != m_user.getNick()) {
    m_user.setNick(nick);
    needWrite = true;
  }
  if (!prefix.isEmpty() && prefix != m_user.getPrefix()) {
    m_user.setPrefix(prefix);
    needWrite = true;
    // TODO: set the previous prefix as left?
  }
  if (!chatroom.isEmpty() && chatroom != m_user.getChatroom()) {
    m_user.setChatroom(chatroom);
    needWrite = true;
    // TODO: perhaps need to do a lot. e.g. use a new SyncAppSokcet
  }
  if (needWrite) {
    writeSettings();
    updateLabels();
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

void
ChatDialog::fitView()
{
  treeViewer->fitInView(treeViewer->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
}
