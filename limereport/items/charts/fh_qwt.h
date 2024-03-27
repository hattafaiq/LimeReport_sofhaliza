#ifndef FH_QWT_H
#define FH_QWT_H

#include "lrlineschart.h"

namespace LimeReport {
class fh_qwt: public LinesChart{
public:fh_qwt(ChartItem* chartItem):LinesChart(chartItem){}

    void paintChart(QPainter *painter, QRectF chartRect,QVector<float> timming, QVector<float> PeakAbs, int odd_evn,
                QString satuan_peak,QString satuan_suhu, QVector<float> suhu_sil,QStringList nama_silinder,QVector<float> peak_ign, QVector<float> derajat_ign, QString satuan_ign);
    int counter_paintGrid_paradeVib;

private:
    void paintSerialLines(QPainter *painter, QRectF barsRect);

};
}
#endif // FH_QWT_H
