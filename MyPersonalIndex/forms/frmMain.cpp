#include "frmMain.h"
#include "frmMain_UI.h"
#include <QApplication>
#include <QCoreApplication>
#include <QtConcurrentMap>
#include <QFutureWatcher>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QPushButton>
#include "frmEdit.h"
#include "settingsFactory.h"
#include "updatePrices.h"
#include "tradeDateCalendar.h"
#include "calculatorTrade.h"
#include "security.h"
#include "historicalNAV.h"
#include "fileState.h"
#include "frmMainAA_State.h"
#include "frmMainSecurity_State.h"
#include "frmMainPerformance_State.h"
#include "frmMainAcct_State.h"
#include "frmMainCorrelation_State.h"
#include "frmMainStatistic_State.h"
#include "frmMainTrade_State.h"
#include "frmMainChart_State.h"
#include "frmPortfolioImport.h"
#include "frmPriceImport.h"
#include "executedTrade.h"
#include "snapshot.h"
#include "currency.h"

#ifdef CLOCKTIME
#include <QTime>
#endif




frmMain::frmMain(const QString &filePath_, QWidget *parent_):
    QMainWindow(parent_),
    ui(new frmMain_UI()),
    m_file(new fileState(this)),
    m_currentPortfolio(-1),
    m_currentView(view_security)
{
    ui->setupUI(this);
    connectSlots();
    loadSettings();
    if (filePath_.isEmpty())
        m_file->newFile();
    else
    {
        m_file->open(filePath_);
        if (m_file->path().isEmpty())
            m_file->newFile();
    }
}

frmMain::~frmMain()
{
    qDeleteAll(m_views);

    delete ui;
    delete m_file;
}

void frmMain::connectSlots()
{
    connect(ui->helpAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->fileOpen, SIGNAL(triggered()), m_file, SLOT(open()));
    connect(ui->fileSave, SIGNAL(triggered()), m_file, SLOT(save()));
    connect(ui->fileSaveAs, SIGNAL(triggered()), m_file, SLOT(saveAs()));
    connect(ui->fileNew, SIGNAL(triggered()), m_file, SLOT(newFile()));
    connect(ui->fileExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(m_file, SIGNAL(fileNameChange(QString,bool)), this, SLOT(fileChange(QString,bool)));
    connect(ui->portfolioSwitch, SIGNAL(triggered()), this, SLOT(nextPortfolio()));
    connect(ui->portfolioAdd, SIGNAL(triggered()), this, SLOT(addPortfolio()));
    connect(ui->portfolioTabsAdd, SIGNAL(clicked()), this, SLOT(addPortfolio()));
    connect(ui->portfolioEdit, SIGNAL(triggered()), this, SLOT(editPortfolio()));
    connect(ui->portfolioTabsEdit, SIGNAL(clicked()), this, SLOT(editPortfolio()));
    connect(ui->portfolioDelete, SIGNAL(triggered()), this, SLOT(deletePortfolio()));
    connect(ui->portfolioTabsDelete, SIGNAL(clicked()), this, SLOT(deletePortfolio()));
    connect(ui->portfolioTabs, SIGNAL(currentChanged(int)), this, SLOT(portfolioTabChange(int)));
    connect(ui->portfolioTabs, SIGNAL(tabMoved(int,int)), this, SLOT(portfolioTabMoved(int,int)));
    connect(ui->priceDownload, SIGNAL(triggered()), this, SLOT(downloadPrices()));
    connect(ui->progressUpdateBtn, SIGNAL(clicked()), this, SLOT(downloadPrices()));
    connect(ui->viewAssetAllocation, SIGNAL(triggered()), this, SLOT(viewChange()));
    connect(ui->viewSecurities, SIGNAL(triggered()), this, SLOT(viewChange()));
    connect(ui->viewPerformance, SIGNAL(triggered()), this, SLOT(viewChange()));
    connect(ui->viewAccounts, SIGNAL(triggered()), this, SLOT(viewChange()));
    connect(ui->viewCorrelations, SIGNAL(triggered()), this, SLOT(viewChange()));
    connect(ui->viewStatistics, SIGNAL(triggered()), this, SLOT(viewChange()));
    connect(ui->viewCharts, SIGNAL(triggered()), this, SLOT(viewChange()));
    connect(ui->viewTrades, SIGNAL(triggered()), this, SLOT(viewChange()));
    connect(ui->portfolioTabsViewCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(viewChange(int)));
    connect(ui->portfolioImport, SIGNAL(triggered()), this, SLOT(importPortfolio()));
    connect(ui->portfolioImportFile, SIGNAL(triggered()), this, SLOT(importPortfolio()));
    connect(ui->priceImport, SIGNAL(triggered()), this, SLOT(importPrice()));
    connect(ui->priceClear, SIGNAL(triggered()), this, SLOT(clearPrice()));
}

void frmMain::loadSettings()
{
    m_settings = settingsFactory().getSettings();
    resize(m_settings.windowSize());

    if (m_settings.windowState() == Qt::WindowActive) // first time opening, no useful location saved yet
        return;

    move(m_settings.windowLocation());
    if (m_settings.windowState() != Qt::WindowNoState)
        this->setWindowState(this->windowState() | m_settings.windowState());
}

void frmMain::fileChange(const QString &filePath_, bool newFile_)
{
    setWindowModified(false);
    if (filePath_.isEmpty())
        setWindowTitle(QString("untitled.mpi[*] - MyPersonalIndex"));
    else
        setWindowTitle(QString("%1[*] - MyPersonalIndex").arg(QFileInfo(filePath_).fileName()));

    updateRecentFileActions(filePath_);

    if (!newFile_)
        return;

    refreshPortfolioPrices();
    refreshPortfolioTabs(-1);
}

void frmMain::refreshPortfolioTabs(int id_)
{
    ui->portfolioTabs->blockSignals(true);
    while(ui->portfolioTabs->count() != 0)
        ui->portfolioTabs->removeTab(0);

    int index = -1;
    foreach(const portfolio &p, m_file->portfolios)
    {
        int id = ui->portfolioTabs->addTab(p.displayText());
        ui->portfolioTabs->setTabData(id, p.id());
        if (p.id() == id_)
            index = id;
    }

    if (index == -1 && ui->portfolioTabs->count() != 0)
        index = 0;

    if (index != -1)
        ui->portfolioTabs->setCurrentIndex(index);

    portfolioTabChange(index);
    ui->portfolioTabs->blockSignals(false);
}

void frmMain::closeEvent(QCloseEvent *event_)
{
    if (ui->progressUpdateBar->isVisible())
    {
        QMessageBox msgBox(this);
        QPushButton *waitButton = msgBox.addButton("Wait", QMessageBox::AcceptRole);
        QPushButton *cancelButton = msgBox.addButton("Exit Now Without Save", QMessageBox::RejectRole);
        msgBox.setDefaultButton(waitButton);

        msgBox.setWindowTitle("MyPersonalIndex");
        msgBox.setText("An update is currently in progress. You must wait for it to finish if you want to save.");
        msgBox.exec();

        if (msgBox.clickedButton() == cancelButton)
        {
            event_->accept();
            saveSettings();
            QApplication::exit();
            return;
        }

        event_->ignore();
        return;
    }

    if (!m_file->maybeSave())
    {
        event_->ignore();
        return;
    }

    event_->accept();
    saveSettings();
    QApplication::exit();
}

void frmMain::saveSettings()
{
    m_settings.setWindowSize(size());
    m_settings.setWindowLocation(pos());
    m_settings.setWindowState(isMaximized() ? Qt::WindowMaximized : isMinimized() ? Qt::WindowMinimized : Qt::WindowNoState);
    m_settings.save();
}

void frmMain::updateRecentFileActions(const QString &newFilePath_)
{
    if (!newFilePath_.isEmpty())
        m_settings.setRecentFile(newFilePath_);

    ui->fileRecent->clear();
    foreach(const QString &s, m_settings.recentFiles())     
        connect(ui->fileRecent->addAction(s), SIGNAL(triggered()), this, SLOT(recentFileSelected()));

    int counter = 1;
    foreach(QAction *action, ui->fileRecent->actions())
        action->setShortcut(QString("Ctrl+%1").arg(QString::number(counter++)));

    if (m_settings.recentFiles().size() == 0)
    {
        ui->fileRecent->addAction("Empty")->setEnabled(false);
        return;
    }

    ui->fileRecent->addSeparator();
    connect(ui->fileRecent->addAction("Clear Recent"), SIGNAL(triggered()), this, SLOT(clearRecentFiles()));
}

void frmMain::recentFileSelected()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action || action->text() == m_file->path())
        return;

    m_file->open(action->text());
}

void frmMain::clearRecentFiles()
{
    m_settings.clearRecentFiles();
    updateRecentFileActions(QString());
}

void frmMain::addPortfolio()
{
    portfolio p;
    p.setID(m_file->identities.nextIdentity(objectType_Portfolio));
    frmEdit f(p, m_file->identities, true, this);
    f.exec();
    if (p == f.getPortfolio())
        return;

    m_file->identities = f.getIdentities();
    portfolioAdded(f.getPortfolio());
}
void frmMain::editPortfolioTrading(const QString& securityName)
{
    if (m_currentPortfolio == -1)
        return;

    frmEdit f(m_file->portfolios.value(m_currentPortfolio), m_file->identities, false, this);
    f.setTradeView(securityName);
    f.exec();
    if (m_file->portfolios.value(m_currentPortfolio) == f.getPortfolio())
        return;

    m_file->identities = f.getIdentities();
    portfolioModified(f.getPortfolio());
}

void frmMain::editPortfolio()
{
    if (m_currentPortfolio == -1)
        return;

    frmEdit f(m_file->portfolios.value(m_currentPortfolio), m_file->identities, false, this);
    f.exec();
    if (m_file->portfolios.value(m_currentPortfolio) == f.getPortfolio())
        return;

    m_file->identities = f.getIdentities();
    portfolioModified(f.getPortfolio());
}

void frmMain::deletePortfolio()
{
    if (m_currentPortfolio == -1)
        return;

    if (QMessageBox::question(this, "MyPersonalIndex", QString("Are you sure you want to delete portfolio %1?")
        .arg(m_file->portfolios.value(m_currentPortfolio).displayText()), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    setWindowModified(true);
    m_file->modified = true;
    m_file->portfolios.remove(m_currentPortfolio);
    ui->portfolioTabs->removeTab(ui->portfolioTabs->currentIndex());
}

void frmMain::portfolioAdded(const portfolio &portfolio_)
{
    setWindowModified(true);
    m_file->modified = true;
    m_file->portfolios.insert(portfolio_.id(), portfolio_);
    refreshPortfolioPrices();

    ui->portfolioTabs->blockSignals(true);
    int tabId = ui->portfolioTabs->addTab(portfolio_.displayText());
    ui->portfolioTabs->setTabData(tabId, portfolio_.id());
    ui->portfolioTabs->setCurrentIndex(tabId);
    ui->portfolioTabs->blockSignals(false);

    portfolioTabChange(tabId);
    recalculateTrades(portfolio_);
}

void frmMain::portfolioModified(const portfolio &portfolio_)
{
    setWindowModified(true);
    m_file->modified = true;
    m_file->portfolios[m_currentPortfolio] = portfolio_;
    refreshPortfolioPrices();
    ui->portfolioTabs->setTabText(ui->portfolioTabs->currentIndex(), portfolio_.displayText());
    recalculateTrades(portfolio_);
}

void frmMain::priceModified(int beginDate_)
{
    setWindowModified(true);
    m_file->modified = true;
    refreshPortfolioPrices();
    recalculateTrades(m_file->portfolios.values(), beginDate_);
}

void frmMain::refreshPortfolioPrices()
{
    foreach(const portfolio &p, m_file->portfolios)
        foreach(security s, p.securities())
        {
            if (s.cashAccount())
                continue;

            s.setHistoricalPrices(m_file->prices.getHistoricalPrice(s.description()));
        }
}

void frmMain::portfolioTabChange(int currentIndex_)
{
    clearViews();
    m_currentPortfolio = currentIndex_ == -1 ? -1 : ui->portfolioTabs->tabData(currentIndex_).toInt();
    ui->portfolioDelete->setDisabled(currentIndex_ == -1);
    ui->portfolioEdit->setDisabled(currentIndex_ == -1);
    ui->portfolioTabsDelete->setHidden(currentIndex_ == -1);
    ui->portfolioTabsEdit->setHidden(currentIndex_ == -1);
    ui->portfolioTabsViewCmb->setHidden(currentIndex_ == -1);
    switchToView(m_currentView, true);
}

void frmMain::portfolioTabMoved(int from_, int to_)
{
    // reset identities to get all new ones
    m_file->identities.resetIdentity(objectType_Portfolio);

    portfolio current = m_file->portfolios.value(m_currentPortfolio);
    QList<portfolio> portfolios = m_file->portfolios.values();
    portfolios.move(from_, to_);

    QMap<int, portfolio> newPortfolioMap;
    for(int i = 0; i < portfolios.count(); ++i)
    {
        portfolio p = portfolios.at(i);
        p.setID(m_file->identities.nextIdentity(objectType_Portfolio));
        newPortfolioMap.insert(p.id(), p);
        ui->portfolioTabs->setTabData(i, p.id());
    }

    m_currentPortfolio = current.id();
    m_file->portfolios = newPortfolioMap;
    setWindowModified(true);
    m_file->modified = true;
}

void frmMain::nextPortfolio()
{
    if (ui->portfolioTabs->count() == 0)
        return;

    int id = ui->portfolioTabs->currentIndex();
    if (id == ui->portfolioTabs->count() - 1)
        id = 0;
    else
        ++id;
    ui->portfolioTabs->setCurrentIndex(id);
}

void frmMain::about()
{
    QMessageBox::about(this, "About MyPersonalIndex", "<h2>MyPersonalIndex " + QString::number(APP_VERSION / 100.0) + "</h2>"
        "<p>Copyright &copy; 2011"
        "<p>By Matthew Wikler"
        "<p>Create personal indexes and perform analysis to make better investing decisions."
        "<br><a href='http://code.google.com/p/mypersonalindex/'>http://code.google.com/p/mypersonalindex/</a></p>");
}

void frmMain::downloadPrices()
{
    if (!updatePrices::isInternetConnection())
    {
        QMessageBox::critical(this, "MyPersonalIndex", "Cannot contact Yahoo! Finance, please check your internet connection.");
        return;
    }

	//Map between symbol name to the creation date of it's portfolio.
    QMap<QString, int> symbols = portfolio::symbols(m_file->portfolios);
    if (symbols.isEmpty())
        return;

    disableForUpdate(true);
    showProgressBar(symbols.count());

	//if among all the securities with the same symbol name (like IBB), all the securities hold 0 shares (you can know that from CalculatorNAV)
	//in the day of the last trade AND we already have prices in the getHistoricalPrice for that symbol after that last trade -> then do not update price.
	QMap<QString, bool> SymbolToLastestTradeDateMap;
	foreach(const portfolio &p, m_file->portfolios)
		foreach(const security &s, p.securities())
	{
		QString& symbolName = s.description();//A security's symbol name is its description()
		if (SymbolToLastestTradeDateMap.contains(symbolName))
			continue;

		executedTradeMap& tradeMap = s.executedTrades();
        int lastTradeDate = 0, firstTradeDate = INT_MAX;
		int lastPriceDate = m_file->prices.getHistoricalPrice(symbolName).endDate(historicalPrices::type_price);
		int firstPriceDate = m_file->prices.getHistoricalPrice(symbolName).beginDate(historicalPrices::type_price);
        for(QMap<int, executedTrade>::const_iterator i = tradeMap.constBegin(); i != tradeMap.constEnd(); ++i)
		{
			if (i.key() > lastTradeDate)
				lastTradeDate = i.key();

			if (i.key() < firstTradeDate)
				firstTradeDate = i.key();
		}

        double shares = 0;
        if(lastTradeDate == 0)
        {
            shares = p.securitySnapshot( lastPriceDate, s.id(), 0).shares;
        }
        else
        {
            shares = p.securitySnapshot( lastTradeDate, s.id(), 0).shares;
        }

		if(shares > 0)
		{
            SymbolToLastestTradeDateMap.insert(symbolName, true);
            continue;
		}

        if(lastPriceDate <= lastTradeDate)
		{
            //currencyNIS has no good historical data
			SymbolToLastestTradeDateMap.insert(symbolName, true);
			continue;
		}

        if(firstPriceDate > firstTradeDate && ( !currencyNIS == s.getSecurityCurrency()))
		{
			SymbolToLastestTradeDateMap.insert(symbolName, true);
			continue;
		}
	}
		
		
	updatePrices u;
    QList<int> results;
    for(QMap<QString, int>::const_iterator i = symbols.constBegin(); i != symbols.constEnd(); ++i)
    {
		const QString& symbolName = i.key();
        if (!SymbolToLastestTradeDateMap.contains(symbolName))
		{
			continue;
		}

		m_file->prices.getHistoricalPrice(symbolName).removeTempPrices();
        results.append(u.get(m_file->prices.getHistoricalPrice(symbolName), i.value(), tradeDateCalendar::endDate()));
        ui->progressUpdateBar->setValue(ui->progressUpdateBar->value() + 1);
    }

    hideProgressBar();

    int earliestUpdate = tradeDateCalendar::endDate() + 1;
    foreach(int result, results)
        if (result != -1)
            earliestUpdate = qMin(earliestUpdate, result);

    if (earliestUpdate > tradeDateCalendar::endDate())
    {
        disableForUpdate(false);
        return;
    }

    priceModified(earliestUpdate);
}

void frmMain::recalculateTrades(const portfolio &portfolio_, int beginDate_)
{
    recalculateTrades(QList<portfolio>() << portfolio_, beginDate_);
}

void frmMain::recalculateTrades(const QList<portfolio> &portfolios_, int beginDate_)
{
    disableForUpdate(true);
    showProgressBar(portfolios_.count());
    calculatorTrade c(beginDate_);
    foreach(portfolio p, portfolios_)
    {
        c.calculate(p);
        ui->progressUpdateBar->setValue(ui->progressUpdateBar->value() + 1);
    }

    hideProgressBar();
    disableForUpdate(false);
    clearViews();
    switchToView(m_currentView, true);
}

void frmMain::showProgressBar(int steps_)
{
    ui->progressUpdateBar->setMaximum(steps_);
    ui->progressUpdateBar->setValue(0);
    ui->progressWidget->setCurrentIndex(1);
}

void frmMain::hideProgressBar()
{
    ui->progressWidget->setCurrentIndex(0);
}

void frmMain::viewChange()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    switchToView((view)action->data().toInt(), false);
}

void frmMain::viewChange(int currentIndex_)
{
    switchToView((view)ui->portfolioTabsViewCmb->itemData(currentIndex_).toInt(), false);
}

void frmMain::clearViews()
{
    foreach(frmMainState *state, m_views)
    {
        ui->viewWidget->removeWidget(state->mainWidget());
        state->mainWidget()->setAttribute(Qt::WA_DeleteOnClose, true);
        //delete state->mainWidget();
    }

    qDeleteAll(m_views);
    m_views.clear();
}

void frmMain::switchToView(view view_, bool force_)
{
    if ((!force_ && m_currentView == view_) || m_currentPortfolio == -1 )
        return;

    ui->portfolioTabsViewCmb->blockSignals(true);
    ui->portfolioTabsViewCmb->setCurrentIndex(ui->portfolioTabsViewCmb->findData(view_));
    ui->portfolioTabsViewCmb->blockSignals(false);

    if (m_views.contains(view_))
    {
        ui->viewWidget->setCurrentWidget(m_views.value(view_)->mainWidget());
        m_currentView = view_;
        return;
    }

    switch (view_)
    {
        case view_assetAllocation:
            m_views.insert(view_assetAllocation, new frmMainAA_State(m_file->portfolios.value(m_currentPortfolio), m_settings, this));
            break;
        case view_account:
            m_views.insert(view_account, new frmMainAcct_State(m_file->portfolios.value(m_currentPortfolio), m_settings, this));
            break;
        case view_security:
            m_views.insert(view_security, new frmMainSecurity_State(m_file->portfolios.value(m_currentPortfolio), m_settings, this));
            break;
        case view_performance:
            m_views.insert(view_performance, new frmMainPerformance_State(m_currentPortfolio, m_file->portfolios, m_settings, m_file->prices.getHistoricalPrices(), this));
            break;
        case view_correlation:
            m_views.insert(view_correlation, new frmMainCorrelation_State(m_currentPortfolio, m_file->portfolios, m_settings, m_file->prices.getHistoricalPrices(), this));
            break;
        case view_statistic:
            m_views.insert(view_statistic, new frmMainStatistic_State(m_currentPortfolio, m_file->portfolios, m_settings, m_file->prices.getHistoricalPrices(), this));
            break;
        case view_chart:
            m_views.insert(view_chart, new frmMainChart_State(m_currentPortfolio, m_file->portfolios, m_file->prices.getHistoricalPrices(), this));
            break;
        case view_trade:
            m_views.insert(view_trade, new frmMainTrade_State(m_file->portfolios.value(m_currentPortfolio), this));
            break;
    }

    ui->viewWidget->addWidget(m_views.value(view_)->mainWidget());
    ui->viewWidget->setCurrentWidget(m_views.value(view_)->mainWidget());
    m_currentView = view_;
}

void frmMain::importPortfolio()
{
    QMap<int, portfolio> portfolios;

    if (static_cast<QAction*>(sender()) == ui->portfolioImportFile)
    {
        fileState file(this);
        file.open(false);
        if (file.path().isEmpty())
            return;
        else
            portfolios = file.portfolios;
    }
    else
        portfolios = m_file->portfolios;

    frmPortfolioImport f(m_currentPortfolio == -1 ? portfolio() : m_file->portfolios.value(m_currentPortfolio), portfolios, m_file->identities, this);
    if (f.exec() != QDialog::Accepted)
        return;

    if (f.getPortfolio().id() != m_file->portfolios.value(m_currentPortfolio).id())
        portfolioAdded(f.getPortfolio());
    else
        portfolioModified(f.getPortfolio());
}

void frmMain::importPrice()
{
    frmPriceImport f(m_file->prices, this);
    if (f.exec() != QDialog::Accepted)
        return;

    m_file->prices = f.getHistoricalPricesMap();
    priceModified();
}

void frmMain::clearPrice()
{
    if (m_file->prices.isEmpty())
        return;

    if (QMessageBox::question(this, "MyPersonalIndex", "Are you sure you want to clear all download and imported price data?",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return;

    m_file->prices.clear();
    priceModified();
}

void frmMain::disableForUpdate(bool disable_)
{
    ui->menubar->setDisabled(disable_);
    ui->viewWidget->setDisabled(disable_);
    ui->portfolioTabs->setDisabled(disable_);
    ui->portfolioTabsAdd->setDisabled(disable_);
    ui->portfolioTabsEdit->setDisabled(disable_);
    ui->portfolioTabsDelete->setDisabled(disable_);
    ui->portfolioTabsViewCmb->setDisabled(disable_);
}
