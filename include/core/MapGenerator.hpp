#pragma once

#include <vector>

#include "core/Map.hpp"
#include "core/Tile.hpp"

struct MapGenerationParams
{
    int width = 16;
    int height = 12;

    unsigned int seed = 0;

    float waterFraction = 0.12f;
    float mountainFraction = 0.10f;
    float forestFraction = 0.18f;
    float desertFraction = 0.10f;

    std::vector<Coords> guaranteedLandCenters;
    int guaranteedLandRadius = 2;

    int maxAttempts = 20;
};

class MapGenerator
{
public:
    static Map generate(const MapGenerationParams &params);

private:
    static Map tryGenerateOnce(const MapGenerationParams &params, unsigned int attemptSeed);
    static bool isFullyConnected(const Map &map);
    static std::vector<Coords> ringAround(Coords center, int radius, int width, int height);
};
