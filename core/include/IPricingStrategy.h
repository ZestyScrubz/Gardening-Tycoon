#pragma once
#include "HarvestBatch.h"
#include <string>

class IPricingStrategy {
public:
    virtual ~IPricingStrategy() = default;
    virtual double calculatePrice(const HarvestBatch& batch, int saturation) const = 0;
    virtual std::string getStrategyName() const = 0;
};

class LinearSaturationPricing : public IPricingStrategy {
private:
    double penaltyRate;
public:
    explicit LinearSaturationPricing(double rate = 0.12) : penaltyRate(rate) {}
    double calculatePrice(const HarvestBatch& batch, int saturation) const override;
    std::string getStrategyName() const override { return "Standard Linear Market"; }
};

class ThresholdSaturationPricing : public IPricingStrategy {
private:
    int threshold;
    double penaltyDrop;
public:
    ThresholdSaturationPricing(int thresh = 3, double drop = 0.40)
        : threshold(thresh), penaltyDrop(drop) {}
    double calculatePrice(const HarvestBatch& batch, int saturation) const override;
    std::string getStrategyName() const override { return "Threshold Bulk Market"; }
};

class FestivalBoomPricing : public IPricingStrategy {
public:
    double calculatePrice(const HarvestBatch& batch, int saturation) const override;
    std::string getStrategyName() const override { return "Festival Boom (1.5x Surge)"; }
};