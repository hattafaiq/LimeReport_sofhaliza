#ifndef PIECHART_H
#define PIECHART_H

#include "lrchartitem.h"

namespace LimeReport{

class PieChart : public AbstractChart{
public:
    PieChart(ChartItem* chartItem):AbstractChart(chartItem){}
    QSizeF calcChartLegendSize(const QFont &font, qreal maxWidth = 0);
    void paintChart(QPainter *painter, QRectF chartRect, QVector<float> timming, QVector<float> PeakAbs, int odd_evn
                    , QString satuan_peak,QString satuan_suhu, QVector<float> suhu_sil,QStringList nama_silinder,
                    QVector<float> peak_ign, QVector<float> derajat_ign, QString satuan_ign);
    void paintChartLegend(QPainter *painter, QRectF legendRect, QVector<float> timming);
protected:
    void drawPercent(QPainter *painter, QRectF chartRect, qreal startAngle, qreal angle);
};

} // namespace LimeReport
#endif // PIECHART_H
