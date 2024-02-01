#ifndef FH_GRIDLINESCHART_PARADEVIB2_H
#define FH_GRIDLINESCHART_PARADEVIB2_H

#include "lrlineschart.h"

namespace LimeReport {
class fh_gridlineschart_vibrecip2: public LinesChart{
public:
    fh_gridlineschart_vibrecip2(ChartItem* chartItem):LinesChart(chartItem){}
    void paintChart(QPainter *painter, QRectF chartRect,QVector<float> timming, QVector<float> PeakAbs, int odd_evn);
    int counter_paintGrid_paradeVib;

private:
    void paintSerialLines(QPainter *painter, QRectF barsRect);

};
}
#endif // FH_GRIDLINESCHART_PARADEVIB2_H
