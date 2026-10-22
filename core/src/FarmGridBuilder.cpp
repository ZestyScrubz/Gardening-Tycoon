#include "FarmGridBuilder.h"

FarmGridBuilder& FarmGridBuilder::setDimensions(int w, int h) {
    width = (w > 0) ? w : 8;
    height = (h > 0) ? h : 8;
    return *this;
}

FarmGridBuilder& FarmGridBuilder::setInitialMoisture(double moisture) {
    initialMoisture = moisture;
    return *this;
}

FarmGridBuilder& FarmGridBuilder::setPopulateObstacles(bool populate) {
    populateObstaclesFlag = populate;
    return *this;
}

std::unique_ptr<Farm> FarmGridBuilder::build(IRandomSource& rng) {
    auto farm = std::make_unique<Farm>(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::string obstacle = "";
            if (populateObstaclesFlag) {
                int roll = rng.getRandomInt(1, 100);
                if (roll <= 15) obstacle = "Tree";
                else if (roll <= 25) obstacle = "Boulder";
                else if (roll <= 30) obstacle = "Pond";
            }
            Tile* t = farm->getTileAt(x, y);
            t->restoreState(false, obstacle, initialMoisture, false, nullptr);
        }
    }
    return farm;
}