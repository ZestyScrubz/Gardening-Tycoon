#pragma once
#include "Farm.h"
#include "Treasury.h"
#include "Inventory.h"
#include "Market.h"
#include "Simulation.h"
#include <string>

class ISaveStore {
public:
    virtual ~ISaveStore() = default;
    virtual void saveGame(const std::string& filepath,
                          const Farm& farm,
                          const Treasury& treasury,
                          const Inventory& inventory,
                          const Market& market,
                          const Simulation& sim) = 0;

    virtual void loadGame(const std::string& filepath,
                          Farm& farm,
                          Treasury& treasury,
                          Inventory& inventory,
                          Market& market,
                          Simulation& sim) = 0;
};

class JsonSaveStore : public ISaveStore {
public:
    void saveGame(const std::string& filepath,
                  const Farm& farm,
                  const Treasury& treasury,
                  const Inventory& inventory,
                  const Market& market,
                  const Simulation& sim) override;

    void loadGame(const std::string& filepath,
                  Farm& farm,
                  Treasury& treasury,
                  Inventory& inventory,
                  Market& market,
                  Simulation& sim) override;
};