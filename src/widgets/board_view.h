#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QFrame>

class BoardView : public QFrame
{
    Q_OBJECT
public:
    BoardView(QWidget *parent = nullptr);


private:
    QString styleSheet();
};

#endif // BOARDVIEW_H
