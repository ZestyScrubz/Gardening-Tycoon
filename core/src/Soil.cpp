#include "Soil.h"
#include <algorithm>

Soil::Soil(double initialMoisture, double baseEvap)
    : moistureLevel(std::clamp(initialMoisture, 0.0, 100.0)),
    baseEvaporationRate(baseEvap),
    isMulched(false) {}

void Soil::applyWater(double amount) {
    if (amount > 0.0) {
        moistureLevel = std::clamp(moistureLevel + amount, 0.0, 100.0);
    }
}

void Soil::applyMulch() {
    isMulched = true;
}

void Soil::evaporate(double multiplier) {
    double effectiveFactor = isMulched ? 0.5 : 1.0; // FR13: Mulch reduces moisture loss by 50%
    double loss = baseEvaporationRate * multiplier * effectiveFactor;
    moistureLevel = std::clamp(moistureLevel - loss, 0.0, 100.0);
}

double Soil::getMoisture() const {
    return moistureLevel;
}

bool Soil::getIsMulched() const {
    return isMulched;
}

void Soil::setMoisture(double moisture) {
    moistureLevel = std::clamp(moisture, 0.0, 100.0);
}

void Soil::setMulched(bool mulched) {
    isMulched = mulched;
}