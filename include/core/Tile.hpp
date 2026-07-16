#pragma once

#include <iostream>

struct Coords
{
    int q;
    int r;

    bool is_nearby(const Coords &) const;
    Coords() {};
    Coords(int q, int r) : q(q), r(r) {}
};

enum class TileType
{
    Grass,
    Water,
    Mountain,
    Desert,
    Forest,
};

class Tile
{
private:
    Coords coords;
    TileType type = TileType::Grass;

public:
    Tile();
    Tile(int, int);
    Tile(const Tile &);
    Coords getCoords() const { return coords; }
    TileType getType() const { return type; }
    bool is_nearby(const Tile &other) const { return this->coords.is_nearby(other.coords); }
};

inline std::ostream &operator<<(std::ostream &os, const TileType &type)
{
    switch (type)
    {
    case TileType::Grass:
        os << "Grass";
        break;
    case TileType::Water:
        os << "Water";
        break;
    case TileType::Mountain:
        os << "Mountain";
        break;
    case TileType::Desert:
        os << "Desert";
        break;
    case TileType::Forest:
        os << "Forest";
        break;
    default:
        os << "Unknown";
        break;
    }
    return os;
}
