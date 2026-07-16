#include "test_framework.hpp"

#include "core/Settlement.hpp"

namespace {

Building makeBuilding(const std::string &id, int cost, std::vector<std::string> unlocksUnitIds = {},
                       bool removesPenalty = false) {
    json obj = {
        {"id", id},
        {"name", id},
        {"category", "economic"},
        {"resourceType", "science"},
        {"bonusMultiplier", 1.15},
        {"industryCost", cost},
        {"unlocksUnitIds", unlocksUnitIds},
        {"removesUntrainedPenalty", removesPenalty},
    };
    return Building::loadFromJson(obj);
}

Unit makeUnit(const std::string &id, int tier, const std::string &requiredBuildingId,
              int recruitCost, int populationCost) {
    json obj = {
        {"id", id},
        {"name", id},
        {"stats", {{"health", 20.0}, {"attack", 3.0}, {"defense", 2.0}}},
        {"tier", tier},
        {"requiredBuildingId", requiredBuildingId},
        {"recruitCost", recruitCost},
        {"populationCost", populationCost},
    };
    return Unit::loadFromJson(obj);
}

}  // namespace

// --- Очередь построек (день 3) --------------------------------------------

TEST_CASE("Settlement::queueBuilding + progressBuildingQueue: недостаточно очков — здание не готово") {
    Settlement s(1, SettlementType::Village);
    s.queueBuilding(makeBuilding("library", 40));

    std::vector<Building> completed = s.progressBuildingQueue(25);

    CHECK(completed.empty());
    CHECK(s.builtBuildings().empty());
    CHECK(s.buildingQueue().size() == 1u);
    CHECK(s.buildingQueue()[0].remainingCost() == 15);
}

TEST_CASE("Settlement::progressBuildingQueue: ровно достаточно очков — здание завершается") {
    Settlement s(1, SettlementType::Village);
    s.queueBuilding(makeBuilding("library", 40));

    std::vector<Building> completed = s.progressBuildingQueue(40);

    CHECK(completed.size() == 1u);
    CHECK(completed[0].getId() == "library");
    CHECK(s.hasBuilt("library"));
    CHECK(s.buildingQueue().empty());
}

TEST_CASE("Settlement::progressBuildingQueue: переполнение переходит следующему зданию в очереди") {
    Settlement s(1, SettlementType::Village);
    s.queueBuilding(makeBuilding("library", 40));
    s.queueBuilding(makeBuilding("observatory", 90));

    std::vector<Building> completed = s.progressBuildingQueue(50);  // 40 на library, 10 на observatory

    CHECK(completed.size() == 1u);
    CHECK(completed[0].getId() == "library");
    CHECK(s.hasBuilt("library"));
    CHECK(!s.hasBuilt("observatory"));
    CHECK(s.buildingQueue().size() == 1u);
    CHECK(s.buildingQueue()[0].remainingCost() == 80);
}

TEST_CASE("Settlement::progressBuildingQueue: одного вызова хватает на несколько дешёвых зданий") {
    Settlement s(1, SettlementType::Village);
    s.queueBuilding(makeBuilding("a", 10));
    s.queueBuilding(makeBuilding("b", 10));
    s.queueBuilding(makeBuilding("c", 10));

    std::vector<Building> completed = s.progressBuildingQueue(25);

    CHECK(completed.size() == 2u);  // a и b завершены, на c осталось 5 из 10
    CHECK(s.hasBuilt("a"));
    CHECK(s.hasBuilt("b"));
    CHECK(!s.hasBuilt("c"));
}

TEST_CASE("Settlement::progressBuildingQueue: пустая очередь ничего не делает") {
    Settlement s(1, SettlementType::Village);

    std::vector<Building> completed = s.progressBuildingQueue(100);

    CHECK(completed.empty());
}

// --- Очередь найма юнитов и гарнизон (день 5-6) -----------------------------

TEST_CASE("Settlement::queueUnitRecruitment: tier 0 юнит встаёт в очередь и сразу списывает population") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);

    Unit militia = makeUnit("militia", 0, "", 15, 30);
    s.queueUnitRecruitment(militia);

    CHECK(s.recruitmentQueue().size() == 1u);
    CHECK(s.allocation().idle == 70);       // 100 - 30 (populationCost)
    CHECK(s.populationCount() == 70);
}

TEST_CASE("Settlement::queueUnitRecruitment: tier 1+ юнит без нужного здания бросает исключение") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);

    Unit swordsman = makeUnit("swordsman", 1, "barracks", 30, 50);

    CHECK_THROWS(s.queueUnitRecruitment(swordsman));
    // Население не должно было списаться при неудачной попытке.
    CHECK(s.allocation().idle == 100);
}

TEST_CASE("Settlement::queueUnitRecruitment: недостаточно population бросает исключение") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(20);

    Unit militia = makeUnit("militia", 0, "", 15, 30);  // populationCost=30 > 20 доступных

    CHECK_THROWS(s.queueUnitRecruitment(militia));
    CHECK(s.allocation().idle == 20);  // не списалось
}

TEST_CASE("Settlement::queueUnitRecruitment: tier 1+ юнит с построенной казармой встаёт в очередь") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.queueBuilding(makeBuilding("barracks", 60, {"swordsman"}, true));
    s.progressBuildingQueue(60);  // достроили казарму

    Unit swordsman = makeUnit("swordsman", 1, "barracks", 30, 50);
    s.queueUnitRecruitment(swordsman);

    CHECK(s.recruitmentQueue().size() == 1u);
}

TEST_CASE("Settlement::progressRecruitmentQueue: недостаточно очков — юнит не готов") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.queueUnitRecruitment(makeUnit("militia", 0, "", 15, 30));

    std::vector<Unit> completed = s.progressRecruitmentQueue(10);

    CHECK(completed.empty());
    CHECK(s.garrison().empty());
    CHECK(s.recruitmentQueue()[0].remainingCost() == 5);
}

TEST_CASE("Settlement::progressRecruitmentQueue: юнит завершается и попадает в гарнизон") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.queueUnitRecruitment(makeUnit("militia", 0, "", 15, 30));

    std::vector<Unit> completed = s.progressRecruitmentQueue(15);

    CHECK(completed.size() == 1u);
    CHECK(completed[0].getId() == "militia");
    CHECK(s.garrison().size() == 1u);
    CHECK(s.recruitmentQueue().empty());
}

TEST_CASE("Settlement::progressRecruitmentQueue: переполнение переходит следующему юниту в очереди") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(200);
    s.queueUnitRecruitment(makeUnit("militia_1", 0, "", 15, 30));
    s.queueUnitRecruitment(makeUnit("militia_2", 0, "", 15, 30));

    std::vector<Unit> completed = s.progressRecruitmentQueue(20);  // 15 на первого, 5 на второго

    CHECK(completed.size() == 1u);
    CHECK(completed[0].getId() == "militia_1");
    CHECK(s.garrison().size() == 1u);
    CHECK(s.recruitmentQueue().size() == 1u);
    CHECK(s.recruitmentQueue()[0].remainingCost() == 10);
}
