#include "Simulation.h"

Simulation::Simulation(std::unique_ptr<IBiomePolicy> biome, IRandomSource& rng)
    : currentDay(1),
    weatherModel(rng),
    activeBiome(std::move(biome)),
    randomSource(rng),
    lastHazardReport("None") {}

void Simulation::advanceDay(Farm& farm) {
    currentDay++;
    weatherModel.generateDailyWeather();

    double evapMult = activeBiome->getEvaporationMultiplier();
    if (weatherModel.getCurrentWeather() == "Heatwave / Drought") {
        evapMult *= 1.35;
    }

    double rain = weatherModel.getPrecipitationAmount();
    double effectiveSolar = activeBiome->getSolarExposure() * weatherModel.getSolarMultiplier();

    for (int y = 0; y < farm.getHeight(); ++y) {
        for (int x = 0; x < farm.getWidth(); ++x) {
            Tile* tile = farm.getTileAt(x, y);
            if (tile) {
                tile->updateDailyTick(evapMult, rain, effectiveSolar);
            }
        }
    }

    lastHazardReport = "No hazards today.";
    int hazardRoll = randomSource.getRandomInt(1, 100);
    if (hazardRoll <= 25) {
        triggerHazard(farm);
    }
}

void Simulation::triggerHazard(Farm& farm) {
    int targetX = randomSource.getRandomInt(0, farm.getWidth() - 1);
    int targetY = randomSource.getRandomInt(0, farm.getHeight() - 1);
    Tile* t = farm.getTileAt(targetX, targetY);
    if (t && t->isOccupied()) {
        Hazard rabbitAttack("Wild Rabbit Pest", 25.0);
        rabbitAttack.resolveHazard(*t);
        lastHazardReport = "Wild Rabbit chewed crop at (" + std::to_string(targetX) +
                           ", " + std::to_string(targetY) + ")! (-25 Vitality)";
    }
}

void Simulation::setBiome(std::unique_ptr<IBiomePolicy> biome) {
    if (biome) {
        activeBiome = std::move(biome);
    }
}