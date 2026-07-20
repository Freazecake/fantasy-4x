#include "core/Tile.hpp"

#include <cmath>

bool Coords::is_nearby(const Coords &other) const
{
    return distanceTo(other) <= 1;
}

const std::vector<Coords> &Coords::neighborOffsets()
{
    static const std::vector<Coords> offsets = {
        {+1, 0}, {-1, 0}, {0, +1}, {0, -1}, {+1, -1}, {-1, +1}};
    return offsets;
}

int Coords::distanceTo(const Coords &b) const
{
    int dq = q - b.q, dr = r - b.r;
    return (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2;
}

std::vector<Coords> Coords::neighbors() const
{
    std::vector<Coords> result;
    for (const auto &off : neighborOffsets())
    {
        result.emplace_back(q + off.q, r + off.r);
    }
    return result;
}

Tile::Tile() {}

Tile::Tile(int q, int r) : coords(q, r) {}

Tile::Tile(const Tile &other)
{
    coords.q = other.coords.q;
    coords.r = other.coords.r;
    type = other.type;
}

int Tile::movementCost() const
{
    switch (type)
    {
    case TileType::Grass:
        return 1;
    case TileType::Desert:
        return 1;
    case TileType::Forest:
        return 2;
    case TileType::Mountain:
        return 3;
    case TileType::Water:
        return -1;
    }
    return 1;
}