#ifndef FRMMAIN_H
#define FRMMAIN_H

#include <QMainWindow>
#include <QMap>
#include "settings.h"

template<class T>
class QFutureWatcher;
class frmMain_UI;
class fileState;
class frmMainState;
class portfolio;
class frmMain : public QMainWindow
{
    Q_OBJECT

public:
    frmMain(const QString &filePath_, QWidget *parent_ = 0);
    ~frmMain();
    void editPortfolioTrading(const QString&);

    enum view {
        view_trade,
        view_account,
        view_assetAllocation,
        view_security,
        view_statistic,
        view_chart,
        view_correlation,
        view_performance
    };

private slots:
    void about();
    void recentFileSelected();
    void clearRecentFiles();
    void addPortfolio();
    void editPortfolio();
    void deletePortfolio();
    void portfolioTabChange(int currentIndex_);
    void portfolioTabMoved(int from_, int to_);
    void nextPortfolio();
    void downloadPrices();
    void viewChange();
    void viewChange(int currentIndex_);
    void fileChange(const QString &filePath_, bool newFile_);
    void importPortfolio();
    void importPrice();
    void clearPrice();

private:

    // ui pointers (to delete on destory)
    frmMain_UI *ui;

    // current file
    fileState *m_file;

    // track the currently selected portfolio
    int m_currentPortfolio;

    // track the currently selected tab
    QMap<int, frmMainState*> m_views;
    view m_currentView;

    // settings saved to INI
    settings m_settings;

    void connectSlots();
    void closeEvent(QCloseEvent *event_);
    void loadSettings();
    void saveSettings();

    void updateRecentFileActions(const QString &newFilePath_);
    void refreshPortfolioTabs(int id_ = -1);
    void refreshPortfolioPrices();
    void showProgressBar(int steps_);
    void hideProgressBar();
    void switchToView(view view_, bool force_ = false);
    void clearViews();
    void portfolioAdded(const portfolio &portfolio_);
    void portfolioModified(const portfolio &portfolio_);
    void priceModified(int beginDate_ = 0);
    void disableForUpdate(bool disable_);

    void recalculateTrades(const portfolio &portfolio_, int beginDate_ = 0);
    void recalculateTrades(const QList<portfolio> &portfolios_, int beginDate_ = 0);
};

#endif // FRMMAIN_H
