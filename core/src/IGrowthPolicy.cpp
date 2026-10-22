#include "IGrowthPolicy.h"

MoistureGrowthPolicy::MoistureGrowthPolicy(double minH, double minS)
    : minHydration(minH), minSolar(minS) {}

double MoistureGrowthPolicy::computeGrowthDelta(double hydration, double solar) const {
    if (hydration >= minHydration && solar >= minSolar) {
        return 35.0; // Progresses stage at 100.0
    }
    return 0.0;
}

double MoistureGrowthPolicy::computeVitalityDelta(double hydration, double solar) const {
    auto [optMin, optMax] = getOptimalHydrationRange();
    if (hydration >= optMin && hydration <= optMax && solar >= minSolar) {
        return 6.0;
    }
    if (hydration < 15.0 || hydration > 92.0) {
        return -15.0; // Severe drought or waterlogging stress
    }
    return -5.0;
}

std::pair<double, double> MoistureGrowthPolicy::getOptimalHydrationRange() const {
    return {40.0, 85.0};
}

SunlightGrowthPolicy::SunlightGrowthPolicy(double minH, double minS)
    : minHydration(minH), minSolar(minS) {}

double SunlightGrowthPolicy::computeGrowthDelta(double hydration, double solar) const {
    if (hydration >= minHydration && solar >= minSolar) {
        return 40.0;
    }
    return 0.0;
}

double SunlightGrowthPolicy::computeVitalityDelta(double hydration, double solar) const {
    auto [optMin, optMax] = getOptimalHydrationRange();
    if (hydration >= optMin && hydration <= optMax && solar >= minSolar) {
        return 8.0;
    }
    if (solar < minSolar) {
        return -10.0;
    }
    return -6.0;
}

std::pair<double, double> SunlightGrowthPolicy::getOptimalHydrationRange() const {
    return {25.0, 70.0};
}