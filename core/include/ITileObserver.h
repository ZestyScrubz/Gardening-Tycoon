#pragma once
#include <string>

struct TileSnapshot {
    int x;
    int y;
    bool isTilled;
    bool isMulched;
    std::string obstacle;
    double moisture;
    bool hasCrop;
    std::string cropSpecies;
    std::string cropStage;
    double cropVitality;
};

class ITileObserver {
public:
    virtual ~ITileObserver() = default;
    virtual void onTileUpdated(const TileSnapshot& state) = 0;
};