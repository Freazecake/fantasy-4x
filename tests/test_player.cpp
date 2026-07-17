#include "test_framework.hpp"

#include "core/Player.hpp"

namespace {

Race makeRace(const std::string &id, float science, float industry, float trade,
              float diplomacy, float military, float growth) {
    json obj = {
        {"id", id},
        {"name", id},
        {"startingSettlementType", "village"},
        {"modifiers",
         {{"science", science}, {"industry", industry}, {"trade", trade},
          {"diplomacy", diplomacy}, {"military", military}, {"growth", growth}}},
    };
    return Race::loadFromJson(obj);
}

Technology makeTech(const std::string &id, int cost, std::vector<std::string> prereqs = {}) {
    json obj = {
        {"id", id},
        {"name", id},
        {"category", "science"},
        {"scienceCost", cost},
        {"prerequisiteIds", prereqs},
        {"unlocksBuildingIds", json::array()},
        {"bonusCategories", json::array()},
        {"globalBonusMultiplier", 1.0},
    };
    return Technology::loadFromJson(obj);
}

}  // namespace

TEST_CASE("Player: setRace/race и setId/id хранят переданные значения") {
    Player p;
    p.setId(3);
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));

    CHECK(p.id() == 3);
    CHECK(p.race().getId() == "humans");
}

TEST_CASE("Player::addHero добавляет героя в список") {
    Player p;
    json heroObj = {
        {"id", "hero_lira"}, {"name", "Лира"}, {"raceId", "humans"},
        {"modifiers", {{"science", 1.0}, {"industry", 1.0}, {"trade", 1.0},
                       {"diplomacy", 1.3}, {"military", 1.0}, {"growth", 1.0}}},
    };
    p.addHero(Hero::loadFromJson(heroObj));

    CHECK(p.heroes().size() == 1u);
    CHECK(p.heroes()[0].getId() == "hero_lira");
}

TEST_CASE("Player::setCurrentResearch: false для несуществующей технологии") {
    Player p;
    p.loadTechnologies({makeTech("tech_1", 50)});

    CHECK(!p.setCurrentResearch("tech_unknown"));
}

TEST_CASE("Player::setCurrentResearch: false, если не выполнены пререквизиты") {
    Player p;
    p.loadTechnologies({makeTech("tech_1", 50), makeTech("tech_2", 50, {"tech_1"})});

    CHECK(!p.setCurrentResearch("tech_2"));
}

TEST_CASE("Player::setCurrentResearch: true для доступной технологии без пререквизитов") {
    Player p;
    p.loadTechnologies({makeTech("tech_1", 50)});

    CHECK(p.setCurrentResearch("tech_1"));
    CHECK(p.currentResearchId() == "tech_1");
}

TEST_CASE("Player::investScience: накопление очков, исследование завершается и currentResearchId сбрасывается") {
    Player p;
    p.loadTechnologies({makeTech("tech_1", 50)});
    p.setCurrentResearch("tech_1");

    p.investScience(30);
    CHECK(p.currentResearchId() == "tech_1");  // ещё не хватило
    CHECK(!p.hasResearched("tech_1"));

    p.investScience(20);
    CHECK(p.currentResearchId().empty());
    CHECK(p.hasResearched("tech_1"));
}

TEST_CASE("Player::investScience: следующая технология доступна после исследования пререквизита") {
    Player p;
    p.loadTechnologies({makeTech("tech_1", 10), makeTech("tech_2", 10, {"tech_1"})});
    p.setCurrentResearch("tech_1");
    p.investScience(10);

    CHECK(p.hasResearched("tech_1"));
    CHECK(p.setCurrentResearch("tech_2"));
}

TEST_CASE("Player::advanceTurn: доход считается по всем поселениям и копится в economy") {
    Player p;
    p.setRace(makeRace("dwarves", 0.9f, 1.8f, 0.9f, 0.4f, 1.5f, 0.7f));

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Industry, 40);
    p.addSettlement(s);

    p.advanceTurn();

    // Без очередей вся промышленность банкуется
    CHECK_NEAR(p.resources().industry, 40 * 1.8f, 0.01f);
}

TEST_CASE("Player::advanceTurn: industry поселения продвигает именно его собственную очередь построек") {
    Player p;
    p.setRace(makeRace("dwarves", 0.9f, 1.8f, 0.9f, 0.4f, 1.5f, 0.7f));

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Industry, 40);  // доход = 40*1.8 = 72

    json buildingObj = {
        {"id", "workshop"}, {"name", "Мастерская"}, {"category", "economic"},
        {"resourceType", "industry"}, {"bonusMultiplier", 1.15}, {"industryCost", 40},
        {"unlocksUnitIds", json::array()}, {"removesUntrainedPenalty", false},
    };
    s.queueBuilding(Building::loadFromJson(buildingObj));
    p.addSettlement(s);

    p.advanceTurn();

    CHECK(p.settlements()[0].hasBuilt("workshop"));
}

TEST_CASE("Player::advanceTurn: наука из дохода автоматически вкладывается в текущее исследование") {
    Player p;
    p.setRace(makeRace("gnomes", 1.7f, 1.2f, 1.1f, 0.9f, 0.5f, 0.9f));
    p.loadTechnologies({makeTech("tech_1", 10)});
    p.setCurrentResearch("tech_1");

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Science, 10);  // доход = 10*1.7 = 17, достаточно на tech_1(10)
    p.addSettlement(s);

    p.advanceTurn();

    CHECK(p.hasResearched("tech_1"));
}

TEST_CASE("Player::exchangeTrade: false при нехватке накопленной торговли") {
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));

    CHECK(!p.exchangeTrade("science", 10));  // накоплено 0
}

TEST_CASE("Player::exchangeTrade: успешный обмен списывает торговлю и добавляет целевой ресурс") {
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Trade, 10);  // доход = 10*1.3 = 13
    p.addSettlement(s);
    p.advanceTurn();  // копим торговлю

    bool ok = p.exchangeTrade("science", 10);

    CHECK(ok);
    CHECK_NEAR(p.resources().trade, 3.0f, 0.1f);  // 13 - 10
    CHECK(p.resources().science > 0.0f);          // получили науку взамен
}

TEST_CASE("Player::trySpendOnPoliticalAction: false при нехватке дипломатии") {
    Player p;
    json actionObj = {{"id", "pa_alliance"}, {"name", "Союз"}, {"type", "alliance"}, {"diplomacyCost", 50}};
    PoliticalAction action = PoliticalAction::loadFromJson(actionObj);

    CHECK(!p.trySpendOnPoliticalAction(action));
}

TEST_CASE("Player::trySpendOnPoliticalAction: успешно списывает при достатке дипломатии") {
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Diplomacy, 40);  // доход = 40*1.4 = 56
    p.addSettlement(s);
    p.advanceTurn();

    json actionObj = {{"id", "pa_alliance"}, {"name", "Союз"}, {"type", "alliance"}, {"diplomacyCost", 50}};
    PoliticalAction action = PoliticalAction::loadFromJson(actionObj);

    CHECK(p.trySpendOnPoliticalAction(action));
    CHECK_NEAR(p.resources().diplomacy, 6.0f, 0.1f);  // 56 - 50
}
