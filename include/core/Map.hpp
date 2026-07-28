#pragma once

#include <vector>

#include "Tile.hpp"

class Map
{
private:
    std::vector<Tile> m_map;

public:
    Map();
    static Map generateMap(int width, int height);
    const std::vector<Tile> &getMap() const { return m_map; }
    std::vector<Tile> &getMap() { return m_map; }

    const Tile *tileAt(int q, int r) const;
    Tile *tileAt(int q, int r);
    std::vector<Coords> reachableTiles(Coords from, int movementBudget) const;
    int movementCostTo(Coords from, Coords to, int budget) const;
};
