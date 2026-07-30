#include "core/MapGenerator.hpp"

#include <algorithm>
#include <queue>
#include <random>
#include <unordered_set>

namespace
{

    long long packKey(int q, int r)
    {
        return (static_cast<long long>(q) << 32) | (r & 0xffffffff);
    }

    void growBlob(Map &map, Coords seed, TileType type, int targetSize,
                  const std::unordered_set<long long> &protectedTiles,
                  std::mt19937 &rng)
    {
        std::queue<Coords> frontier;
        frontier.push(seed);

        std::unordered_set<long long> visited;
        int placed = 0;
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);

        while (!frontier.empty() && placed < targetSize)
        {
            Coords cur = frontier.front();
            frontier.pop();

            long long k = packKey(cur.q, cur.r);
            if (visited.count(k) || protectedTiles.count(k))
                continue;
            visited.insert(k);

            Tile *tile = map.tileAt(cur.q, cur.r);
            if (!tile || tile->getType() != TileType::Grass)
                continue;

            tile->setType(type);
            ++placed;

            for (const auto &n : cur.neighbors())
            {
                if (chance(rng) < 0.9f)
                    frontier.push(n);
            }
        }
    }

}

std::vector<Coords> MapGenerator::ringAround(Coords center, int radius, int width, int height)
{
    std::vector<Coords> result;
    for (int q = 0; q < width; ++q)
    {
        for (int r = 0; r < height; ++r)
        {
            if (center.distanceTo({q, r}) <= radius)
                result.emplace_back(q, r);
        }
    }
    return result;
}

bool MapGenerator::isFullyConnected(const Map &map)
{
    const auto &tiles = map.getMap();

    Coords start{};
    bool foundStart = false;
    int landCount = 0;
    for (const auto &t : tiles)
    {
        if (t.getType() == TileType::Water)
            continue;
        ++landCount;
        if (!foundStart)
        {
            start = t.getCoords();
            foundStart = true;
        }
    }

    if (!foundStart)
        return true;

    int hugeBudget = static_cast<int>(tiles.size()) * 3 + 10;

    auto reachable = map.reachableTiles(start, hugeBudget);
    return static_cast<int>(reachable.size()) + 1 >= landCount;
}

Map MapGenerator::tryGenerateOnce(const MapGenerationParams &params, unsigned int attemptSeed)
{
    Map map = Map::generateMap(params.width, params.height);
    std::mt19937 rng(attemptSeed);

    std::unordered_set<long long> protectedTiles;
    for (const auto &center : params.guaranteedLandCenters)
    {
        for (const auto &c : ringAround(center, params.guaranteedLandRadius, params.width, params.height))
            protectedTiles.insert(packKey(c.q, c.r));
    }

    struct BlobSpec
    {
        TileType type;
        float fraction;
    };
    const std::vector<BlobSpec> specs = {
        {TileType::Water, params.waterFraction},
        {TileType::Mountain, params.mountainFraction},
        {TileType::Forest, params.forestFraction},
        {TileType::Desert, params.desertFraction},
    };

    const int totalTiles = params.width * params.height;
    const int BLOB_SIZE = 15;

    std::uniform_int_distribution<int> qDist(0, std::max(0, params.width - 1));
    std::uniform_int_distribution<int> rDist(0, std::max(0, params.height - 1));

    for (const auto &spec : specs)
    {
        int targetTotal = static_cast<int>(totalTiles * spec.fraction);
        if (targetTotal <= 0)
            continue;

        int blobsNeeded = std::max(1, targetTotal / BLOB_SIZE);
        for (int i = 0; i < blobsNeeded; ++i)
        {
            Coords seed{qDist(rng), rDist(rng)};
            growBlob(map, seed, spec.type, BLOB_SIZE, protectedTiles, rng);
        }
    }

    return map;
}

Map MapGenerator::generate(const MapGenerationParams &params)
{
    for (int attempt = 0; attempt < params.maxAttempts; ++attempt)
    {
        unsigned int attemptSeed = params.seed != 0
                                       ? params.seed + static_cast<unsigned int>(attempt)
                                       : std::random_device{}();

        Map map = tryGenerateOnce(params, attemptSeed);
        if (isFullyConnected(map))
            return map;
    }

    return Map::generateMap(params.width, params.height);
}
