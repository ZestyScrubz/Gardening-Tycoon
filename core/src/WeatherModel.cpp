#include "WeatherModel.h"

WeatherModel::WeatherModel(IRandomSource& rng)
    : currentWeather("Sunny"),
    precipitationAmount(0.0),
    solarMultiplier(1.0),
    randomSource(rng) {}

std::string WeatherModel::generateDailyWeather() {
    int roll = randomSource.getRandomInt(1, 100);
    if (roll <= 50) {
        currentWeather = "Sunny";
        precipitationAmount = 0.0;
        solarMultiplier = 1.1;
    } else if (roll <= 80) {
        currentWeather = "Rainstorm";
        precipitationAmount = 25.0;
        solarMultiplier = 0.7;
    } else {
        currentWeather = "Heatwave / Drought";
        precipitationAmount = 0.0;
        solarMultiplier = 1.4;
    }
    return currentWeather;
}