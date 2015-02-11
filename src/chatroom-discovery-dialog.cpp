#include "chatroom-discovery-dialog.hpp"
#include "ui_chatroom-discovery-dialog.h"

namespace chronochat {

using ndn::Name;

ChatroomDiscoveryDialog::ChatroomDiscoveryDialog(QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::ChatroomDiscoveryDialog)
  , m_standardItemModel(new QStandardItemModel(0, 3, this))
  , m_chatroomDiscoveryViewDialog(new ChatroomDiscoveryViewDialog)
{
  ui->setupUi(this);

  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelButtonClicked()));
  connect(ui->joinButton, SIGNAL(clicked()),
          this, SLOT(onJoinButtonClicked()));
  connect(ui->viewButton, SIGNAL(clicked()),
          this, SLOT(onViewButtonClicked()));
  connect(ui->chatroomListView, SIGNAL(clicked(QModelIndex)),
          this, SLOT(onChatroomListViewClicked(QModelIndex)));
  connect(ui->chatroomListView, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(onChatroomListViewDoubleClicked(QModelIndex)));

  updateChatroomList();
}

ChatroomDiscoveryDialog::~ChatroomDiscoveryDialog()
{
  delete ui;
}

void
ChatroomDiscoveryDialog::updateChatroomList()
{
  m_standardItemModel->clear();

  m_standardItemModel
    ->setHorizontalHeaderItem(0, new QStandardItem(QString("Chatroom Name")));
  m_standardItemModel
    ->setHorizontalHeaderItem(1, new QStandardItem(QString("Chatroom Trust Model")));
  m_standardItemModel
    ->setHorizontalHeaderItem(2, new QStandardItem(QString("Participants or Contacts")));

  QHeaderView *m_headerView = ui->chatroomListView->horizontalHeader();
  m_headerView->setResizeMode((QHeaderView::ResizeMode)3);
  m_headerView->setStretchLastSection(true);

  ui->chatroomListView->setModel(m_standardItemModel);

  int i = 0;
  for (Chatrooms::const_iterator it = m_chatrooms.begin();
       it != m_chatrooms.end(); ++it, ++i) {
    QStandardItem *item = new QStandardItem(QString::fromStdString(it->first.toUri()));
    item->setEditable(false);
    m_standardItemModel->setItem(i, 0, item);

    if (it->second.getTrustModel() == ChatroomInfo::TRUST_MODEL_WEBOFTRUST)
      item = new QStandardItem(QString("Web of trust"));
    else
      item = new QStandardItem(QString("Hierarchical"));
    item->setEditable(false);
    m_standardItemModel->setItem(i, 1, item);

    QString content;

    for (std::list<Name>::const_iterator nameIt = it->second.getParticipants().begin();
         nameIt != it->second.getParticipants().end(); nameIt++) {
      content.append(QString::fromStdString(nameIt->toUri())).append(",");
    }
    item = new QStandardItem(content);
    item->setEditable(false);
    m_standardItemModel->setItem(i, 2, item);
  }
}

void
ChatroomDiscoveryDialog::onDiscoverChatroomChanged(const chronochat::ChatroomInfo& info, bool isAdd)
{
  if (isAdd)
    m_chatrooms[info.getName()] = info;
  else
    m_chatrooms.erase(info.getName());

  updateChatroomList();
}

void
ChatroomDiscoveryDialog::onCancelButtonClicked()
{
  this->close();
}

void
ChatroomDiscoveryDialog::onJoinButtonClicked()
{
  if(ui->chatroomListView->selectionModel()->selectedRows().size() == 0) {
    QMessageBox::information(this, tr("ChronoChat"), tr("Please select a chatroom to join"));
  }
  else {
    m_selectedRow = ui->chatroomListView->selectionModel()->selectedRows()[0].row();
    QStandardItem* selectedChatroomName = m_standardItemModel->item(m_selectedRow, 0);
    emit startChatroom(selectedChatroomName->text(), false);

    {
      // Tmp disabled
      // QStandardItem* selectedChatroomTrustModel = m_standardItemModel->item(m_selectedRow, 1);
      // if(selectedChatroomTrustModel->text() == "Hierarchical") {
      //   emit startChatroom(selectedChatroomName->text(), false);
      // }
      // else if(selectedChatroomTrustModel->text() == "Web of trust") {
      //   ChatroomDiscoveryLogic::ChatroomList::const_iterator it;
      //   it = m_chatroomList->find(Name::Component(selectedChatroomName->text().toStdString()));

      //   if(it->second.getContacts().size() == 0) {
      //     QMessageBox messageBox;
      //     messageBox.addButton(QMessageBox::Ok);
      //     messageBox.setIcon(QMessageBox::Information);
      //     messageBox.setText
      //       ("You do not have a contact in this chatroom. Please choose another chatroom to join");
      //     messageBox.exec();
      //   }
      //   else {
      //     m_sendInvitationRequestDialog->setChatroomName(selectedChatroomName->text());
      //     m_sendInvitationRequestDialog->setContacts(it->second.getContacts());
      //     m_sendInvitationRequestDialog->show();
      //   }
      // }
    }
  }
}

void
ChatroomDiscoveryDialog::onChatroomListViewDoubleClicked(const QModelIndex &index)
{
  onJoinButtonClicked();
}

void
ChatroomDiscoveryDialog::onChatroomListViewClicked(const QModelIndex &index)
{
  ui->chatroomListView->selectRow(index.row());
  m_selectedRow = index.row();
}

void
ChatroomDiscoveryDialog::onViewButtonClicked()
{
  if(ui->chatroomListView->selectionModel()->selectedRows().size() == 0) {
    QMessageBox messageBox;
    messageBox.addButton(QMessageBox::Ok);
    messageBox.setIcon(QMessageBox::Information);
    messageBox.setText("Please select a chatroom to view");
    messageBox.exec();
  }
  else {
    m_selectedRow = ui->chatroomListView->selectionModel()->selectedRows()[0].row();

    QStandardItem* selectedChatroomName = m_standardItemModel->item(m_selectedRow);
    m_chatroomDiscoveryViewDialog->setChatroomName(selectedChatroomName->text());

    // QStandardItem *m_selectedTrustModel = m_standardItemModel->item(m_selectedRow, 1);
    // m_chatroomDiscoveryViewDialog->setChatroomTrustModel(m_selectedTrustModel->text());

    //use chatroomlist as parameter to call set participants
    //maybe for different chatroom with different trust model???
    //participants can be contacts

    Chatrooms::const_iterator it =
      m_chatrooms.find(Name::Component(selectedChatroomName->text().toStdString()));

    if (it != m_chatrooms.end()) {
      m_chatroomDiscoveryViewDialog->setChatroomParticipants(it->second.getParticipants());
      m_chatroomDiscoveryViewDialog->show();
    }
  }
}

} // namespace chronochat


#if WAF
#include "chatroom-discovery-dialog.moc"
#endif
