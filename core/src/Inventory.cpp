#include "Inventory.h"
#include <algorithm>
#include <stdexcept>

Inventory::Inventory() : mulchCount(2) {
    // Starter kit
    toolsHeld.push_back("Hoe");
    seedStock["Carrot"] = 3;
    seedStock["Wheat"] = 2;
    seedStock["Corn"] = 1;
}

void Inventory::addSeeds(const std::string& species, int count) {
    if (count > 0) {
        seedStock[species] += count;
    }
}

bool Inventory::consumeSeed(const std::string& species) {
    auto it = seedStock.find(species);
    if (it != seedStock.end() && it->second > 0) {
        it->second--;
        return true;
    }
    return false;
}

int Inventory::getSeedCount(const std::string& species) const {
    auto it = seedStock.find(species);
    return (it != seedStock.end()) ? it->second : 0;
}

void Inventory::addTool(const std::string& toolName) {
    if (!hasTool(toolName)) {
        toolsHeld.push_back(toolName);
    }
}

bool Inventory::hasTool(const std::string& toolName) const {
    return std::find(toolsHeld.begin(), toolsHeld.end(), toolName) != toolsHeld.end();
}

void Inventory::addMulch(int count) {
    if (count > 0) mulchCount += count;
}

bool Inventory::consumeMulch() {
    if (mulchCount > 0) {
        mulchCount--;
        return true;
    }
    return false;
}

void Inventory::addHarvest(const HarvestBatch& batch) {
    storageBatches.push_back(batch);
}

HarvestBatch Inventory::removeBatch(int index) {
    if (index < 0 || index >= static_cast<int>(storageBatches.size())) {
        throw std::out_of_range("Invalid inventory batch index.");
    }
    HarvestBatch b = storageBatches[index];
    storageBatches.erase(storageBatches.begin() + index);
    return b;
}

void Inventory::clearAll() {
    seedStock.clear();
    toolsHeld.clear();
    storageBatches.clear();
    mulchCount = 0;
}