#ifndef ADDCONTACTPANEL_H
#define ADDCONTACTPANEL_H

#include <QDialog>

namespace Ui {
class AddContactPanel;
}

class AddContactPanel : public QDialog
{
    Q_OBJECT

public:
    explicit AddContactPanel(QWidget *parent = 0);
    ~AddContactPanel();

private:
    Ui::AddContactPanel *ui;
};

#endif // ADDCONTACTPANEL_H
