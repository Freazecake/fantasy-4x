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
};