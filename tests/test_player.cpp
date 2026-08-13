#include "test_framework.hpp"

#include "core/Player.hpp"
#include "core/Map.hpp"
#include "core/GameState.hpp"

namespace
{

    Race makeRace(const std::string &id, float science, float industry, float trade,
                  float diplomacy, float military, float growth)
    {
        json obj = {
            {"id", id},
            {"name", id},
            {"startingSettlementType", "village"},
            {"modifiers",
             {{"science", science}, {"industry", industry}, {"trade", trade}, {"diplomacy", diplomacy}, {"military", military}, {"growth", growth}}},
        };
        return Race::loadFromJson(obj);
    }

    Technology makeTech(const std::string &id, int cost, std::vector<std::string> prereqs = {})
    {
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

    Building makeBuilding(const std::string &id, int cost)
    {
        json obj = {
            {"id", id},
            {"name", id},
            {"category", "economic"},
            {"resourceType", "industry"},
            {"bonusMultiplier", 1.0},
            {"industryCost", cost},
            {"unlocksUnitIds", json::array()},
            {"removesUntrainedPenalty", false},
        };
        return Building::loadFromJson(obj);
    }

    Unit makeUnit(const std::string &id, int tier, const std::string &requiredBuildingId,
                  int recruitCost, int populationCost)
    {
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

} // namespace

TEST_CASE("Player: setRace/race и setId/id хранят переданные значения")
{
    Player p;
    p.setId(3);
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));

    CHECK(p.id() == 3);
    CHECK(p.race().getId() == "humans");
}

TEST_CASE("Player::addHero добавляет героя в список")
{
    Player p;
    json heroObj = {
        {"id", "hero_lira"},
        {"name", "Лира"},
        {"raceId", "humans"},
        {"modifiers", {{"science", 1.0}, {"industry", 1.0}, {"trade", 1.0}, {"diplomacy", 1.3}, {"military", 1.0}, {"growth", 1.0}}},
    };
    p.addHero(Hero::loadFromJson(heroObj));

    CHECK(p.heroes().size() == 1u);
    CHECK(p.heroes()[0].getId() == "hero_lira");
}

TEST_CASE("Player::setCurrentResearch: false для несуществующей технологии")
{
    Player p;
    p.loadTechnologies({makeTech("tech_1", 50)});

    CHECK(!p.setCurrentResearch("tech_unknown"));
}

TEST_CASE("Player::setCurrentResearch: false, если не выполнены пререквизиты")
{
    Player p;
    p.loadTechnologies({makeTech("tech_1", 50), makeTech("tech_2", 50, {"tech_1"})});

    CHECK(!p.setCurrentResearch("tech_2"));
}

TEST_CASE("Player::setCurrentResearch: true для доступной технологии без пререквизитов")
{
    Player p;
    p.loadTechnologies({makeTech("tech_1", 50)});

    CHECK(p.setCurrentResearch("tech_1"));
    CHECK(p.currentResearchId() == "tech_1");
}

TEST_CASE("Player::investScience: накопление очков, исследование завершается и currentResearchId сбрасывается")
{
    Player p;
    p.loadTechnologies({makeTech("tech_1", 50)});
    p.setCurrentResearch("tech_1");

    p.investScience(30);
    CHECK(p.currentResearchId() == "tech_1"); // ещё не хватило
    CHECK(!p.hasResearched("tech_1"));

    p.investScience(20);
    CHECK(p.currentResearchId().empty());
    CHECK(p.hasResearched("tech_1"));
}

TEST_CASE("Player::investScience: следующая технология доступна после исследования пререквизита")
{
    Player p;
    p.loadTechnologies({makeTech("tech_1", 10), makeTech("tech_2", 10, {"tech_1"})});
    p.setCurrentResearch("tech_1");
    p.investScience(10);

    CHECK(p.hasResearched("tech_1"));
    CHECK(p.setCurrentResearch("tech_2"));
}

TEST_CASE("Player::advanceTurn: доход считается по всем поселениям и копится в economy")
{
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

TEST_CASE("Player::advanceTurn: industry поселения продвигает именно его собственную очередь построек")
{
    Player p;
    p.setRace(makeRace("dwarves", 0.9f, 1.8f, 0.9f, 0.4f, 1.5f, 0.7f));

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Industry, 40); // доход = 40*1.8 = 72

    json buildingObj = {
        {"id", "workshop"},
        {"name", "Мастерская"},
        {"category", "economic"},
        {"resourceType", "industry"},
        {"bonusMultiplier", 1.15},
        {"industryCost", 40},
        {"unlocksUnitIds", json::array()},
        {"removesUntrainedPenalty", false},
    };
    s.queueBuilding(Building::loadFromJson(buildingObj));
    p.addSettlement(s);

    p.advanceTurn();

    CHECK(p.settlements()[0].hasBuilt("workshop"));
}

TEST_CASE("Player::advanceTurn: наука из дохода автоматически вкладывается в текущее исследование")
{
    Player p;
    p.setRace(makeRace("gnomes", 1.7f, 1.2f, 1.1f, 0.9f, 0.5f, 0.9f));
    p.loadTechnologies({makeTech("tech_1", 10)});
    p.setCurrentResearch("tech_1");

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Science, 10); // доход = 10*1.7 = 17, достаточно на tech_1(10)
    p.addSettlement(s);

    p.advanceTurn();

    CHECK(p.hasResearched("tech_1"));
}

TEST_CASE("Player::exchangeTrade: false при нехватке накопленной торговли")
{
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));

    CHECK(!p.exchangeTrade("science", 10)); // накоплено 0
}

TEST_CASE("Player::exchangeTrade: успешный обмен списывает торговлю и добавляет целевой ресурс")
{
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Trade, 10); // доход = 10*1.3 = 13
    p.addSettlement(s);
    p.advanceTurn(); // копим торговлю

    bool ok = p.exchangeTrade("science", 10);

    CHECK(ok);
    CHECK_NEAR(p.resources().trade, 3.0f, 0.1f); // 13 - 10
    CHECK(p.resources().science > 0.0f);         // получили науку взамен
}

TEST_CASE("Player::trySpendOnPoliticalAction: false при нехватке дипломатии")
{
    Player p;
    json actionObj = {{"id", "pa_alliance"}, {"name", "Союз"}, {"type", "alliance"}, {"diplomacyCost", 50}};
    PoliticalAction action = PoliticalAction::loadFromJson(actionObj);

    CHECK(!p.trySpendOnPoliticalAction(action));
}

TEST_CASE("Player::trySpendOnPoliticalAction: успешно списывает при достатке дипломатии")
{
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Diplomacy, 40); // доход = 40*1.4 = 56
    p.addSettlement(s);
    p.advanceTurn();

    json actionObj = {{"id", "pa_alliance"}, {"name", "Союз"}, {"type", "alliance"}, {"diplomacyCost", 50}};
    PoliticalAction action = PoliticalAction::loadFromJson(actionObj);

    CHECK(p.trySpendOnPoliticalAction(action));
    CHECK_NEAR(p.resources().diplomacy, 6.0f, 0.1f); // 56 - 50
}

TEST_CASE("Player::spendIndustryOnQueues: тратит уже накопленный банк на очередь сразу, "
          "не дожидаясь следующего advanceTurn")
{
    Player p;
    p.setRace(makeRace("dwarves", 0.9f, 1.8f, 0.9f, 0.4f, 1.5f, 0.7f));

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Industry, 40); // доход = 40*1.8 = 72
    p.addSettlement(s);

    p.advanceTurn(); // очередь пуста -> весь доход банкуется
    CHECK_NEAR(p.resources().industry, 72.0f, 0.01f);

    Settlement &live = p.settlements()[0];
    live.queueBuilding(makeBuilding("library", 50));

    p.spendIndustryOnQueues(live);

    CHECK(live.hasBuilt("library"));
    CHECK_NEAR(p.resources().industry, 22.0f, 0.01f); // 72 - 50, остаток снова в банке
}

TEST_CASE("Player::spendIndustryOnQueues: постройки в приоритете над наймом, "
          "остаток при неполном покрытии не уходит в очередь юнитов")
{
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f)); // industry x1

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Industry, 50); // доход = 50
    p.addSettlement(s);

    p.advanceTurn(); // банкуем 50
    CHECK_NEAR(p.resources().industry, 50.0f, 0.01f);

    Settlement &live = p.settlements()[0];
    live.queueBuilding(makeBuilding("keep", 80));                 // дороже, чем есть в банке
    live.queueUnitRecruitment(makeUnit("militia", 0, "", 30, 5)); // tier 0, без требований к зданию

    p.spendIndustryOnQueues(live);

    CHECK(!live.hasBuilt("keep"));
    CHECK(live.buildingQueue().size() == 1u);
    CHECK(live.buildingQueue()[0].remainingCost() == 30); // 80 - 50 вложенных
    CHECK(live.garrison().empty());                       // до найма очередь не дошла
    CHECK(live.recruitmentQueue().size() == 1u);
    CHECK_NEAR(p.resources().industry, 0.0f, 0.01f); // банк потрачен полностью, банковать нечего
}

TEST_CASE("Player::spendIndustryOnQueues: пустой банк — ничего не меняет и не падает")
{
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    p.addSettlement(s);

    Settlement &live = p.settlements()[0];
    live.queueBuilding(makeBuilding("library", 50));

    CHECK_NOTHROW(p.spendIndustryOnQueues(live));

    CHECK(!live.hasBuilt("library"));
    CHECK(live.buildingQueue()[0].investedIndustry == 0);
    CHECK_NEAR(p.resources().industry, 0.0f, 0.01f);
}

TEST_CASE("Player::advanceTurn + spendIndustryOnQueues одной парой (как в GameEngine::endTurn) "
          "не расходуют и не теряют промышленность за один ход")
{
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1.3f, 1.4f, 1, 1.1f));

    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Industry, 50); // доход = 50
    Settlement sCopy = s;
    sCopy.queueBuilding(makeBuilding("workshop", 30));
    p.addSettlement(sCopy);

    Settlement &live = p.settlements()[0];

    // Ровно та же последовательность вызовов, что в GameEngine::endTurn()
    p.advanceTurn();
    p.spendIndustryOnQueues(live);

    CHECK(live.hasBuilt("workshop"));
    CHECK_NEAR(p.resources().industry, 20.0f, 0.01f); // 50 - 30, ни потери, ни задвоения
}
// ═══════════════════════════════════════════════════════════════════════
// Блок 3.6: туман войны — Player::recomputeVisibility
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("Player::recomputeVisibility: видимость растёт вокруг поселения в пределах радиуса")
{
    Map map = Map::generateMap(20, 20);
    Player p;
    p.setId(0);
    Settlement s(0, SettlementType::Village);
    s.addTile(Tile(10, 10));
    p.addSettlement(s);

    p.recomputeVisibility(map, 2);

    CHECK(p.isVisible(Coords(10, 10)));  // сама клетка поселения
    CHECK(p.isVisible(Coords(10, 12)));  // в пределах радиуса 2
    CHECK(!p.isVisible(Coords(10, 20))); // далеко за радиусом
    CHECK(p.isExplored(Coords(10, 10))); // explored пополняется из visible
}

TEST_CASE("Player::recomputeVisibility: видимость растёт вокруг армии")
{
    Map map = Map::generateMap(20, 20);
    Player p;
    p.setId(0);
    p.addArmy(Army("a1", 0, Coords(5, 5), {}));

    p.recomputeVisibility(map, 2);

    CHECK(p.isVisible(Coords(5, 5)));
    CHECK(p.isVisible(Coords(6, 5)));
    CHECK(!p.isVisible(Coords(15, 15)));
}

TEST_CASE("Player::recomputeVisibility: explored не убывает, а visible пересчитывается заново")
{
    Map map = Map::generateMap(20, 20);
    Player p;
    p.setId(0);
    p.addArmy(Army("a1", 0, Coords(2, 2), {}));

    p.recomputeVisibility(map, 1);
    CHECK(p.isVisible(Coords(2, 2)));
    CHECK(p.isExplored(Coords(2, 2)));

    // Армия ушла далеко — старая клетка больше не visible, но остаётся explored.
    p.armies()[0].moveTo(Coords(15, 15), 0);
    p.recomputeVisibility(map, 1);

    CHECK(!p.isVisible(Coords(2, 2)));  // больше не видно прямо сейчас
    CHECK(p.isExplored(Coords(2, 2)));  // но разведано навсегда
    CHECK(p.isVisible(Coords(15, 15))); // а новая позиция видна
}

TEST_CASE("Player::recomputeVisibility: без поселений и армий ничего не видно")
{
    Map map = Map::generateMap(10, 10);
    Player p;
    p.setId(0);

    p.recomputeVisibility(map, 3);

    CHECK(p.visibleTiles().empty());
    CHECK(p.exploredTiles().empty());
}

// ═══════════════════════════════════════════════════════════════════════
// Блок 3.6: GameState::moveArmy — ограничение по разведанности для человека
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("GameState::moveArmy: человек не может пойти в неразведанное")
{
    GameState state;
    state.setMap(Map::generateMap(20, 20));

    Player p;
    p.setId(0);
    p.setIsHuman(true);
    p.addArmy(Army("a1", 0, Coords(5, 5), {}));
    state.addPlayer(p);
    state.recomputeVisibility(2); // видно только вокруг (5,5)

    auto result = state.moveArmy("a1", Coords(19, 19)); // далеко за пределами видимости

    CHECK(!result.success);
    CHECK(!result.errorMessage.empty());
}

TEST_CASE("GameState::moveArmy: человек может пойти в разведанную клетку")
{
    GameState state;
    state.setMap(Map::generateMap(20, 20));

    Player p;
    p.setId(0);
    p.setIsHuman(true);
    Unit scout = Unit::loadFromJson({
        {"id", "scout"}, {"name", "scout"},
        {"stats", {{"health", 10}, {"attack", 1}, {"defense", 1}, {"movementPoints", 3}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 10}, {"populationCost", 10}
    });
    p.addArmy(Army("a1", 0, Coords(5, 5), {scout}));
    state.addPlayer(p);
    state.recomputeVisibility(3);

    auto result = state.moveArmy("a1", Coords(6, 5)); // рядом, в пределах радиуса 3

    CHECK(result.success);
}

TEST_CASE("GameState::moveArmy: ИИ (не-человек) игнорирует туман войны — может идти куда угодно")
{
    GameState state;
    state.setMap(Map::generateMap(20, 20));

    Player p;
    p.setId(0);
    p.setIsHuman(false);
    Unit scout = Unit::loadFromJson({
        {"id", "scout"}, {"name", "scout"},
        {"stats", {{"health", 10}, {"attack", 1}, {"defense", 1}, {"movementPoints", 3}}},
        {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 10}, {"populationCost", 10}
    });
    p.addArmy(Army("a1", 0, Coords(5, 5), {scout}));
    state.addPlayer(p);
    // recomputeVisibility специально НЕ вызываем — у бота нет разведанных клеток вовсе.

    auto result = state.moveArmy("a1", Coords(6, 5));

    CHECK(result.success); // ограничение на ИИ не распространяется
}
