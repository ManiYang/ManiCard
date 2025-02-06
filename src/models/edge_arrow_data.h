#ifndef EDGE_ARROW_DATA_H
#define EDGE_ARROW_DATA_H

#include <QColor>
#include <QPointF>
#include <QVector>

struct EdgeArrowData
{
    double lineWidth;
    QColor lineColor;
    QColor labelColor;
    QVector<QPointF> joints;
};

#endif // EDGE_ARROW_DATA_H
