#include <QtGui>
#include "chatdialog.h"
#include <ctime>

ChatDialog::ChatDialog(QWidget *parent)
  : QDialog(parent)
{
  setupUi(this);
  lineEdit->setFocusPolicy(Qt::StrongFocus);


  connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
}

void
ChatDialog::appendMessage(const SyncDemo::ChatMessage &msg) 
{
  if (msg.from().isEmpty || message.msgData().isEmpty())
    return;

  QTextCursor cursor(textEdit->textCursor());
  cursor.movePosition(QTextCursor::End);
  QTextTableFormat tableFormat;
  tableFormat.setBorder(0);
  QTextTable *table = cursor.insertTable(1, 2, tableFormat);
  table->cellAt(0, 0).firstCursorPosition().insertText("<" + msg.from() + ">: ");
  table->cellAt(0, 1).firstCursorPosition().insertText(msg.msgData());
  QScrollBar *bar = textEdit->verticalScrollBar();
  bar->setValue(bar->maximum());
}

void 
ChatDialog::updateTreeView() 
{
}

void 
ChatDialog::returnPressed()
{
  QString text = lineEdit->text();
  if (text.isEmpty())
    return;
 
  SyncDemo::ChatMessage msg;
  formChatMessage(text, msg);
  
  // TODO:
  // send message
  appendMessage(msg);
  updateTreeView();

}

void
ChatDialog::formChatMessage(const QString &text, SyncDemo::ChatMessage &msg) {
  msg.set_from(m_nick);
  msg.set_to(m_chatroom);
  msg.set_msgData(text);
  time_t seconds = time(NULL);
  msg.set_timestamp(seconds);
}
