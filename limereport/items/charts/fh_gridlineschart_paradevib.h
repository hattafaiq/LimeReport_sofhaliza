#ifndef FH_GRIDLINESCHART_PARADEVIB_H
#define FH_GRIDLINESCHART_PARADEVIB_H

#include "lrlineschart.h"

namespace LimeReport {
class fh_gridlineschart_vibrecip: public LinesChart{
public:
    fh_gridlineschart_vibrecip(ChartItem* chartItem):LinesChart(chartItem){}
    void paintChart(QPainter *painter, QRectF chartRect,QVector<float> timming);

private:
    void paintSerialLines(QPainter *painter, QRectF barsRect);

};
}
#endif // FH_GRIDLINESCHART_PARADEVIB_H
