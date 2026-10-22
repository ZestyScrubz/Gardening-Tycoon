#pragma once
#include "IPricingStrategy.h"
#include "HarvestBatch.h"
#include <unordered_map>
#include <memory>
#include <string>

class Market {
private:
    std::unordered_map<std::string, int> saturationLevels;
    std::unique_ptr<IPricingStrategy> activeStrategy;

public:
    explicit Market(std::unique_ptr<IPricingStrategy> strategy);
    void setPricingStrategy(std::unique_ptr<IPricingStrategy> strategy);
    double quotePrice(const HarvestBatch& batch) const;
    double executeSale(const HarvestBatch& batch);
    void recordSaleVolume(const std::string& species, int count);
    int getSaturation(const std::string& species) const;
    const std::unordered_map<std::string, int>& getAllSaturation() const { return saturationLevels; }
    void setSaturation(const std::string& species, int val) { saturationLevels[species] = val; }
    void clearSaturation() { saturationLevels.clear(); }
    std::string getStrategyName() const { return activeStrategy->getStrategyName(); }
};