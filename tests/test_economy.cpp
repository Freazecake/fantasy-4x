#include "test_framework.hpp"

#include "core/Economy.hpp"

namespace {

Building makeBuilding(const std::string &id, const std::string &resourceType, float bonus)
{
    json obj = {
        {"id", id},
        {"name", id},
        {"category", "economic"},
        {"resourceType", resourceType},
        {"bonusMultiplier", bonus},
        {"industryCost", 10},
        {"unlocksUnitIds", json::array()},
        {"removesUntrainedPenalty", false},
    };
    return Building::loadFromJson(obj);
}

Hero makeHero(const std::string &id, const std::string &raceId, Sphere specialty, float bonus)
{
    json obj = {
        {"id", id},
        {"name", id},
        {"raceId", raceId},
        {"modifiers",
         {{"science", 1.0}, {"industry", 1.0}, {"trade", 1.0}, {"diplomacy", 1.0},
          {"military", 1.0}, {"growth", 1.0}}},
    };
    obj["modifiers"][sphereToString(specialty)] = bonus;
    return Hero::loadFromJson(obj);
}

}  // namespace

TEST_CASE("Economy::computeSettlementIncome: без построек/героев доход = население × модификатор расы") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Science, 30);
    s.assignFromIdle(Sphere::Industry, 20);

    Modifiers raceMods;
    raceMods.science = 1.7f;
    raceMods.industry = 1.2f;

    Resources income = Economy::computeSettlementIncome(s, raceMods, {}, {});

    CHECK_NEAR(income.science, 30 * 1.7f, 0.01f);
    CHECK_NEAR(income.industry, 20 * 1.2f, 0.01f);
    CHECK_NEAR(income.trade, 0.0f, 0.01f);
    CHECK_NEAR(income.diplomacy, 0.0f, 0.01f);
}

TEST_CASE("Economy::computeSettlementIncome: бонус постройки умножается поверх расового модификатора") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(50);
    s.assignFromIdle(Sphere::Science, 10);

    Modifiers raceMods;  // все 1.0
    std::vector<Building> buildings = {makeBuilding("library", "science", 1.15f)};

    Resources income = Economy::computeSettlementIncome(s, raceMods, buildings, {});

    CHECK_NEAR(income.science, 10 * 1.15f, 0.01f);
}

TEST_CASE("Economy::computeSettlementIncome: несколько построек на одну сферу перемножаются") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(50);
    s.assignFromIdle(Sphere::Science, 10);

    Modifiers raceMods;
    std::vector<Building> buildings = {
        makeBuilding("library", "science", 1.15f),
        makeBuilding("observatory", "science", 1.3f),
    };

    Resources income = Economy::computeSettlementIncome(s, raceMods, buildings, {});

    CHECK_NEAR(income.science, 10 * 1.15f * 1.3f, 0.01f);
}

TEST_CASE("Economy::computeSettlementIncome: постройка не задевает другие сферы (industry-здание не влияет на science)") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(50);
    s.assignFromIdle(Sphere::Science, 10);
    s.assignFromIdle(Sphere::Industry, 10);

    Modifiers raceMods;
    std::vector<Building> buildings = {makeBuilding("workshop", "industry", 1.15f)};

    Resources income = Economy::computeSettlementIncome(s, raceMods, buildings, {});

    CHECK_NEAR(income.science, 10.0f, 0.01f);          // без изменений
    CHECK_NEAR(income.industry, 10 * 1.15f, 0.01f);    // бонус применился
}

TEST_CASE("Economy::computeSettlementIncome: бонус героя применяется вместе с расой и постройками") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(50);
    s.assignFromIdle(Sphere::Diplomacy, 10);

    Modifiers raceMods;
    raceMods.diplomacy = 1.4f;
    std::vector<Hero> heroes = {makeHero("hero_lira", "humans", Sphere::Diplomacy, 1.3f)};

    Resources income = Economy::computeSettlementIncome(s, raceMods, {}, heroes);

    CHECK_NEAR(income.diplomacy, 10 * 1.4f * 1.3f, 0.01f);
}

TEST_CASE("Economy::accumulate и totals(): накопление по ходам суммируется") {
    Economy economy;
    Resources turn1;
    turn1.science = 10;
    turn1.trade = 5;
    economy.accumulate(turn1);

    Resources turn2;
    turn2.science = 7;
    turn2.diplomacy = 3;
    economy.accumulate(turn2);

    CHECK_NEAR(economy.totals().science, 17.0f, 0.01f);
    CHECK_NEAR(economy.totals().trade, 5.0f, 0.01f);
    CHECK_NEAR(economy.totals().diplomacy, 3.0f, 0.01f);
}

TEST_CASE("Economy::trySpendTrade: списывает при достатке, false при нехватке") {
    Economy economy;
    Resources income;
    income.trade = 10;
    economy.accumulate(income);

    CHECK(economy.trySpendTrade(6));
    CHECK_NEAR(economy.totals().trade, 4.0f, 0.01f);
    CHECK(!economy.trySpendTrade(5));  // осталось только 4
    CHECK_NEAR(economy.totals().trade, 4.0f, 0.01f);  // неудачная попытка не списывает
}

TEST_CASE("Economy::trySpendDiplomacy: списывает при достатке, false при нехватке") {
    Economy economy;
    Resources income;
    income.diplomacy = 50;
    economy.accumulate(income);

    CHECK(economy.trySpendDiplomacy(50));
    CHECK_NEAR(economy.totals().diplomacy, 0.0f, 0.01f);
    CHECK(!economy.trySpendDiplomacy(1));
}

TEST_CASE("Economy::addResource добавляет очки в указанную сферу") {
    Economy economy;
    economy.addResource("science", 12);
    economy.addResource("science", 3);

    CHECK_NEAR(economy.totals().science, 15.0f, 0.01f);
}

TEST_CASE("Economy::addResource с неизвестным типом бросает исключение") {
    Economy economy;
    CHECK_THROWS(economy.addResource("magic", 5));
}
