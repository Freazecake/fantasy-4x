#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

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

    inline std::vector<Coords> capitalsForPlayerCount(int numPlayers, int width = MAP_WIDTH,
                                                      int height = MAP_HEIGHT)
    {
        if (numPlayers == 5)
            return pentagonCapitals();

        std::vector<Coords> positions;
        if (numPlayers <= 0)
            return positions;

        double centerQ = width / 2.0;
        double centerR = height / 2.0;
        double radiusQ = width * 0.35;
        double radiusR = height * 0.35;
        double startAngle = -M_PI / 2.0;

        int margin = CAPITAL_LAND_RADIUS + 1;

        for (int i = 0; i < numPlayers; ++i)
        {
            double angle = startAngle + (2.0 * M_PI * i) / numPlayers;
            int q = static_cast<int>(std::lround(centerQ + radiusQ * std::cos(angle)));
            int r = static_cast<int>(std::lround(centerR + radiusR * std::sin(angle)));
            q = std::clamp(q, margin, width - 1 - margin);
            r = std::clamp(r, margin, height - 1 - margin);
            positions.push_back(Coords(q, r));
        }
        return positions;
    }
}
