#include "lrchartitem.h"
#include <QStyleOptionGraphicsItem>
#include <QPainter>

#include "lrdesignelementsfactory.h"
#include "lrchartitemeditor.h"
#include "lrdatasourcemanager.h"
#include "lrpagedesignintf.h"
#include "lrreportengine_p.h"
#include "lrdatadesignintf.h"
#include "lrchartaxiseditor.h"

#include "charts/lrpiechart.h"
#include "charts/lrverticalbarchart.h"
#include "charts/lrhorizontalbarchart.h"
#include "charts/lrlineschart.h"
#include "charts/lrgridlineschart.h"
#include "charts/fh_gridlineschart_paradevib.h"
#include "charts/fh_gridlineschart_paradevib2.h"
#include "charts/fh_gridlineschart_parade_ign.h"
#include "charts/fh_gridlineschart_parade_ign2.h"

#include "qwt_plot_canvas.h"


namespace{

const QString xmlTag = "ChartItem";

LimeReport::BaseDesignIntf * createChartItem(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::ChartItem(owner,parent);
}
bool registred = LimeReport::DesignElementsFactory::instance().registerCreator(
                     xmlTag, LimeReport::ItemAttribs(QObject::tr("Chart Item"),"Item"), createChartItem
                 );
}

namespace LimeReport{

QColor generateColor()
{
    int red = (rand()%(256 - 1)) + 1;
    int green = (rand()%(256 - 1)) + 1;
    int blue = (rand()%(256 - 1)) + 1;;
    return QColor(red,green,blue);
}

QColor color_map[39] = {
        QColor(51,102,204),    QColor(220,57,18),   QColor(225, 153, 0),  QColor(16, 150, 24),
        QColor(153,0,153),     QColor(0,153,198),   QColor(221, 68, 119), QColor(255,0,0),
        QColor(0,0,139),       QColor(0,205,0),     QColor(233,30,99),    QColor(255,255,0),
        QColor(244,67,54),     QColor(156,39,176),  QColor(103,58,183),   QColor(63,81,181),
        QColor(33,153,243),    QColor(0,150,136),   QColor(78,175,80),    QColor(139,195,74),
        QColor(205,228,57),    QColor(0,139,0),     QColor(0,0,255),      QColor(255,235,59),
        QColor(255,193,7),     QColor(255,152,0),   QColor(255,87,34),    QColor(121,85,72),
        QColor(158,158,158),   QColor(96,125,139),  QColor(241,153,185),  QColor(64,64,64),
        QColor(188,229,218),   QColor(139,0,0),     QColor(139,139,0),    QColor(171, 130, 255),
        QColor(139, 123, 139), QColor(255, 0, 255), QColor(139, 69, 19)
};

QString SeriesItem::name() const
{
    return m_name;
}

void SeriesItem::setName(const QString &name)
{
    m_name = name;
}

QString SeriesItem::valuesColumn() const
{
    return m_valuesColumn;
}

void SeriesItem::setValuesColumn(const QString &valuesColumn)
{
    m_valuesColumn = valuesColumn;
}

QString SeriesItem::labelsColumn() const
{
    return m_labelsColumn;
}

void SeriesItem::setLabelsColumn(const QString &labelsColumn)
{
    m_labelsColumn = labelsColumn;
}

QString SeriesItem::xAxisColumn() const
{
    return m_xAxisColumn;
}

void SeriesItem::setXAxisColumn(const QString &xAxisColumn)
{
    m_xAxisColumn = xAxisColumn;
}

SeriesItem *SeriesItem::clone()
{
    SeriesItem* result = new SeriesItem();
    for (int i = 0; i < this->metaObject()->propertyCount(); ++i){
        result->setProperty(this->metaObject()->property(i).name(),property(this->metaObject()->property(i).name()));
    }
    return result;
}

void SeriesItem::fillSeriesData(IDataSource *dataSource)
{
    m_data.clear();

    if (dataSource){
        dataSource->first();
        int currentColorIndex = 0;
        while(!dataSource->eof()){
            if (!m_labelsColumn.isEmpty()){
                m_data.labels().append(dataSource->data(m_labelsColumn).toString());
                qDebug()<<"lab:"<<dataSource->data(m_labelsColumn).toString();
            }
            if (!m_xAxisColumn.isEmpty()){
                m_data.xAxisValues().append(dataSource->data(m_xAxisColumn).toDouble());
                qDebug()<<"Xaxis:"<<dataSource->data(m_xAxisColumn).toDouble();
            }
            qDebug()<<"data:"<<dataSource->data(m_valuesColumn).toDouble();
            m_data.values().append(dataSource->data(m_valuesColumn).toDouble());
            m_data.colors().append((currentColorIndex<32)?color_map[currentColorIndex]:generateColor());
            dataSource->next();
            currentColorIndex++;
        }
    }
}

QColor SeriesItem::color() const
{
    return m_color;
}

void SeriesItem::setColor(const QColor &color)
{
    m_color = color;
}

SeriesItem::SeriesItemPreferredType SeriesItem::preferredType() const
{
    return m_preferredType;
}

void SeriesItem::setPreferredType(const SeriesItemPreferredType& type)
{
    m_preferredType = type;
}

ChartItem::ChartItem(QObject *owner, QGraphicsItem *parent)
    : ItemDesignIntf(xmlTag, owner, parent), m_legendBorder(true),
      m_legendAlign(LegendAlignRightCenter), m_titleAlign(TitleAlignCenter),
      m_chartType(Pie), m_labelsField(""), m_isEmpty(true),
      m_showLegend(true), m_drawPoints(true), m_seriesLineWidth(4),
      m_horizontalAxisOnTop(false), m_gridChartLines(AllLines),
      m_legendStyle(LegendPoints)
{

    m_xAxisData = new AxisData(AxisData::XAxis, this);
    m_xAxisData->setReverseDirection(true);
    m_yAxisData = new AxisData(AxisData::YAxis, this);
    m_labels<<"First"<<"Second"<<"Thrid";
    m_chart = new PieChart(this);
    m_chart->setTitleFont(font());
}

ChartItem::~ChartItem()
{
    foreach (SeriesItem* series, m_series) {
        delete series;
    }
    m_series.clear();
    delete m_chart;
}

ChartItem::TitleAlign ChartItem::titleAlign() const
{
    return m_titleAlign;
}

void ChartItem::setTitleAlign(const TitleAlign &titleAlign)
{
    if (m_titleAlign != titleAlign){
        TitleAlign oldValue = m_titleAlign;
        m_titleAlign = titleAlign;
        notify("titleAlign",oldValue,m_titleAlign);
        update();
    }
}

void ChartItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    setupPainter(painter);
    painter->setFont(transformToSceneFont(painter->font()));
    painter->setRenderHint(QPainter::Antialiasing,true);
    painter->setRenderHint(QPainter::TextAntialiasing,true);
    qreal borderMargin = (rect().height()*0.01>10)?(10):(rect().height()*0.01);
    qreal maxTitleHeight = rect().height()*0.5;

    QFont tmpFont = painter->font();

    qreal titleOffset = 0;
    if (!m_title.isEmpty()) {
        QFontMetrics fm(titleFont());
        const qreal titleHeight = fm.boundingRect(rect().toRect(), Qt::TextWordWrap,chartTitle()).height() + borderMargin * 2;
        titleOffset = std::min(titleHeight, maxTitleHeight);
    }

    const QRectF titleRect = QRectF(borderMargin,borderMargin,rect().width()-borderMargin*2,titleOffset);
    QRectF legendRect = QRectF(0, 0, 0, 0);
    QRectF diagramRect = rect().adjusted(borderMargin, titleOffset + borderMargin,
                                         -(borderMargin * 2), -borderMargin);
    if (m_showLegend) {
        legendRect = m_chart->calcChartLegendRect(painter->font(), rect(), false, borderMargin, titleOffset);
        switch(legendAlign()) {
        case LegendAlignRightTop:
        case LegendAlignRightBottom:
        case LegendAlignRightCenter:
            diagramRect.adjust(0, 0, -legendRect.width(), 0);
            break;
        case LegendAlignBottomLeft:
        case LegendAlignBottomCenter:
        case LegendAlignBottomRight:
            diagramRect.adjust(0, 0, 0, -(legendRect.height() + borderMargin * 2));
            break;
        }
    }

    QVector<float> timming;
     timming.push_back( timming_EC);
     timming.push_back( timming_EO);
     timming.push_back( timming_IC);
     timming.push_back( timming_IO);
     timming.push_back( timming_sil);

    paintChartTitle(painter, titleRect);
    if (m_showLegend)
        m_chart->paintChartLegend(painter,legendRect,timming);
   // qDebug()<<"1 cek nilai:"<<peak_data<<even_odd<<peak_satuan<<suhu_satuan<<suhu_val;

    m_chart->paintChart(painter,diagramRect,timming,peak_data,even_odd,peak_satuan,suhu_satuan,suhu_val,nama_silinder,
                        peak_ign,derajat_ign,satuan_ign);

    painter->restore();
    ItemDesignIntf::paint(painter,option,widget);
}

QObject *ChartItem::xAxisSettings()
{
    return m_xAxisData;
}

void ChartItem::setYAxisSettings(QObject *axis)
{
    AxisData *data = dynamic_cast<AxisData*>(axis);
    if (data) {
        m_yAxisData->copy(data);
    }
}

QObject *ChartItem::yAxisSettings()
{
    return m_yAxisData;
}

void ChartItem::setXAxisSettings(QObject *axis)
{
    AxisData *data = static_cast<AxisData*>(axis);
    if (data) {
        m_xAxisData->copy(data);
    }
}

AxisData *ChartItem::xAxisData()
{
    return m_xAxisData;
}

AxisData *ChartItem::yAxisData()
{
    return m_yAxisData;
}

void ChartItem::showAxisEditorDialog(bool isXAxis)
{
    showDialog(new ChartAxisEditor(this, page(), isXAxis, settings()));
}

BaseDesignIntf *ChartItem::createSameTypeItem(QObject *owner, QGraphicsItem *parent)
{
    ChartItem* result = new ChartItem(owner,parent);
    foreach (SeriesItem* series, m_series) {
        result->m_series.append(series->clone());
    }
    return result;
}

QObject *ChartItem::createElement(const QString &collectionName, const QString &elementType)
{
    Q_UNUSED(elementType);
    if (collectionName.compare("series")==0){
        SeriesItem* seriesItem = new SeriesItem();
        m_series.append(seriesItem);
        return seriesItem;
    }
    return 0;
}

int ChartItem::elementsCount(const QString &collectionName)
{
    if (collectionName.compare("series")==0)
        return m_series.count();
    return 0;
}

QObject *ChartItem::elementAt(const QString &collectionName, int index)
{
    if (collectionName.compare("series")==0)
        return m_series.at(index);
    return 0;
}

void ChartItem::updateItemSize(DataSourceManager *dataManager, RenderPass , int )
{

    m_isEmpty = false;
    if (dataManager && dataManager->dataSource(m_datasource)){
        IDataSource* ds =  dataManager->dataSource(m_datasource);
        foreach (SeriesItem* series, m_series) {
            if (series->isEmpty()){
                series->setLabelsColumn(m_labelsField);
                series->setXAxisColumn(m_xAxisField);
                series->fillSeriesData(ds);
            }
        }
        fillLabels(ds);
    }
    timming_EC = dataManager->timming_EC;
    timming_EO = dataManager->timming_EO;
    timming_IC = dataManager->timming_IC;
    timming_IO = dataManager->timming_IO;
    timming_sil = dataManager->timming_Silinder;
    peak_data = dataManager->list_peak;
    even_odd = dataManager->odd_evens;
    peak_satuan = dataManager->peak_satuans;
    suhu_satuan  = dataManager->suhu_satuans;
    suhu_val = dataManager->list_suhu;
    nama_silinder = dataManager->list_silinder;
    peak_ign = dataManager->peak_igns;
    derajat_ign = dataManager->derajat_igns;
    satuan_ign = dataManager->satuan_igns;
}

void ChartItem::fillLabels(IDataSource *dataSource)
{
    m_labels.clear();
    if (dataSource && !m_labelsField.isEmpty()){
        dataSource->first();
        while(!dataSource->eof()){
            m_labels.append(dataSource->data(m_labelsField).toString());
            dataSource->next();
        }
    }

}

QWidget *ChartItem::defaultEditor()
{
    QWidget* editor = new ChartItemEditor(this, page(), settings());
    editor->setAttribute(Qt::WA_DeleteOnClose);
    return editor;
}

bool ChartItem::isNeedUpdateSize(RenderPass pass) const
{
    return  pass == FirstPass && m_isEmpty;
}

QSettings *ChartItem::settings()
{
    PageDesignIntf *page = this->page();
    if (page->settings()) {
        return page->settings();
    }
    if (page->reportEditor()) {
        return page->reportEditor()->settings();
    }
    return 0;
}

bool ChartItem::showLegend() const
{
    return m_showLegend;
}

void ChartItem::setShowLegend(bool showLegend)
{
    if (m_showLegend != showLegend){
        m_showLegend = showLegend;
        notify("showLegend", !m_showLegend, m_showLegend);
        update();
    }
    m_showLegend = showLegend;
}

QList<QString> ChartItem::labels() const
{
    return m_labels;
}

void ChartItem::setLabels(const QList<QString> &labels)
{
    m_labels = labels;
}

QString ChartItem::labelsField() const
{
    return m_labelsField;
}

void ChartItem::setLabelsField(const QString &labelsField)
{
    m_labelsField = labelsField;
}

ChartItem::ChartType ChartItem::chartType() const
{
    return m_chartType;
}

void ChartItem::setChartType(const ChartType &chartType)
{
    if (m_chartType != chartType){
        ChartType oldValue = m_chartType;
        m_chartType = chartType;
        QFont oldTitleFont = m_chart->titleFont();
        delete m_chart;
        switch (m_chartType) {
        case Pie:
            m_chart = new PieChart(this);
            break;
        case VerticalBar:
            m_chart = new VerticalBarChart(this);
            break;
        case HorizontalBar:
            m_chart = new HorizontalBarChart(this);
            break;
        case Lines:
            m_chart = new LinesChart(this);
            break;
        case GridLines:
            m_chart = new GridLinesChart(this);
            break;
        case GridLines_paradeVib:
            m_chart = new fh_gridlineschart_vibrecip(this);
            break;
        case GridLines_paradeVib2:
            m_chart = new fh_gridlineschart_vibrecip2(this);
            break;
        case GridLines_paradeISEC:
            m_chart = new fh_gridlineschart_parade_ign(this);
            break;
        case GridLines_paradeISEC2:
            m_chart = new fh_gridlineschart_parade_ign2(this);
            break;
        }
        m_chart->setTitleFont(oldTitleFont);
        notify("chartType",oldValue,m_chartType);
        update();
    }
}

QString ChartItem::datasource() const
{
    return m_datasource;
}

void ChartItem::setDatasource(const QString &datasource)
{
    m_datasource = datasource;
}

void ChartItem::paintChartTitle(QPainter *painter, QRectF titleRect)
{
    painter->save();
    QFont tmpFont = transformToSceneFont(titleFont());
    QRect titleBoundingRect = QFontMetrics(tmpFont).boundingRect(rect().toRect(), Qt::TextWordWrap, chartTitle());

    while ((titleBoundingRect.height() > titleRect.height() || titleBoundingRect.width() > titleRect.width())
           && tmpFont.pixelSize() > 1)
    {
        tmpFont.setPixelSize(tmpFont.pixelSize()-1);
        titleBoundingRect = QFontMetrics(tmpFont).boundingRect(rect().toRect(), Qt::TextWordWrap, chartTitle());
    }

    painter->setFont(tmpFont);
    Qt::AlignmentFlag align = Qt::AlignCenter;
    switch (m_titleAlign) {
    case TitleAlignLeft:
        align = Qt::AlignLeft;
        break;
    case TitleAlignCenter:
        align = Qt::AlignCenter;
        break;
    case TitleAlignRight:
        align = Qt::AlignRight;
        break;
    }
    painter->drawText(titleRect, align | Qt::TextWordWrap, m_title);
    painter->restore();
}


ChartItem::LegendAlign ChartItem::legendAlign() const
{
    return m_legendAlign;
}

void ChartItem::setLegendAlign(const LegendAlign &legendAlign)
{
    if (m_legendAlign != legendAlign){
        LegendAlign oldValue = m_legendAlign;
        m_legendAlign = legendAlign;
        notify("legendAlign",oldValue,m_legendAlign);
        update();
    }
}

ChartItem::LegendStyle ChartItem::legendStyle() const
{
    return m_legendStyle;
}

void ChartItem::setLegendStyle(const LegendStyle &legendStyle)
{
    if (m_legendStyle == legendStyle) {
        return;
    }
    LegendStyle oldValue = m_legendStyle;
    m_legendStyle = legendStyle;
    notify("legendStyle", oldValue, m_legendStyle);
    update();
}

bool ChartItem::drawLegendBorder() const
{
    return m_legendBorder;
}

void ChartItem::setDrawLegendBorder(bool legendBorder)
{
    if (m_legendBorder!=legendBorder){
        m_legendBorder = legendBorder;
        notify("legendBorder",!m_legendBorder,m_legendBorder);
        update();
    }
}

QString ChartItem::chartTitle() const
{
    return m_title;
}

void ChartItem::setChartTitle(const QString &title)
{
    if (m_title != title){
        QString oldValue = m_title;
        m_title = title;
        update();
        notify("chartTitle",oldValue,title);
    }
}

QList<SeriesItem *> &ChartItem::series()
{
    return m_series;
}

void ChartItem::setSeries(const QList<SeriesItem *> &series)
{
    m_series = series;
}

bool ChartItem::isSeriesExists(const QString &name)
{
    foreach (SeriesItem* series, m_series) {
        if (series->name().compare(name)==0) return true;
    }
    return false;
}

int ChartItem::seriesLineWidth() const
{
    return m_seriesLineWidth;
}

bool ChartItem::drawPoints() const
{
    return m_drawPoints;
}

void ChartItem::setDrawPoints(bool drawPoints)
{
    if (m_drawPoints != drawPoints){
        m_drawPoints = drawPoints;
        notify("drawPoints", !m_drawPoints, m_drawPoints);
        update();
    }
    m_drawPoints = drawPoints;
}

void ChartItem::setSeriesLineWidth(int newSeriesLineWidth)
{
    if (m_seriesLineWidth != newSeriesLineWidth){
        int oldValue = m_seriesLineWidth;
        m_seriesLineWidth = newSeriesLineWidth;
        notify("seriesLineWidth", oldValue, m_seriesLineWidth);
        update();
    }
    m_seriesLineWidth = newSeriesLineWidth;
}

QString ChartItem::xAxisField() const
{
    return m_xAxisField;
}

void ChartItem::setXAxisField(const QString &xAxisField)
{
    m_xAxisField = xAxisField;
}

bool ChartItem::horizontalAxisOnTop() const
{
    return m_horizontalAxisOnTop;
}

void ChartItem::setHorizontalAxisOnTop(bool horizontalAxisOnTop)
{
    if (m_horizontalAxisOnTop != horizontalAxisOnTop){
        m_horizontalAxisOnTop = horizontalAxisOnTop;
        notify("horizontalAxisOnTop", !m_horizontalAxisOnTop, m_horizontalAxisOnTop);
        update();
    }
    m_horizontalAxisOnTop = horizontalAxisOnTop;
}

ChartItem::GridChartLines ChartItem::gridChartLines() const
{
    return m_gridChartLines;
}

void ChartItem::setGridChartLines(GridChartLines flags)
{
    if (m_gridChartLines == flags) {
        return;
    }
    GridChartLines oldValue = m_gridChartLines;
    m_gridChartLines = flags;
    if (isLoading()) {
        return;
    }
    update(rect());
    notify("gridChartLines",QVariant(oldValue),QVariant(flags));
}

void ChartItem::setCharItemFont(QFont value)
{
    if (font() == value) {
        return;
    }
    QFont oldValue = font();
    setFont(value);
    if (!isLoading()) update();
    notify("font",oldValue,value);
}

QFont ChartItem::titleFont() const
{
    return m_chart->titleFont();
}

void ChartItem::setTitleFont(QFont value)
{
    if (m_chart->titleFont() == value){
        return;
    }
    QFont oldValue = value;
    m_chart->setTitleFont(value);
    if (!isLoading()) update();
    notify("titleFont", oldValue, value);
}

AbstractChart::AbstractChart(ChartItem *chartItem)
    :m_chartItem(chartItem)
{
    m_designLabels<<QObject::tr("First")<<QObject::tr("Second")<<QObject::tr("Thrid");
}

QRectF AbstractChart::calcChartLegendRect(const QFont &font, const QRectF &parentRect, bool takeAllRect, qreal borderMargin, qreal titleOffset)
{
    const QSizeF legendSize = calcChartLegendSize(font, parentRect.width() * 0.9);
    qreal legendTopMargin = 0;
    qreal legendBottomMargin = 0;
    qreal legendLeftMargin = 0;

    bool isVertical = true;
    switch (m_chartItem->legendAlign()) {
    case ChartItem::LegendAlignRightTop:
        legendTopMargin = titleOffset + borderMargin;
        legendBottomMargin = parentRect.height() - (legendSize.height() + titleOffset);
        isVertical = true;
        break;
    case ChartItem::LegendAlignRightCenter:
        legendTopMargin = titleOffset + (parentRect.height() - titleOffset - legendSize.height()) / 2;
        legendBottomMargin = (parentRect.height() - titleOffset - legendSize.height()) / 2;
        isVertical = true;
        break;
    case ChartItem::LegendAlignRightBottom:
        legendTopMargin = parentRect.height() - (legendSize.height() + titleOffset);
        legendBottomMargin = borderMargin;
        isVertical = true;
        break;
    case ChartItem::LegendAlignBottomLeft:
        legendLeftMargin = QFontMetrics(font).height() / 2;
        isVertical = false;
        break;
    case ChartItem::LegendAlignBottomCenter:
        legendLeftMargin = (parentRect.width() - legendSize.width()) / 2;
        isVertical = false;
        break;
    case ChartItem::LegendAlignBottomRight:
        legendLeftMargin = parentRect.width() - legendSize.width() - QFontMetrics(font).height() / 2;
        isVertical = false;
        break;
    }

    if (isVertical) {
        qreal rightOffset = !takeAllRect ? ((legendSize.width() > parentRect.width() / 2 - borderMargin) ?
                                                (parentRect.width() / 2) :
                                                (parentRect.width() - legendSize.width())) : 0;
        return parentRect.adjusted(
            rightOffset,
            (legendSize.height()>(parentRect.height()-titleOffset))?(titleOffset):(legendTopMargin),
            -borderMargin,
            (legendSize.height()>(parentRect.height()-titleOffset))?(0):(-legendBottomMargin)
            );
    } else {
        const qreal verticalOffset = borderMargin * 2;
        return parentRect.adjusted(
            legendLeftMargin,
            (parentRect.height()) - (legendSize.height() + verticalOffset),
            -(parentRect.width() - (legendSize.width() + legendLeftMargin)),
            -verticalOffset
            );
    }
}


QFont AbstractChart::titleFont()
{
    return m_titleFont;
}

void AbstractChart::setTitleFont(const QFont &value)
{
    m_titleFont = value;
}

void AbstractChart::prepareLegendToPaint(QRectF &legendRect, QPainter *painter)
{
    QFont tmpFont = painter->font();
    switch(m_chartItem->legendAlign()) {
    case ChartItem::LegendAlignBottomLeft:
    case ChartItem::LegendAlignBottomCenter:
    case ChartItem::LegendAlignBottomRight: {
        const qreal maxWidth = legendRect.width() * 0.95;
        qreal legendWidth = std::accumulate(m_legendColumnWidths.cbegin(), m_legendColumnWidths.cend(), 0.0);
        if (legendWidth < maxWidth) {
            return;
        }
        while ( (legendWidth > maxWidth) && tmpFont.pixelSize() > 1) {
            tmpFont.setPixelSize(tmpFont.pixelSize() - 1);
            calcChartLegendSize(tmpFont, legendRect.width());
            legendWidth = std::accumulate(m_legendColumnWidths.cbegin(), m_legendColumnWidths.cend(), 0.0);
        }
        painter->setFont(tmpFont);
        break;
    }
    case ChartItem::LegendAlignRightTop:
    case ChartItem::LegendAlignRightCenter:
    case ChartItem::LegendAlignRightBottom:
        QSizeF legendSize = calcChartLegendSize(tmpFont, legendRect.width());
        if ((legendSize.height() <= legendRect.height() && legendSize.width() <= legendRect.width())) {
            return;
        }
        while ((legendSize.height() > legendRect.height() || legendSize.width() > legendRect.width())
               && tmpFont.pixelSize() > 1)
        {
            tmpFont.setPixelSize(tmpFont.pixelSize() - 1);
            legendSize = calcChartLegendSize(tmpFont, legendRect.width());
        }
        painter->setFont(tmpFont);
        legendRect = calcChartLegendRect(tmpFont, legendRect, true, 0, 0);
        break;
    }
}

AbstractSeriesChart::AbstractSeriesChart(ChartItem *chartItem)
    :AbstractChart(chartItem)
{
    counter=0;
    m_designValues[0] = 10;
    m_designValues[1] = 35;
    m_designValues[2] = 15;
    m_designValues[3] = 5;
    m_designValues[4] = 20;
    m_designValues[5] = 10;
    m_designValues[6] = 40;
    m_designValues[7] = 20;
    m_designValues[8] = 5;
}

qreal AbstractSeriesChart::maxValue()
{
    return m_chartItem->yAxisData()->maxValue();
}

qreal AbstractSeriesChart::minValue()
{
    return m_chartItem->yAxisData()->minValue();
}

AxisData &AbstractSeriesChart::xAxisData() const
{
    return *m_chartItem->xAxisData();
}

AxisData &AbstractSeriesChart::yAxisData() const
{
    return *m_chartItem->yAxisData();
}

void AbstractSeriesChart::updateMinAndMaxValues()
{
    if (m_chartItem->itemMode() == DesignMode) {
        m_chartItem->xAxisData()->updateForDesignMode();
        m_chartItem->yAxisData()->updateForDesignMode();
        return;
    }

    qreal maxYValue = 0;
    qreal minYValue = std::numeric_limits<qreal>::max();
    qreal maxXValue = 0;
    qreal minXValue = std::numeric_limits<qreal>::max();

    for (SeriesItem* series : m_chartItem->series()){
        for (qreal value : series->data()->values()){
            minYValue = std::min(minYValue, value);
            maxYValue = std::max(maxYValue, value);
        }
        if (series->data()->xAxisValues().isEmpty()) {
            // Grid plot starts from 0 on x axis so x range must be decresed by 1
            const bool startingFromZero = m_chartItem->chartType() == ChartItem::GridLines;
            const qreal valuesCount = this->valuesCount() - (startingFromZero ? 1 : 0);
            minXValue = std::min(0.0, minXValue);
            maxXValue = std::max(valuesCount, maxXValue);
        } else {
            for (qreal value : series->data()->xAxisValues()){
                minXValue = std::min(value, minXValue);
                maxXValue = std::max(value, maxXValue);
            }
        }
    }

    m_chartItem->xAxisData()->update(minXValue, maxXValue);
    m_chartItem->yAxisData()->update(minYValue, maxYValue);
}

qreal AbstractSeriesChart::hPadding(QRectF chartRect)
{
    return (chartRect.width() * 0.02);
}

qreal AbstractSeriesChart::vPadding(QRectF chartRect)
{
    return (chartRect.height() * 0.02);
}

int AbstractSeriesChart::valuesCount()
{
    if (m_chartItem->itemMode() == DesignMode) return 3;
    return (m_chartItem->series().isEmpty()) ? 0 : m_chartItem->series().at(0)->data()->values().count();
}

int AbstractSeriesChart::seriesCount()
{
    if (m_chartItem->itemMode() == DesignMode) return 3;
    return m_chartItem->series().count();
}

QSizeF AbstractSeriesChart::calcChartLegendSize(const QFont &font, const qreal maxWidth)
{
    QFontMetrics fm(font);

    switch(m_chartItem->legendAlign()) {
    case ChartItem::LegendAlignBottomLeft:
    case ChartItem::LegendAlignBottomCenter:
    case ChartItem::LegendAlignBottomRight: {
        const qreal seriesCount = m_chartItem->series().isEmpty() ? m_designLabels.size() : m_chartItem->series().size();
        const qreal indicatorWidth = fm.height() * 1.5;
        m_legendColumnWidths.clear();
        while (!calculateLegendColumnWidths(indicatorWidth, maxWidth, fm)) {
            // Nothing to do here
        }
        if (m_legendColumnWidths.isEmpty()) {
            m_legendColumnWidths.append(0);
        }
        const qreal columnCount = m_legendColumnWidths.size();
        const qreal rowCount = std::ceil(seriesCount / columnCount);
        QSizeF legendSize(std::accumulate(m_legendColumnWidths.cbegin(), m_legendColumnWidths.cend(), 0.0) + fm.height() / 2,
                          (rowCount + 1) * fm.height());
        if (legendSize.width() > maxWidth) {
            legendSize.setWidth(maxWidth);
        }
        return legendSize;
    }
    default: {
        qreal cw = 0;
        qreal maxWidth = 0;

        if (m_chartItem->series().isEmpty()) {
            foreach(QString label, m_designLabels){
                cw += fm.height();
                if (maxWidth<fm.boundingRect(label).width())
                    maxWidth = fm.boundingRect(label).width()+10;
            }
        } else {
            foreach(SeriesItem* series, m_chartItem->series()){
                cw += fm.height();
                if (maxWidth<fm.boundingRect(series->name()).width())
                    maxWidth = fm.boundingRect(series->name()).width()+10;
            }
        }
        cw += fm.height();
        return QSizeF(maxWidth+fm.height()*2,cw);
    }
    }
    return QSizeF();
}

bool AbstractSeriesChart::verticalLabels(QPainter* painter, QRectF labelsRect)
{
    if (valuesCount() == 0) return false;

    qreal hStep = (labelsRect.width() / valuesCount());
    QFontMetrics fm = painter->fontMetrics();
    foreach(QString label, m_chartItem->labels()){
        if (fm.boundingRect(label).width() > hStep){
            return  true;
        }
    }
    return false;
}

void AbstractSeriesChart::paintHorizontalLabels(QPainter *painter, QRectF labelsRect)
{
    if (valuesCount() == 0) return;

    painter->save();
    qreal hStep = (labelsRect.width() / valuesCount());
    if (!m_chartItem->labels().isEmpty()){
        if (verticalLabels(painter, labelsRect)){
            painter->rotate(270);
            painter->translate( -(labelsRect.top()+labelsRect.height()), labelsRect.left() );
            foreach (QString label, m_chartItem->labels()) {
                painter->drawText(QRectF(QPoint(0,0),
                                         QSize(labelsRect.height()-4, hStep)), Qt::AlignVCenter | Qt::AlignRight, label);
                painter->translate(0,hStep);
            }
            painter->rotate(-270);
        } else {
            painter->translate( labelsRect.left(), labelsRect.top() );
            foreach (QString label, m_chartItem->labels()) {
//                qDebug()<<"paintHorizontalLabels "<<" label:"<<label;
                painter->drawText(QRectF(QPoint(0, 4),
                                         QSize(hStep, labelsRect.height()-4)), Qt::AlignHCenter | Qt::AlignTop, label);
                painter->translate(hStep, 0);
            }
        }
    }
    painter->restore();
}

void AbstractSeriesChart::paintVerticalLabels(QPainter *painter, QRectF labelsRect)
{
    if (valuesCount() == 0) return;

    painter->save();
    painter->setFont(adaptLabelsFont(labelsRect.adjusted(0, 0, -hPadding(m_chartItem->rect()), 0),
                                     painter->font()));
    qreal vStep = (labelsRect.height() / valuesCount());
    int curLabel = 0;

    painter->translate(labelsRect.topLeft());
    if (!m_chartItem->labels().isEmpty()){
        foreach (QString label, m_chartItem->labels()) {
//            qDebug()<<"paintVerticalLabels "<<" label:"<<label;
            painter->drawText(QRectF(QPoint(0,vStep*curLabel),
                                     QSize(labelsRect.width()-hPadding(m_chartItem->rect()),vStep)),
                              Qt::AlignVCenter | Qt::AlignRight | Qt::TextWordWrap,label);

            curLabel++;
        }
    }
    painter->restore();
}

void AbstractSeriesChart::paintHorizontalGrid(QPainter *painter, QRectF gridRect)
{
    painter->save();

    const AxisData &yAxisData = this->yAxisData();

    const int segmentCount = yAxisData.segmentCount();
    const int lineCount = segmentCount + 1;

    painter->setRenderHint(QPainter::Antialiasing,false);
    qreal hStep = (gridRect.width() - painter->fontMetrics().boundingRect(QString::number(maxValue())).width()) / segmentCount;

    painter->setFont(adaptFont(hStep-4, painter->font(), yAxisData));

    QPointF textPos;
    if (m_chartItem->horizontalAxisOnTop()) {
        textPos.setY(gridRect.top());
    } else {
        textPos.setY(gridRect.bottom() - painter->fontMetrics().height());
    }
    for (int i = 0 ; i < lineCount ; i++ ) {
        const qreal x = gridRect.left() + hStep * i;
        textPos.setX(x + 4);
        painter->drawText(QRectF(textPos, QSizeF(hStep, painter->fontMetrics().height())),
                           axisLabel(i, yAxisData));
        painter->drawLine(x, gridRect.bottom(), x, gridRect.top());
    }
    painter->restore();
}

void AbstractSeriesChart::paintVerticalGrid(QPainter *painter, QRectF gridRect)
{
    const AxisData &yAxisData = this->yAxisData();

    painter->setRenderHint(QPainter::Antialiasing,false);

    const int segmentCount = yAxisData.segmentCount();
    const int lineCount = segmentCount + 1;
    qreal vStep = gridRect.height() / segmentCount;

    const qreal valuesHMargin = this->valuesHMargin(painter);
    const int fontHeight = painter->fontMetrics().height();
    const int halfFontHeight = fontHeight / 2;
    const qreal textPositionOffset = valuesHMargin * 0.2;

    const QTextOption verticalTextOption(Qt::AlignRight);
    for (int i = 0 ; i < lineCount ; i++ ) {
        const qreal y = vStep * i;
        painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionOffset,y+halfFontHeight),
                                 QSizeF(valuesHMargin,fontHeight)),
                          axisLabel(i, yAxisData),
                          verticalTextOption);
        painter->drawLine(gridRect.bottomLeft()-QPointF(-valuesHMargin,y),
                          gridRect.bottomRight()-QPointF(0,y));
    }

    painter->setRenderHint(QPainter::Antialiasing,true);
}

void AbstractSeriesChart::paintGrid(QPainter *painter, QRectF gridRect)
{
    painter->save();

    const AxisData &yAxisData = this->yAxisData();
    AxisData &xAxisData = this->xAxisData();

    painter->setRenderHint(QPainter::Antialiasing,false);

    const int xAxisSegmentCount = xAxisData.segmentCount();
    const int xAxisLineCount = xAxisSegmentCount + 1;
    const int yAxisSegmentCount = yAxisData.segmentCount();
    const int yAxisLineCount = yAxisSegmentCount + 1;

    const int fontHeight = painter->fontMetrics().height();
    const int halfFontHeight = fontHeight / 2;
    const QSizeF gridOffset = QSizeF(hPadding(gridRect), vPadding(gridRect));
    const qreal valuesHMargin = this->valuesHMargin(painter);
    const qreal vStep = gridRect.height() / yAxisSegmentCount;
    const qreal hStep = (gridRect.width() - valuesHMargin - gridOffset.width()) / xAxisSegmentCount;
    const qreal textPositionHOffset = valuesHMargin * 0.1;

    // Vertical axis lines
    const QTextOption verticalTextOption(Qt::AlignRight);
    for (int i = 0 ; i < yAxisLineCount ; i++ ) {
        const qreal y = vStep * i;
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::HorizontalLine
                                  || i == 0 || i == xAxisSegmentCount;
        painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset, y + halfFontHeight),
                                 QSizeF(valuesHMargin,fontHeight)),
                           axisLabel(i, yAxisData),
                          verticalTextOption);

        QPointF lineEndPos = gridRect.bottomRight() - QPointF(0, y);
        if (!drawFullLine) {
            lineEndPos.setX(gridRect.left() + valuesHMargin + gridOffset.width());
        }
        painter->drawLine(gridRect.bottomLeft() - QPointF(-valuesHMargin, y), lineEndPos);
    }

    // Horizontal axis lines
    for (int i = 0 ; i < xAxisLineCount ; i++) {
        const qreal x = gridRect.left() + hStep * i + valuesHMargin + gridOffset.width();
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::VerticalLine
                                  || i == 0 || i == xAxisSegmentCount;
        const QString text = axisLabel(i, xAxisData);

        if (m_chartItem->horizontalAxisOnTop()) {
            painter->drawLine(x, gridRect.top() - gridOffset.height(),
                              x, (drawFullLine ? gridRect.bottom() : gridRect.top()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.top() - (fontHeight + gridOffset.height()),
                                     hStep, fontHeight),
                              text);
        } else {
            painter->drawLine(x, gridRect.bottom() + gridOffset.height(),
                              x, (drawFullLine ? gridRect.top() : gridRect.bottom()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.bottom() + halfFontHeight * 0 + gridOffset.height(),
                                     hStep, fontHeight),
                              text);
        }
    }
    painter->restore();
}

void AbstractSeriesChart::paintGrid_paradeVib(QPainter *painter, QRectF gridRect,
                                             const float derajat_EC,const float derajat_IC,
                                              const float derajat_EO,const float derajat_IO,
                                              const float derajat_silinder, QVector<float> list_peak, int odd_event,
                                              QString satuan_peak, QString satuan_suhu, QVector<float> suhu_list,
                                              QStringList nama_silinder)
{

    painter->save();

    const AxisData &yAxisData = this->yAxisData();
    AxisData &xAxisData = this->xAxisData();

    painter->setRenderHint(QPainter::Antialiasing,false);

    const int xAxisSegmentCount = xAxisData.segmentCount();
    const int xAxisLineCount = xAxisSegmentCount + 1;
    const int yAxisSegmentCount = yAxisData.segmentCount();
    const int yAxisLineCount = yAxisSegmentCount + 1;

    const int fontHeight = painter->fontMetrics().height();
    const int halfFontHeight = fontHeight / 2;
    const QSizeF gridOffset = QSizeF(hPadding(gridRect), vPadding(gridRect));
    const qreal valuesHMargin = this->valuesHMargin(painter);
    const qreal vStep = gridRect.height() / yAxisSegmentCount;
    const qreal hStep = (gridRect.width() - valuesHMargin - gridOffset.width()) / xAxisSegmentCount;
    const qreal textPositionHOffset = valuesHMargin * 0.1;
    const qreal lebar_pixel = gridRect.right() - ( gridRect.left() + valuesHMargin + gridOffset.width());
    const float resolusi = lebar_pixel /  derajat_silinder;
  //   qDebug()<<"<vi1 LR Derajat="<< derajat_silinder<<"Resolusi = "<<resolusi <<" lebar pixel= "<<lebar_pixel;

    // Vertical axis lines
    const QTextOption verticalTextOption(Qt::AlignRight);

    for (int i = 0 ; i < yAxisLineCount ; i++ ) {
        const qreal y = vStep * i;
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::HorizontalLine
                                  || i == 0 || i == xAxisSegmentCount;

        QPointF lineEndPos = gridRect.bottomRight() - QPointF(0, y);

        if (!drawFullLine) {
            lineEndPos.setX(gridRect.left() + valuesHMargin + gridOffset.width());
        }
        else{
            painter->drawLine(gridRect.bottomLeft() - QPointF(-valuesHMargin- gridOffset.width(), y), lineEndPos);}
    }

//    const qreal tinggi_graph = gridRect.top()+gridRect.bottom();
//    int jumlah_data = (list_peak.size()/2);
// //   const qreal peak_step = tinggi_graph / jumlah_data;
//  //  const qreal tinggi_pakai = tinggi_graph - peak_step;
////    const qreal peak_step_pakai = tinggi_pakai/9;
////    const qreal half_pos = peak_step/2;

   const qreal tinggiA = (gridRect.top()-gridRect.bottom())/(list_peak.size()/2);
   const qreal half_tinggiA = tinggiA/2;
   //int half_table = list_peak.size()/2;
   QString value;

   for(int i=0; i<(list_peak.size()/2); i++){
       const qreal pos = (tinggiA*((list_peak.size()/2)-(i)))+gridRect.bottom()-half_tinggiA;
       value.sprintf("%.2f%s",list_peak[i],satuan_peak.toUtf8().data());
  //     qDebug()<<"ini cek:"<<value<<list_peak.size()<<list_peak.size()/2;
       QString data_suhu;
       QString sil = nama_silinder[i];
       data_suhu.sprintf("%.1f",suhu_list[i]);
       QString text = QString(nama_silinder[i]+"\n("+value+")\n"+data_suhu+satuan_suhu);

       painter->drawText(QRectF( gridRect.left()-(painter->fontMetrics().boundingRect(text).width()/4),
                                    pos - (fontHeight),
                                    painter->fontMetrics().boundingRect(text).width(),
                                    fontHeight +fontHeight +fontHeight), Qt::AlignCenter,
                                    text);
   }

    // Horizontal axis lines
    for (int i = 0 ; i < xAxisLineCount ; i++) {
        const qreal x = gridRect.left() + hStep * i + valuesHMargin + gridOffset.width();
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::VerticalLine
                                  || i == 0 || i == xAxisSegmentCount;
        const QString text = axisLabel(i, xAxisData);

        if (m_chartItem->horizontalAxisOnTop()) {
            painter->drawLine(x, gridRect.top() - gridOffset.height(),
                              x, (drawFullLine ? gridRect.bottom() : gridRect.top()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.top() - (fontHeight + gridOffset.height()),
                                     hStep, fontHeight),
                              text);
        } else {
            painter->drawLine(x, gridRect.bottom() + gridOffset.height(),
                              x, (drawFullLine ? gridRect.top() : gridRect.bottom()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.bottom() + halfFontHeight * 0 + gridOffset.height(),
                                     hStep, fontHeight),
                              text);
        }
    }
    QString offset_top = "0";
    QString text_EC = "EC\n"+QString::number(derajat_EC);
    QString text_IC = "IC\n"+QString::number(derajat_IC);
    QString text_EO = "EO\n"+QString::number(derajat_EO);
    QString text_IO = "IO\n"+QString::number(derajat_IO);

//    const qreal lebar_pixel_top = gridRect.right() - ( gridRect.left());
//    const float resolusi_top = lebar_pixel_top /  derajat_silinder;

//    const float pos_EC_top = ((float) derajat_EC * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
//    const float pos_IC_top = ((float) derajat_IC * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
//    const float pos_EO_top = ((float) derajat_EO * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
//    const float pos_IO_top = ((float) derajat_IO * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);


    const float pos_EC = ((float) derajat_EC * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_IC = ((float) derajat_IC * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_EO = ((float) derajat_EO * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_IO = ((float) derajat_IO * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());



    //top
    painter->setPen(Qt::red);
    painter->drawText(QRectF( pos_EC -painter->fontMetrics().boundingRect(text_EC).width() / 2,
                                 gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                                 painter->fontMetrics().boundingRect(text_EC).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                                 text_EC);
    painter->drawLine(pos_EC,gridRect.bottom()/*- gridOffset.height()*/,pos_EC,gridRect.top());

    //top
    painter->setPen(Qt::blue);
    painter->drawText(QRectF(pos_IC -painter->fontMetrics().boundingRect(text_IC).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_IC).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_IC);
    painter->drawLine(pos_IC,gridRect.bottom()/*- gridOffset.height()*/,pos_IC,gridRect.top() );

    //top
    painter->setPen(Qt::red);
    painter->drawText(QRectF(pos_EO -painter->fontMetrics().boundingRect(text_EO).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_EO).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_EO);
    painter->drawLine(pos_EO,gridRect.bottom()/*- gridOffset.height()*/,pos_EO,gridRect.top() );

    //top
    painter->setPen(Qt::blue);
    painter->drawText(QRectF(pos_IO -painter->fontMetrics().boundingRect(text_IO).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_IO).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_IO);
    painter->drawLine(pos_IO,gridRect.bottom()/*- gridOffset.height()*/,pos_IO,gridRect.top() );
    painter->setPen(Qt::black);

    //bottom
    //    painter->drawText(QRectF(pos_IC - painter->fontMetrics().boundingRect(QString::number(derajat_IC)).width() / 2,
    //                              gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                              hStep, fontHeight + fontHeight), Qt::AlignCenter, text_IC);

    //bottom
    //    painter->drawText(QRectF(pos_EC - painter->fontMetrics().boundingRect(QString::number(derajat_EC)).width() / 2,
    //                             gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                             hStep, fontHeight + fontHeight),Qt::AlignCenter,
    //                             text_EC);

    //    painter->drawText(QRectF(pos_EO - painter->fontMetrics().boundingRect(QString::number(derajat_EO)).width() / 2,
    //                             gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                             hStep, fontHeight + fontHeight), Qt::AlignCenter,
    //                             text_EO);

    painter->restore();
}

void AbstractSeriesChart::paintGrid_paradeIGN(QPainter *painter, QRectF gridRect,
                                             const float derajat_EC,const float derajat_IC,
                                              const float derajat_EO,const float derajat_IO,
                                              const float derajat_silinder, QVector<float> list_peak, int odd_event,
                                              QString satuan_peak, QString satuan_suhu, QVector<float> suhu_list,
                                              QStringList nama_silinder,
                                              QVector<float> peak_ign, QVector<float> derajat_ign, QString satuan_ign)
{

    painter->save();

    const AxisData &yAxisData = this->yAxisData();
    AxisData &xAxisData = this->xAxisData();

    painter->setRenderHint(QPainter::Antialiasing,false);

    const int xAxisSegmentCount = xAxisData.segmentCount();
    const int xAxisLineCount = xAxisSegmentCount + 1;
    const int yAxisSegmentCount = yAxisData.segmentCount();
    const int yAxisLineCount = yAxisSegmentCount + 1;

    const int fontHeight = painter->fontMetrics().height();
    const int halfFontHeight = fontHeight / 2;
    const QSizeF gridOffset = QSizeF(hPadding(gridRect), vPadding(gridRect));
    const qreal valuesHMargin = this->valuesHMargin(painter);
    const qreal vStep = gridRect.height() / yAxisSegmentCount;
    const qreal hStep = (gridRect.width() - valuesHMargin - gridOffset.width()) / xAxisSegmentCount;
    const qreal textPositionHOffset = valuesHMargin * 0.1;
    const qreal lebar_pixel = gridRect.right() - ( gridRect.left() + valuesHMargin + gridOffset.width());
    const float resolusi = lebar_pixel /  derajat_silinder;
  //   qDebug()<<"<vi1 LR Derajat="<< derajat_silinder<<"Resolusi = "<<resolusi <<" lebar pixel= "<<lebar_pixel;

    // Vertical axis lines
    const QTextOption verticalTextOption(Qt::AlignRight);

    for (int i = 0 ; i < yAxisLineCount ; i++ ) {
        const qreal y = vStep * i;
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::HorizontalLine
                                  || i == 0 || i == xAxisSegmentCount;

        QPointF lineEndPos = gridRect.bottomRight() - QPointF(0, y);

        if (!drawFullLine) {
            lineEndPos.setX(gridRect.left() + valuesHMargin + gridOffset.width());
        }
        else{
            painter->drawLine(gridRect.bottomLeft() - QPointF(-valuesHMargin- gridOffset.width(), y), lineEndPos);}
    }

//    const qreal tinggi_graph = gridRect.top()+gridRect.bottom();
//    int jumlah_data = (peak_ign.size()/2);
// //   const qreal peak_step = tinggi_graph / jumlah_data;
//  //  const qreal tinggi_pakai = tinggi_graph - peak_step;
////    const qreal peak_step_pakai = tinggi_pakai/9;
////    const qreal half_pos = peak_step/2;

   const qreal tinggiA = (gridRect.top()-gridRect.bottom())/(peak_ign.size()/2);
   const qreal half_tinggiA = tinggiA/2;
   //int half_table = peak_ign.size()/2;
   QString value;

   for(int i=0; i<(peak_ign.size()/2); i++){
       const qreal pos = (tinggiA*((peak_ign.size()/2)-(i)))+gridRect.bottom()-half_tinggiA;
       value.sprintf("%.2f%s",peak_ign[i],satuan_ign.toUtf8().data());
  //     qDebug()<<"ini cek:"<<value<<peak_ign.size()<<peak_ign.size()/2;
       QString data_suhu;
       QString sil = nama_silinder[i];
       data_suhu.sprintf("%.1f",derajat_ign[i]);
       QString text = QString(nama_silinder[i]+"\n("+value+")\n"+data_suhu+satuan_suhu);

       painter->drawText(QRectF( gridRect.left()-(painter->fontMetrics().boundingRect(text).width()/4),
                                    pos - (fontHeight),
                                    painter->fontMetrics().boundingRect(text).width(),
                                    fontHeight +fontHeight +fontHeight), Qt::AlignCenter,
                                    text);
   }

    // Horizontal axis lines
    for (int i = 0 ; i < xAxisLineCount ; i++) {
        const qreal x = gridRect.left() + hStep * i + valuesHMargin + gridOffset.width();
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::VerticalLine
                                  || i == 0 || i == xAxisSegmentCount;
        const QString text = axisLabel(i, xAxisData);

        if (m_chartItem->horizontalAxisOnTop()) {
            painter->drawLine(x, gridRect.top() - gridOffset.height(),
                              x, (drawFullLine ? gridRect.bottom() : gridRect.top()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.top() - (fontHeight + gridOffset.height()),
                                     hStep, fontHeight),
                              text);
        } else {
            painter->drawLine(x, gridRect.bottom() + gridOffset.height(),
                              x, (drawFullLine ? gridRect.top() : gridRect.bottom()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.bottom() + halfFontHeight * 0 + gridOffset.height(),
                                     hStep, fontHeight),
                              text);
        }
    }
    QString offset_top = "0";
    QString text_EC = "EC\n"+QString::number(derajat_EC);
    QString text_IC = "IC\n"+QString::number(derajat_IC);
    QString text_EO = "EO\n"+QString::number(derajat_EO);
    QString text_IO = "IO\n"+QString::number(derajat_IO);

//    const qreal lebar_pixel_top = gridRect.right() - ( gridRect.left());
//    const float resolusi_top = lebar_pixel_top /  derajat_silinder;

//    const float pos_EC_top = ((float) derajat_EC * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
//    const float pos_IC_top = ((float) derajat_IC * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
//    const float pos_EO_top = ((float) derajat_EO * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
//    const float pos_IO_top = ((float) derajat_IO * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);


    const float pos_EC = ((float) derajat_EC * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_IC = ((float) derajat_IC * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_EO = ((float) derajat_EO * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_IO = ((float) derajat_IO * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());



    //top
    painter->setPen(Qt::red);
    painter->drawText(QRectF( pos_EC -painter->fontMetrics().boundingRect(text_EC).width() / 2,
                                 gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                                 painter->fontMetrics().boundingRect(text_EC).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                                 text_EC);
    painter->drawLine(pos_EC,gridRect.bottom()/*- gridOffset.height()*/,pos_EC,gridRect.top());

    //top
    painter->setPen(Qt::blue);
    painter->drawText(QRectF(pos_IC -painter->fontMetrics().boundingRect(text_IC).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_IC).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_IC);
    painter->drawLine(pos_IC,gridRect.bottom()/*- gridOffset.height()*/,pos_IC,gridRect.top() );

    //top
    painter->setPen(Qt::red);
    painter->drawText(QRectF(pos_EO -painter->fontMetrics().boundingRect(text_EO).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_EO).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_EO);
    painter->drawLine(pos_EO,gridRect.bottom()/*- gridOffset.height()*/,pos_EO,gridRect.top() );

    //top
    painter->setPen(Qt::blue);
    painter->drawText(QRectF(pos_IO -painter->fontMetrics().boundingRect(text_IO).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_IO).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_IO);
    painter->drawLine(pos_IO,gridRect.bottom()/*- gridOffset.height()*/,pos_IO,gridRect.top() );
    painter->setPen(Qt::black);

    //bottom
    //    painter->drawText(QRectF(pos_IC - painter->fontMetrics().boundingRect(QString::number(derajat_IC)).width() / 2,
    //                              gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                              hStep, fontHeight + fontHeight), Qt::AlignCenter, text_IC);

    //bottom
    //    painter->drawText(QRectF(pos_EC - painter->fontMetrics().boundingRect(QString::number(derajat_EC)).width() / 2,
    //                             gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                             hStep, fontHeight + fontHeight),Qt::AlignCenter,
    //                             text_EC);

    //    painter->drawText(QRectF(pos_EO - painter->fontMetrics().boundingRect(QString::number(derajat_EO)).width() / 2,
    //                             gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                             hStep, fontHeight + fontHeight), Qt::AlignCenter,
    //                             text_EO);

    painter->restore();
}

void AbstractSeriesChart::paintGrid_paradeIGN2(QPainter *painter, QRectF gridRect,
                                             const float derajat_EC,const float derajat_IC,
                                              const float derajat_EO,const float derajat_IO,
                                              const float derajat_silinder, QVector<float> list_peak, int odd_event,
                                               QString satuan_peak, QString satuan_suhu, QVector<float> suhu_list,
                                               QStringList nama_silinder,QVector<float> peak_ign, QVector<float> derajat_ign, QString satuan_ign)
{

    painter->save();

    const AxisData &yAxisData = this->yAxisData();
    AxisData &xAxisData = this->xAxisData();

    painter->setRenderHint(QPainter::Antialiasing,false);

    const int xAxisSegmentCount = xAxisData.segmentCount();
    const int xAxisLineCount = xAxisSegmentCount + 1;
    const int yAxisSegmentCount = yAxisData.segmentCount();
    const int yAxisLineCount = yAxisSegmentCount + 1;

    const int fontHeight = painter->fontMetrics().height();
    const int halfFontHeight = fontHeight / 2;
    const QSizeF gridOffset = QSizeF(hPadding(gridRect), vPadding(gridRect));
    const qreal valuesHMargin = this->valuesHMargin(painter);
    const qreal vStep = gridRect.height() / yAxisSegmentCount;
    const qreal hStep = (gridRect.width() - valuesHMargin - gridOffset.width()) / xAxisSegmentCount;
    const qreal textPositionHOffset = valuesHMargin * 0.1;
    const qreal lebar_pixel = gridRect.right() - ( gridRect.left() + valuesHMargin + gridOffset.width());
    const float resolusi = lebar_pixel /  derajat_silinder;
    // qDebug()<<"<vi2 LR Derajat="<< derajat_silinder<<"Resolusi = "<<resolusi <<" lebar pixel= "<<lebar_pixel;

    // Vertical axis lines
    const QTextOption verticalTextOption(Qt::AlignRight);

    for (int i = 0 ; i < yAxisLineCount ; i++ ) {
        const qreal y = vStep * i;
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::HorizontalLine
                                  || i == 0 || i == xAxisSegmentCount;

        QPointF lineEndPos = gridRect.bottomRight() - QPointF(0, y);

        if (!drawFullLine) {
            lineEndPos.setX(gridRect.left() + valuesHMargin + gridOffset.width());
        }
        else{
            painter->drawLine(gridRect.bottomLeft() - QPointF(-valuesHMargin- gridOffset.width(), y), lineEndPos);}
    }

    const qreal tinggi_graph = gridRect.top()+gridRect.bottom();
    const qreal peak_step = tinggi_graph / 9;
    const qreal tinggi_pakai = tinggi_graph - peak_step;
    const qreal peak_step_pakai = tinggi_pakai/9;
    const qreal half_pos = peak_step/2;
//   for (int i = 0 ; i < 10 ; i++ ) {
//       const qreal y = peak_step_pakai * i;
////                painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset, y + halfFontHeight),
////                                                 QSizeF(valuesHMargin,fontHeight)),
////                                          "x"/*axisLabel(i, yAxisData)*/,
////                                          verticalTextOption);

//            }
//   painter->drawText(QPointF(0,0),"0");
//   painter->drawText(QPointF(0,100),"0,100");
//   painter->drawText(QPointF(0,200),"0,200");
//   painter->drawText(QPointF(100,0),"100,0");
//   painter->drawText(QPointF(200,0),"200,0");

//   painter->drawText(QPointF(gridRect.left(),gridRect.top()),"(l,t)");
//   painter->drawText(QPointF(gridRect.left(),gridRect.bottom()),"(l,b)");

   const qreal tinggiA = (gridRect.top()-gridRect.bottom())/(peak_ign.size()/2);
   const qreal half_tinggiA = tinggiA/2;
   //int half_table = peak_ign.size()/2;
   int pos_pertama = peak_ign.size()/2;
   QString value;

   for(int i=0; i<(peak_ign.size()/2); i++){
       value.sprintf("%.2f",peak_ign[i+pos_pertama]);
       //const qreal pos = (tinggiA*(i+1))+gridRect.bottom()-half_tinggiA;
       const qreal pos = (tinggiA*((peak_ign.size()/2)-i))+gridRect.bottom()-half_tinggiA;
//       if((odd_event>0) && (i==(peak_ign.size()/2)-1)){

//       }else{
        //painter->drawText(QPointF(gridRect.left(),pos),"("+value+")");
           QString data_suhu;
           data_suhu.sprintf("%.1f",derajat_ign[i+pos_pertama]);
           QString text = QString(nama_silinder[i+pos_pertama]+"\n("+value+")\n"+data_suhu+satuan_ign);

           painter->drawText(QRectF( gridRect.left()-(painter->fontMetrics().boundingRect(text).width()/4),
                                        pos - (fontHeight),
                                        painter->fontMetrics().boundingRect(text).width(),
                                        fontHeight +fontHeight +fontHeight), Qt::AlignCenter,
                                        text);
//       }
   }

//   for(int i=1; i<10; i++){
//       const qreal pos = tinggiA*i;
//       painter->drawText(QPointF(gridRect.left()+20,pos),"0");
//   }


//   painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset,100),
//                                    QSizeF(valuesHMargin,fontHeight)),
//                             "100"/*axisLabel(i, yAxisData)*/,
//                             verticalTextOption);
//   painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset,0),
//                                    QSizeF(valuesHMargin,fontHeight)),
//                             "0"/*axisLabel(i, yAxisData)*/,
//                             verticalTextOption);

//   painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset,0),
//                                    QSizeF(valuesHMargin,fontHeight)),
//                             "0"/*axisLabel(i, yAxisData)*/,
//                             verticalTextOption);


    // Horizontal axis lines
    for (int i = 0 ; i < xAxisLineCount ; i++) {
        const qreal x = gridRect.left() + hStep * i + valuesHMargin + gridOffset.width();
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::VerticalLine
                                  || i == 0 || i == xAxisSegmentCount;
        const QString text = axisLabel(i, xAxisData);

        if (m_chartItem->horizontalAxisOnTop()) {
            painter->drawLine(x, gridRect.top() - gridOffset.height(),
                              x, (drawFullLine ? gridRect.bottom() : gridRect.top()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.top() - (fontHeight + gridOffset.height()),
                                     hStep, fontHeight),
                              text);
        } else {
            painter->drawLine(x, gridRect.bottom() + gridOffset.height(),
                              x, (drawFullLine ? gridRect.top() : gridRect.bottom()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.bottom() + halfFontHeight * 0 + gridOffset.height(),
                                     hStep, fontHeight),
                              text);
        }
    }
    QString offset_top = "0";
    QString text_EC = "EC\n"+QString::number(derajat_EC);
    QString text_IC = "IC\n"+QString::number(derajat_IC);
    QString text_EO = "EO\n"+QString::number(derajat_EO);
    QString text_IO = "IO\n"+QString::number(derajat_IO);

    const qreal lebar_pixel_top = gridRect.right() - ( gridRect.left());
    const float resolusi_top = lebar_pixel_top /  derajat_silinder;

    const float pos_EC_top = ((float) derajat_EC * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
    const float pos_IC_top = ((float) derajat_IC * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
    const float pos_EO_top = ((float) derajat_EO * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
    const float pos_IO_top = ((float) derajat_IO * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);


    const float pos_EC = ((float) derajat_EC * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_IC = ((float) derajat_IC * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_EO = ((float) derajat_EO * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_IO = ((float) derajat_IO * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());



    //top
    painter->setPen(Qt::red);
    painter->drawText(QRectF( pos_EC -painter->fontMetrics().boundingRect(text_EC).width() / 2,
                                 gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                                 painter->fontMetrics().boundingRect(text_EC).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                                 text_EC);
    painter->drawLine(pos_EC,gridRect.bottom()/*- gridOffset.height()*/,pos_EC,gridRect.top());

    //top
    painter->setPen(Qt::blue);
    painter->drawText(QRectF(pos_IC -painter->fontMetrics().boundingRect(text_IC).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_IC).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_IC);
    painter->drawLine(pos_IC,gridRect.bottom()/*- gridOffset.height()*/,pos_IC,gridRect.top() );

    //top
    painter->setPen(Qt::red);
    painter->drawText(QRectF(pos_EO -painter->fontMetrics().boundingRect(text_EO).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_EO).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_EO);
    painter->drawLine(pos_EO,gridRect.bottom()/*- gridOffset.height()*/,pos_EO,gridRect.top() );

    //top
    painter->setPen(Qt::blue);
    painter->drawText(QRectF(pos_IO -painter->fontMetrics().boundingRect(text_IO).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_IO).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_IO);
    painter->drawLine(pos_IO,gridRect.bottom()/*- gridOffset.height()*/,pos_IO,gridRect.top() );
    painter->setPen(Qt::black);

    //bottom
    //    painter->drawText(QRectF(pos_IC - painter->fontMetrics().boundingRect(QString::number(derajat_IC)).width() / 2,
    //                              gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                              hStep, fontHeight + fontHeight), Qt::AlignCenter, text_IC);

    //bottom
    //    painter->drawText(QRectF(pos_EC - painter->fontMetrics().boundingRect(QString::number(derajat_EC)).width() / 2,
    //                             gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                             hStep, fontHeight + fontHeight),Qt::AlignCenter,
    //                             text_EC);

    //    painter->drawText(QRectF(pos_EO - painter->fontMetrics().boundingRect(QString::number(derajat_EO)).width() / 2,
    //                             gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                             hStep, fontHeight + fontHeight), Qt::AlignCenter,
    //                             text_EO);

    painter->restore();
}


void AbstractSeriesChart::paintGrid_paradeVib2(QPainter *painter, QRectF gridRect,
                                             const float derajat_EC,const float derajat_IC,
                                              const float derajat_EO,const float derajat_IO,
                                              const float derajat_silinder, QVector<float> list_peak, int odd_event,
                                               QString satuan_peak, QString satuan_suhu, QVector<float> suhu_list,
                                               QStringList nama_silinder)
{

    painter->save();

    const AxisData &yAxisData = this->yAxisData();
    AxisData &xAxisData = this->xAxisData();

    painter->setRenderHint(QPainter::Antialiasing,false);

    const int xAxisSegmentCount = xAxisData.segmentCount();
    const int xAxisLineCount = xAxisSegmentCount + 1;
    const int yAxisSegmentCount = yAxisData.segmentCount();
    const int yAxisLineCount = yAxisSegmentCount + 1;

    const int fontHeight = painter->fontMetrics().height();
    const int halfFontHeight = fontHeight / 2;
    const QSizeF gridOffset = QSizeF(hPadding(gridRect), vPadding(gridRect));
    const qreal valuesHMargin = this->valuesHMargin(painter);
    const qreal vStep = gridRect.height() / yAxisSegmentCount;
    const qreal hStep = (gridRect.width() - valuesHMargin - gridOffset.width()) / xAxisSegmentCount;
    const qreal textPositionHOffset = valuesHMargin * 0.1;
    const qreal lebar_pixel = gridRect.right() - ( gridRect.left() + valuesHMargin + gridOffset.width());
    const float resolusi = lebar_pixel /  derajat_silinder;
    // qDebug()<<"<vi2 LR Derajat="<< derajat_silinder<<"Resolusi = "<<resolusi <<" lebar pixel= "<<lebar_pixel;

    // Vertical axis lines
    const QTextOption verticalTextOption(Qt::AlignRight);

    for (int i = 0 ; i < yAxisLineCount ; i++ ) {
        const qreal y = vStep * i;
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::HorizontalLine
                                  || i == 0 || i == xAxisSegmentCount;

        QPointF lineEndPos = gridRect.bottomRight() - QPointF(0, y);

        if (!drawFullLine) {
            lineEndPos.setX(gridRect.left() + valuesHMargin + gridOffset.width());
        }
        else{
            painter->drawLine(gridRect.bottomLeft() - QPointF(-valuesHMargin- gridOffset.width(), y), lineEndPos);}
    }

    const qreal tinggi_graph = gridRect.top()+gridRect.bottom();
    const qreal peak_step = tinggi_graph / 9;
    const qreal tinggi_pakai = tinggi_graph - peak_step;
    const qreal peak_step_pakai = tinggi_pakai/9;
    const qreal half_pos = peak_step/2;
//   for (int i = 0 ; i < 10 ; i++ ) {
//       const qreal y = peak_step_pakai * i;
////                painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset, y + halfFontHeight),
////                                                 QSizeF(valuesHMargin,fontHeight)),
////                                          "x"/*axisLabel(i, yAxisData)*/,
////                                          verticalTextOption);

//            }
//   painter->drawText(QPointF(0,0),"0");
//   painter->drawText(QPointF(0,100),"0,100");
//   painter->drawText(QPointF(0,200),"0,200");
//   painter->drawText(QPointF(100,0),"100,0");
//   painter->drawText(QPointF(200,0),"200,0");

//   painter->drawText(QPointF(gridRect.left(),gridRect.top()),"(l,t)");
//   painter->drawText(QPointF(gridRect.left(),gridRect.bottom()),"(l,b)");

   const qreal tinggiA = (gridRect.top()-gridRect.bottom())/(list_peak.size()/2);
   const qreal half_tinggiA = tinggiA/2;
   //int half_table = list_peak.size()/2;
   int pos_pertama = list_peak.size()/2;
   QString value;

   for(int i=0; i<(list_peak.size()/2); i++){
       value.sprintf("%.2f",list_peak[i+pos_pertama]);
       //const qreal pos = (tinggiA*(i+1))+gridRect.bottom()-half_tinggiA;
       const qreal pos = (tinggiA*((list_peak.size()/2)-i))+gridRect.bottom()-half_tinggiA;
       if((odd_event>0) && (i==(list_peak.size()/2)-1)){

       }else{
        //painter->drawText(QPointF(gridRect.left(),pos),"("+value+")");
           QString data_suhu;
           data_suhu.sprintf("%.1f",suhu_list[i+pos_pertama]);
           QString text = QString(nama_silinder[i+pos_pertama]+"\n("+value+")\n"+data_suhu+satuan_suhu);

           painter->drawText(QRectF( gridRect.left()-(painter->fontMetrics().boundingRect(text).width()/4),
                                        pos - (fontHeight),
                                        painter->fontMetrics().boundingRect(text).width(),
                                        fontHeight +fontHeight +fontHeight), Qt::AlignCenter,
                                        text);
       }
   }

//   for(int i=1; i<10; i++){
//       const qreal pos = tinggiA*i;
//       painter->drawText(QPointF(gridRect.left()+20,pos),"0");
//   }


//   painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset,100),
//                                    QSizeF(valuesHMargin,fontHeight)),
//                             "100"/*axisLabel(i, yAxisData)*/,
//                             verticalTextOption);
//   painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset,0),
//                                    QSizeF(valuesHMargin,fontHeight)),
//                             "0"/*axisLabel(i, yAxisData)*/,
//                             verticalTextOption);

//   painter->drawText(QRectF(gridRect.bottomLeft()-QPointF(textPositionHOffset,0),
//                                    QSizeF(valuesHMargin,fontHeight)),
//                             "0"/*axisLabel(i, yAxisData)*/,
//                             verticalTextOption);


    // Horizontal axis lines
    for (int i = 0 ; i < xAxisLineCount ; i++) {
        const qreal x = gridRect.left() + hStep * i + valuesHMargin + gridOffset.width();
        const bool drawFullLine = m_chartItem->gridChartLines() & ChartItem::VerticalLine
                                  || i == 0 || i == xAxisSegmentCount;
        const QString text = axisLabel(i, xAxisData);

        if (m_chartItem->horizontalAxisOnTop()) {
            painter->drawLine(x, gridRect.top() - gridOffset.height(),
                              x, (drawFullLine ? gridRect.bottom() : gridRect.top()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.top() - (fontHeight + gridOffset.height()),
                                     hStep, fontHeight),
                              text);
        } else {
            painter->drawLine(x, gridRect.bottom() + gridOffset.height(),
                              x, (drawFullLine ? gridRect.top() : gridRect.bottom()));
            painter->drawText(QRectF(x - painter->fontMetrics().boundingRect(text).width() / 2,
                                     gridRect.bottom() + halfFontHeight * 0 + gridOffset.height(),
                                     hStep, fontHeight),
                              text);
        }
    }
    QString offset_top = "0";
    QString text_EC = "EC\n"+QString::number(derajat_EC);
    QString text_IC = "IC\n"+QString::number(derajat_IC);
    QString text_EO = "EO\n"+QString::number(derajat_EO);
    QString text_IO = "IO\n"+QString::number(derajat_IO);

    const qreal lebar_pixel_top = gridRect.right() - ( gridRect.left());
    const float resolusi_top = lebar_pixel_top /  derajat_silinder;

    const float pos_EC_top = ((float) derajat_EC * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
    const float pos_IC_top = ((float) derajat_IC * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
    const float pos_EO_top = ((float) derajat_EO * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);
    const float pos_IO_top = ((float) derajat_IO * resolusi_top) + ((float) gridRect.left() /*+ 5 + (float) gridOffset.width()*/);


    const float pos_EC = ((float) derajat_EC * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_IC = ((float) derajat_IC * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_EO = ((float) derajat_EO * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());
    const float pos_IO = ((float) derajat_IO * resolusi) + ((float) gridRect.left() + (float) valuesHMargin + (float) gridOffset.width());



    //top
    painter->setPen(Qt::red);
    painter->drawText(QRectF( pos_EC -painter->fontMetrics().boundingRect(text_EC).width() / 2,
                                 gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                                 painter->fontMetrics().boundingRect(text_EC).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                                 text_EC);
    painter->drawLine(pos_EC,gridRect.bottom()/*- gridOffset.height()*/,pos_EC,gridRect.top());

    //top
    painter->setPen(Qt::blue);
    painter->drawText(QRectF(pos_IC -painter->fontMetrics().boundingRect(text_IC).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_IC).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_IC);
    painter->drawLine(pos_IC,gridRect.bottom()/*- gridOffset.height()*/,pos_IC,gridRect.top() );

    //top
    painter->setPen(Qt::red);
    painter->drawText(QRectF(pos_EO -painter->fontMetrics().boundingRect(text_EO).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_EO).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_EO);
    painter->drawLine(pos_EO,gridRect.bottom()/*- gridOffset.height()*/,pos_EO,gridRect.top() );

    //top
    painter->setPen(Qt::blue);
    painter->drawText(QRectF(pos_IO -painter->fontMetrics().boundingRect(text_IO).width() / 2,
                             gridRect.top() - (fontHeight +fontHeight + gridOffset.height()),
                             painter->fontMetrics().boundingRect(text_IO).width(),  fontHeight +fontHeight), Qt::AlignCenter,
                             text_IO);
    painter->drawLine(pos_IO,gridRect.bottom()/*- gridOffset.height()*/,pos_IO,gridRect.top() );
    painter->setPen(Qt::black);

    //bottom
    //    painter->drawText(QRectF(pos_IC - painter->fontMetrics().boundingRect(QString::number(derajat_IC)).width() / 2,
    //                              gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                              hStep, fontHeight + fontHeight), Qt::AlignCenter, text_IC);

    //bottom
    //    painter->drawText(QRectF(pos_EC - painter->fontMetrics().boundingRect(QString::number(derajat_EC)).width() / 2,
    //                             gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                             hStep, fontHeight + fontHeight),Qt::AlignCenter,
    //                             text_EC);

    //    painter->drawText(QRectF(pos_EO - painter->fontMetrics().boundingRect(QString::number(derajat_EO)).width() / 2,
    //                             gridRect.bottom() + halfFontHeight * 0 + gridOffset.height() + fontHeight,
    //                             hStep, fontHeight + fontHeight), Qt::AlignCenter,
    //                             text_EO);

    painter->restore();
}



void AbstractSeriesChart::drawSegment(QPainter *painter, QPoint startPoint, QPoint endPoint, QColor color)
{
    int radius = m_chartItem->seriesLineWidth();
    QPen pen(color);
    pen.setWidth(radius);
    painter->setPen(pen);
    painter->drawLine(startPoint, endPoint);
    if (!m_chartItem->drawPoints()) {
        return;
    }
    QRect startPointRect(startPoint,startPoint);
    QRect endPointRect(endPoint,endPoint);
    painter->setBrush(color);
    painter->drawEllipse(startPointRect.adjusted(radius,radius,-radius,-radius));
    painter->drawEllipse(endPointRect.adjusted(radius,radius,-radius,-radius));
}

qreal AbstractSeriesChart::valuesHMargin(QPainter *painter)
{
    qreal max = 0;
    const AxisData &yAxisData = this->yAxisData();
    const int offset = 4;
    const int yAxisLineCount = yAxisData.segmentCount() + 1;

    for (int i = 0 ; i < yAxisLineCount ; i++) {
        const QString label =  axisLabel(i, yAxisData);
        max = std::max(max, (qreal)painter->fontMetrics().boundingRect(label).width()+offset);
    }
    return max;
}

qreal AbstractSeriesChart::valuesVMargin(QPainter *painter)
{
    return painter->fontMetrics().height();
}

QFont AbstractSeriesChart::adaptLabelsFont(QRectF rect, QFont font)
{
    QString maxWord;
    QFontMetrics fm(font);

    foreach(QString label, m_chartItem->labels()){
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 1))
        foreach (QString currentWord, label.split(QRegExp("\\W+"))){
#else
        foreach (QString currentWord, label.split(QRegularExpression("\\W+"))){
#endif
            if (fm.boundingRect(maxWord).width() < fm.boundingRect(currentWord).width()) maxWord = currentWord;
        }
    }

    qreal curWidth = fm.boundingRect(maxWord).width();
    QFont tmpFont = font;
    while (curWidth>rect.width() && tmpFont.pixelSize() > 1){
        tmpFont.setPixelSize(tmpFont.pixelSize() - 1);
        QFontMetricsF tmpFM(tmpFont);
        curWidth = tmpFM.boundingRect(maxWord).width();
    }
    return tmpFont;
}

QFont AbstractSeriesChart::adaptFont(qreal width, QFont font, const AxisData &axisData)
{
    QFont tmpFont = font;
    const int axisLineCount = axisData.segmentCount() + 1;
    QScopedPointer<QFontMetricsF> fm(new QFontMetricsF(tmpFont));
    for (int i = 0 ; i < axisLineCount ; i++) {
        QString strValue = axisLabel(i, axisData);
        qreal curWidth = fm->boundingRect(strValue).width();
        while (curWidth > width && tmpFont.pixelSize() > 1){
            tmpFont.setPixelSize(tmpFont.pixelSize() - 1);
            fm.reset(new QFontMetricsF(tmpFont));
            curWidth = fm->boundingRect(strValue).width();
        }
    }
    return tmpFont;
}

QString AbstractSeriesChart::axisLabel(int i, const AxisData &axisData)
{
    const qreal min = axisData.rangeMin();
    const qreal step = axisData.step();
    qreal value = 0;
    if (axisData.type() == AxisData::YAxis && axisData.reverseDirection() && min >= 0) {
        value = min + (axisData.segmentCount() - i) * step;
    } else {
        value = min + i * step;
    }
    if (std::floor(step) == step) {
        return QString::number(value);
    }
    // For float round numbers to small precision
    return QString::number(round(value * 100.0) / 100.0);
}

bool AbstractSeriesChart::calculateLegendColumnWidths(qreal indicatorWidth, qreal maxWidth, const QFontMetrics &fm)
{
    // This method is called in the loop, because to handle case when we get
    // 3 small series names in first row and then in second row small name and long name.
    // In this case we need to set maximum column count to 2 and iterate from the start to recalculate
    // all the sizes
    qreal currentRowWidth = 0;
    int currentColumn = 0;
    // During first iteration it is updated when moving to second row
    // After first iteration some column width are already calculated and are set as max,
    // because all rows need to have same column count (except last one)
    int maxColumnCount = m_legendColumnWidths.size();
    if (m_chartItem->series().isEmpty()) {
        for (int i=0 ; i < m_designLabels.size() ; ++i) {
            const qreal itemWidth = (qreal)(fm.boundingRect(m_designLabels[i]).width()) + indicatorWidth;
            if (!calculateLegendSingleColumnWidth(currentRowWidth, currentColumn, maxColumnCount, itemWidth, maxWidth)) {
                return false;
            }
        }
    } else {
        for (int i = 0 ; i < m_chartItem->series().size() ; ++i) {
            SeriesItem* series = m_chartItem->series().at(i);
            const qreal itemWidth = (qreal)(fm.boundingRect(series->name()).width()) + indicatorWidth;
            if (!calculateLegendSingleColumnWidth(currentRowWidth, currentColumn, maxColumnCount, itemWidth, maxWidth)) {
                return false;
            }
        }
    }
    return true;
}

bool AbstractSeriesChart::calculateLegendSingleColumnWidth(qreal &currentRowWidth, int &currentColumn, int &maxColumnCount,
                                                           const qreal itemWidth, const qreal maxRowWidth)
{
    const bool maxColumnCountDefined = maxColumnCount > 0;
    // Check if there is enough space for current item in the row
    const bool isEnoughSpaceInRowForItem = currentRowWidth + itemWidth > maxRowWidth;
    // Check if it is last column already
    const bool lastColumnReached = (maxColumnCountDefined && currentColumn >= maxColumnCount);
    if (isEnoughSpaceInRowForItem || lastColumnReached) {
        // Move to next row
        currentColumn = 0;
        // Set column count when moving to second row (next rows cannot have more columns)
        if (!maxColumnCountDefined) {
            maxColumnCount = currentColumn + 1;
        }
        currentRowWidth = itemWidth;
    } else {
        // Add next column in the row
        currentRowWidth += itemWidth;
    }

    // Add new column or update already existing column width
    if (currentColumn >= m_legendColumnWidths.size()) {
        // Append new column
        m_legendColumnWidths.append(itemWidth);
    } else if (m_legendColumnWidths.at(currentColumn) < itemWidth) {
        // Update size if item in column is bigger than items in same column in previous rows
        m_legendColumnWidths[currentColumn] = itemWidth;
        // After any updating column size we must recheck if all columns fit in the max row width
        qreal rowWidth = itemWidth;
        for (int c = 1 ; c < m_legendColumnWidths.size() ; c++) {
            rowWidth += m_legendColumnWidths.at(c);
            // When column widths exceed max row width remove columns at the end
            if (rowWidth > maxRowWidth) {
                m_legendColumnWidths.remove(c, m_legendColumnWidths.size() - c);
                break;
            }
        }
        // Return back and re-iterate from start to make sure everything fits
        return false;
    }
    ++currentColumn;
    return true;
}

QVector<qreal> AbstractChart::legendColumnWidths() const
{
    return m_legendColumnWidths;
}

void AbstractBarChart::paintChartLegend(QPainter *painter, QRectF legendRect, QVector<float> timming)
{
    counter+=1;
    prepareLegendToPaint(legendRect, painter);
    painter->setPen(Qt::black);
    painter->setRenderHint(QPainter::Antialiasing,false);
    if (m_chartItem->drawLegendBorder())
        painter->drawRect(legendRect);
    painter->setRenderHint(QPainter::Antialiasing,true);

    const qreal halfFontSize = painter->fontMetrics().height() / 2;
    int indicatorSize = halfFontSize;
    const QRectF indicatorsRect = legendRect.adjusted(halfFontSize, halfFontSize, 0, 0);

    bool isHorizontal = false;
    switch(m_chartItem->legendAlign()) {
    case ChartItem::LegendAlignBottomLeft:
    case ChartItem::LegendAlignBottomCenter:
    case ChartItem::LegendAlignBottomRight:
        isHorizontal = true;
        break;
    default:
        isHorizontal = false;
        break;
    }

    if (!m_chartItem->series().isEmpty()){
        for (int i = 0 ; i < m_chartItem->series().size() ; ++i) {
            SeriesItem* series = m_chartItem->series().at(i);

                if (isHorizontal) {
                    drawHorizontalLegendItem(painter, i, series->name(), indicatorSize, indicatorsRect, series->color());
                } else {
                    drawVerticalLegendItem(painter, i, series->name(), indicatorSize, indicatorsRect, series->color());
                }

        }
    } else if (m_chartItem->itemMode() == DesignMode) {
        for (int i = 0 ; i < m_designLabels.size() ; ++i){
            if (isHorizontal) {
                drawHorizontalLegendItem(painter, i, m_designLabels.at(i), indicatorSize, indicatorsRect, color_map[i]);
            } else {
                drawVerticalLegendItem(painter, i, m_designLabels.at(i), indicatorSize, indicatorsRect, color_map[i]);
            }
        }

    }
}

QRectF AbstractBarChart::verticalLabelsRect(QPainter *painter, QRectF labelsRect)
{
    qreal maxWidth = 0;

    foreach (QString label, m_chartItem->labels()) {
        if (painter->fontMetrics().boundingRect(label).width()>maxWidth)
            maxWidth = painter->fontMetrics().boundingRect(label).width();
    }

    if (maxWidth + hPadding(m_chartItem->rect()) * 2 < labelsRect.width())
        return labelsRect;
    else
        return labelsRect.adjusted(0, 0, -(labelsRect.width() - (maxWidth + hPadding(m_chartItem->rect()) * 2)), 0);
}

QRectF AbstractBarChart::horizontalLabelsRect(QPainter *painter, QRectF labelsRect)
{
    qreal maxWidth = 0;

    foreach (QString label, m_chartItem->labels()) {
        if (painter->fontMetrics().boundingRect(label).width()>maxWidth)
            maxWidth = painter->fontMetrics().boundingRect(label).width();
    }

    if ((maxWidth + vPadding(m_chartItem->rect()) < labelsRect.height()) || !verticalLabels(painter, labelsRect))
        return labelsRect;
    else
        return labelsRect.adjusted(0, (labelsRect.height() - maxWidth), 0, 0);
}

void AbstractBarChart::drawVerticalLegendItem(QPainter *painter, int i, const QString &text, int indicatorSize,
                                              const QRectF &indicatorsRect, const QColor &indicatorColor)
{
    const qreal y = i * painter->fontMetrics().height();
    painter->drawText(indicatorsRect.adjusted(indicatorSize+indicatorSize * 1.5, y, 0, 0),text);
//    qDebug()<<"Vertic ChartLegend:"<<text <<i ;
//    if(i%2)qDebug()<<"B"<<i;

    switch(m_chartItem->legendStyle()) {
    case ChartItem::LegendPoints: {
        painter->setBrush(indicatorColor);
        painter->drawEllipse(
            indicatorsRect.adjusted(
                0,
                y+indicatorSize/2,
                -(indicatorsRect.width()-indicatorSize),
                -(indicatorsRect.height()-(y+indicatorSize+indicatorSize/2))
                )
            );
        break;
    }
    case ChartItem::LegendLines: {
        const QPen tmpPen = painter->pen();
        QPen indicatorPen(indicatorColor);
        indicatorPen.setWidth(4);
        painter->setPen(indicatorPen);
        const QPointF linePos = QPointF(indicatorsRect.left(), indicatorsRect.top() + y + painter->fontMetrics().height()/2);
        painter->drawLine(linePos, linePos + QPointF(indicatorSize, 0));
        painter->setPen(tmpPen);
        break;
    }
    }
}

void AbstractBarChart::drawHorizontalLegendItem(QPainter *painter, int i, const QString &text,
                                                int indicatorSize, const QRectF &indicatorsRect, const QColor &indicatorColor)
{
    const QVector<qreal> &columnWidths = legendColumnWidths();
    if (columnWidths.isEmpty())
        return;
    const int column = i % columnWidths.size();
    const int row = std::floor(i / columnWidths.size());

    const qreal halfTextSize = painter->fontMetrics().height() / 2;

    const qreal x = indicatorsRect.x() + std::accumulate(columnWidths.cbegin(), columnWidths.cbegin() + column, 0.0);
    const qreal y = indicatorsRect.y() + (row + 1) * painter->fontMetrics().height();

//    painter->drawText(QPointF(x + indicatorSize * 1.5, y), text);

//    qDebug()<<"Horiz ChartLegend:"<<text <<i ;
//    if(i%2)qDebug()<<"A"<<i;

    switch(m_chartItem->legendStyle()) {
    case ChartItem::LegendPoints: {
        painter->setBrush(indicatorColor);
        painter->drawEllipse(x, y - halfTextSize, indicatorSize, indicatorSize);
        painter->drawText(QPointF(x + indicatorSize * 1.5, y), text);
        break;
    }
    case ChartItem::LegendLines: {
        const QPen tmpPen = painter->pen();
        QPen indicatorPen(indicatorColor);
        indicatorPen.setWidth(4);
        painter->setPen(indicatorPen);
        painter->drawLine(x, y - halfTextSize * 0.7, x + indicatorSize, y - halfTextSize * 0.7);
        painter->setPen(tmpPen);
        painter->drawText(QPointF(x + indicatorSize * 1.5, y), text);
        break;
    }
    case ChartItem::LegendVibRecip: {
        if(i%2){
            //masih belum nemu hitungannya harusnya sisa nya masih banyak ke kanan
//            const int columns = i-1 % columnWidths.size();
//            const int rows = std::floor(i / columnWidths.size());
            const qreal xs = indicatorsRect.x() + std::accumulate(columnWidths.cbegin(), columnWidths.cbegin() + (column-(column/2)), 0.0);
            const qreal ys = indicatorsRect.y() + (row + 1) * painter->fontMetrics().height();
           // qDebug()<<"pos:"<<i<<"col:"<<columns<<"row:"<<rows<<" x:" <<xs <<"y:"<< ys<<"==="<<columnWidths.size()<<"|"<<indicatorsRect.x()<<indicatorsRect.x();
            painter->setBrush(indicatorColor);
            painter->drawEllipse(xs, ys - halfTextSize, indicatorSize, indicatorSize);
            painter->drawText(QPointF(xs + indicatorSize * 1.5, ys), text);
        }
        else{

        }
        break;
    }

    }
}

} // namespace LimeReport
