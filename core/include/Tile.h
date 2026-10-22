#pragma once
#include "Soil.h"
#include "Crop.h"
#include "HarvestBatch.h"
#include "ITileObserver.h"
#include <string>
#include <memory>
#include <vector>

class Tile {
private:
    int coordinateX;
    int coordinateY;
    bool isTilled;
    std::string currentObstacle; // "", "Tree", "Boulder", "Pond"
    Soil soil;
    std::unique_ptr<Crop> hostedCrop;
    std::vector<ITileObserver*> observers;

    void notifyObservers();

public:
    Tile(int x, int y, std::string obstacle = "", double initialMoisture = 50.0);

    void addObserver(ITileObserver* obs);
    void clearObstacle(const std::string& tool);
    void tillSoil();
    void plantSeed(std::unique_ptr<Crop> crop);
    HarvestBatch harvest();
    void clearRottenCrop();

    void waterTile(double amount);
    void applyMulch();
    void updateDailyTick(double evapMultiplier, double rainAmount, double solarHours);
    void triggerPestDamage(double damage);

    bool hasObstacle() const;
    bool isOccupied() const;
    bool getIsTilled() const;
    std::string getObstacle() const;
    int getX() const { return coordinateX; }
    int getY() const { return coordinateY; }
    const Soil& getSoil() const { return soil; }
    const Crop* getCrop() const { return hostedCrop.get(); }

    TileSnapshot getSnapshot() const;
    void restoreState(bool tilled, std::string obstacle, double moisture, bool mulched, std::unique_ptr<Crop> crop);
};