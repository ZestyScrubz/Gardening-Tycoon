#include "MainWindow.h"
#include "CropFactory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>

MainWindow::MainWindow(FarmController* ctrl, QWidget* parent)
    : QMainWindow(parent), controller(ctrl) {
    setWindowTitle("Gardening Tycoon — CS3307A (Group 120)");
    resize(1180, 820);

    // High-contrast, modern light stylesheet for effortless readability
    setStyleSheet(
        "QMainWindow, QWidget { background-color: #f8fafc; color: #0f172a; font-family: -apple-system, 'Segoe UI', Roboto, sans-serif; }"
        "QGroupBox { font-weight: bold; font-size: 13px; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 6px; margin-top: 10px; padding-top: 12px; background-color: #ffffff; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
        "QPushButton { background-color: #e2e8f0; color: #0f172a; border: 1px solid #94a3b8; border-radius: 5px; padding: 6px 10px; font-weight: 600; }"
        "QPushButton:hover { background-color: #cbd5e1; }"
        "QPushButton:disabled { background-color: #f1f5f9; color: #94a3b8; border-color: #e2e8f0; }"
        "QComboBox { background-color: #ffffff; color: #0f172a; border: 1px solid #94a3b8; border-radius: 4px; padding: 5px; font-weight: 500; }"
        "QTabWidget::pane { border: 1px solid #cbd5e1; border-radius: 6px; background: #ffffff; }"
        "QTabBar::tab { background: #e2e8f0; color: #334155; padding: 9px 16px; margin-right: 4px; border-top-left-radius: 6px; border-top-right-radius: 6px; font-weight: bold; }"
        "QTabBar::tab:selected { background: #2563eb; color: #ffffff; }"
        "QTableWidget, QListWidget { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; font-size: 13px; }"
        "QHeaderView::section { background-color: #f1f5f9; color: #0f172a; font-weight: bold; padding: 6px; border: 1px solid #cbd5e1; }"
        );

    setupUi();

    connect(controller, &FarmController::tileChanged, this, &MainWindow::onTileUpdated);
    connect(controller, &FarmController::stateRefreshed, this, &MainWindow::refreshAllViews);
    connect(controller, &FarmController::statusNotification, this, &MainWindow::showNotification);

    refreshAllViews();
    updateInspectorPanel();
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // 1. Top Persistent HUD Bar with Simulation Tick Controls directly on the front page
    QWidget* hudCard = new QWidget();
    hudCard->setStyleSheet("background-color: #1e293b; color: #ffffff; border-radius: 6px; padding: 6px;");
    QHBoxLayout* hudLayout = new QHBoxLayout(hudCard);

    lblTreasury = new QLabel();
    lblTreasury->setStyleSheet("font-size: 16px; font-weight: bold; color: #4ade80; background: transparent;");

    lblDayWeather = new QLabel();
    lblDayWeather->setStyleSheet("font-size: 14px; font-weight: bold; color: #f8fafc; background: transparent;");

    btnFrontStepTick = new QPushButton("⏭ Advance Day (+1 Tick)");
    btnFrontStepTick->setStyleSheet(
        "QPushButton { background-color: #2563eb; color: #ffffff; border: 1px solid #60a5fa; font-size: 13px; font-weight: bold; padding: 7px 14px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #1d4ed8; }");
    connect(btnFrontStepTick, &QPushButton::clicked, controller, &FarmController::advanceSimulationTick);

    btnFrontAutoSim = new QPushButton("▶ Auto-Run: OFF");
    btnFrontAutoSim->setCheckable(true);
    btnFrontAutoSim->setStyleSheet(
        "QPushButton { background-color: #059669; color: #ffffff; border: 1px solid #34d399; font-size: 13px; font-weight: bold; padding: 7px 14px; border-radius: 5px; }"
        "QPushButton:checked { background-color: #d97706; border-color: #fbbf24; }");
    connect(btnFrontAutoSim, &QPushButton::toggled, this, [this](bool checked) {
        controller->toggleAutoRun(checked, 2000);
    });

    hudLayout->addWidget(lblTreasury);
    hudLayout->addSpacing(20);
    hudLayout->addWidget(lblDayWeather);
    hudLayout->addStretch();
    hudLayout->addWidget(btnFrontStepTick);
    hudLayout->addWidget(btnFrontAutoSim);
    mainLayout->addWidget(hudCard);

    // Status / Error Notification Banner (DP9)
    lblStatusBanner = new QLabel("Welcome to Gardening Tycoon! Click any tile on the grid to highlight and inspect it.");
    lblStatusBanner->setStyleSheet("padding: 8px 12px; background-color: #dcfce7; color: #14532d; font-weight: bold; border: 1px solid #86efac; border-radius: 5px;");
    mainLayout->addWidget(lblStatusBanner);

    // 4 Views required by DP10
    QTabWidget* tabs = new QTabWidget(this);
    tabs->addTab(createFarmCanvasTab(), "1. Farm Grid Canvas");
    tabs->addTab(createMarketDashboardTab(), "2. Market & Exchange");
    tabs->addTab(createTownStoreTab(), "3. Town Supply Store");
    tabs->addTab(createSimulationMonitorTab(), "4. Simulation & Persistence HUD");

    mainLayout->addWidget(tabs);
    setCentralWidget(central);
}

QWidget* MainWindow::createFarmCanvasTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    // Left Column: Directional Biome Navigation Bar + 2D Grid
    QVBoxLayout* leftCol = new QVBoxLayout();

    QGroupBox* navBox = new QGroupBox("Explore Regional Farm Plots & Biomes (Use Arrows to Travel)");
    QHBoxLayout* navLayout = new QHBoxLayout(navBox);

    btnNavWest  = new QPushButton("◀ West");
    btnNavNorth = new QPushButton("▲ North");
    btnNavSouth = new QPushButton("▼ South");
    btnNavEast  = new QPushButton("East ▶");

    connect(btnNavWest,  &QPushButton::clicked, this, [this]() { controller->navigateRegion(-1, 0); });
    connect(btnNavEast,  &QPushButton::clicked, this, [this]() { controller->navigateRegion(1, 0); });
    connect(btnNavNorth, &QPushButton::clicked, this, [this]() { controller->navigateRegion(0, -1); });
    connect(btnNavSouth, &QPushButton::clicked, this, [this]() { controller->navigateRegion(0, 1); });

    lblRegionBanner = new QLabel();
    lblRegionBanner->setAlignment(Qt::AlignCenter);

    navLayout->addWidget(btnNavWest);
    navLayout->addWidget(btnNavNorth);
    navLayout->addWidget(lblRegionBanner, 1);
    navLayout->addWidget(btnNavSouth);
    navLayout->addWidget(btnNavEast);
    leftCol->addWidget(navBox);

    // 2D Farm Grid
    QGroupBox* gridGroup = new QGroupBox("2D Interactive Farm Plots (Gold Border = Currently Selected Tile)");
    QGridLayout* gridLayout = new QGridLayout(gridGroup);
    gridLayout->setSpacing(5);

    int w = controller->getGridWidth();
    int h = controller->getGridHeight();
    gridButtons.resize(h, std::vector<QPushButton*>(w, nullptr));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            QPushButton* btn = new QPushButton();
            btn->setMinimumSize(84, 68);
            connect(btn, &QPushButton::clicked, this, [this, x, y]() { onTileClicked(x, y); });
            gridButtons[y][x] = btn;
            gridLayout->addWidget(btn, y, x);
        }
    }
    leftCol->addWidget(gridGroup, 1);
    layout->addLayout(leftCol, 3);

    // Right Column: Plot Inspector & Action Controls
    QGroupBox* actionGroup = new QGroupBox("Selected Plot Inspector & Actions");
    QVBoxLayout* actLayout = new QVBoxLayout(actionGroup);

    lblInspector = new QLabel("Select a tile...");
    lblInspector->setWordWrap(true);
    lblInspector->setStyleSheet("font-size: 13px; padding: 10px; background: #f8fafc; color: #0f172a; border: 2px solid #cbd5e1; border-radius: 5px;");
    actLayout->addWidget(lblInspector);

    // UC1: Clear Obstacle
    QHBoxLayout* clearRow = new QHBoxLayout();
    comboClearTool = new QComboBox();
    comboClearTool->addItems({"Axe", "Pickaxe", "Shovel"});
    QPushButton* btnClear = new QPushButton("Clear Obstacle ($15)");
    connect(btnClear, &QPushButton::clicked, this, [this]() {
        controller->clearTileObstacle(selectedX, selectedY, comboClearTool->currentText().toStdString());
    });
    clearRow->addWidget(comboClearTool);
    clearRow->addWidget(btnClear);
    actLayout->addLayout(clearRow);

    // FR3: Till with Hoe
    QPushButton* btnTill = new QPushButton("Till Soil with Hoe (FR3)");
    connect(btnTill, &QPushButton::clicked, this, [this]() {
        controller->tillTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnTill);

    // UC3: Plant Seed
    QHBoxLayout* plantRow = new QHBoxLayout();
    comboSeedSelect = new QComboBox();
    comboSeedSelect->addItems({"Carrot", "Wheat", "Corn"});
    QPushButton* btnPlant = new QPushButton("Plant Seed (UC3)");
    btnPlant->setStyleSheet("background-color: #dbeafe; color: #1e3a8a; border: 1px solid #93c5fd;");
    connect(btnPlant, &QPushButton::clicked, this, [this]() {
        controller->plantSeedOnTile(selectedX, selectedY, comboSeedSelect->currentText().toStdString());
    });
    plantRow->addWidget(comboSeedSelect);
    plantRow->addWidget(btnPlant);
    actLayout->addLayout(plantRow);

    // Water & Mulch (FR13)
    QPushButton* btnWater = new QPushButton("💧 Water Plot (+25% Hydration)");
    btnWater->setStyleSheet("background-color: #e0f2fe; color: #0c4a6e; border: 1px solid #7dd3fc;");
    connect(btnWater, &QPushButton::clicked, this, [this]() {
        controller->waterTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnWater);

    QPushButton* btnMulch = new QPushButton("🌿 Apply Organic Mulch (-50% Evap, FR13)");
    connect(btnMulch, &QPushButton::clicked, this, [this]() {
        controller->applyMulchToTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnMulch);

    // UC4: Harvest Mature Crop
    QPushButton* btnHarvest = new QPushButton("🌾 Harvest Mature Crop (FR6 / UC4)");
    btnHarvest->setStyleSheet("font-weight: bold; background-color: #fef08a; color: #713f12; border: 2px solid #eab308; padding: 8px;");
    connect(btnHarvest, &QPushButton::clicked, this, [this]() {
        controller->harvestTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnHarvest);

    QPushButton* btnClearRot = new QPushButton("Remove Rotten Crop (FR12)");
    btnClearRot->setStyleSheet("background-color: #fee2e2; color: #7f1d1d; border: 1px solid #fca5a5;");
    connect(btnClearRot, &QPushButton::clicked, this, [this]() {
        controller->clearRottenOnTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnClearRot);

    actLayout->addStretch();
    layout->addWidget(actionGroup, 1);
    return tab;
}

QWidget* MainWindow::createMarketDashboardTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // Strategy Switcher (FR8 / UC8)
    QGroupBox* stratGroup = new QGroupBox("Runtime Market Pricing Strategy (FR8 / DP2)");
    QHBoxLayout* stratLayout = new QHBoxLayout(stratGroup);
    lblActivePricing = new QLabel();
    comboStrategy = new QComboBox();
    comboStrategy->addItems({"Standard", "Threshold", "Festival"});
    QPushButton* btnSwapStrategy = new QPushButton("Swap Pricing Policy");
    connect(btnSwapStrategy, &QPushButton::clicked, this, [this]() {
        controller->changePricingStrategy(comboStrategy->currentText().toStdString());
    });
    stratLayout->addWidget(lblActivePricing);
    stratLayout->addStretch();
    stratLayout->addWidget(new QLabel("Select Strategy:"));
    stratLayout->addWidget(comboStrategy);
    stratLayout->addWidget(btnSwapStrategy);
    layout->addWidget(stratGroup);

    // Saturation Tracker (FR9)
    lblSaturationInfo = new QLabel();
    lblSaturationInfo->setStyleSheet("padding: 8px; font-weight: bold; background-color: #f1f5f9; border-radius: 4px;");
    layout->addWidget(lblSaturationInfo);

    // Harvested Batches Table (UC5 / FR7)
    tableMarketBatches = new QTableWidget(0, 4);
    tableMarketBatches->setHorizontalHeaderLabels({"Crop Species", "Quantity", "Harvest Vitality", "Dynamic Market Quote ($)"});
    tableMarketBatches->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableMarketBatches->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(tableMarketBatches);

    QPushButton* btnSellSelected = new QPushButton("💰 Sell Selected Harvest Batch at Market (UC5)");
    btnSellSelected->setStyleSheet("font-size: 14px; font-weight: bold; padding: 10px; background-color: #fef08a; color: #713f12; border: 2px solid #eab308;");
    connect(btnSellSelected, &QPushButton::clicked, this, [this]() {
        int row = tableMarketBatches->currentRow();
        controller->sellHarvestBatch(row);
    });
    layout->addWidget(btnSellSelected);

    return tab;
}

QWidget* MainWindow::createTownStoreTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    lblInventorySummary = new QLabel();
    lblInventorySummary->setStyleSheet("font-size: 14px; padding: 10px; background: #eff6ff; color: #1e3a8a; border: 1px solid #bfdbfe; border-radius: 5px;");
    layout->addWidget(lblInventorySummary);

    QGroupBox* toolsBox = new QGroupBox("Purchase Clearing Tools (FR1 / FR10 Solvency Checked)");
    QHBoxLayout* toolsLayout = new QHBoxLayout(toolsBox);

    QPushButton* btnBuyAxe = new QPushButton("Buy Clearing Axe ($40.00) — Clears Trees");
    connect(btnBuyAxe, &QPushButton::clicked, this, [this]() { controller->buyTool("Axe", 40.0); });

    QPushButton* btnBuyPick = new QPushButton("Buy Pickaxe ($50.00) — Clears Boulders");
    connect(btnBuyPick, &QPushButton::clicked, this, [this]() { controller->buyTool("Pickaxe", 50.0); });

    QPushButton* btnBuyShovel = new QPushButton("Buy Shovel ($35.00) — Clears Ponds");
    connect(btnBuyShovel, &QPushButton::clicked, this, [this]() { controller->buyTool("Shovel", 35.0); });

    toolsLayout->addWidget(btnBuyAxe);
    toolsLayout->addWidget(btnBuyPick);
    toolsLayout->addWidget(btnBuyShovel);
    layout->addWidget(toolsBox);

    QGroupBox* seedsBox = new QGroupBox("Purchase Seed Packets & Soil Amendments (UC6 / FR13)");
    QGridLayout* seedsLayout = new QGridLayout(seedsBox);

    QPushButton* btnBuyCarrot = new QPushButton("Buy 3x Carrot Seeds ($36.00) — Moisture Lover");
    connect(btnBuyCarrot, &QPushButton::clicked, this, [this]() { controller->buySeed("Carrot", 3); });

    QPushButton* btnBuyWheat = new QPushButton("Buy 3x Wheat Seeds ($30.00) — Sunlight Lover");
    connect(btnBuyWheat, &QPushButton::clicked, this, [this]() { controller->buySeed("Wheat", 3); });

    QPushButton* btnBuyCorn = new QPushButton("Buy 3x Corn Seeds ($60.00) — High Value Solar");
    connect(btnBuyCorn, &QPushButton::clicked, this, [this]() { controller->buySeed("Corn", 3); });

    QPushButton* btnBuyMulch = new QPushButton("Buy 3x Organic Mulch ($25.00) — Cuts Evaporation 50%");
    connect(btnBuyMulch, &QPushButton::clicked, this, [this]() { controller->buyMulch(3, 25.0); });

    seedsLayout->addWidget(btnBuyCarrot, 0, 0);
    seedsLayout->addWidget(btnBuyWheat, 0, 1);
    seedsLayout->addWidget(btnBuyCorn, 1, 0);
    seedsLayout->addWidget(btnBuyMulch, 1, 1);
    layout->addWidget(seedsBox);

    layout->addStretch();
    return tab;
}

QWidget* MainWindow::createSimulationMonitorTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* simBox = new QGroupBox("Simulation Engine & Biome Controls (UC2 / FR2 / FR11)");
    QHBoxLayout* simLayout = new QHBoxLayout(simBox);

    btnAutoSim = new QPushButton("▶ Start Continuous Simulation (Auto-Tick)");
    btnAutoSim->setCheckable(true);
    btnAutoSim->setStyleSheet("font-weight: bold; padding: 8px; background-color: #dbeafe; color: #1e3a8a;");
    connect(btnAutoSim, &QPushButton::toggled, this, [this](bool checked) {
        controller->toggleAutoRun(checked, 2000);
    });

    QPushButton* btnStepTick = new QPushButton("⏭ Manual Step (+1 Tick)");
    connect(btnStepTick, &QPushButton::clicked, controller, &FarmController::advanceSimulationTick);

    comboBiome = new QComboBox();
    comboBiome->addItems({"Plains", "Desert", "Savanna"});
    QPushButton* btnSetBiome = new QPushButton("Override Current Sector Biome (FR2)");
    connect(btnSetBiome, &QPushButton::clicked, this, [this]() {
        controller->changeBiome(comboBiome->currentText().toStdString());
    });

    simLayout->addWidget(btnAutoSim);
    simLayout->addWidget(btnStepTick);
    simLayout->addStretch();
    simLayout->addWidget(new QLabel("Override Biome:"));
    simLayout->addWidget(comboBiome);
    simLayout->addWidget(btnSetBiome);
    layout->addWidget(simBox);

    // Persistence Controls (UC7 / DP8 / NFR3)
    QGroupBox* saveBox = new QGroupBox("Session Persistence & Fault Tolerance (UC7 / DP8 / NFR3)");
    QHBoxLayout* saveLayout = new QHBoxLayout(saveBox);

    QPushButton* btnSave = new QPushButton("💾 Save Game to JSON (savegame.json)");
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        controller->saveSession("savegame.json");
    });

    QPushButton* btnLoad = new QPushButton("📂 Load Game from JSON (savegame.json)");
    connect(btnLoad, &QPushButton::clicked, this, [this]() {
        controller->loadSession("savegame.json");
    });

    QPushButton* btnTestCorrupt = new QPushButton("⚠ Simulate Corrupt Save File Load (DP9 #5 / NFR3)");
    btnTestCorrupt->setStyleSheet("background-color: #fee2e2; color: #991b1b; border: 1px solid #f87171;");
    connect(btnTestCorrupt, &QPushButton::clicked, this, [this]() {
        controller->injectCorruptSaveTest("corrupt_save.json");
    });

    saveLayout->addWidget(btnSave);
    saveLayout->addWidget(btnLoad);
    saveLayout->addWidget(btnTestCorrupt);
    layout->addWidget(saveBox);

    layout->addWidget(new QLabel("Simulation & Weather Event Log:"));
    listSimulationLog = new QListWidget();
    layout->addWidget(listSimulationLog);

    return tab;
}

void MainWindow::onTileClicked(int x, int y) {
    int prevX = selectedX;
    int prevY = selectedY;
    selectedX = x;
    selectedY = y;

    // Repaint previous and newly selected tiles so the highlight border moves immediately
    updateTileButtonVisual(controller->getTileSnapshot(prevX, prevY));
    updateTileButtonVisual(controller->getTileSnapshot(selectedX, selectedY));
    updateInspectorPanel();
}

void MainWindow::onTileUpdated(const TileSnapshot& snap) {
    updateTileButtonVisual(snap);
    if (snap.x == selectedX && snap.y == selectedY) {
        updateInspectorPanel();
    }
}

void MainWindow::updateTileButtonVisual(const TileSnapshot& snap) {
    QPushButton* btn = gridButtons[snap.y][snap.x];
    bool isSelected = (snap.x == selectedX && snap.y == selectedY);

    QString label = isSelected
                        ? QString("★ (%1,%2) ★\n").arg(snap.x).arg(snap.y)
                        : QString("(%1,%2)\n").arg(snap.x).arg(snap.y);

    // High-contrast, accessible colors (Dark crisp text on distinct soft backgrounds)
    QString bgColor = "#f1f5f9";   // Cleared untamed ground (Soft Slate)
    QString textColor = "#0f172a"; // Deep Navy/Black text

    if (!snap.obstacle.empty()) {
        label += QString::fromStdString(snap.obstacle);
        if (snap.obstacle == "Tree") {
            bgColor = "#bbf7d0";   // Soft Forest Mint
            textColor = "#14532d"; // Deep Forest Green
        } else if (snap.obstacle == "Boulder") {
            bgColor = "#cbd5e1";   // Crisp Stone Grey
            textColor = "#1e293b"; // Dark Slate
        } else if (snap.obstacle == "Pond") {
            bgColor = "#bae6fd";   // Clear Sky Blue
            textColor = "#0c4a6e"; // Deep Ocean Blue
        }
    } else if (snap.hasCrop) {
        label += QString("%1\n[%2]\nVit: %3%")
        .arg(QString::fromStdString(snap.cropSpecies))
            .arg(QString::fromStdString(snap.cropStage))
            .arg(static_cast<int>(snap.cropVitality));
        if (snap.cropStage == "Mature") {
            bgColor = "#fef08a";   // Golden Harvest Yellow
            textColor = "#713f12"; // Dark Amber Brown
        } else if (snap.cropStage == "Rotten") {
            bgColor = "#fecaca";   // Soft Rose Red
            textColor = "#7f1d1d"; // Deep Crimson
        } else {
            bgColor = "#dcfce7";   // Vibrant Sprout Green
            textColor = "#14532d"; // Dark Green
        }
    } else if (snap.isTilled) {
        label += snap.isMulched ? "Tilled + Mulch\n" : "Tilled Soil\n";
        label += QString("H2O: %1%").arg(static_cast<int>(snap.moisture));
        bgColor = "#fed7aa";       // Warm Terracotta Earth
        textColor = "#431407";     // Deep Espresso Brown
    } else {
        label += QString("Cleared\nH2O: %1%").arg(static_cast<int>(snap.moisture));
    }

    // Selected tile gets a bright 3px solid amber-gold outline
    QString borderStyle = isSelected ? "3px solid #f59e0b" : "1px solid #94a3b8";

    btn->setText(label);
    btn->setStyleSheet(QString(
                           "QPushButton { background-color: %1; color: %2; border: %3; border-radius: 6px; font-size: 11px; font-weight: bold; padding: 2px; }"
                           "QPushButton:hover { border: 2px solid #2563eb; }"
                           ).arg(bgColor, textColor, borderStyle));
}

void MainWindow::updateInspectorPanel() {
    TileSnapshot s = controller->getTileSnapshot(selectedX, selectedY);
    QString info = QString(
                       "<div style='font-size: 14px; font-weight: bold; color: #b45309; margin-bottom: 4px;'>"
                       "★ Selected Tile: (%1, %2) | Sector (%3, %4)</div>")
                       .arg(s.x).arg(s.y).arg(controller->getRegionX()).arg(controller->getRegionY());

    info += QString("<b>Regional Biome:</b> %1<br>").arg(QString::fromStdString(controller->getActiveBiomeName()));
    info += QString("<b>Obstacle:</b> %1<br>").arg(s.obstacle.empty() ? "None (Cleared)" : QString::fromStdString(s.obstacle));
    info += QString("<b>Tilled Status:</b> %1<br>").arg(s.isTilled ? "Yes (Ready for Seed)" : "No (Untilled)");
    info += QString("<b>Soil Moisture:</b> %1%<br>").arg(s.moisture, 0, 'f', 1);
    info += QString("<b>Organic Mulch:</b> %1<br>").arg(s.isMulched ? "Installed (-50% Evap)" : "None");

    if (s.hasCrop) {
        info += QString("<hr><b>Planted Crop:</b> %1<br>").arg(QString::fromStdString(s.cropSpecies));
        info += QString("<b>Lifecycle Stage:</b> %1<br>").arg(QString::fromStdString(s.cropStage));
        info += QString("<b>Vitality Score:</b> %1 / 100.0").arg(s.cropVitality, 0, 'f', 1);
    } else {
        info += "<hr><i>No crop currently planted on this plot.</i>";
    }
    lblInspector->setText(info);
}

void MainWindow::refreshAllViews() {
    lblTreasury->setText(QString("💰 Treasury: $%1").arg(controller->getBalance(), 0, 'f', 2));
    lblDayWeather->setText(QString("📅 Day %1   |   🌍 Sector (%2,%3): %4   |   ⛅ Weather: %5")
                               .arg(controller->getCurrentDay())
                               .arg(controller->getRegionX())
                               .arg(controller->getRegionY())
                               .arg(QString::fromStdString(controller->getActiveBiomeName()))
                               .arg(QString::fromStdString(controller->getCurrentWeather())));

    // Sync both front-page and tab Auto-Run buttons without re-triggering signals
    bool running = controller->isAutoRunning();
    btnFrontAutoSim->blockSignals(true);
    btnFrontAutoSim->setChecked(running);
    btnFrontAutoSim->setText(running ? "⏸ Auto-Run: ON (Pause)" : "▶ Auto-Run: OFF");
    btnFrontAutoSim->blockSignals(false);

    btnAutoSim->blockSignals(true);
    btnAutoSim->setChecked(running);
    btnAutoSim->setText(running ? "⏸ Pause Continuous Simulation" : "▶ Start Continuous Simulation (Auto-Tick)");
    btnAutoSim->blockSignals(false);

    // Update Regional Navigation Bar & Biome Badge
    std::string bName = controller->getActiveBiomeName();
    QString badgeStyle = "background-color: #dcfce7; color: #14532d; border: 1px solid #86efac;";
    if (bName == "Desert") {
        badgeStyle = "background-color: #fef08a; color: #713f12; border: 1px solid #fde047;";
    } else if (bName == "Savanna") {
        badgeStyle = "background-color: #ffedd5; color: #7c2d12; border: 1px solid #fdba74;";
    }

    lblRegionBanner->setText(QString("Map Sector (%1, %2) — Active Biome: %3")
                                 .arg(controller->getRegionX())
                                 .arg(controller->getRegionY())
                                 .arg(QString::fromStdString(bName)));
    lblRegionBanner->setStyleSheet(QString("font-size: 13px; font-weight: bold; padding: 6px; border-radius: 5px; %1").arg(badgeStyle));

    btnNavWest->setEnabled(controller->getRegionX() > 0);
    btnNavEast->setEnabled(controller->getRegionX() < controller->getWorldWidth() - 1);
    btnNavNorth->setEnabled(controller->getRegionY() > 0);
    btnNavSouth->setEnabled(controller->getRegionY() < controller->getWorldHeight() - 1);

    for (int y = 0; y < controller->getGridHeight(); ++y) {
        for (int x = 0; x < controller->getGridWidth(); ++x) {
            updateTileButtonVisual(controller->getTileSnapshot(x, y));
        }
    }
    updateInspectorPanel();

    // Update Market Tab
    const Market& mkt = controller->getMarket();
    lblActivePricing->setText(QString("<b>Active Pricing Strategy:</b> %1").arg(QString::fromStdString(mkt.getStrategyName())));
    lblSaturationInfo->setText(QString("Market Saturation Volumes — Carrot: %1 | Wheat: %2 | Corn: %3")
                                   .arg(mkt.getSaturation("Carrot"))
                                   .arg(mkt.getSaturation("Wheat"))
                                   .arg(mkt.getSaturation("Corn")));

    const auto& batches = controller->getInventory().getBatches();
    tableMarketBatches->setRowCount(static_cast<int>(batches.size()));
    for (size_t i = 0; i < batches.size(); ++i) {
        tableMarketBatches->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(batches[i].getSpecies())));
        tableMarketBatches->setItem(i, 1, new QTableWidgetItem(QString::number(batches[i].getQuantity())));
        tableMarketBatches->setItem(i, 2, new QTableWidgetItem(QString("%1%").arg(batches[i].getVitality(), 0, 'f', 1)));
        double quote = mkt.quotePrice(batches[i]);
        tableMarketBatches->setItem(i, 3, new QTableWidgetItem(QString("$%1").arg(quote, 0, 'f', 2)));
    }

    // Update Store Inventory Summary
    const Inventory& inv = controller->getInventory();
    QString toolsList;
    for (const auto& t : inv.getTools()) toolsList += QString::fromStdString(t) + "  ";
    lblInventorySummary->setText(
        QString("<b>Owned Tools:</b> %1 | <b>Organic Mulch:</b> %2<br>"
                "<b>Seed Stock:</b> Carrot: %3 | Wheat: %4 | Corn: %5")
            .arg(toolsList)
            .arg(inv.getMulchCount())
            .arg(inv.getSeedCount("Carrot"))
            .arg(inv.getSeedCount("Wheat"))
            .arg(inv.getSeedCount("Corn"))
        );
}

void MainWindow::showNotification(const QString& msg, bool isError) {
    lblStatusBanner->setText(msg);
    if (isError) {
        lblStatusBanner->setStyleSheet("padding: 8px 12px; background-color: #fee2e2; color: #991b1b; font-weight: bold; border: 1px solid #f87171; border-radius: 5px;");
    } else {
        lblStatusBanner->setStyleSheet("padding: 8px 12px; background-color: #dcfce7; color: #14532d; font-weight: bold; border: 1px solid #86efac; border-radius: 5px;");
    }
    listSimulationLog->insertItem(0, QString("[Day %1] %2").arg(controller->getCurrentDay()).arg(msg));
}