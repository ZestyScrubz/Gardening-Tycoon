#pragma once
#include "IRandomSource.h"
#include <string>

class WeatherModel {
private:
    std::string currentWeather;
    double precipitationAmount;
    double solarMultiplier;
    IRandomSource& randomSource;

public:
    explicit WeatherModel(IRandomSource& rng);
    std::string generateDailyWeather();
    std::string getCurrentWeather() const { return currentWeather; }
    double getSolarMultiplier() const { return solarMultiplier; }
    double getPrecipitationAmount() const { return precipitationAmount; }
};