#pragma once
#include <string>

class HarvestBatch {
private:
    std::string speciesName;
    int quantity;
    double harvestVitality;

public:
    HarvestBatch(std::string species, int qty, double vitality)
        : speciesName(std::move(species)), quantity(qty), harvestVitality(vitality) {}

    std::string getSpecies() const { return speciesName; }
    int getQuantity() const { return quantity; }
    double getVitality() const { return harvestVitality; }
    void setQuantity(int q) { quantity = q; }
};