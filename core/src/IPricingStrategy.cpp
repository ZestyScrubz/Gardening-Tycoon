#include "IPricingStrategy.h"
#include "CropFactory.h"
#include <algorithm>

double LinearSaturationPricing::calculatePrice(const HarvestBatch& batch, int saturation) const {
    if (batch.getVitality() <= 0.0) return 0.0; // DP4: Rotten crops have zero value
    double base = CropFactory::getBaseValue(batch.getSpecies());
    double qualityMultiplier = 0.5 + (batch.getVitality() / 100.0);
    double saturationDiscount = std::clamp(1.0 - (saturation * penaltyRate), 0.25, 1.0);
    return base * batch.getQuantity() * qualityMultiplier * saturationDiscount;
}

double ThresholdSaturationPricing::calculatePrice(const HarvestBatch& batch, int saturation) const {
    if (batch.getVitality() <= 0.0) return 0.0;
    double base = CropFactory::getBaseValue(batch.getSpecies());
    double qualityMultiplier = 0.5 + (batch.getVitality() / 100.0);
    double discount = (saturation >= threshold) ? (1.0 - penaltyDrop) : 1.0;
    return base * batch.getQuantity() * qualityMultiplier * discount;
}

double FestivalBoomPricing::calculatePrice(const HarvestBatch& batch, int /*saturation*/) const {
    if (batch.getVitality() <= 0.0) return 0.0;
    double base = CropFactory::getBaseValue(batch.getSpecies());
    double qualityMultiplier = 0.5 + (batch.getVitality() / 100.0);
    return base * batch.getQuantity() * qualityMultiplier * 1.5; // FR8: 1.5x Festival Surge
}