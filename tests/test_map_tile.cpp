#include "test_framework.hpp"

#include "core/Map.hpp"
#include "core/Tile.hpp"

TEST_CASE("Coords::is_nearby: соседняя клетка (dq=1, dr=0)")
{
    Coords a(5, 5);
    Coords b(6, 5);

    CHECK(a.is_nearby(b));
}

TEST_CASE("Coords::is_nearby: та же клетка считается соседней (dq=0, dr=0)")
{
    Coords a(5, 5);
    Coords b(5, 5);

    CHECK(a.is_nearby(b));
}

TEST_CASE("Coords::is_nearby: клетка через одну — не соседняя (dq=2)")
{
    Coords a(5, 5);
    Coords b(7, 5);

    CHECK(!a.is_nearby(b));
}

TEST_CASE("Coords::is_nearby: диагональная клетка (dq=1, dr=1) не считается соседней")
{
    Coords a(5, 5);
    Coords b(6, 6);

    CHECK(!a.is_nearby(b));
}

TEST_CASE("Tile: конструктор по координатам сохраняет q/r и тип по умолчанию Grass")
{
    Tile t(3, 4);

    CHECK(t.getCoords().q == 3);
    CHECK(t.getCoords().r == 4);
    CHECK(t.getType() == TileType::Grass);
}

TEST_CASE("Tile: конструктор копирования переносит координаты и тип")
{
    Tile original(1, 2);
    Tile copy(original);

    CHECK(copy.getCoords().q == 1);
    CHECK(copy.getCoords().r == 2);
    CHECK(copy.getType() == original.getType());
}

TEST_CASE("Tile::is_nearby делегирует в Coords::is_nearby")
{
    Tile a(0, 0);
    Tile b(1, 0);
    Tile far(10, 10);

    CHECK(a.is_nearby(b));
    CHECK(!a.is_nearby(far));
}

TEST_CASE("Map::generateMap создаёт width*height клеток")
{
    Map map = Map::generateMap(4, 3);

    CHECK(map.getMap().size() == 12u);
}

TEST_CASE("Map::generateMap: клетка с координатами (0,0) существует")
{
    Map map = Map::generateMap(2, 2);

    bool found = false;
    for (const auto &tile : map.getMap())
    {
        if (tile.getCoords().q == 0 && tile.getCoords().r == 0)
        {
            found = true;
            break;
        }
    }

    CHECK(found);
}

TEST_CASE("Map::generateMap с нулевым размером не падает и даёт пустую карту")
{
    Map map = Map::generateMap(0, 0);

    CHECK(map.getMap().empty());
}
