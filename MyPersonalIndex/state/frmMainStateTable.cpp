#include "frmMainStateTable.h"
#include <QComboBox>
#include "frmSort.h"
#include "frmColumns.h"
#include "frmMainTableView_UI.h"
#include "mpiViewModelBase.h"
#include <QMessageBox>
#include "frmMain.h"
#ifdef CLOCKTIME
#include <QTime>
#endif

class frmMain;

frmMainStateTable::frmMainStateTable(const portfolio &portfolio_, const settings &settings_, QWidget *parent_):
    frmMainStateToolbar(portfolio_, parent_),
    m_settings(settings_)
{
}

QWidget* frmMainStateTable::mainWidget()
{
    return ui->widget;
}

frmMainTableView_UI* frmMainStateTable::createUI()
{
    return new frmMainTableView_UI();
}

frmMainToolbar_UI* frmMainStateTable::toolbarUI()
{
    return ui;
}

void frmMainStateTable::cellSelected(const QModelIndex & index)
{
    frmMain* main = (frmMain*)this->parent();
    mpiViewModelBase* temp = (mpiViewModelBase*)index.model();
    QVariant value = temp->data(index, Qt::UserRole);
    main->editPortfolioTrading(value.toString());
}

void frmMainStateTable::setupUI(bool hasRowLabels_)
{
    ui = createUI();
    ui->setupUI(tableColumns(), hasRowLabels_, static_cast<QWidget*>(this->parent()));

    ui->toolbarDateBeginEdit->setDate(QDate::fromJulianDay(m_portfolio.startDate()));
    ui->toolbarDateEndEdit->setDate(QDate::fromJulianDay(m_portfolio.endDate()));
    setSortDropDown();

    connect(ui->toolbarDateBeginEdit, SIGNAL(dateChanged(QDate)), this, SLOT(refreshTab()));
    connect(ui->toolbarDateEndEdit, SIGNAL(dateChanged(QDate)), this, SLOT(refreshTab()));
    connect(ui->toolbarSortCmb, SIGNAL(activated(int)), this, SLOT(sortIndexChanged(int)));
    connect(ui->toolbarReorder, SIGNAL(triggered()), this, SLOT(modifyColumns()));
    connect(ui->toolbarExport, SIGNAL(triggered()), ui->table, SLOT(exportTable()));
    connect(ui->tableCopy, SIGNAL(activated()), ui->table, SLOT(copyTable()));
	connect(ui->toolbar1D, SIGNAL(triggered()), this, SLOT(dateClicked()));
    connect(ui->toolbar3M, SIGNAL(triggered()), this, SLOT(dateClicked()));
    connect(ui->toolbar6M, SIGNAL(triggered()), this, SLOT(dateClicked()));
    connect(ui->toolbarYTD, SIGNAL(triggered()), this, SLOT(dateClicked()));
    connect(ui->toolbar1Y, SIGNAL(triggered()), this, SLOT(dateClicked()));
    connect(ui->toolbar5Y, SIGNAL(triggered()), this, SLOT(dateClicked()));
    connect(ui->toolbarMax, SIGNAL(triggered()), this, SLOT(dateClicked()));
    refreshTab();

    connect(ui->table,SIGNAL(doubleClicked(const QModelIndex &)),this,SLOT(cellSelected(const QModelIndex &)));

}

frmMainStateTable::~frmMainStateTable()
{
    delete ui;
}

void frmMainStateTable::refreshTab()
{
#ifdef CLOCKTIME
    QTime t;
    t.start();
#endif
    //ui->table->setVisible(false);
    QAbstractItemModel *model = ui->table->model();
    ui->table->setModel(createModel(ui->toolbarDateBeginEdit->date().toJulianDay(), ui->toolbarDateEndEdit->date().toJulianDay()));
    delete model;
    //ui->table->resizeColumnsToContents();
   // ui->table->setVisible(true);

#ifdef CLOCKTIME
    qDebug("Time elapsed: %d ms (load tab)", t.elapsed());
#endif
}

void frmMainStateTable::setSortDropDown()
{
    QList<orderBy> sort = m_settings.viewableColumnsSorting(columnEnumValue());
    ui->toolbarSortCmb->blockSignals(true);

    if (sort.isEmpty()) // no sort
        ui->toolbarSortCmb->setCurrentIndex(0);
    else if (sort.at(0).direction == orderBy::order_descending || sort.count() > 1) // custom sort
        ui->toolbarSortCmb->setCurrentIndex(ui->toolbarSortCmb->count() - 1);
    else
        ui->toolbarSortCmb->setCurrentIndex(ui->toolbarSortCmb->findData(sort.at(0).column));

    ui->toolbarSortCmb->blockSignals(false);
}

void frmMainStateTable::sortIndexChanged(int index_)
{
    int columnID = ui->toolbarSortCmb->itemData(index_).toInt();
    switch(columnID)
    {
    case -1:
        m_settings.setViewableColumnsSorting(columnEnumValue(), QList<orderBy>());
        break;
    case -2:
        {
            frmSort f(m_settings.viewableColumnsSorting(columnEnumValue()), tableColumns(), static_cast<QWidget*>(this->parent()));
            if (f.exec())
                 m_settings.setViewableColumnsSorting(columnEnumValue(), f.getReturnValues());
            else
            {
                setSortDropDown();
                return;
            }
        }
        break;
    default:
        m_settings.setViewableColumnsSorting(columnEnumValue(), QList<orderBy>() << orderBy(columnID, orderBy::order_ascending));
        break;
    }
    setSortDropDown();
    static_cast<mpiViewModelBase*>(ui->table->model())->setColumnSort(m_settings.viewableColumnsSorting(columnEnumValue()));
}

void frmMainStateTable::modifyColumns()
{
    frmColumns f(m_settings.viewableColumns(columnEnumValue()), tableColumns(), static_cast<QWidget*>(this->parent()));
    if (!f.exec())
        return;

    m_settings.setViewableColumns(columnEnumValue(), f.getReturnValues());
    static_cast<mpiViewModelBase*>(ui->table->model())->setViewableColumns(m_settings.viewableColumns(columnEnumValue()));
}
