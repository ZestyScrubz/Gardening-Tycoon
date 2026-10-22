#include "Tile.h"
#include "Exceptions.h"

Tile::Tile(int x, int y, std::string obstacle, double initialMoisture)
    : coordinateX(x),
    coordinateY(y),
    isTilled(false),
    currentObstacle(std::move(obstacle)),
    soil(initialMoisture),
    hostedCrop(nullptr) {}

void Tile::addObserver(ITileObserver* obs) {
    if (obs) {
        observers.push_back(obs);
    }
}

void Tile::notifyObservers() {
    TileSnapshot snap = getSnapshot();
    for (auto* obs : observers) {
        obs->onTileUpdated(snap);
    }
}

void Tile::clearObstacle(const std::string& tool) {
    if (!hasObstacle()) {
        return;
    }
    if (currentObstacle == "Tree" && tool != "Axe") {
        throw IncompatibleToolException("Incompatible tool: Trees require an Axe to clear.");
    }
    if (currentObstacle == "Boulder" && tool != "Pickaxe") {
        throw IncompatibleToolException("Incompatible tool: Boulders require a Pickaxe to clear.");
    }
    if (currentObstacle == "Pond" && tool != "Shovel") {
        throw IncompatibleToolException("Incompatible tool: Ponds require a Shovel to fill/clear.");
    }
    currentObstacle = "";
    notifyObservers();
}

void Tile::tillSoil() {
    if (hasObstacle()) {
        throw UntilledPlotException("Cannot till: Tile is obstructed by a " + currentObstacle + ".");
    }
    if (isOccupied()) {
        throw UntilledPlotException("Cannot till: Tile already contains an active crop.");
    }
    isTilled = true;
    notifyObservers();
}

void Tile::plantSeed(std::unique_ptr<Crop> crop) {
    if (hasObstacle() || !isTilled) {
        throw UntilledPlotException("Cannot plant: Plot must be cleared of obstacles and tilled with a hoe first.");
    }
    if (isOccupied()) {
        throw UntilledPlotException("Cannot plant: Plot is already occupied by a crop.");
    }
    hostedCrop = std::move(crop);
    notifyObservers();
}

HarvestBatch Tile::harvest() {
    if (!isOccupied()) {
        throw InvalidHarvestStateException("Cannot harvest: No crop planted on this tile.");
    }
    if (!hostedCrop->isMature()) {
        throw InvalidHarvestStateException("Cannot harvest: Crop is in state '" +
                                           hostedCrop->getStageString() + "' (must be Mature).");
    }

    HarvestBatch batch(hostedCrop->getSpecies(), 1, hostedCrop->getVitality());
    hostedCrop.reset();
    isTilled = false; // Reverts to uncultivated state after harvest (FR6 / DP3)
    notifyObservers();
    return batch;
}

void Tile::clearRottenCrop() {
    if (isOccupied() && hostedCrop->isRotten()) {
        hostedCrop.reset();
        isTilled = false;
        notifyObservers();
    } else {
        throw IncompatibleToolException("No rotten crop on this tile to clear.");
    }
}

void Tile::waterTile(double amount) {
    soil.applyWater(amount);
    notifyObservers();
}

void Tile::applyMulch() {
    if (!isTilled) {
        throw UntilledPlotException("Cannot apply mulch: Plot must be tilled first.");
    }
    soil.applyMulch();
    notifyObservers();
}

void Tile::updateDailyTick(double evapMultiplier, double rainAmount, double solarHours) {
    if (rainAmount > 0.0) {
        soil.applyWater(rainAmount);
    }
    soil.evaporate(evapMultiplier);

    if (hostedCrop) {
        hostedCrop->updateTick(soil.getMoisture(), solarHours);
    }
    notifyObservers();
}

void Tile::triggerPestDamage(double damage) {
    if (hostedCrop) {
        hostedCrop->applyHazardDamage(damage);
        notifyObservers();
    }
}

bool Tile::hasObstacle() const {
    return !currentObstacle.empty();
}

bool Tile::isOccupied() const {
    return hostedCrop != nullptr;
}

bool Tile::getIsTilled() const {
    return isTilled;
}

std::string Tile::getObstacle() const {
    return currentObstacle;
}

TileSnapshot Tile::getSnapshot() const {
    TileSnapshot s;
    s.x = coordinateX;
    s.y = coordinateY;
    s.isTilled = isTilled;
    s.isMulched = soil.getIsMulched();
    s.obstacle = currentObstacle;
    s.moisture = soil.getMoisture();
    s.hasCrop = (hostedCrop != nullptr);
    if (hostedCrop) {
        s.cropSpecies = hostedCrop->getSpecies();
        s.cropStage = hostedCrop->getStageString();
        s.cropVitality = hostedCrop->getVitality();
    } else {
        s.cropSpecies = "";
        s.cropStage = "";
        s.cropVitality = 0.0;
    }
    return s;
}

void Tile::restoreState(bool tilled, std::string obstacle, double moisture, bool mulched, std::unique_ptr<Crop> crop) {
    isTilled = tilled;
    currentObstacle = std::move(obstacle);
    soil.setMoisture(moisture);
    soil.setMulched(mulched);
    hostedCrop = std::move(crop);
    notifyObservers();
}