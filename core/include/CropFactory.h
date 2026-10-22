#pragma once
#include "Crop.h"
#include <memory>
#include <string>

class CropFactory {
public:
    static std::unique_ptr<Crop> createCrop(const std::string& speciesName);
    static std::unique_ptr<Crop> restoreCrop(const std::string& speciesName,
                                             double progress, double vitality,
                                             GrowthStage stage, int matureTicks);
    static double getBaseValue(const std::string& speciesName);
    static double getSeedCost(const std::string& speciesName);
};