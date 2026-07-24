#include "test_framework.hpp"

#include "core/Military.hpp"

namespace {

Unit makeUnit(const std::string &id, int tier, const std::string &requiredBuildingId,
              float attack, float defense, float health) {
    json obj = {
        {"id", id},
        {"name", id},
        {"stats", {{"health", health}, {"attack", attack}, {"defense", defense}}},
        {"tier", tier},
        {"requiredBuildingId", requiredBuildingId},
        {"recruitCost", 20},
        {"populationCost", 30},
    };
    return Unit::loadFromJson(obj);
}

Building makeBuilding(const std::string &id, std::vector<std::string> unlocksUnitIds,
                       bool removesPenalty) {
    json obj = {
        {"id", id},
        {"name", id},
        {"category", "military"},
        {"resourceType", ""},
        {"bonusMultiplier", 0.0},
        {"industryCost", 60},
        {"unlocksUnitIds", unlocksUnitIds},
        {"removesUntrainedPenalty", removesPenalty},
    };
    return Building::loadFromJson(obj);
}

}  // namespace

TEST_CASE("Military::canRecruit: юнит tier 0 доступен без построек") {
    Unit militia = makeUnit("militia", 0, "", 3, 2, 20);

    CHECK(Military::canRecruit(militia, {}));
}

TEST_CASE("Military::canRecruit: юнит tier 1+ недоступен без нужного здания") {
    Unit swordsman = makeUnit("swordsman", 1, "barracks", 7, 5, 35);

    CHECK(!Military::canRecruit(swordsman, {}));
}

TEST_CASE("Military::canRecruit: юнит tier 1+ доступен, если здание построено и открывает его") {
    Unit swordsman = makeUnit("swordsman", 1, "barracks", 7, 5, 35);
    Building barracks = makeBuilding("barracks", {"swordsman", "archer"}, true);

    CHECK(Military::canRecruit(swordsman, {barracks}));
}

TEST_CASE("Military::canRecruit: здание, не открывающее этот юнит, не считается") {
    Unit swordsman = makeUnit("swordsman", 1, "barracks", 7, 5, 35);
    Building irrelevant = makeBuilding("stables", {"cavalry"}, true);

    CHECK(!Military::canRecruit(swordsman, {irrelevant}));
}

TEST_CASE("Military::queueUnit: успешно создаёт RecruitmentItem для доступного юнита") {
    Unit militia = makeUnit("militia", 0, "", 3, 2, 20);

    RecruitmentItem item = Military::queueUnit(militia, {});

    CHECK(item.unit.getId() == "militia");
    CHECK(item.investedPoints == 0);
    CHECK(item.remainingCost() == militia.getRecruitCost());
}

TEST_CASE("Military::queueUnit: бросает исключение для недоступного юнита") {
    Unit swordsman = makeUnit("swordsman", 1, "barracks", 7, 5, 35);

    CHECK_THROWS(Military::queueUnit(swordsman, {}));
}

TEST_CASE("Military::effectiveStats: без построек с removesUntrainedPenalty применяется штраф 0.7") {
    Unit militia = makeUnit("militia", 0, "", 10.0f, 10.0f, 10.0f);

    UnitStats stats = Military::effectiveStats(militia, {}, 1.0f, {});

    CHECK_NEAR(stats.attack, 10.0f * Military::UNTRAINED_PENALTY_MULTIPLIER, 0.01f);
    CHECK_NEAR(stats.defense, 10.0f * Military::UNTRAINED_PENALTY_MULTIPLIER, 0.01f);
    CHECK_NEAR(stats.health, 10.0f * Military::UNTRAINED_PENALTY_MULTIPLIER, 0.01f);
}

TEST_CASE("Military::effectiveStats: с казармой (removesUntrainedPenalty) штрафа нет") {
    Unit militia = makeUnit("militia", 0, "", 10.0f, 10.0f, 10.0f);
    militia.setTrained(true);

    UnitStats stats = Military::effectiveStats(militia, {}, 1.0f, {});

    CHECK_NEAR(stats.attack, 10.0f, 0.01f);
    CHECK_NEAR(stats.defense, 10.0f, 0.01f);
    CHECK_NEAR(stats.health, 10.0f, 0.01f);
}

TEST_CASE("Military::effectiveStats: расовый модификатор military усиливает статы независимо от штрафа") {
    Unit militia = makeUnit("militia", 0, "", 10.0f, 10.0f, 10.0f);
    militia.setTrained(true);

    UnitStats stats = Military::effectiveStats(militia, {}, 1.5f, {});

    CHECK_NEAR(stats.attack, 15.0f, 0.01f);
}

TEST_CASE("Military::effectiveStats: бонус героя тоже применяется к статам") {
    Unit militia = makeUnit("militia", 0, "", 10.0f, 10.0f, 10.0f);
    militia.setTrained(true);

    json heroObj = {
        {"id", "hero_kagran"},
        {"name", "Кагран"},
        {"raceId", "orcs"},
        {"modifiers",
         {{"science", 1.0}, {"industry", 1.0}, {"trade", 1.0}, {"diplomacy", 1.0},
          {"military", 1.3}, {"growth", 1.0}}},
    };
    Hero kagran = Hero::loadFromJson(heroObj);

    UnitStats stats = Military::effectiveStats(militia, {}, 1.0f, {kagran});

    CHECK_NEAR(stats.attack, 13.0f, 0.01f);
}
