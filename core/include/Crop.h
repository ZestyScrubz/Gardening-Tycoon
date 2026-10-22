#pragma once
#include "IGrowthPolicy.h"
#include <string>
#include <memory>

enum class GrowthStage {
    Seed,
    Sprout,
    Vegetative,
    Mature,
    Rotten
};

class Crop {
private:
    std::string speciesName;
    double growthProgress;
    double vitalityScore;
    GrowthStage currentStage;
    int ticksSinceMaturity;
    std::shared_ptr<const IGrowthPolicy> growthPolicy;

public:
    Crop(std::string name, std::shared_ptr<const IGrowthPolicy> policy,
         double progress = 0.0, double vitality = 80.0,
         GrowthStage stage = GrowthStage::Seed, int matureTicks = 0);

    void updateTick(double hydration, double solar);
    void advanceStage();
    void applyHazardDamage(double damage);

    bool isMature() const;
    bool isRotten() const;
    double getVitality() const;
    double getGrowthProgress() const;
    int getTicksSinceMaturity() const;
    std::string getSpecies() const;
    GrowthStage getStage() const;
    std::string getStageString() const;
};