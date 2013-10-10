#ifndef CONTACTPANEL_H
#define CONTACTPANEL_H

#include <QDialog>

namespace Ui {
class ContactPanel;
}

class ContactPanel : public QDialog
{
    Q_OBJECT

public:
    explicit ContactPanel(QWidget *parent = 0);
    ~ContactPanel();

private:
    Ui::ContactPanel *ui;
};

#endif // CONTACTPANEL_H
