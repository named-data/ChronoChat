#ifndef SENDINVITATIONREQUESTDIALOG_H
#define SENDINVITATIONREQUESTDIALOG_H

#include <QDialog>
#include <QStringListModel>
#include <QMessageBox>
#include <QDebug>
#include <QAbstractItemView>

#ifndef Q_MOC_RUN
#include "chatroom-discovery.hpp"
#endif

namespace Ui {
class SendInvitationRequestDialog;
}

namespace chronos {

class SendInvitationRequestDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SendInvitationRequestDialog(QWidget *parent = 0);
  ~SendInvitationRequestDialog();

  void
  setContacts(const std::vector<ndn::Name>& contacts);

  void
  setChatroomName(const QString chatroomName);

public slots:
  void
  onSendButtonClicked();

  void
  onCancelButtonClicked();

  void
  onContactListViewClicked(QModelIndex modelIndex);

  void
  onContactListViewDoubleClicked(QModelIndex modelIndex);

private:
  Ui::SendInvitationRequestDialog *ui;
  QString m_chatroomName;
  QStringListModel *m_stringListModel;
};

} //namespace chronos

#endif // SENDINVITATIONREQUESTDIALOG_H
