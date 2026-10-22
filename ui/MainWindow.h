#pragma once
#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QListWidget>
#include <QTabWidget>
#include <vector>
#include "FarmController.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    FarmController* controller;
    int selectedX = 0;
    int selectedY = 0;

    // Top HUD (Always visible on the front page)
    QLabel* lblTreasury;
    QLabel* lblDayWeather;
    QPushButton* btnFrontStepTick;
    QPushButton* btnFrontAutoSim;
    QLabel* lblStatusBanner;

    // View 1: Farm Grid Canvas View + Regional Navigation Pad
    QLabel* lblRegionBanner;
    QPushButton* btnNavNorth;
    QPushButton* btnNavSouth;
    QPushButton* btnNavWest;
    QPushButton* btnNavEast;
    std::vector<std::vector<QPushButton*>> gridButtons;
    QLabel* lblInspector;
    QComboBox* comboClearTool;
    QComboBox* comboSeedSelect;

    // View 2: Market & Exchange Dashboard
    QTableWidget* tableMarketBatches;
    QLabel* lblActivePricing;
    QLabel* lblSaturationInfo;
    QComboBox* comboStrategy;

    // View 3: Town Supply Store & Catalog View
    QLabel* lblInventorySummary;

    // View 4: Simulation & Weather Monitor / Persistence View
    QListWidget* listSimulationLog;
    QPushButton* btnAutoSim;
    QComboBox* comboBiome;

    void setupUi();
    QWidget* createFarmCanvasTab();
    QWidget* createMarketDashboardTab();
    QWidget* createTownStoreTab();
    QWidget* createSimulationMonitorTab();

    void updateTileButtonVisual(const TileSnapshot& snap);
    void updateInspectorPanel();

private slots:
    void onTileClicked(int x, int y);
    void onTileUpdated(const TileSnapshot& snap);
    void refreshAllViews();
    void showNotification(const QString& msg, bool isError);

public:
    explicit MainWindow(FarmController* ctrl, QWidget* parent = nullptr);
};