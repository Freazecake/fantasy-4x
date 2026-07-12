#include "core/Tile.hpp"

#include <cmath>

bool Coords::is_nearby(const Coords &other)
{
    return abs(this->q - other.q) <= 1 && abs(this->r - other.r) <= 1;
}

Tile::Tile() {}

Tile::Tile(int q, int r) : coords(q, r) {}

Tile::Tile(const Tile &other)
{
    coords.q = other.coords.q;
    coords.r = other.coords.r;
}