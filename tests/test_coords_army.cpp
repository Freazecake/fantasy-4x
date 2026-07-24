#include "test_framework.hpp"

#include "core/Tile.hpp"
#include "core/Map.hpp"
#include "core/Army.hpp"
#include "core/Player.hpp"
#include "core/GameState.hpp"
#include "core/Unit.hpp"

TEST_CASE("Coords::distanceTo: соседняя клетка (dq=1, dr=0) = 1")
{
    Coords a(5, 5);
    Coords b(6, 5);

    CHECK(a.distanceTo(b) == 1);
}

TEST_CASE("Coords::distanceTo: та же клетка = 0")
{
    Coords a(5, 5);
    Coords b(5, 5);

    CHECK(a.distanceTo(b) == 0);
}

TEST_CASE("Coords::distanceTo: клетка через одну (dq=2) = 2")
{
    Coords a(5, 5);
    Coords b(7, 5);

    CHECK(a.distanceTo(b) == 2);
}

TEST_CASE("Coords::distanceTo: диагональ (dq=1, dr=1) = 2, а не 1")
{
    Coords a(5, 5);
    Coords b(6, 6);

    CHECK(a.distanceTo(b) == 2);
}

TEST_CASE("Coords::neighborOffsets: ровно 6 направлений")
{
    const auto &offsets = Coords::neighborOffsets();

    CHECK(offsets.size() == 6u);
}

TEST_CASE("Coords::neighbors: ровно 6 соседей у центральной клетки")
{
    Coords center(10, 10);
    auto nbrs = center.neighbors();

    CHECK(nbrs.size() == 6u);

    bool hasEast = false, hasWest = false;
    bool hasSe = false, hasNw = false;
    for (const auto &c : nbrs)
    {
        if (c.q == 11 && c.r == 10) hasEast = true;
        if (c.q == 9 && c.r == 10) hasWest = true;
        if (c.q == 10 && c.r == 11) hasSe = true;
        if (c.q == 10 && c.r == 9) hasNw = true;
    }
    CHECK(hasEast);
    CHECK(hasWest);
    CHECK(hasSe);
    CHECK(hasNw);
}

TEST_CASE("Coords::is_nearby: диагональ НЕ считается соседней (dq=1, dr=1)")
{
    Coords a(0, 0);
    Coords b(1, 1);

    CHECK(!a.is_nearby(b));
}

TEST_CASE("Coords::is_nearby: соседняя клетка считается соседней (dq=1, dr=0)")
{
    Coords a(0, 0);
    Coords b(1, 0);

    CHECK(a.is_nearby(b));
}

TEST_CASE("Map::tileAt возвращает null для несуществующей клетки")
{
    Map map = Map::generateMap(4, 4);

    const Tile *t = map.tileAt(10, 10);

    CHECK(t == nullptr);
}

TEST_CASE("Map::tileAt находит существующую клетку")
{
    Map map = Map::generateMap(4, 4);

    const Tile *t = map.tileAt(2, 3);

    CHECK(t != nullptr);
    CHECK(t->getCoords().q == 2);
    CHECK(t->getCoords().r == 3);
}

TEST_CASE("Map::reachableTiles: бюджет 2 на Grass даёт 18 клеток (6+12)")
{
    Map map = Map::generateMap(5, 5);

    auto tiles = map.reachableTiles({2, 2}, 2);

    CHECK(tiles.size() == 18u);
}

TEST_CASE("Map::reachableTiles: бюджет 1 на Grass даёт 6 соседей")
{
    Map map = Map::generateMap(5, 5);

    auto tiles = map.reachableTiles({2, 2}, 1);

    CHECK(tiles.size() == 6u);
}

TEST_CASE("Map::reachableTiles: центр не включается в результат")
{
    Map map = Map::generateMap(3, 3);

    auto tiles = map.reachableTiles({1, 1}, 5);

    for (const auto &c : tiles)
    {
        CHECK(!(c.q == 1 && c.r == 1));
    }
}

TEST_CASE("Map::reachableTiles: бюджет 0 даёт пустой результат")
{
    Map map = Map::generateMap(5, 5);

    auto tiles = map.reachableTiles({2, 2}, 0);

    CHECK(tiles.empty());
}

TEST_CASE("Army: movementPointsMax берёт минимум по юнитам")
{
    Unit fast, slow;
    fast = Unit::loadFromJson({
        {"id", "fast"}, {"name", "fast"},
        {"stats", {{"health", 1}, {"attack", 1}, {"defense", 1}, {"movementPoints", 4}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 1}, {"populationCost", 1}
    });
    slow = Unit::loadFromJson({
        {"id", "slow"}, {"name", "slow"},
        {"stats", {{"health", 1}, {"attack", 1}, {"defense", 1}, {"movementPoints", 2}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 1}, {"populationCost", 1}
    });

    Army army("test", 0, {0, 0}, {fast, slow});

    CHECK(army.movementPointsMax() == 2);
}

TEST_CASE("Army: moveTo списывает очки движения")
{
    Unit u = Unit::loadFromJson({
        {"id", "u"}, {"name", "u"},
        {"stats", {{"health", 1}, {"attack", 1}, {"defense", 1}, {"movementPoints", 5}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 1}, {"populationCost", 1}
    });

    Army army("test", 0, {0, 0}, {u});
    CHECK(army.movementPointsRemaining() == 5);

    army.moveTo({1, 0}, 2);
    CHECK(army.movementPointsRemaining() == 3);
    CHECK(army.position().q == 1);
    CHECK(army.position().r == 0);
}

TEST_CASE("Army: resetMovement восстанавливает очки")
{
    Unit u = Unit::loadFromJson({
        {"id", "u"}, {"name", "u"},
        {"stats", {{"health", 1}, {"attack", 1}, {"defense", 1}, {"movementPoints", 3}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 1}, {"populationCost", 1}
    });

    Army army("test", 0, {0, 0}, {u});
    army.moveTo({1, 0}, 2);
    army.resetMovement();

    CHECK(army.movementPointsRemaining() == 3);
}

TEST_CASE("Army: isDestroyed возвращает true когда юнитов нет")
{
    Army empty("empty", 0, {0, 0}, {});

    CHECK(empty.isDestroyed());
}

TEST_CASE("Player::detachArmy создаёт армию из гарнизона")
{
    Settlement s(0, SettlementType::Village);

    Unit u = Unit::loadFromJson({
        {"id", "militia"}, {"name", "militia"},
        {"stats", {{"health", 20}, {"attack", 3}, {"defense", 2}, {"movementPoints", 2}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 15}, {"populationCost", 30}
    });
    s.addToGarrison({u, u});

    CHECK(s.garrison().size() == 2u);

    Player p;
    p.setId(0);
    p.addSettlement(s);

    Army army = p.detachArmy(p.settlements()[0], {"militia", "militia"});

    CHECK(army.getId().find("army_p0_") == 0);
    CHECK(army.getOwnerId() == 0);
    CHECK(army.units().size() == 2u);
    CHECK(p.settlements()[0].garrison().empty());
}

TEST_CASE("Player::disbandArmyIntoSettlement возвращает юниты в гарнизон")
{
    Settlement s(0, SettlementType::Village);

    Unit u = Unit::loadFromJson({
        {"id", "militia"}, {"name", "militia"},
        {"stats", {{"health", 20}, {"attack", 3}, {"defense", 2}, {"movementPoints", 2}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 15}, {"populationCost", 30}
    });
    Army army("test_army", 0, {5, 5}, {u, u});

    Player p;
    p.setId(0);
    p.addSettlement(s);

    CHECK(p.settlements()[0].garrison().empty());

    p.disbandArmyIntoSettlement(army, p.settlements()[0]);

    CHECK(p.settlements()[0].garrison().size() == 2u);
}

TEST_CASE("GameState::findArmy возвращает nullptr для несуществующей армии")
{
    GameState state;

    CHECK(state.findArmy("nonexistent") == nullptr);
}

TEST_CASE("GameState::findArmy ищет по всем игрокам")
{
    GameState state;
    Player p1, p2;
    p1.setId(1);
    p2.setId(2);
    state.addPlayer(p1);
    state.addPlayer(p2);

    Unit u;
    state.players()[0].addArmy(Army("a1", 1, {0, 0}, {u}));
    state.players()[1].addArmy(Army("a2", 2, {1, 0}, {u}));

    CHECK(state.findArmy("a1") != nullptr);
    CHECK(state.findArmy("a2") != nullptr);
    CHECK(state.findArmy("a1")->getOwnerId() == 1);
    CHECK(state.findArmy("a2")->getOwnerId() == 2);
}

TEST_CASE("Player::removeArmy удаляет армию из списка игрока")
{
    Player p;
    p.setId(0);
    Unit u;
    p.addArmy(Army("a1", 0, {0, 0}, {u}));
    p.addArmy(Army("a2", 0, {1, 0}, {u}));

    CHECK(p.armies().size() == 2u);

    p.removeArmy("a1");

    CHECK(p.armies().size() == 1u);
    CHECK(p.armies()[0].getId() == "a2");
}
