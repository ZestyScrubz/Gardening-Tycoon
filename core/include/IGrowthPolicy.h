#pragma once
#include <utility>

class IGrowthPolicy {
public:
    virtual ~IGrowthPolicy() = default;
    virtual double computeGrowthDelta(double hydration, double solar) const = 0;
    virtual double computeVitalityDelta(double hydration, double solar) const = 0;
    virtual std::pair<double, double> getOptimalHydrationRange() const = 0;
};

class MoistureGrowthPolicy : public IGrowthPolicy {
private:
    double minHydration;
    double minSolar;
public:
    MoistureGrowthPolicy(double minH = 35.0, double minS = 4.0);
    double computeGrowthDelta(double hydration, double solar) const override;
    double computeVitalityDelta(double hydration, double solar) const override;
    std::pair<double, double> getOptimalHydrationRange() const override;
};

class SunlightGrowthPolicy : public IGrowthPolicy {
private:
    double minHydration;
    double minSolar;
public:
    SunlightGrowthPolicy(double minH = 20.0, double minS = 7.0);
    double computeGrowthDelta(double hydration, double solar) const override;
    double computeVitalityDelta(double hydration, double solar) const override;
    std::pair<double, double> getOptimalHydrationRange() const override;
};