#ifndef HORIZONTALBARCHART_H
#define HORIZONTALBARCHART_H

#include "lrchartitem.h"

namespace LimeReport{

class HorizontalBarChart: public AbstractBarChart{
public:
    HorizontalBarChart(ChartItem* chartItem):AbstractBarChart(chartItem){}
    void paintChart(QPainter *painter, QRectF chartRect, QVector<float> timming, QVector<float> PeakAbs, int odd_evn
                    , QString satuan_peak,QString satuan_suhu, QVector<float> suhu_sil,QStringList nama_silinder,
                    QVector<float> peak_ign, QVector<float> derajat_ign, QString satuan_ign);
    void paintHorizontalBars(QPainter *painter, QRectF barsRect);
};

} // namespace LimeReport

#endif // HORIZONTALBARCHART_H
