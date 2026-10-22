#include "CropFactory.h"

std::unique_ptr<Crop> CropFactory::createCrop(const std::string& speciesName) {
    if (speciesName == "Wheat") {
        auto policy = std::make_shared<SunlightGrowthPolicy>(20.0, 5.5);
        return std::make_unique<Crop>("Wheat", policy);
    } else if (speciesName == "Corn") {
        auto policy = std::make_shared<SunlightGrowthPolicy>(25.0, 7.0);
        return std::make_unique<Crop>("Corn", policy);
    } else {
        // Default: Carrot (Moisture dependent)
        auto policy = std::make_shared<MoistureGrowthPolicy>(35.0, 4.0);
        return std::make_unique<Crop>("Carrot", policy);
    }
}

std::unique_ptr<Crop> CropFactory::restoreCrop(const std::string& speciesName,
                                               double progress, double vitality,
                                               GrowthStage stage, int matureTicks) {
    std::shared_ptr<const IGrowthPolicy> policy;
    if (speciesName == "Wheat") {
        policy = std::make_shared<SunlightGrowthPolicy>(20.0, 5.5);
    } else if (speciesName == "Corn") {
        policy = std::make_shared<SunlightGrowthPolicy>(25.0, 7.0);
    } else {
        policy = std::make_shared<MoistureGrowthPolicy>(35.0, 4.0);
    }
    return std::make_unique<Crop>(speciesName, policy, progress, vitality, stage, matureTicks);
}

double CropFactory::getBaseValue(const std::string& speciesName) {
    if (speciesName == "Wheat") return 25.0;
    if (speciesName == "Corn") return 45.0;
    if (speciesName == "Carrot") return 30.0;
    return 20.0;
}

double CropFactory::getSeedCost(const std::string& speciesName) {
    if (speciesName == "Wheat") return 10.0;
    if (speciesName == "Corn") return 20.0;
    if (speciesName == "Carrot") return 12.0;
    return 10.0;
}