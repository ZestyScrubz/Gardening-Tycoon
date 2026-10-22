#include "Market.h"

Market::Market(std::unique_ptr<IPricingStrategy> strategy)
    : activeStrategy(std::move(strategy)) {}

void Market::setPricingStrategy(std::unique_ptr<IPricingStrategy> strategy) {
    if (strategy) {
        activeStrategy = std::move(strategy);
    }
}

double Market::quotePrice(const HarvestBatch& batch) const {
    int sat = getSaturation(batch.getSpecies());
    return activeStrategy->calculatePrice(batch, sat);
}

double Market::executeSale(const HarvestBatch& batch) {
    double payout = quotePrice(batch);
    recordSaleVolume(batch.getSpecies(), batch.getQuantity());
    return payout;
}

void Market::recordSaleVolume(const std::string& species, int count) {
    saturationLevels[species] += count;
}

int Market::getSaturation(const std::string& species) const {
    auto it = saturationLevels.find(species);
    return (it != saturationLevels.end()) ? it->second : 0;
}