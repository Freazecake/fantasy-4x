#include "test_framework.hpp"

#include "core/GameState.hpp"
#include "core/Player.hpp"
#include "core/Settlement.hpp"
#include "core/Army.hpp"
#include "core/Unit.hpp"
#include "core/Race.hpp"

static Unit mkSettler() {
    return Unit::loadFromJson({
        {"id", "settler"}, {"name", "Поселенец"},
        {"stats", {{"health", 10}, {"attack", 0}, {"defense", 1}, {"movementPoints", 2}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 60}, {"populationCost", 80},
        {"isSettler", true}
    });
}

static Race mkRace(const std::string &id) {
    return Race::loadFromJson({
        {"id", id}, {"name", id}, {"startingSettlementType", "village"},
        {"modifiers", {{"science", 1}, {"industry", 1}, {"trade", 1},
                       {"diplomacy", 1}, {"military", 1}, {"growth", 1}}}
    });
}

TEST_CASE("repro: two settler armies march then found") {
    GameState state;
    state.setMap(Map::generateMap(20, 20));
    Player p;
    p.setId(0);
    p.setIsHuman(true);
    p.setRace(mkRace("humans"));
    Settlement s(0, SettlementType::Village);
    s.addTile(Tile(10, 10));
    s.addToGarrison({mkSettler()});
    p.addSettlement(s);
    state.addPlayer(p);

    auto& hs = state.players()[0];
    std::vector<std::string> ids;
    for (auto& u : hs.settlements()[0].garrison())
        ids.push_back(u.getId());
    hs.detachArmy(hs.settlements()[0], ids);
    CHECK(hs.armies().size() == 1);

    auto& hs2 = state.players()[0];
    hs2.settlements()[0].addToGarrison({mkSettler()});
    std::vector<std::string> ids2;
    for (auto& u : hs2.settlements()[0].garrison())
        ids2.push_back(u.getId());
    hs2.detachArmy(hs2.settlements()[0], ids2);
    CHECK(hs2.armies().size() == 2);

    state.recomputeVisibility();
}