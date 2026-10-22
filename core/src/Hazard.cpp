#include "Hazard.h"

Hazard::Hazard(std::string type, double severity)
    : hazardType(std::move(type)), severityScore(severity) {}

void Hazard::resolveHazard(Tile& targetTile) const {
    targetTile.triggerPestDamage(severityScore);
}