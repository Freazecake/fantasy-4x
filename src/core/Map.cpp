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

int Map::movementCostTo(Coords from, Coords to, int budget) const
{
    long long targetKey = (static_cast<long long>(to.q) << 32) | (to.r & 0xffffffff);

    auto cmp = [](const std::pair<int, long long> &a, const std::pair<int, long long> &b)
    { return a.first > b.first; };
    std::priority_queue<std::pair<int, long long>,
                        std::vector<std::pair<int, long long>>, decltype(cmp)> pq(cmp);
    std::unordered_map<long long, int> best;

    auto key = [](int q, int r) { return (static_cast<long long>(q) << 32) | (r & 0xffffffff); };

    long long startKey = key(from.q, from.r);
    pq.push({0, startKey});
    best[startKey] = 0;

    while (!pq.empty())
    {
        auto cur = pq.top();
        pq.pop();
        int curCost = cur.first;
        long long curKey = cur.second;

        if (curKey == targetKey)
            return curCost;

        auto it = best.find(curKey);
        if (it != best.end() && it->second < curCost)
            continue;

        int cq = static_cast<int>(curKey >> 32);
        int cr = static_cast<int>(curKey & 0xffffffff);

        for (const auto &off : Coords::neighborOffsets())
        {
            int nq = cq + off.q;
            int nr = cr + off.r;

            const Tile *t = tileAt(nq, nr);
            if (!t)
                continue;

            int stepCost = t->movementCost();
            if (stepCost < 0)
                continue;

            int newCost = curCost + stepCost;
            if (newCost > budget)
                continue;

            long long nk = key(nq, nr);
            auto it2 = best.find(nk);
            if (it2 == best.end() || newCost < it2->second)
            {
                best[nk] = newCost;
                pq.push({newCost, nk});
            }
        }
    }

    return -1;
}