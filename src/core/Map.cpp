#include "core/Map.hpp"

Map::Map() {}

Map Map::generateMap(int width, int height)
{
    Map map;
    for (int q = 0; q < width; ++q)
    {
        for (int r = 0; r < height; ++r)
        {
            map.m_map.emplace_back(q, r);
        }
    }
    return map;
}