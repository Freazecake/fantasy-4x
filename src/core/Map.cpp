#include "core/Map.hpp"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

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

const Tile *Map::tileAt(int q, int r) const
{
    for (const auto &tile : m_map)
    {
        if (tile.getCoords().q == q && tile.getCoords().r == r)
            return &tile;
    }
    return nullptr;
}

std::vector<Coords> Map::reachableTiles(Coords from, int movementBudget) const
{
    struct Node
    {
        int q, r;
        int cost;
    };

    auto cmp = [](const Node &a, const Node &b) { return a.cost > b.cost; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
    std::unordered_map<long long, int> best;

    auto key = [](int q, int r) { return (static_cast<long long>(q) << 32) | (r & 0xffffffff); };

    pq.push({from.q, from.r, 0});
    best[key(from.q, from.r)] = 0;

    std::vector<Coords> result;

    while (!pq.empty())
    {
        Node cur = pq.top();
        pq.pop();

        auto it = best.find(key(cur.q, cur.r));
        if (it != best.end() && it->second < cur.cost)
            continue;

        if (cur.cost > 0)
            result.emplace_back(cur.q, cur.r);

        for (const auto &off : Coords::neighborOffsets())
        {
            int nq = cur.q + off.q;
            int nr = cur.r + off.r;

            const Tile *t = tileAt(nq, nr);
            if (!t)
                continue;

            int stepCost = t->movementCost();
            if (stepCost < 0)
                continue;

            int newCost = cur.cost + stepCost;
            if (newCost > movementBudget)
                continue;

            long long k = key(nq, nr);
            auto it2 = best.find(k);
            if (it2 == best.end() || newCost < it2->second)
            {
                best[k] = newCost;
                pq.push({nq, nr, newCost});
            }
        }
    }

    return result;
}