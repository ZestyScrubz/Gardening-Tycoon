#pragma once
#include "Tile.h"
#include <string>

class Hazard {
private:
    std::string hazardType;
    double severityScore;

public:
    Hazard(std::string type, double severity);
    void resolveHazard(Tile& targetTile) const;
    std::string getHazardType() const { return hazardType; }
    double getSeverity() const { return severityScore; }
};