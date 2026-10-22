#pragma once
#include <string>

class IBiomePolicy {
public:
    virtual ~IBiomePolicy() = default;
    virtual double getEvaporationMultiplier() const = 0;
    virtual double getSolarExposure() const = 0;
    virtual std::string getBiomeName() const = 0;
};

class PlainsBiome : public IBiomePolicy {
public:
    double getEvaporationMultiplier() const override { return 1.0; }
    double getSolarExposure() const override { return 6.0; }
    std::string getBiomeName() const override { return "Plains"; }
};

class DesertBiome : public IBiomePolicy {
public:
    double getEvaporationMultiplier() const override { return 1.8; }
    double getSolarExposure() const override { return 9.5; }
    std::string getBiomeName() const override { return "Desert"; }
};

class SavannaBiome : public IBiomePolicy {
public:
    double getEvaporationMultiplier() const override { return 1.4; }
    double getSolarExposure() const override { return 8.0; }
    std::string getBiomeName() const override { return "Savanna"; }
};