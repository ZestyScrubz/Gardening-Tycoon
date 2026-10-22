#include "Crop.h"
#include <algorithm>

Crop::Crop(std::string name, std::shared_ptr<const IGrowthPolicy> policy,
           double progress, double vitality, GrowthStage stage, int matureTicks)
    : speciesName(std::move(name)),
    growthProgress(progress),
    vitalityScore(std::clamp(vitality, 0.0, 100.0)),
    currentStage(stage),
    ticksSinceMaturity(matureTicks),
    growthPolicy(std::move(policy)) {}

void Crop::updateTick(double hydration, double solar) {
    if (currentStage == GrowthStage::Rotten) {
        return;
    }

    double vDelta = growthPolicy->computeVitalityDelta(hydration, solar);
    vitalityScore = std::clamp(vitalityScore + vDelta, 0.0, 100.0);

    // DP4: Zero vitality means dead/rotten crop
    if (vitalityScore <= 0.0) {
        currentStage = GrowthStage::Rotten;
        return;
    }

    if (currentStage == GrowthStage::Mature) {
        ticksSinceMaturity++;
        // FR12 & DP3: Transitions to Rotten on the 6th subsequent tick without harvesting
        if (ticksSinceMaturity >= 6) {
            currentStage = GrowthStage::Rotten;
            vitalityScore = 0.0;
        }
        return;
    }

    double gDelta = growthPolicy->computeGrowthDelta(hydration, solar);
    growthProgress += gDelta;
    if (growthProgress >= 100.0) {
        growthProgress = 0.0;
        advanceStage();
    }
}

void Crop::advanceStage() {
    switch (currentStage) {
    case GrowthStage::Seed:
        currentStage = GrowthStage::Sprout;
        break;
    case GrowthStage::Sprout:
        currentStage = GrowthStage::Vegetative;
        break;
    case GrowthStage::Vegetative:
        currentStage = GrowthStage::Mature;
        ticksSinceMaturity = 0;
        break;
    default:
        break;
    }
}

void Crop::applyHazardDamage(double damage) {
    vitalityScore = std::clamp(vitalityScore - damage, 0.0, 100.0);
    if (vitalityScore <= 0.0) {
        currentStage = GrowthStage::Rotten;
    }
}

bool Crop::isMature() const {
    return currentStage == GrowthStage::Mature;
}

bool Crop::isRotten() const {
    return currentStage == GrowthStage::Rotten;
}

double Crop::getVitality() const {
    return vitalityScore;
}

double Crop::getGrowthProgress() const {
    return growthProgress;
}

int Crop::getTicksSinceMaturity() const {
    return ticksSinceMaturity;
}

std::string Crop::getSpecies() const {
    return speciesName;
}

GrowthStage Crop::getStage() const {
    return currentStage;
}

std::string Crop::getStageString() const {
    switch (currentStage) {
    case GrowthStage::Seed: return "Seed";
    case GrowthStage::Sprout: return "Sprout";
    case GrowthStage::Vegetative: return "Vegetative";
    case GrowthStage::Mature: return "Mature";
    case GrowthStage::Rotten: return "Rotten";
    }
    return "Unknown";
}