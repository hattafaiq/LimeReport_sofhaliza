#include "fh_gridlineschart_paradevib2.h"
//#include "kumpulan_struct.h"
//extern struct t_chart_parade_vib_recip vib_recip;

namespace LimeReport {
void fh_gridlineschart_vibrecip2::paintChart(QPainter *painter, QRectF chartRect, QVector<float> timming, QVector<float> PeakAbs, int odd_evn
                                             , QString satuan_peak,QString satuan_suhu, QVector<float> suhu_sil,QStringList nama_silinder)
{
    updateMinAndMaxValues();

    const qreal hPadding = this->hPadding(chartRect);
    const qreal vPadding = this->vPadding(chartRect);

    const qreal valuesVMargin = this->valuesVMargin(painter);

    QRectF gridRect = chartRect.adjusted(
        hPadding,
        vPadding + valuesVMargin * 2,
        -hPadding * 3,
        -vPadding * 3
        );

    if (!m_chartItem->horizontalAxisOnTop()) {
        // If horizontal axis is on the bottom, move grid a little up
        gridRect.adjust(0, -valuesVMargin, 0 , -valuesVMargin);
    }

    // Adapt font for horizontal axis
    painter->setFont(adaptFont((gridRect.width() - this->valuesHMargin(painter)) / xAxisData().segmentCount() * 0.8,
                               painter->font(),
                               xAxisData()));

    const qreal valuesHMargin = this->valuesHMargin(painter);

    // Adjust vertical axis labels padding
    gridRect.adjust(valuesHMargin * 0.2, 0, 0, 0);

    //paintGrid_paradeVib(painter, gridRect,100,180,260,360,750);
//    qDebug()<<"<LR-> cek peak!!! counter:"<<counter_paintGrid_paradeVib;
//    for(int i=0; i<PeakAbs.size(); i++){
//        qDebug()<<"->"<<PeakAbs[i];
//    }
    qDebug()<<"pv2 cek nilai:"<<PeakAbs<<odd_evn<<satuan_peak<<satuan_suhu<<suhu_sil;
    paintGrid_paradeVib2(painter, gridRect,timming[0],timming[2],timming[1],timming[3],timming[4],PeakAbs,odd_evn
            ,satuan_peak, satuan_suhu, suhu_sil,nama_silinder);
    paintSerialLines(
        painter,
        gridRect.adjusted(hPadding + valuesHMargin, 0, 0, 0)
        );
    counter_paintGrid_paradeVib+=1;
}

void fh_gridlineschart_vibrecip2::paintSerialLines(QPainter* painter, QRectF barsRect)
{
    if (valuesCount() == 0) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing,true);

    const AxisData &yAxisData = this->yAxisData();
    const qreal delta = yAxisData.delta();

    if (m_chartItem->itemMode() == DesignMode){
        const qreal hStep = barsRect.width() / valuesCount();
        const qreal vStep = barsRect.height() / delta;
        const qreal topShift = (delta - (maxValue() - minValue())) * vStep + barsRect.top();
        drawDesignMode(painter, hStep, vStep, topShift, barsRect);
        painter->restore();
        return;
    }

    const AxisData &xAxisData = this->xAxisData();
    const qreal hStep = barsRect.width() / (xAxisData.rangeMax() - xAxisData.rangeMin());

    qreal leftMargin = 0;
    const qreal topMargin = barsRect.top();

    float tot_high=0;
    int counters=0;
    //--------------------------plot ngitung------------------------------//
    for (SeriesItem* series : m_chartItem->series()) {
        counters+=1;

        QPen pen(series->color());
        pen.setWidth(m_chartItem->seriesLineWidth());
        painter->setPen(pen);

        const QList<qreal> &xAxisValues = series->data()->xAxisValues();
        const QList<qreal> &values = series->data()->values();
        const int xAxisValuesSize = xAxisValues.size();
        qreal lastXPos = 0;
        qreal lastYPos = 0;
        if (!values.isEmpty()) {
            // Calculate first point position on plot before loop
            lastYPos = calculatePos(yAxisData, values.first(), barsRect.height());
        }
        if (xAxisValues.isEmpty()) {
            leftMargin = barsRect.left();
        } else {
            leftMargin = barsRect.left();
            lastXPos = calculatePos(xAxisData, xAxisValues.first(), barsRect.width());
        }

        float peak_max=0;
        float peak_min=0;
        float last_data=0;

        for (int i = 0; i < values.count() - 1; ++i ) {

            if(peak_max<last_data){
                peak_max=values.at(i);
            }
            if(peak_min>last_data){
               peak_min=values.at(i);
            }
            last_data=values.at(i);

            const qreal startY = lastYPos;
            const qreal endY = calculatePos(yAxisData, values.at(i+1), barsRect.height());
            // Record last used Y position to only calculate new one
            lastYPos = endY;

            qreal startX = lastXPos;
            qreal endX = 0;
            if (i + 1 < xAxisValuesSize) {
                endX = calculatePos(xAxisData, xAxisValues.at(i+1), barsRect.width());
            } else {
                endX = startX + hStep;
            }
            // Record last used X position to only calculate new one
            lastXPos = endX;

            QPoint startPoint = QPoint(startX + leftMargin, startY + topMargin);
            QPoint endPoint = QPoint(endX + leftMargin, endY + topMargin);
            //drawSegment(painter, startPoint, endPoint, series->color());

        }
        tot_high+= fabs(peak_max-peak_min);
//        float data_max = peak_max;
//        float data_min = peak_min;
      //  slast_data=startY + topMargin;
        //tot_high+= fabs(peak_max-peak_min);
//        qDebug()<<"berapa count chart:"<<counter << "| tinggi:"<<barsRect.height() << "last pos Y:"<< lastYPos;
//        qDebug()<<"max:"<<peak_max<<"| min:"<<peak_min;
//        qDebug()<<"smax:"<<speak_max<<"| smin:"<<speak_min;
//        qDebug()<<"Y:"<<yAxisData.rangeMax() << yAxisData.rangeMin() << yAxisData.delta() << barsRect.height();
//        qDebug()<<"X:"<<xAxisData.rangeMax() << xAxisData.rangeMin() << xAxisData.delta() << barsRect.width();

    }
    //--------------------------plot tambahan------------------------------//
    float bagi_rata = tot_high/(counters/2);     /*mencari tinggi rata-rata kurva persilinder*/
    float mean_rata = bagi_rata/2;

    int counter=0;
    float sparators=0;
    sparators+=mean_rata;
    for (SeriesItem* series : m_chartItem->series()) {
//        sparators+=bagi_rata;
        counter+=1;
        QPen pen(series->color());
        pen.setWidth(m_chartItem->seriesLineWidth());
        painter->setPen(pen);

        const QList<qreal> &xAxisValues = series->data()->xAxisValues();
        const QList<qreal> &values = series->data()->values();
        const int xAxisValuesSize = xAxisValues.size();
        qreal lastXPos = 0;
        qreal lastYPos = 0;
        if (!values.isEmpty()) {
            // Calculate first point position on plot before loop
            lastYPos = (/*yAxisData.rangeMax() -*/sparators- values.first()) / tot_high/*yAxisData.delta()*/ * barsRect.height()/*calculatePos(yAxisData, values.first(), barsRect.height())*/;
        }
        if (xAxisValues.isEmpty()) {
            leftMargin = barsRect.left();
        } else {
            leftMargin = barsRect.left();
            lastXPos = (/*xAxisData.rangeMax() -*/xAxisData.rangeMax()- xAxisValues.first()) / xAxisData.delta() * barsRect.width();//calculatePos(xAxisData, xAxisValues.first(), barsRect.width());
        }
        float peak_max=0;
        float peak_min=0;
        float last_data=0;

       // if(counter==1){
            for (int i = 0; i < values.count() - 1; ++i ) {

                if(peak_max<last_data){
                    peak_max=values.at(i);
                }
                if(peak_min>last_data){
                   peak_min=values.at(i);
                }
                last_data=values.at(i);

                const qreal startY = lastYPos;
                const qreal endY = /*calculatePos(yAxisData, values.at(i+1), barsRect.height());*/(/*yAxisData.rangeMax() -*/sparators- values.at(i+1)) / tot_high/*yAxisData.delta()*/ * barsRect.height();
                // Record last used Y position to only calculate new one
                lastYPos = endY;

                qreal startX = lastXPos;
                qreal endX = 0;
                if (i + 1 < xAxisValuesSize) {
                    endX = /*calculatePos(xAxisData, xAxisValues.at(i+1), barsRect.width());*/(/*xAxisData.rangeMax() -*/xAxisData.rangeMax()- xAxisValues.at(i+1)) / xAxisData.delta() * barsRect.width();
                } else {
                    endX = startX + hStep;
                }
                // Record last used X position to only calculate new one
                lastXPos = endX;

                QPoint startPoint = QPoint(startX + leftMargin, startY + topMargin /*+ (mean_rata + (counters * bagi_rata)) */ );
                QPoint endPoint = QPoint(endX + leftMargin, endY + topMargin /*+ (mean_rata + (counters * bagi_rata))*/ );
                //drawSegment(painter, startPoint, endPoint, series->color());
                if (((startPoint.y() <= barsRect.bottom()) && (startPoint.y() >= barsRect.top()))&&
                    ((endPoint.y() <= barsRect.bottom()) && (endPoint.y() >= barsRect.top()))){
                     /*if(counter<2)*/drawSegment(painter, startPoint, endPoint, series->color());
                }
         //   }
        }
        //tot_high+= fabs(peak_max-peak_min);
//        qDebug()<<"max:"<<peak_max<<"| min:"<<peak_min;
//        qDebug()<<"berapa count chart:"<<counter << "| tinggi:"<<barsRect.height() << "last pos Y:"<< lastYPos;
        if((counter%2)==0)sparators+=bagi_rata;
    }

//    for (SeriesItem* series : m_chartItem->series()){
//        qDebug()<<"pos"<<counter<<":"<<(mean_rata + (counter * bagi_rata));
//    }
//        qDebug()<<"Y:"<<yAxisData.rangeMax() << yAxisData.rangeMin() << yAxisData.delta() << barsRect.height();
//        qDebug()<<"X:"<<xAxisData.rangeMax() << xAxisData.rangeMin() << xAxisData.delta() << barsRect.width();
//        qDebug()<<"tot:"<<tot_high<<"| bagi rat:"<<bagi_rata<<" |mean rat:"<<mean_rata;



    painter->restore();
}
}
