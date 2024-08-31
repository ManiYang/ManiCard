#ifndef SIMPLETOOLBAR_H
#define SIMPLETOOLBAR_H

#include <QFrame>
#include <QHBoxLayout>

class SimpleToolBar : public QFrame
{
    Q_OBJECT
public:
    explicit SimpleToolBar(QWidget *parent = nullptr);

protected:
    QHBoxLayout *hLayout;
};

#endif // SIMPLETOOLBAR_H
