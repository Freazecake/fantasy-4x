#pragma once

#include <vector>

#include "core/Tile.hpp"

namespace StartLayout
{
    constexpr int MAP_WIDTH = 20;
    constexpr int MAP_HEIGHT = 18;

    constexpr int CAPITAL_LAND_RADIUS = 2;

    inline const std::vector<Coords> &pentagonCapitals()
    {
        static const std::vector<Coords> positions = {
            {10, 2},
            {3, 7},
            {6, 15},
            {14, 15},
            {17, 7},
        };
        return positions;
    }
}
