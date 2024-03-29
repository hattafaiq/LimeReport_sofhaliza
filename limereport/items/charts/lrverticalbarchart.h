#ifndef VERTICALBARCHART_H
#define VERTICALBARCHART_H

#include "lrlineschart.h"

namespace LimeReport{

class VerticalBarChart: public LinesChart{
public:
    VerticalBarChart(ChartItem* chartItem):LinesChart(chartItem){}
    void paintChart(QPainter *painter, QRectF chartRect, QVector<float> timming, QVector<float> PeakAbs, int odd_evn
                    , QString satuan_peak,QString satuan_suhu, QVector<float> suhu_sil,QStringList nama_silinder,
                    QVector<float> peak_ign, QVector<float> derajat_ign, QString satuan_ign);
//    void paintVerticalGrid(QPainter *painter, QRectF gridRect);
    void paintVerticalBars(QPainter *painter, QRectF barsRect);
    void paintSerialLines(QPainter *painter, QRectF barsRect);
};

} //namespace LimeReport

#endif // VERTICALBARCHART_H
