#include "ISaveStore.h"
#include "Exceptions.h"
#include "CropFactory.h"
#include <fstream>
#include <sstream>
#include <vector>

void JsonSaveStore::saveGame(const std::string& filepath,
                             const Farm& farm,
                             const Treasury& treasury,
                             const Inventory& inventory,
                             const Market& market,
                             const Simulation& sim) {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        throw CorruptSaveFileException("Failed to open file for writing: " + filepath);
    }

    out << "{\n";
    out << "  \"schema_version\": 1,\n";
    out << "  \"day\": " << sim.getCurrentDay() << ",\n";
    out << "  \"biome\": \"" << sim.getActiveBiome().getBiomeName() << "\",\n";
    out << "  \"balance\": " << treasury.getBalance() << ",\n";
    out << "  \"mulch\": " << inventory.getMulchCount() << ",\n";

    // Tools
    out << "  \"tools\": [";
    const auto& tools = inventory.getTools();
    for (size_t i = 0; i < tools.size(); ++i) {
        out << "\"" << tools[i] << "\"" << (i + 1 < tools.size() ? ", " : "");
    }
    out << "],\n";

    // Seeds
    out << "  \"seeds\": {\n";
    out << "    \"Carrot\": " << inventory.getSeedCount("Carrot") << ",\n";
    out << "    \"Wheat\": " << inventory.getSeedCount("Wheat") << ",\n";
    out << "    \"Corn\": " << inventory.getSeedCount("Corn") << "\n";
    out << "  },\n";

    // Harvest Batches
    out << "  \"batches\": [\n";
    const auto& batches = inventory.getBatches();
    for (size_t i = 0; i < batches.size(); ++i) {
        out << "    {\"species\": \"" << batches[i].getSpecies()
        << "\", \"qty\": " << batches[i].getQuantity()
        << ", \"vitality\": " << batches[i].getVitality() << "}"
        << (i + 1 < batches.size() ? "," : "") << "\n";
    }
    out << "  ],\n";

    // Tiles
    out << "  \"tiles\": [\n";
    int total = farm.getWidth() * farm.getHeight();
    int idx = 0;
    for (int y = 0; y < farm.getHeight(); ++y) {
        for (int x = 0; x < farm.getWidth(); ++x) {
            const Tile* t = farm.getTileAt(x, y);
            const Crop* c = t->getCrop();
            out << "    {\"x\": " << x << ", \"y\": " << y
                << ", \"tilled\": " << (t->getIsTilled() ? 1 : 0)
                << ", \"mulched\": " << (t->getSoil().getIsMulched() ? 1 : 0)
                << ", \"moisture\": " << t->getSoil().getMoisture()
                << ", \"obstacle\": \"" << t->getObstacle() << "\"";
            if (c) {
                out << ", \"hasCrop\": 1"
                    << ", \"species\": \"" << c->getSpecies() << "\""
                    << ", \"stage\": " << static_cast<int>(c->getStage())
                    << ", \"progress\": " << c->getGrowthProgress()
                    << ", \"vitality\": " << c->getVitality()
                    << ", \"matureTicks\": " << c->getTicksSinceMaturity();
            } else {
                out << ", \"hasCrop\": 0";
            }
            out << "}" << (++idx < total ? "," : "") << "\n";
        }
    }
    out << "  ]\n";
    out << "}\n";
}

// Helper functions for safe validation parsing without third-party dependencies
static std::string extractToken(const std::string& src, const std::string& key) {
    auto pos = src.find("\"" + key + "\":");
    if (pos == std::string::npos) {
        throw CorruptSaveFileException("Missing required key: " + key);
    }
    pos += key.size() + 3;
    while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t')) pos++;
    if (pos >= src.size()) throw CorruptSaveFileException("Truncated payload after " + key);

    if (src[pos] == '"') {
        auto endQuote = src.find('"', pos + 1);
        if (endQuote == std::string::npos) throw CorruptSaveFileException("Unclosed string for " + key);
        return src.substr(pos + 1, endQuote - pos - 1);
    } else {
        size_t endPos = pos;
        while (endPos < src.size() && (std::isdigit(src[endPos]) || src[endPos] == '.' || src[endPos] == '-')) {
            endPos++;
        }
        return src.substr(pos, endPos - pos);
    }
}

void JsonSaveStore::loadGame(const std::string& filepath,
                             Farm& farm,
                             Treasury& treasury,
                             Inventory& inventory,
                             Market& market,
                             Simulation& sim) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        throw CorruptSaveFileException("Failed to open save file: " + filepath);
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();

    if (content.find("\"schema_version\": 1") == std::string::npos || content.find("\"tiles\":") == std::string::npos) {
        throw CorruptSaveFileException("Failed to load save file: Missing schema_version 1 or corrupted structure.");
    }

    try {
        int day = std::stoi(extractToken(content, "day"));
        double balance = std::stod(extractToken(content, "balance"));
        int mulch = std::stoi(extractToken(content, "mulch"));
        std::string biomeName = extractToken(content, "biome");

        if (day < 1 || balance < 0.0 || mulch < 0) {
            throw CorruptSaveFileException("Save file contains negative/invalid domain numbers.");
        }

        int carrots = std::stoi(extractToken(content, "Carrot"));
        int wheat = std::stoi(extractToken(content, "Wheat"));
        int corn = std::stoi(extractToken(content, "Corn"));

        // Stage all parsed tiles in temporary memory first (NFR3: Prevent partial overwrite on failure)
        struct TempTileData {
            int x, y;
            bool tilled, mulched;
            double moisture;
            std::string obstacle;
            bool hasCrop;
            std::string species;
            int stage;
            double progress, vitality;
            int matureTicks;
        };
        std::vector<TempTileData> parsedTiles;

        auto tilesPos = content.find("\"tiles\":");
        auto pos = content.find('{', tilesPos);
        while (pos != std::string::npos) {
            auto endObj = content.find('}', pos);
            if (endObj == std::string::npos) throw CorruptSaveFileException("Unclosed tile object in JSON.");
            std::string objStr = content.substr(pos, endObj - pos + 1);

            TempTileData td;
            td.x = std::stoi(extractToken(objStr, "x"));
            td.y = std::stoi(extractToken(objStr, "y"));
            if (!farm.validateCoordinates(td.x, td.y)) {
                throw CorruptSaveFileException("Out-of-bounds tile coordinates in save file.");
            }
            td.tilled = (std::stoi(extractToken(objStr, "tilled")) != 0);
            td.mulched = (std::stoi(extractToken(objStr, "mulched")) != 0);
            td.moisture = std::stod(extractToken(objStr, "moisture"));
            td.obstacle = extractToken(objStr, "obstacle");
            td.hasCrop = (std::stoi(extractToken(objStr, "hasCrop")) != 0);
            if (td.hasCrop) {
                td.species = extractToken(objStr, "species");
                td.stage = std::stoi(extractToken(objStr, "stage"));
                td.progress = std::stod(extractToken(objStr, "progress"));
                td.vitality = std::stod(extractToken(objStr, "vitality"));
                td.matureTicks = std::stoi(extractToken(objStr, "matureTicks"));
            }
            parsedTiles.push_back(td);
            pos = content.find('{', endObj);
        }

        // All validations succeeded — commit atomically to running session
        sim.setCurrentDay(day);
        if (biomeName == "Desert") sim.setBiome(std::make_unique<DesertBiome>());
        else if (biomeName == "Savanna") sim.setBiome(std::make_unique<SavannaBiome>());
        else sim.setBiome(std::make_unique<PlainsBiome>());

        treasury.setBalance(balance);
        inventory.clearAll();
        inventory.addMulch(mulch);
        inventory.addSeeds("Carrot", carrots);
        inventory.addSeeds("Wheat", wheat);
        inventory.addSeeds("Corn", corn);

        if (content.find("\"Hoe\"") != std::string::npos) inventory.addTool("Hoe");
        if (content.find("\"Axe\"") != std::string::npos) inventory.addTool("Axe");
        if (content.find("\"Pickaxe\"") != std::string::npos) inventory.addTool("Pickaxe");
        if (content.find("\"Shovel\"") != std::string::npos) inventory.addTool("Shovel");

        // Restore batches
        auto batchesPos = content.find("\"batches\":");
        if (batchesPos != std::string::npos && batchesPos < tilesPos) {
            auto bPos = content.find('{', batchesPos);
            while (bPos != std::string::npos && bPos < tilesPos) {
                auto bEnd = content.find('}', bPos);
                std::string bStr = content.substr(bPos, bEnd - bPos + 1);
                std::string sp = extractToken(bStr, "species");
                int q = std::stoi(extractToken(bStr, "qty"));
                double vit = std::stod(extractToken(bStr, "vitality"));
                inventory.addHarvest(HarvestBatch(sp, q, vit));
                bPos = content.find('{', bEnd);
            }
        }

        for (const auto& td : parsedTiles) {
            Tile* t = farm.getTileAt(td.x, td.y);
            std::unique_ptr<Crop> crop = nullptr;
            if (td.hasCrop) {
                crop = CropFactory::restoreCrop(td.species, td.progress, td.vitality,
                                                static_cast<GrowthStage>(td.stage), td.matureTicks);
            }
            t->restoreState(td.tilled, td.obstacle, td.moisture, td.mulched, std::move(crop));
        }
        market.clearSaturation();
    } catch (const CorruptSaveFileException&) {
        throw;
    } catch (const std::exception& e) {
        throw CorruptSaveFileException(std::string("Corrupt save file syntax: ") + e.what());
    }
}