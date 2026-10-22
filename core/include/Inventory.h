#pragma once
#include "HarvestBatch.h"
#include <unordered_map>
#include <vector>
#include <string>

class Inventory {
private:
    std::unordered_map<std::string, int> seedStock;
    std::vector<std::string> toolsHeld;
    std::vector<HarvestBatch> storageBatches;
    int mulchCount;

public:
    Inventory();
    void addSeeds(const std::string& species, int count);
    bool consumeSeed(const std::string& species);
    int getSeedCount(const std::string& species) const;

    void addTool(const std::string& toolName);
    bool hasTool(const std::string& toolName) const;
    const std::vector<std::string>& getTools() const { return toolsHeld; }

    void addMulch(int count);
    bool consumeMulch();
    int getMulchCount() const { return mulchCount; }
    void setMulchCount(int count) { mulchCount = count; }

    void addHarvest(const HarvestBatch& batch);
    HarvestBatch removeBatch(int index);
    const std::vector<HarvestBatch>& getBatches() const { return storageBatches; }
    const std::unordered_map<std::string, int>& getSeedStock() const { return seedStock; }
    void clearAll();
};