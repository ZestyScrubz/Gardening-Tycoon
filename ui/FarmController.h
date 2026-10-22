#pragma once
#include <QObject>
#include <QTimer>
#include "Farm.h"
#include "Simulation.h"
#include "Treasury.h"
#include "Inventory.h"
#include "Market.h"
#include "ISaveStore.h"
#include "IRandomSource.h"
#include "ITileObserver.h"
#include <memory>
#include <vector>

class FarmController : public QObject, public ITileObserver {
    Q_OBJECT

private:
    struct RegionSector {
        std::unique_ptr<Farm> farm;
        std::unique_ptr<IBiomePolicy> biome;
        std::string biomeName;
    };

    static constexpr int WORLD_W = 3;
    static constexpr int WORLD_H = 3;
    int currentRegionX = 1; // Start in center sector (1, 1)
    int currentRegionY = 1;

    SystemRandomSource rng;
    std::vector<RegionSector> worldSectors;
    std::unique_ptr<Simulation> simulation;
    Treasury treasury;
    Inventory inventory;
    std::unique_ptr<Market> market;
    std::unique_ptr<ISaveStore> saveStore;
    QTimer* continuousTimer;

    RegionSector& activeSector();
    const RegionSector& activeSector() const;
    std::unique_ptr<IBiomePolicy> createBiomeByName(const std::string& name) const;

public:
    explicit FarmController(QObject* parent = nullptr);

    // ITileObserver implementation (DP6)
    void onTileUpdated(const TileSnapshot& state) override;

    // Directional Region Navigation (Up, Down, Left, Right)
    void navigateRegion(int dx, int dy);
    int getRegionX() const { return currentRegionX; }
    int getRegionY() const { return currentRegionY; }
    int getWorldWidth() const { return WORLD_W; }
    int getWorldHeight() const { return WORLD_H; }

    // Farm Grid Actions (UC1, UC3, UC4, FR13)
    void clearTileObstacle(int x, int y, const std::string& tool);
    void tillTile(int x, int y);
    void plantSeedOnTile(int x, int y, const std::string& species);
    void waterTile(int x, int y);
    void applyMulchToTile(int x, int y);
    void harvestTile(int x, int y);
    void clearRottenOnTile(int x, int y);

    // Economy & Store Actions (UC5, UC6, UC8)
    void buyTool(const std::string& toolName, double cost);
    void buySeed(const std::string& species, int count);
    void buyMulch(int count, double cost);
    void sellHarvestBatch(int index);
    void changePricingStrategy(const std::string& strategyType);

    // Simulation & Persistence Actions (UC2, UC7)
    void advanceSimulationTick();
    void toggleAutoRun(bool running, int intervalMs = 2000);
    bool isAutoRunning() const { return continuousTimer->isActive(); }
    void changeBiome(const std::string& biomeName);
    void saveSession(const std::string& filepath);
    void loadSession(const std::string& filepath);
    void injectCorruptSaveTest(const std::string& filepath);

    // Read-only state accessors for UI rendering
    int getGridWidth() const { return activeSector().farm->getWidth(); }
    int getGridHeight() const { return activeSector().farm->getHeight(); }
    TileSnapshot getTileSnapshot(int x, int y) const;
    double getBalance() const { return treasury.getBalance(); }
    int getCurrentDay() const { return simulation->getCurrentDay(); }
    std::string getCurrentWeather() const { return simulation->getWeatherModel().getCurrentWeather(); }
    std::string getActiveBiomeName() const { return activeSector().biomeName; }
    std::string getLastHazard() const { return simulation->getLastHazardReport(); }
    const Inventory& getInventory() const { return inventory; }
    const Market& getMarket() const { return *market; }

signals:
    void tileChanged(const TileSnapshot& snap);
    void stateRefreshed();
    void statusNotification(const QString& message, bool isError);
};