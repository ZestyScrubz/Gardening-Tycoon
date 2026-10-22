#include "Farm.h"

Farm::Farm(int w, int h) : width(w), height(h) {
    plots.reserve(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            plots.push_back(std::make_unique<Tile>(x, y));
        }
    }
}

bool Farm::validateCoordinates(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

Tile* Farm::getTileAt(int x, int y) {
    if (!validateCoordinates(x, y)) {
        return nullptr;
    }
    return plots[y * width + x].get();
}

const Tile* Farm::getTileAt(int x, int y) const {
    if (!validateCoordinates(x, y)) {
        return nullptr;
    }
    return plots[y * width + x].get();
}

void Farm::attachObserverToAll(ITileObserver* obs) {
    for (auto& tile : plots) {
        tile->addObserver(obs);
    }
}