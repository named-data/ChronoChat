#ifndef CHRONOCHAT_H
#define CHRONOCHAT_H

#include <QDialog>

namespace Ui {
class ChronoChat;
}

class ChronoChat : public QDialog
{
    Q_OBJECT

public:
    explicit ChronoChat(QWidget *parent = 0);
    ~ChronoChat();

private:
    Ui::ChronoChat *ui;
};

#endif // CHRONOCHAT_H
