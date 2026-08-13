#include "test_framework.hpp"

#include "core/Tile.hpp"
#include "core/Map.hpp"
#include "core/Army.hpp"
#include "core/Player.hpp"
#include "core/GameState.hpp"
#include "core/Combat.hpp"
#include "core/Military.hpp"

static Unit makeUnit(const std::string &id, float hp, float atk, float def, int mp = 2)
{
    return Unit::loadFromJson({
        {"id", id},
        {"name", id},
        {"stats", {{"health", hp}, {"attack", atk}, {"defense", def}, {"movementPoints", mp}}},
        {"tier", 0},
        {"requiredBuildingId", ""},
        {"recruitCost", 1},
        {"populationCost", 1},
    });
}

// ─── Combat::resolve ───────────────────────────────────────────────────

TEST_CASE("Combat::resolve: атакующий сильнее — победа атакующего")
{
    auto attacker = makeUnit("strong", 50, 20, 10);
    auto defender = makeUnit("weak", 20, 5, 3);

    auto result = Combat::resolve({attacker}, {defender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(result.attackerWon);
    CHECK(result.attackerSurvivors.size() == 1u);
    CHECK(result.defenderSurvivors.empty());
}

TEST_CASE("Combat::resolve: защитник сильнее — победа защитника")
{
    auto attacker = makeUnit("weak", 20, 5, 3);
    auto defender = makeUnit("strong", 50, 20, 10);

    auto result = Combat::resolve({attacker}, {defender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(!result.attackerWon);
    CHECK(result.attackerSurvivors.empty());
    CHECK(result.defenderSurvivors.size() == 1u);
}

TEST_CASE("Combat::resolve: оба уничтожены — attackerWon = false")
{
    auto a = makeUnit("a", 20, 30, 0);
    auto d = makeUnit("d", 20, 30, 0);

    auto result = Combat::resolve({a}, {d}, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(!result.attackerWon);
    CHECK(result.attackerSurvivors.empty());
    CHECK(result.defenderSurvivors.empty());
}

TEST_CASE("Combat::resolve: лог содержит записи")
{
    auto attacker = makeUnit("a", 50, 10, 5);
    auto defender = makeUnit("d", 30, 8, 4);

    auto result = Combat::resolve({attacker}, {defender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(!result.log.empty());
    bool hasAttackerStrike = false, hasDefenderStrike = false;
    for (const auto &entry : result.log)
    {
        if (entry.attackerUnitId == "a") hasAttackerStrike = true;
        if (entry.attackerUnitId == "d") hasDefenderStrike = true;
    }
    CHECK(hasAttackerStrike);
    CHECK(hasDefenderStrike);
}

TEST_CASE("Combat::resolve: бонус местности защитнику влияет на исход")
{
    auto attacker = makeUnit("a", 30, 8, 3);
    auto defender = makeUnit("d", 20, 6, 5);

    auto noBonus = Combat::resolve({attacker}, {defender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);
    auto withBonus = Combat::resolve({attacker}, {defender}, {}, {}, 2.0f, {}, 1.0f, 1.0f);

    CHECK(noBonus.attackerWon);
    CHECK(!withBonus.attackerWon);
}

TEST_CASE("Combat::resolve: расовый модификатор влияет")
{
    auto unit = makeUnit("u", 30, 10, 5);

    auto weakRace = Combat::resolve({unit}, {unit}, {}, {}, 1.0f, {}, 0.5f, 1.0f);
    auto strongRace = Combat::resolve({unit}, {unit}, {}, {}, 1.0f, {}, 2.0f, 1.0f);

    CHECK(!weakRace.attackerWon);
    CHECK(strongRace.attackerWon);
}

TEST_CASE("Combat::resolve: штраф необученности влияет")
{
    Unit defender = makeUnit("d", 20, 10, 3);
    defender.setTrained(true);

    Unit trained = makeUnit("t", 30, 10, 5);
    trained.setTrained(true);
    auto trainedResult = Combat::resolve({trained}, {defender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    Unit untrained = makeUnit("u", 30, 10, 5);
    // untrained.isTrained() = false by default
    auto untrainedResult = Combat::resolve({untrained}, {defender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(trainedResult.attackerWon);
    CHECK(!untrainedResult.attackerWon);
}

TEST_CASE("Combat::resolve: несколько юнитов с обеих сторон")
{
    std::vector<Unit> attackers = {
        makeUnit("a1", 30, 8, 4),
        makeUnit("a2", 25, 7, 3),
    };
    std::vector<Unit> defenders = {
        makeUnit("d1", 20, 5, 2),
        makeUnit("d2", 20, 5, 2),
    };

    auto result = Combat::resolve(attackers, defenders, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(result.attackerWon);
}

TEST_CASE("Combat::resolve: герой усиливает атакующего")
{
    auto attacker = makeUnit("a", 30, 10, 5);
    auto defender = makeUnit("d", 30, 10, 5);

    json heroJson = {
        {"id", "hero"}, {"name", "hero"}, {"raceId", "any"},
        {"modifiers", {{"science",1},{"industry",1},{"trade",1},{"diplomacy",1},{"military",1.5f},{"growth",1}}},
    };
    Hero hero = Hero::loadFromJson(heroJson);

    auto noHero = Combat::resolve({attacker}, {defender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);
    auto withHero = Combat::resolve({attacker}, {defender}, {hero}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(!noHero.attackerWon);
    CHECK(withHero.attackerWon);
}

// ─── Map::movementCostTo ──────────────────────────────────────────────

TEST_CASE("Map::movementCostTo: соседняя клетка = 1 (Grass)")
{
    Map map = Map::generateMap(5, 5);
    int cost = map.movementCostTo({2, 2}, {3, 2}, 5);
    CHECK(cost == 1);
}

TEST_CASE("Map::movementCostTo: недостижимо за бюджет -> -1")
{
    Map map = Map::generateMap(5, 5);
    int cost = map.movementCostTo({2, 2}, {4, 4}, 2);
    CHECK(cost == -1);
}

TEST_CASE("Map::movementCostTo: та же клетка = 0")
{
    Map map = Map::generateMap(5, 5);
    int cost = map.movementCostTo({2, 2}, {2, 2}, 5);
    CHECK(cost == 0);
}

TEST_CASE("Map::movementCostTo: клетка за картой -> -1 (бюджет не важен)")
{
    Map map = Map::generateMap(5, 5);
    int cost = map.movementCostTo({2, 2}, {10, 10}, 100);
    CHECK(cost == -1);
}

// ─── GameState::moveArmy + resolveMovementCombat ─────────────────────

static void setupTwoPlayerState(GameState &state, Map &map,
                                Player &p1, Player &p2,
                                Settlement &s1, Settlement &s2)
{
    map = Map::generateMap(10, 10);
    state.setMap(map);

    p1.setId(0);
    p2.setId(1);

    s1 = Settlement(0, SettlementType::Village);
    s1.addTile(Tile(1, 1));
    s1.addTile(Tile(1, 2));
    s1.seedInitialPopulation(50);
    p1.addSettlement(s1);

    s2 = Settlement(1, SettlementType::Village);
    s2.addTile(Tile(8, 8));
    s2.addTile(Tile(8, 9));
    s2.seedInitialPopulation(50);
    p2.addSettlement(s2);

    state.addPlayer(p1);
    state.addPlayer(p2);
}

TEST_CASE("GameState::moveArmy: перемещение на пустую клетку")
{
    GameState state;
    Map map;
    Player p1, p2;
    Settlement s1, s2;
    setupTwoPlayerState(state, map, p1, p2, s1, s2);

    Unit u = makeUnit("militia", 20, 3, 2);
    u.setTrained(true);
    state.players()[0].addArmy(Army("army0", 0, {1, 1}, {u}));

    auto result = state.moveArmy("army0", {2, 2});
    CHECK(result.success);
    CHECK(!result.combatOccurred);

    Army *army = state.findArmy("army0");
    CHECK(army != nullptr);
    CHECK(army->position().q == 2);
    CHECK(army->position().r == 2);
}

TEST_CASE("GameState::moveArmy: недостаточно очков движения")
{
    GameState state;
    Map map;
    Player p1, p2;
    Settlement s1, s2;
    setupTwoPlayerState(state, map, p1, p2, s1, s2);

    Unit u = makeUnit("militia", 20, 3, 2, 1);
    state.players()[0].addArmy(Army("army0", 0, {1, 1}, {u}));

    auto army = state.findArmy("army0");
    army->moveTo({2, 2}, 1); // used all movement

    auto result = state.moveArmy("army0", {3, 3});
    CHECK(!result.success);
    CHECK(!result.errorMessage.empty());
}

TEST_CASE("GameState::moveArmy: атака вражеской армии — бой")
{
    GameState state;
    Map map;
    Player p1, p2;
    Settlement s1, s2;
    setupTwoPlayerState(state, map, p1, p2, s1, s2);

    Unit atk = makeUnit("strong", 50, 20, 10);
    atk.setTrained(true);
    state.players()[0].addArmy(Army("attacker", 0, {5, 5}, {atk}));

    Unit def = makeUnit("weak", 10, 2, 1);
    state.players()[1].addArmy(Army("defender", 1, {5, 6}, {def}));

    auto result = state.moveArmy("attacker", {5, 6});
    CHECK(result.success);
    CHECK(result.combatOccurred);
    CHECK(result.combat.attackerWon);

    // Defending army should be gone
    CHECK(state.findArmy("defender") == nullptr);
    // Attacker should still exist (moved to destination)
    CHECK(state.findArmy("attacker") != nullptr);

    // Блок 3.14: бой должен оставить событие в отчёте о ходе.
    CHECK(state.turnEvents().size() == 1u);
    CHECK(state.turnEvents()[0].type == TurnEventType::Combat);
    CHECK(state.turnEvents()[0].actorId == 0);
    CHECK(state.turnEvents()[0].targetId == 1);

    // Блок 3.9: строка лога включает раунд, юнитов и урон.
    const std::string &desc = state.turnEvents()[0].description;
    CHECK(desc.find("Р1:") != std::string::npos);
    CHECK(desc.find("урон") != std::string::npos);
    CHECK(desc.find("strong") != std::string::npos);
    CHECK(desc.find("weak") != std::string::npos);
}

TEST_CASE("GameState::moveArmy: атака вражеского поселения — захват")
{
    GameState state;
    Map map;
    Player p1, p2;
    Settlement s1, s2;
    setupTwoPlayerState(state, map, p1, p2, s1, s2);

    // Give p2 a settlement at a known location
    Settlement target(1, SettlementType::Village);
    target.addTile(Tile(6, 6));
    target.seedInitialPopulation(10);
    state.players()[1].addSettlement(target);

    Unit atk = makeUnit("strong", 50, 20, 10);
    atk.setTrained(true);
    state.players()[0].addArmy(Army("attacker", 0, {5, 5}, {atk}));

    auto result = state.moveArmy("attacker", {6, 6});
    CHECK(result.success);
    CHECK(result.combatOccurred);
    CHECK(result.settlementCaptured);

    // Блок 3.14: захват города даёт ДВА события — сам бой и отдельно захват.
    bool hasCombat = false, hasCaptured = false;
    for (const auto &ev : state.turnEvents())
    {
        if (ev.type == TurnEventType::Combat)
            hasCombat = true;
        if (ev.type == TurnEventType::CityCaptured && ev.actorId == 0 && ev.targetId == 1)
            hasCaptured = true;
    }
    CHECK(hasCombat);
    CHECK(hasCaptured);
}

TEST_CASE("GameState::moveArmy: несуществующая армия")
{
    GameState state;
    auto result = state.moveArmy("ghost", {0, 0});
    CHECK(!result.success);
    CHECK(!result.errorMessage.empty());
}

// ─── Симультанная модель раунда (вариант 2) ───────────────────────────

TEST_CASE("Combat::resolve: численный перевес побеждает одного сильного защитника")
{
    // 5 слабых атакующих (суммарный урон за раунд = 5x) должны задавить
    // одного защитника, который в старой "дуэльной" модели полностью
    // игнорировал бы четверых из них, пока не убьёт первого.
    std::vector<Unit> attackers;
    for (int i = 0; i < 5; ++i)
        attackers.push_back(makeUnit("weak" + std::to_string(i), 15, 4, 1));

    auto strongDefender = makeUnit("tank", 40, 6, 3);

    auto result = Combat::resolve(attackers, {strongDefender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(result.attackerWon);
    CHECK(result.defenderSurvivors.empty());
    // Хотя бы часть слабых атакующих должна была выжить — бой решает
    // суммарный урон за раунд, а не дуэль "один на один"
    CHECK(!result.attackerSurvivors.empty());
}

TEST_CASE("Combat::resolve: обрыв по лимиту раундов — это ничья, а не победа атакующего")
{
    // Урон с обеих сторон всегда упирается в минимум (защита выше атаки),
    // при HP=200 и уроне 1/раунд бой не успевает разрешиться за 100 раундов.
    auto attacker = makeUnit("bulwark_a", 200, 1, 200);
    auto defender = makeUnit("bulwark_d", 200, 1, 200);

    auto result = Combat::resolve({attacker}, {defender}, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(result.isDraw);
    CHECK(!result.attackerWon);
    CHECK(!result.attackerSurvivors.empty());
    CHECK(!result.defenderSurvivors.empty());
}

TEST_CASE("GameState::moveArmy: захват поселения списывает очки движения")
{
    GameState state;
    Map map;
    Player p1, p2;
    Settlement s1, s2;
    setupTwoPlayerState(state, map, p1, p2, s1, s2);

    Settlement target(1, SettlementType::Village);
    target.addTile(Tile(6, 6));
    target.seedInitialPopulation(10);
    state.players()[1].addSettlement(target);

    Unit atk = makeUnit("strong", 50, 20, 10, /*mp=*/3);
    atk.setTrained(true);
    state.players()[0].addArmy(Army("attacker", 0, {5, 5}, {atk}));

    auto result = state.moveArmy("attacker", {6, 6});
    CHECK(result.success);
    CHECK(result.settlementCaptured);

    auto *army = state.findArmy("attacker");
    CHECK(army != nullptr);
    // Раньше захват поселения был "бесплатным" по очкам движения из-за
    // внутреннего moveTo(dest, 0) внутри resolveMovementCombat.
    if (army != nullptr)
        CHECK(army->movementPointsRemaining() < 3);
}