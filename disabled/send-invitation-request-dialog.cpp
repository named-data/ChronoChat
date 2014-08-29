#include "send-invitation-request-dialog.hpp"
#include "ui_send-invitation-request-dialog.h"

using namespace chronos;

SendInvitationRequestDialog::SendInvitationRequestDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SendInvitationRequestDialog),
  m_stringListModel(new QStringListModel)
{
  ui->setupUi(this);
  ui->contactsListView->setEditTriggers(ui->contactsListView->NoEditTriggers);
  ui->contactsListView->setModel(m_stringListModel);

  connect(ui->sendButton, SIGNAL(clicked()),
          this, SLOT(onSendButtonClicked()));
  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelButtonClicked()));
  connect(ui->contactsListView, SIGNAL(clicked(QModelIndex)),
          this, SLOT(onContactListViewClicked(QModelIndex)));
  connect(ui->contactsListView,SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(onContactListViewDoubleClicked(QModelIndex)));
}

SendInvitationRequestDialog::~SendInvitationRequestDialog()
{
  delete ui;
}

void
SendInvitationRequestDialog::setContacts(const std::vector<ndn::Name>& contacts)
{
  QStringList contactsList;
  for(int i = 0; i < contacts.size(); i++){
    contactsList.append(QString::fromStdString(contacts[i].toUri()));
  }
  m_stringListModel->setStringList(contactsList);
}

void
SendInvitationRequestDialog::onSendButtonClicked()
{
  if(ui->contactsListView->selectionModel()->selectedRows().size() == 0){
    QMessageBox messageBox;
    messageBox.addButton(QMessageBox::Ok);
    messageBox.setIcon(QMessageBox::Information);
    messageBox.setText("Please select a contact to send invitation request");
    messageBox.exec();
  }
  else{
    //send invitation request
    int selectedRow = ui->contactsListView->selectionModel()->selectedRows()[0].row();
    QString contactName = m_stringListModel->stringList()[selectedRow];
    qDebug() << contactName;

    QMessageBox messageBox;
    messageBox.addButton(QMessageBox::Yes);
    messageBox.addButton(QMessageBox::No);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setText("Send invitation request to "+ contactName +"?");

    int selection = messageBox.exec();

    if(selection == QMessageBox::Yes){
      //send invitation request
      //need to have another message box when successfully send
      //emit sendInvitationRequest(m_chatroomName, contactName);
    }
  }
}


void
SendInvitationRequestDialog::onCancelButtonClicked()
{
  //qDebug() << "cancel";
  this->close();
}

void
SendInvitationRequestDialog::onContactListViewClicked(QModelIndex modelIndex)
{}

void
SendInvitationRequestDialog::onContactListViewDoubleClicked(QModelIndex modelIndex)
{
  onSendButtonClicked();
}

void
SendInvitationRequestDialog::setChatroomName(const QString chatroomName)
{
  m_chatroomName = chatroomName;
}


#if WAF
#include "send-invitation-request-dialog.moc"
#endif
