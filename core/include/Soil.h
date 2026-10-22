#pragma once

class Soil {
private:
    double moistureLevel;
    double baseEvaporationRate;
    bool isMulched;

public:
    explicit Soil(double initialMoisture = 50.0, double baseEvap = 8.0);
    void applyWater(double amount);
    void applyMulch();
    void evaporate(double multiplier);
    double getMoisture() const;
    bool getIsMulched() const;
    void setMoisture(double moisture);
    void setMulched(bool mulched);
};