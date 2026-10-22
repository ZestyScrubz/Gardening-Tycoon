#include "FarmController.h"
#include "FarmGridBuilder.h"
#include "CropFactory.h"
#include "Exceptions.h"
#include <fstream>

FarmController::FarmController(QObject* parent)
    : QObject(parent),
    treasury(250.0) {
    worldSectors.resize(WORLD_W * WORLD_H);

    // Build a 3x3 world of farm grids, each with a procedural biome
    for (int ry = 0; ry < WORLD_H; ++ry) {
        for (int rx = 0; rx < WORLD_W; ++rx) {
            int idx = ry * WORLD_W + rx;
            std::string bName = "Plains";
            double startMoisture = 55.0;

            // Center plot (1,1) starts as Plains; surrounding plots roll random biomes
            if (rx != 1 || ry != 1) {
                int roll = rng.getRandomInt(1, 3);
                if (roll == 1) {
                    bName = "Plains";
                    startMoisture = 55.0;
                } else if (roll == 2) {
                    bName = "Desert";
                    startMoisture = 30.0;
                } else {
                    bName = "Savanna";
                    startMoisture = 42.0;
                }
            }

            FarmGridBuilder builder;
            worldSectors[idx].farm = builder.setDimensions(8, 8)
                                         .setInitialMoisture(startMoisture)
                                         .setPopulateObstacles(true)
                                         .build(rng);
            worldSectors[idx].biome = createBiomeByName(bName);
            worldSectors[idx].biomeName = bName;
        }
    }

    // Attach observer to the starting center sector
    activeSector().farm->attachObserverToAll(this);

    simulation = std::make_unique<Simulation>(createBiomeByName(activeSector().biomeName), rng);
    market = std::make_unique<Market>(std::make_unique<LinearSaturationPricing>());
    saveStore = std::make_unique<JsonSaveStore>();

    continuousTimer = new QTimer(this);
    connect(continuousTimer, &QTimer::timeout, this, &FarmController::advanceSimulationTick);
}

FarmController::RegionSector& FarmController::activeSector() {
    return worldSectors[currentRegionY * WORLD_W + currentRegionX];
}

const FarmController::RegionSector& FarmController::activeSector() const {
    return worldSectors[currentRegionY * WORLD_W + currentRegionX];
}

std::unique_ptr<IBiomePolicy> FarmController::createBiomeByName(const std::string& name) const {
    if (name == "Desert") return std::make_unique<DesertBiome>();
    if (name == "Savanna") return std::make_unique<SavannaBiome>();
    return std::make_unique<PlainsBiome>();
}

void FarmController::onTileUpdated(const TileSnapshot& state) {
    emit tileChanged(state);
}

void FarmController::navigateRegion(int dx, int dy) {
    int nextX = currentRegionX + dx;
    int nextY = currentRegionY + dy;

    if (nextX < 0 || nextX >= WORLD_W || nextY < 0 || nextY >= WORLD_H) {
        emit statusNotification("Reached the edge of the regional map!", true);
        return;
    }

    currentRegionX = nextX;
    currentRegionY = nextY;
    simulation->setBiome(createBiomeByName(activeSector().biomeName));

    emit stateRefreshed();
    emit statusNotification(
        QString("Moved to Sector (%1, %2) — Regional Biome: %3")
            .arg(currentRegionX)
            .arg(currentRegionY)
            .arg(QString::fromStdString(activeSector().biomeName)),
        false);
}

TileSnapshot FarmController::getTileSnapshot(int x, int y) const {
    return activeSector().farm->getTileAt(x, y)->getSnapshot();
}

void FarmController::clearTileObstacle(int x, int y, const std::string& tool) {
    try {
        if (!inventory.hasTool(tool)) {
            emit statusNotification(QString("You do not own the %1! Buy it in the Town Store.").arg(QString::fromStdString(tool)), true);
            return;
        }
        Tile* t = activeSector().farm->getTileAt(x, y);
        if (!t || !t->hasObstacle()) {
            emit statusNotification("No obstacle on this tile.", true);
            return;
        }
        const double clearCost = 15.0;
        if (!treasury.canAfford(clearCost)) {
            throw InsolventFundsException("Transaction rejected: Clearing an obstacle costs $15.00.");
        }
        t->clearObstacle(tool);
        treasury.debit(clearCost);
        emit stateRefreshed();
        emit statusNotification(QString("Cleared obstacle at (%1, %2) for $15.00.").arg(x).arg(y), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::tillTile(int x, int y) {
    try {
        if (!inventory.hasTool("Hoe")) {
            throw IncompatibleToolException("Tilling requires a Hoe.");
        }
        Tile* t = activeSector().farm->getTileAt(x, y);
        t->tillSoil();
        emit stateRefreshed();
        emit statusNotification(QString("Tilled plot (%1, %2).").arg(x).arg(y), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::plantSeedOnTile(int x, int y, const std::string& species) {
    try {
        if (inventory.getSeedCount(species) <= 0) {
            emit statusNotification(QString("Out of %1 seeds! Purchase more in the Town Store.").arg(QString::fromStdString(species)), true);
            return;
        }
        Tile* t = activeSector().farm->getTileAt(x, y);
        auto crop = CropFactory::createCrop(species);
        t->plantSeed(std::move(crop));
        inventory.consumeSeed(species);
        emit stateRefreshed();
        emit statusNotification(QString("Planted %1 at (%2, %3).").arg(QString::fromStdString(species)).arg(x).arg(y), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::waterTile(int x, int y) {
    Tile* t = activeSector().farm->getTileAt(x, y);
    if (t) {
        t->waterTile(25.0);
        emit stateRefreshed();
        emit statusNotification(QString("Watered tile (%1, %2) (+25% moisture).").arg(x).arg(y), false);
    }
}

void FarmController::applyMulchToTile(int x, int y) {
    try {
        if (inventory.getMulchCount() <= 0) {
            emit statusNotification("No Organic Mulch in inventory! Purchase some in the Town Store.", true);
            return;
        }
        Tile* t = activeSector().farm->getTileAt(x, y);
        t->applyMulch();
        inventory.consumeMulch();
        emit stateRefreshed();
        emit statusNotification(QString("Applied Organic Mulch to (%1, %2). Evaporation reduced by 50%!").arg(x).arg(y), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::harvestTile(int x, int y) {
    try {
        Tile* t = activeSector().farm->getTileAt(x, y);
        HarvestBatch batch = t->harvest();
        inventory.addHarvest(batch);
        emit stateRefreshed();
        emit statusNotification(QString("Harvested Mature %1 (Vitality: %2%)!")
                                    .arg(QString::fromStdString(batch.getSpecies()))
                                    .arg(batch.getVitality(), 0, 'f', 1), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::clearRottenOnTile(int x, int y) {
    try {
        Tile* t = activeSector().farm->getTileAt(x, y);
        t->clearRottenCrop();
        emit stateRefreshed();
        emit statusNotification(QString("Cleared rotten crop carcass from (%1, %2).").arg(x).arg(y), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::buyTool(const std::string& toolName, double cost) {
    try {
        if (inventory.hasTool(toolName)) {
            emit statusNotification("You already own this tool!", true);
            return;
        }
        treasury.debit(cost);
        inventory.addTool(toolName);
        emit stateRefreshed();
        emit statusNotification(QString("Purchased %1 for $%2.").arg(QString::fromStdString(toolName)).arg(cost, 0, 'f', 2), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::buySeed(const std::string& species, int count) {
    try {
        double totalCost = CropFactory::getSeedCost(species) * count;
        treasury.debit(totalCost);
        inventory.addSeeds(species, count);
        emit stateRefreshed();
        emit statusNotification(QString("Purchased %1x %2 seeds for $%3.")
                                    .arg(count).arg(QString::fromStdString(species)).arg(totalCost, 0, 'f', 2), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::buyMulch(int count, double cost) {
    try {
        treasury.debit(cost);
        inventory.addMulch(count);
        emit stateRefreshed();
        emit statusNotification(QString("Purchased %1x Organic Mulch for $%2.").arg(count).arg(cost, 0, 'f', 2), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::sellHarvestBatch(int index) {
    try {
        const auto& batches = inventory.getBatches();
        if (index < 0 || index >= static_cast<int>(batches.size())) {
            emit statusNotification("Select a valid harvest batch to sell.", true);
            return;
        }
        HarvestBatch batch = inventory.removeBatch(index);
        double payout = market->executeSale(batch);
        treasury.credit(payout);
        emit stateRefreshed();
        emit statusNotification(QString("Sold %1 batch for $%2!")
                                    .arg(QString::fromStdString(batch.getSpecies()))
                                    .arg(payout, 0, 'f', 2), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::changePricingStrategy(const std::string& strategyType) {
    if (strategyType == "Threshold") {
        market->setPricingStrategy(std::make_unique<ThresholdSaturationPricing>());
    } else if (strategyType == "Festival") {
        market->setPricingStrategy(std::make_unique<FestivalBoomPricing>());
    } else {
        market->setPricingStrategy(std::make_unique<LinearSaturationPricing>());
    }
    emit stateRefreshed();
    emit statusNotification(QString("Swapped Market Strategy to: %1")
                                .arg(QString::fromStdString(market->getStrategyName())), false);
}

void FarmController::advanceSimulationTick() {
    int startDay = simulation->getCurrentDay();

    // Advance the active sector through the primary Simulation engine
    simulation->setBiome(createBiomeByName(activeSector().biomeName));
    simulation->advanceDay(*activeSector().farm);
    int newDay = simulation->getCurrentDay();

    double rain = simulation->getWeatherModel().getPrecipitationAmount();
    double weatherSolarMult = simulation->getWeatherModel().getSolarMultiplier();
    bool isDrought = (simulation->getWeatherModel().getCurrentWeather() == "Heatwave / Drought");

    // Advance all surrounding sectors according to their local biomes
    for (int i = 0; i < WORLD_W * WORLD_H; ++i) {
        if (i == currentRegionY * WORLD_W + currentRegionX) continue;
        auto& sec = worldSectors[i];
        double evap = sec.biome->getEvaporationMultiplier() * (isDrought ? 1.35 : 1.0);
        double solar = sec.biome->getSolarExposure() * weatherSolarMult;
        for (int y = 0; y < sec.farm->getHeight(); ++y) {
            for (int x = 0; x < sec.farm->getWidth(); ++x) {
                sec.farm->getTileAt(x, y)->updateDailyTick(evap, rain, solar);
            }
        }
    }

    simulation->setCurrentDay(newDay);
    emit stateRefreshed();
    emit statusNotification(QString("Day %1 — Weather: %2 | %3")
                                .arg(simulation->getCurrentDay())
                                .arg(QString::fromStdString(simulation->getWeatherModel().getCurrentWeather()))
                                .arg(QString::fromStdString(simulation->getLastHazardReport())), false);
}

void FarmController::toggleAutoRun(bool running, int intervalMs) {
    if (running) {
        continuousTimer->start(intervalMs);
        emit statusNotification("Continuous simulation running...", false);
    } else {
        continuousTimer->stop();
        emit statusNotification("Simulation paused.", false);
    }
    emit stateRefreshed();
}

void FarmController::changeBiome(const std::string& biomeName) {
    activeSector().biomeName = biomeName;
    activeSector().biome = createBiomeByName(biomeName);
    simulation->setBiome(createBiomeByName(biomeName));
    emit stateRefreshed();
    emit statusNotification(QString("Sector (%1, %2) Biome changed to %3.")
                                .arg(currentRegionX).arg(currentRegionY)
                                .arg(QString::fromStdString(biomeName)), false);
}

void FarmController::saveSession(const std::string& filepath) {
    try {
        simulation->setBiome(createBiomeByName(activeSector().biomeName));
        saveStore->saveGame(filepath, *activeSector().farm, treasury, inventory, *market, *simulation);
        emit statusNotification(QString("Game saved successfully to %1").arg(QString::fromStdString(filepath)), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::loadSession(const std::string& filepath) {
    try {
        saveStore->loadGame(filepath, *activeSector().farm, treasury, inventory, *market, *simulation);
        activeSector().biomeName = simulation->getActiveBiome().getBiomeName();
        activeSector().biome = createBiomeByName(activeSector().biomeName);
        emit stateRefreshed();
        emit statusNotification(QString("Game loaded successfully from %1").arg(QString::fromStdString(filepath)), false);
    } catch (const std::exception& e) {
        emit statusNotification(QString::fromStdString(e.what()), true);
    }
}

void FarmController::injectCorruptSaveTest(const std::string& filepath) {
    std::ofstream bad(filepath);
    bad << "{ \"schema_version\": 1, \"day\": -99, \"balance\": -500, CORRUPTED_TRUNCATED_JSON ";
    bad.close();
    loadSession(filepath);
}