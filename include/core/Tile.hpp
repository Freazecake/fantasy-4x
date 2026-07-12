#pragma once

struct Coords
{
    int q;
    int r;

    bool is_nearby(const Coords &);
    Coords() {};
    Coords(int q, int r) : q(q), r(r) {}
};

class Tile
{
private:
    Coords coords;

public:
    Tile();
    Tile(int, int);
    Tile(const Tile &);
};