#ifndef APP_EVENT_SOURCE_H
#define APP_EVENT_SOURCE_H

#include <QWidget>

struct EventSource
{
    explicit EventSource(QWidget *w) : sourceWidget(w) {}
    QWidget *sourceWidget {nullptr};
};

#endif // APP_EVENT_SOURCE_H
