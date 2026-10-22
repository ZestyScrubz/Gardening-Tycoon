#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QListWidget>
#include <QTabWidget>
#include <QPolygonF>
#include <vector>
#include "FarmController.h"

// Custom Isometric/Hexagonal Farm Canvas painted with QPainter
class HexFarmWidget : public QWidget {
    Q_OBJECT

private:
    FarmController* controller;
    int selectedX = 0;
    int selectedY = 0;
    int hoveredX = -1;
    int hoveredY = -1;

    QPointF hexCenter(int col, int row, double radius) const;
    QPolygonF createHexPolygon(const QPointF& center, double radius) const;
    double calculateHexRadius() const;

    void drawHexTile(QPainter& p, const TileSnapshot& snap, const QPointF& center, double radius, bool isSelected, bool isHovered);
    void drawTree(QPainter& p, const QPointF& c, double r);
    void drawBoulder(QPainter& p, const QPointF& c, double r);
    void drawPond(QPainter& p, const QPointF& c, double r);
    void drawTilledSoil(QPainter& p, const QPointF& c, double r, double moisture, bool isMulched);
    void drawCropSprite(QPainter& p, const QPointF& c, double r, const TileSnapshot& snap);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

public:
    explicit HexFarmWidget(FarmController* ctrl, QWidget* parent = nullptr);
    void setSelectedTile(int x, int y);
    int getSelectedX() const { return selectedX; }
    int getSelectedY() const { return selectedY; }

signals:
    void tileSelected(int x, int y);
};

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

    // View 1: Hexagonal Farm Grid Canvas View + Regional Navigation
    QLabel* lblRegionBanner;
    QPushButton* btnNavNorth;
    QPushButton* btnNavSouth;
    QPushButton* btnNavWest;
    QPushButton* btnNavEast;
    HexFarmWidget* hexCanvas;
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

    // View 5: Interactive Plant Index & Environment Guide
    QListWidget* listPlantIndex;
    QLabel* lblPlantGuideDetails;

    void setupUi();
    QWidget* createFarmCanvasTab();
    QWidget* createMarketDashboardTab();
    QWidget* createTownStoreTab();
    QWidget* createSimulationMonitorTab();
    QWidget* createPlantIndexTab();

    void updateInspectorPanel();
    void updatePlantIndexDetails(const QString& species);

private slots:
    void onTileClicked(int x, int y);
    void onTileUpdated(const TileSnapshot& snap);
    void refreshAllViews();
    void showNotification(const QString& msg, bool isError);

public:
    explicit MainWindow(FarmController* ctrl, QWidget* parent = nullptr);
};