#pragma once
#include "Farm.h"
#include "IRandomSource.h"
#include <memory>

class FarmGridBuilder {
private:
    int width = 8;
    int height = 8;
    double initialMoisture = 55.0;
    bool populateObstaclesFlag = true;

public:
    FarmGridBuilder& setDimensions(int w, int h);
    FarmGridBuilder& setInitialMoisture(double moisture);
    FarmGridBuilder& setPopulateObstacles(bool populate);
    std::unique_ptr<Farm> build(IRandomSource& rng);
};