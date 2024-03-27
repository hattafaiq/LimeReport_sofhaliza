#ifndef GRIDLINESCHART_H
#define GRIDLINESCHART_H

#include "lrlineschart.h"

namespace LimeReport {
class GridLinesChart : public LinesChart{
public:
    GridLinesChart(ChartItem* chartItem):LinesChart(chartItem){}
    void paintChart(QPainter *painter, QRectF chartRect, QVector<float> timming, QVector<float> PeakAbs, int odd_evn
                    , QString satuan_peak,QString satuan_suhu, QVector<float> suhu_sil,QStringList nama_silinder,
                    QVector<float> peak_ign, QVector<float> derajat_ign, QString satuan_ign);

private:
    void paintSerialLines(QPainter *painter, QRectF barsRect);
};
}

#endif // GRIDLINESCHART_H
