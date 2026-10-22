#pragma once
#include "Farm.h"
#include "WeatherModel.h"
#include "IBiomePolicy.h"
#include "Hazard.h"
#include "IRandomSource.h"
#include <memory>
#include <string>

class Simulation {
private:
    int currentDay;
    WeatherModel weatherModel;
    std::unique_ptr<IBiomePolicy> activeBiome;
    IRandomSource& randomSource;
    std::string lastHazardReport;

public:
    Simulation(std::unique_ptr<IBiomePolicy> biome, IRandomSource& rng);
    void advanceDay(Farm& farm);
    void triggerHazard(Farm& farm);
    void setBiome(std::unique_ptr<IBiomePolicy> biome);

    int getCurrentDay() const { return currentDay; }
    void setCurrentDay(int day) { currentDay = day; }
    const IBiomePolicy& getActiveBiome() const { return *activeBiome; }
    const WeatherModel& getWeatherModel() const { return weatherModel; }
    std::string getLastHazardReport() const { return lastHazardReport; }
};