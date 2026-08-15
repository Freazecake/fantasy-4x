// Юнит-тесты на логику, добавленную/переписанную в рамках "умного ИИ":
// CombatSimulator (честная симуляция боя), BotMemory (доверие/кулдаун
// союза), AIController::detectRivalRush (реакция на перекос соперника по
// сферам), AIController::worthAttacking (ранняя осторожность / поздняя
// эскалация / оборонительное исключение), observeEvents (обновление
// доверия по событиям хода), maybeReassessGoal (цель бота).
//
// Раньше вся эта логика проверялась только вручную через playtest-инструменты
// (tools/playtest*.cpp, tools/human_rush_test.cpp) — полезно для баланса, но
// не защищает от регрессий в самой логике при дальнейших правках. Этот файл
// закрывает именно эту дыру.

#include "test_framework.hpp"

#include "core/Tile.hpp"
#include "core/Map.hpp"
#include "core/Army.hpp"
#include "core/Player.hpp"
#include "core/GameState.hpp"
#include "core/Combat.hpp"
#include "core/Hero.hpp"
#include "core/PoliticalAction.hpp"

#include "ai/AIController.hpp"
#include "ai/AIStrategy.hpp"
#include "ai/CombatSimulator.hpp"
#include "ai/BotMemory.hpp"

#include <vector>

namespace
{

Race makeRace(const std::string &id, float science, float industry, float trade,
              float diplomacy, float military, float growth)
{
    return Race::loadFromJson({
        {"id", id}, {"name", id}, {"startingSettlementType", "village"},
        {"modifiers", {{"science", science}, {"industry", industry}, {"trade", trade},
                       {"diplomacy", diplomacy}, {"military", military}, {"growth", growth}}},
    });
}

Unit makeUnit(const std::string &id, float attack, float defense, float health,
              int mp = 2)
{
    return Unit::loadFromJson({
        {"id", id}, {"name", id},
        {"stats", {{"health", health}, {"attack", attack}, {"defense", defense}, {"movementPoints", mp}}},
        {"tier", 0}, {"requiredBuildingId", ""},
        {"recruitCost", 20}, {"populationCost", 15},
    });
}

std::vector<Unit> makeUnits(const std::string &id, int count, float attack, float defense, float health)
{
    std::vector<Unit> result;
    for (int i = 0; i < count; ++i)
        result.push_back(makeUnit(id, attack, defense, health));
    return result;
}

PoliticalAction makeAction(const std::string &id, const std::string &type, int cost)
{
    return PoliticalAction::loadFromJson(
        {{"id", id}, {"name", id}, {"type", type}, {"diplomacyCost", cost}});
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════
// CombatSimulator: честный "сухой прогон" боя
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("CombatSimulator::predict: явный перевес атакующего — победа с малыми потерями")
{
    auto attackers = makeUnits("strong", 10, 20.0f, 20.0f, 20.0f);
    auto defenders = makeUnits("weak", 2, 2.0f, 2.0f, 5.0f);

    CombatPrediction pred = CombatSimulator::predict(attackers, defenders, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(pred.attackerWins);
    CHECK(pred.attackerLossFraction < 0.3f);
    CHECK(pred.attackerUnitsBefore == 10);
    CHECK(pred.defenderUnitsBefore == 2);
}

TEST_CASE("CombatSimulator::predict: явный перевес защитника — атакующий проигрывает")
{
    auto attackers = makeUnits("weak", 2, 2.0f, 2.0f, 5.0f);
    auto defenders = makeUnits("strong", 10, 20.0f, 20.0f, 20.0f);

    CombatPrediction pred = CombatSimulator::predict(attackers, defenders, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(!pred.attackerWins);
}

TEST_CASE("CombatSimulator::predict: пустой гарнизон защитника — атакующий проходит без боя")
{
    auto attackers = makeUnits("any", 1, 5.0f, 5.0f, 10.0f);
    std::vector<Unit> emptyDefenders;

    CombatPrediction pred = CombatSimulator::predict(attackers, emptyDefenders, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(pred.attackerWins);
    CHECK(pred.attackerLossFraction == 0.0f);
    CHECK(pred.attackerSurvivors == 1);
}

TEST_CASE("CombatSimulator::predict: пустая армия атакующего — не побеждает")
{
    std::vector<Unit> emptyAttackers;
    auto defenders = makeUnits("any", 1, 5.0f, 5.0f, 10.0f);

    CombatPrediction pred = CombatSimulator::predict(emptyAttackers, defenders, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(!pred.attackerWins);
}

TEST_CASE("CombatSimulator::predict: бонус местности защитника не облегчает бой атакующему")
{
    auto attackers = makeUnits("a", 5, 6.0f, 6.0f, 10.0f);
    auto defenders = makeUnits("d", 5, 6.0f, 6.0f, 10.0f);

    CombatPrediction flat = CombatSimulator::predict(attackers, defenders, {}, {}, 1.0f, {}, 1.0f, 1.0f);
    CombatPrediction fortified = CombatSimulator::predict(attackers, defenders, {}, {}, 1.5f, {}, 1.0f, 1.0f);

    CHECK(fortified.attackerLossFraction >= flat.attackerLossFraction - 0.001f);
}

TEST_CASE("CombatSimulator::predict: не мутирует исходные вектора юнитов")
{
    auto attackers = makeUnits("a", 5, 20.0f, 20.0f, 20.0f);
    auto defenders = makeUnits("d", 5, 2.0f, 2.0f, 5.0f);
    size_t attackersBefore = attackers.size();
    size_t defendersBefore = defenders.size();

    CombatSimulator::predict(attackers, defenders, {}, {}, 1.0f, {}, 1.0f, 1.0f);

    CHECK(attackers.size() == attackersBefore);
    CHECK(defenders.size() == defendersBefore);
}

// ═══════════════════════════════════════════════════════════════════════
// AIController::worthAttacking: ранняя осторожность / поздняя эскалация /
// оборонительное исключение
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::worthAttacking: явная победа с большим запасом принимается на любом ходу")
{
    Race attackerRace = makeRace("a", 1, 1, 1, 1, 1, 1);
    Race defenderRace = makeRace("d", 1, 1, 1, 1, 1, 1);
    Player attacker; attacker.setId(0); attacker.setRace(attackerRace);
    Player defender; defender.setId(1); defender.setRace(defenderRace);

    Army mine("a1", 0, Coords{0, 0}, makeUnits("strong", 10, 30.0f, 30.0f, 30.0f));
    auto defenderUnits = makeUnits("weak", 1, 1.0f, 1.0f, 3.0f);

    bool worth = AIController::worthAttacking(attacker, mine, defender, defenderUnits,
                                              1.0f, {}, false, 1, false);
    CHECK(worth);
}

TEST_CASE("AIController::worthAttacking: рискованная но выигрышная атака: поздний этап не строже раннего")
{
    Race attackerRace = makeRace("a", 1, 1, 1, 1, 1, 1);
    Race defenderRace = makeRace("d", 1, 1, 1, 1, 1, 1);
    Player attacker; attacker.setId(0); attacker.setRace(attackerRace);
    Player defender; defender.setId(1); defender.setRace(defenderRace);

    Army mine("a1", 0, Coords{0, 0}, makeUnits("a", 6, 6.0f, 4.0f, 10.0f));
    auto defenderUnits = makeUnits("d", 5, 5.0f, 4.0f, 10.0f);

    bool earlyNotDefensive = AIController::worthAttacking(attacker, mine, defender, defenderUnits,
                                                           1.0f, {}, false, 1, false);
    bool lateEscalated = AIController::worthAttacking(attacker, mine, defender, defenderUnits,
                                                       1.0f, {}, false, 90, false);

    CHECK(!earlyNotDefensive || lateEscalated);
    CHECK(lateEscalated);
}

TEST_CASE("AIController::worthAttacking: оборонительный флаг не строже инициативной атаки")
{
    Race attackerRace = makeRace("a", 1, 1, 1, 1, 1, 1);
    Race defenderRace = makeRace("d", 1, 1, 1, 1, 1, 1);
    Player attacker; attacker.setId(0); attacker.setRace(attackerRace);
    Player defender; defender.setId(1); defender.setRace(defenderRace);

    Army mine("a1", 0, Coords{0, 0}, makeUnits("a", 6, 6.0f, 4.0f, 10.0f));
    auto defenderUnits = makeUnits("d", 5, 5.0f, 4.0f, 10.0f);

    bool earlyOffensive = AIController::worthAttacking(attacker, mine, defender, defenderUnits,
                                                        1.0f, {}, false, 1, false);
    bool earlyDefensive = AIController::worthAttacking(attacker, mine, defender, defenderUnits,
                                                        1.0f, {}, false, 1, true);

    CHECK(earlyDefensive || !earlyOffensive);
}

TEST_CASE("AIController::worthAttacking: явное поражение отклоняется даже поздно и даже в обороне")
{
    Race attackerRace = makeRace("a", 1, 1, 1, 1, 1, 1);
    Race defenderRace = makeRace("d", 1, 1, 1, 1, 1, 1);
    Player attacker; attacker.setId(0); attacker.setRace(attackerRace);
    Player defender; defender.setId(1); defender.setRace(defenderRace);

    Army mine("a1", 0, Coords{0, 0}, makeUnits("weak", 1, 1.0f, 1.0f, 3.0f));
    auto defenderUnits = makeUnits("strong", 10, 30.0f, 30.0f, 30.0f);

    bool worth = AIController::worthAttacking(attacker, mine, defender, defenderUnits,
                                              1.0f, {}, false, 90, true);
    CHECK(!worth);
}

// ═══════════════════════════════════════════════════════════════════════
// AIController::detectRivalRush: реакция на перекос соперника по сферам
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::detectRivalRush: сбалансированная экономика соперника не даёт сигнала")
{
    GameState state;
    Map map = Map::generateMap(10, 10);
    state.setMap(map);

    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    Settlement s0(0, SettlementType::Village);
    s0.addTile(Tile(1, 1));
    s0.seedInitialPopulation(20);
    me.addSettlement(s0);

    Player rival; rival.setId(1); rival.setRace(makeRace("rival", 1, 1, 1, 1, 1, 1));
    Settlement s1(1, SettlementType::Village);
    s1.addTile(Tile(8, 8));
    s1.seedInitialPopulation(100);
    s1.assignFromIdle(Sphere::Science, 25);
    s1.assignFromIdle(Sphere::Industry, 25);
    s1.assignFromIdle(Sphere::Trade, 25);
    s1.assignFromIdle(Sphere::Diplomacy, 25);
    rival.addSettlement(s1);

    state.addPlayer(me);
    state.addPlayer(rival);

    RushSignal signal = AIController::detectRivalRush(state.players()[0], state);

    CHECK(signal.industry == 0.0f);
    CHECK(signal.science == 0.0f);
    CHECK(signal.diplomacy == 0.0f);
    CHECK(signal.trade == 0.0f);
}

TEST_CASE("AIController::detectRivalRush: соперник, вложивший почти всё в промышленность, "
          "даёт сигнал именно по industry")
{
    GameState state;
    Map map = Map::generateMap(10, 10);
    state.setMap(map);

    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    Settlement s0(0, SettlementType::Village);
    s0.addTile(Tile(1, 1));
    s0.seedInitialPopulation(20);
    me.addSettlement(s0);

    Player rival; rival.setId(1); rival.setRace(makeRace("rival", 1, 1, 1, 1, 1, 1));
    Settlement s1(1, SettlementType::Village);
    s1.addTile(Tile(8, 8));
    s1.seedInitialPopulation(100);
    s1.assignFromIdle(Sphere::Industry, 90);
    s1.assignFromIdle(Sphere::Science, 10);
    rival.addSettlement(s1);

    state.addPlayer(me);
    state.addPlayer(rival);

    RushSignal signal = AIController::detectRivalRush(state.players()[0], state);

    CHECK(signal.industry > 0.0f);
    CHECK(signal.diplomacy == 0.0f);
    CHECK(signal.trade == 0.0f);
}

TEST_CASE("AIController::detectRivalRush: союзника не считаем угрозой, даже если он перекошен")
{
    GameState state;
    Map map = Map::generateMap(10, 10);
    state.setMap(map);

    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    Settlement s0(0, SettlementType::Village);
    s0.addTile(Tile(1, 1));
    s0.seedInitialPopulation(20);
    me.addSettlement(s0);

    Player ally; ally.setId(1); ally.setRace(makeRace("ally", 1, 1, 1, 1, 1, 1));
    Settlement s1(1, SettlementType::Village);
    s1.addTile(Tile(8, 8));
    s1.seedInitialPopulation(100);
    s1.assignFromIdle(Sphere::Industry, 90);
    ally.addSettlement(s1);

    state.addPlayer(me);
    state.addPlayer(ally);
    state.newAlliance(state.players()[0], state.players()[1], state.turnNumber() + 10);

    RushSignal signal = AIController::detectRivalRush(state.players()[0], state);

    CHECK(signal.industry == 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════
// observeEvents / доверие / кулдаун союза
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::observeEvents: атака соперника снижает доверие к нему")
{
    GameState state;
    Map map = Map::generateMap(10, 10);
    state.setMap(map);
    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    state.addPlayer(me);

    state.pushEvent(TurnEventType::Combat, /*actor*/ 1, /*target*/ 0, "игрок 1 атаковал игрока 0");

    BotMemory memory;
    AIController::observeEvents(state.players()[0], state, memory);

    CHECK(memory.relationship(1).trust < 0.0f);
}

TEST_CASE("AIController::observeEvents: атака на кого-то ДРУГОГО не портит доверие лично к нам")
{
    GameState state;
    Map map = Map::generateMap(10, 10);
    state.setMap(map);
    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    state.addPlayer(me);

    state.pushEvent(TurnEventType::Combat, /*actor*/ 1, /*target*/ 2, "игрок 1 атаковал игрока 2");

    BotMemory memory;
    AIController::observeEvents(state.players()[0], state, memory);

    CHECK(memory.relationship(1).trust == 0.0f);
}

TEST_CASE("AIController::observeEvents: союз повышает доверие")
{
    GameState state;
    Map map = Map::generateMap(10, 10);
    state.setMap(map);
    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    state.addPlayer(me);

    state.pushEvent(TurnEventType::AllianceFormed, 0, 1, "союз между 0 и 1");

    BotMemory memory;
    AIController::observeEvents(state.players()[0], state, memory);

    CHECK(memory.relationship(1).trust > 0.0f);
}

TEST_CASE("AIController::observeEvents: разрыв союза с нами ставит кулдаун на повторный союз")
{
    GameState state;
    Map map = Map::generateMap(10, 10);
    state.setMap(map);
    state.setTurnNumber(1);
    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    state.addPlayer(me);

    state.pushEvent(TurnEventType::AllianceBroken, 0, 1, "союз между 0 и 1 расторгнут");

    BotMemory memory;
    AIController::observeEvents(state.players()[0], state, memory);

    CHECK(memory.relationship(1).allianceCooldownUntilTurn > state.turnNumber());
}

// ═══════════════════════════════════════════════════════════════════════
// decidePoliticalAction: не заискиваем перед тем, кто на кулдауне
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::decidePoliticalAction: не предлагает союз сопернику на кулдауне союза")
{
    GameState state;
    Map map = Map::generateMap(10, 10);
    state.setMap(map);

    Player human; human.setId(0); human.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));
    Settlement s0(0, SettlementType::Village);
    s0.addTile(Tile(1, 1));
    s0.seedInitialPopulation(20);
    human.addSettlement(s0);

    Player rival; rival.setId(1); rival.setRace(makeRace("rival", 1, 1, 1, 1, 1, 1));
    Settlement s1(1, SettlementType::Village);
    s1.addTile(Tile(8, 8));
    s1.seedInitialPopulation(20);
    rival.addSettlement(s1);

    state.addPlayer(human);
    state.addPlayer(rival);

    Resources budget;
    budget.diplomacy = 500.0f;
    state.players()[0].addResources(budget);

    auto alliance = makeAction("pa_alliance", "alliance", 10);
    std::vector<PoliticalAction> catalog = {alliance};

    BotMemory memory;
    memory.relationship(1).trust = 50.0f;
    memory.relationship(1).allianceCooldownUntilTurn = state.turnNumber() + 100;

    float before = state.players()[0].resources().diplomacy;
    AIController::decidePoliticalAction(state.players()[0], catalog, state, memory);
    float after = state.players()[0].resources().diplomacy;

    CHECK(before == after);
}

// ═══════════════════════════════════════════════════════════════════════
// maybeReassessGoal: цель бота
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::maybeReassessGoal: угроза у границ форсирует пересмотр цели немедленно")
{
    GameState state;
    Map map = Map::generateMap(20, 20);
    state.setMap(map);

    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    Settlement s0(0, SettlementType::Village);
    s0.addTile(Tile(5, 5));
    s0.seedInitialPopulation(50);
    me.addSettlement(s0);

    Player enemy; enemy.setId(1); enemy.setRace(makeRace("enemy", 1, 1, 1, 1, 1, 1));
    Settlement s1(1, SettlementType::Village);
    s1.addTile(Tile(12, 12));
    s1.seedInitialPopulation(50);
    enemy.addSettlement(s1);

    state.addPlayer(me);
    state.addPlayer(enemy);

    Army threat("t1", 1, Coords{6, 5}, {makeUnit("militia", 5, 5, 10)});
    state.players()[1].addArmy(threat);

    BotMemory memory;
    memory.goal = BotGoal::Grow;
    memory.goalSetOnTurn = state.turnNumber();

    AIController::maybeReassessGoal(state.players()[0], state, memory);

    CHECK(memory.goal != BotGoal::Grow);
}

TEST_CASE("AIController::maybeReassessGoal: без угроз рядом мирная цель не меняется раньше срока")
{
    GameState state;
    Map map = Map::generateMap(20, 20);
    state.setMap(map);

    Player me; me.setId(0); me.setRace(makeRace("me", 1, 1, 1, 1, 1, 1));
    Settlement s0(0, SettlementType::Village);
    s0.addTile(Tile(5, 5));
    s0.seedInitialPopulation(50);
    me.addSettlement(s0);

    Player enemy; enemy.setId(1); enemy.setRace(makeRace("enemy", 1, 1, 1, 1, 1, 1));
    Settlement s1(1, SettlementType::Village);
    s1.addTile(Tile(19, 19));
    s1.seedInitialPopulation(50);
    enemy.addSettlement(s1);

    state.addPlayer(me);
    state.addPlayer(enemy);

    BotMemory memory;
    memory.goal = BotGoal::Grow;
    memory.goalSetOnTurn = state.turnNumber();

    AIController::maybeReassessGoal(state.players()[0], state, memory);

    CHECK(memory.goal == BotGoal::Grow);
}

// ═══════════════════════════════════════════════════════════════════════
// decideAllocations + RushSignal: ответ на перекос соперника
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::decideAllocations: ненулевой RushSignal по industry увеличивает долю "
          "промышленности относительно пустого сигнала")
{
    Settlement withoutSignal(0, SettlementType::Village);
    withoutSignal.seedInitialPopulation(100);

    Settlement withSignal(0, SettlementType::Village);
    withSignal.seedInitialPopulation(100);

    RushSignal signal;
    signal.industry = 0.3f;

    AIController::decideAllocations(withoutSignal, GlobalPhase::Phase2_OpenMap, BotGoal::Grow, RushSignal{});
    AIController::decideAllocations(withSignal, GlobalPhase::Phase2_OpenMap, BotGoal::Grow, signal);

    CHECK(withSignal.allocation().industry >= withoutSignal.allocation().industry);
}
