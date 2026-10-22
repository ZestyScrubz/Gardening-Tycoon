#pragma once
#include "Tile.h"
#include <vector>
#include <memory>

class Farm {
private:
    int width;
    int height;
    std::vector<std::unique_ptr<Tile>> plots;

public:
    Farm(int w, int h);
    Tile* getTileAt(int x, int y);
    const Tile* getTileAt(int x, int y) const;
    bool validateCoordinates(int x, int y) const;
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    void attachObserverToAll(ITileObserver* obs);
};