#ifndef WARNINGDIALOG_H
#define WARNINGDIALOG_H

#include <QDialog>

namespace Ui {
class WarningDialog;
}

class WarningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WarningDialog(QWidget *parent = 0);
    ~WarningDialog();

private:
    Ui::WarningDialog *ui;
};

#endif // WARNINGDIALOG_H
