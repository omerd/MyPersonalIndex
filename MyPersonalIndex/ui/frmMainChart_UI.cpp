#include "frmMainChart_UI.h"
#include <QHeaderView>
#include <qwt_plot_layout.h>
#include <qwt_legend.h>
#include "mpiDateScale.h"
#include "mpiPercentScale.h"

void frmMainChart_UI::setupUI(QWidget *parent_)
{
    frmMainTableViewTree_UI::setupUI(QMap<int, QString>(), false, parent_);
    delete frmMainTableViewTree_UI::tableCopy;
    delete frmMainTableViewTree_UI::table;

    chart = new mpiChart(widget);
    chart->setAxisScaleDraw(QwtPlot::xBottom, new mpiDateScale());
    chart->setAxisScaleDraw(QwtPlot::yLeft, new mpiPercentScale());
    chart->plotLayout()->setCanvasMargin(20);
    //chart->plotLayout()->setMargin(20);
    QwtPlotCanvas *temp_canvas = qobject_cast<QwtPlotCanvas*>(chart->canvas());
    chartZoomer = new mpiPlotZoomer(temp_canvas);
    chartLegend = new QwtLegend(chart);
    chart->insertLegend(chartLegend, QwtPlot::RightLegend);

    QLinearGradient grad(0, 0, 1, 1);
    grad.setCoordinateMode(QGradient::StretchToDeviceMode);
    grad.setColorAt(0, Qt::white);
    grad.setColorAt(1, QColor(220, 220, 220));

    QPalette palette(chart->canvas()->palette());
    palette.setBrush(QPalette::Window, QBrush(grad));
    chart->canvas()->setPalette(palette);

    palette = chart->palette();
    palette.setColor(QPalette::Window, Qt::white);
    chart->setPalette(palette);
    chart->setAutoFillBackground(true);

    QFont cFont = chart->axisFont(QwtPlot::xBottom);
    cFont.setBold(true);
    cFont.setPointSize(cFont.pointSize() + 3);

    QwtText xAxis("Date");
    xAxis.setFont(cFont);
    chart->setAxisTitle(QwtPlot::xBottom, xAxis);
    QwtText yAxis("Percent");
    yAxis.setFont(cFont);
    chart->setAxisTitle(QwtPlot::yLeft, yAxis);

    chartGridLines = new QwtPlotGrid();
    chartGridLines->enableX(false);
    chartGridLines->setPen(QPen(Qt::DotLine));
    chartGridLines->attach(chart);

    tableSplitter->addWidget(chart);
    tableSplitter->setStretchFactor(1, 2);
}
