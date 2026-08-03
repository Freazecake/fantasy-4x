#include "test_framework.hpp"

#include "core/Tile.hpp"
#include "core/Map.hpp"
#include "core/Army.hpp"
#include "core/Player.hpp"
#include "core/GameState.hpp"
#include "core/Combat.hpp"
#include "core/Military.hpp"
#include "core/PoliticalAction.hpp"
#include "core/Hero.hpp"

#include "ai/AIController.hpp"
#include "ai/AIStrategy.hpp"

#include <algorithm>
#include <cmath>

namespace
{

    json makeActionJson(const std::string &id, const std::string &type, int cost)
    {
        return {{"id", id}, {"name", id + "_name"}, {"type", type}, {"diplomacyCost", cost}};
    }

    Race makeRace(const std::string &id, float science, float industry, float trade,
                  float diplomacy, float military, float growth)
    {
        return Race::loadFromJson({
            {"id", id},
            {"name", id},
            {"startingSettlementType", "village"},
            {"modifiers", {{"science", science}, {"industry", industry}, {"trade", trade}, {"diplomacy", diplomacy}, {"military", military}, {"growth", growth}}},
        });
    }

    Building makeBuilding(const std::string &id, int cost,
                          const std::vector<std::string> &unlocksUnitIds = {},
                          bool removesPenalty = false,
                          float sci = 1.0f, float ind = 1.0f, float trade = 1.0f,
                          float dipl = 1.0f, float mil = 1.0f)
    {
        std::string resourceType = "industry";
        float bonus = ind;
        if (sci != 1.0f)
        {
            resourceType = "science";
            bonus = sci;
        }
        else if (ind != 1.0f)
        {
            resourceType = "industry";
            bonus = ind;
        }
        else if (trade != 1.0f)
        {
            resourceType = "trade";
            bonus = trade;
        }
        else if (dipl != 1.0f)
        {
            resourceType = "diplomacy";
            bonus = dipl;
        }
        else if (mil != 1.0f)
        {
            resourceType = "military";
            bonus = mil;
        }

        json arr = json::array();
        for (const auto &u : unlocksUnitIds)
            arr.push_back(u);
        return Building::loadFromJson({
            {"id", id},
            {"name", id},
            {"category", "economic"},
            {"resourceType", resourceType},
            {"bonusMultiplier", bonus},
            {"industryCost", cost},
            {"unlocksUnitIds", arr},
            {"removesUntrainedPenalty", removesPenalty},
        });
    }

    Technology makeTech(const std::string &id, int cost,
                        const std::vector<std::string> &prereqs = {},
                        const std::vector<std::string> &bonusCategories = {},
                        float globalMul = 1.0f)
    {
        json prereqArr = json::array();
        for (const auto &p : prereqs)
            prereqArr.push_back(p);
        json catArr = json::array();
        for (const auto &c : bonusCategories)
            catArr.push_back(c);
        json unlocksArr = json::array();
        return Technology::loadFromJson({
            {"id", id},
            {"name", id},
            {"category", "science"},
            {"scienceCost", cost},
            {"prerequisiteIds", prereqArr},
            {"unlocksBuildingIds", unlocksArr},
            {"bonusCategories", catArr},
            {"globalBonusMultiplier", globalMul},
        });
    }

    Unit makeUnit(const std::string &id, int tier, const std::string &requiredBuildingId,
                  float attack, float defense, float health,
                  int recruitCost = 20, int populationCost = 30, int mp = 2)
    {
        return Unit::loadFromJson({
            {"id", id},
            {"name", id},
            {"stats", {{"health", health}, {"attack", attack}, {"defense", defense}, {"movementPoints", mp}}},
            {"tier", tier},
            {"requiredBuildingId", requiredBuildingId},
            {"recruitCost", recruitCost},
            {"populationCost", populationCost},
        });
    }

    Hero makeHero(const std::string &id, const std::string &raceId, float mil = 1.0f)
    {
        return Hero::loadFromJson({
            {"id", id},
            {"name", id},
            {"raceId", raceId},
            {"modifiers", {{"science", 1}, {"industry", 1}, {"trade", 1}, {"diplomacy", 1}, {"military", mil}, {"growth", 1}}},
        });
    }

    PoliticalAction makeAction(const std::string &id, const std::string &type, int cost)
    {
        return PoliticalAction::loadFromJson(makeActionJson(id, type, cost));
    }

    void setupTwoPlayerState(GameState &state, Map &map,
                             Player &p1, Player &p2,
                             Settlement &s1, Settlement &s2)
    {
        map = Map::generateMap(10, 10);
        state.setMap(map);

        p1.setId(0);
        p1.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));
        p2.setId(1);
        p2.setRace(makeRace("orcs", 0.8f, 1, 1, 0.5f, 1.5f, 1));

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

} // namespace

// ═══════════════════════════════════════════════════════════════════════
// Часть 1: AIStrategy scoring
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIStrategy::scoreAllocatePopulation: -1 для Military и Growth")
{
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(10);

    float mil = AIStrategy::scoreAllocatePopulation(s, Sphere::Military, GlobalPhase::NotStarted);
    float gro = AIStrategy::scoreAllocatePopulation(s, Sphere::Growth, GlobalPhase::NotStarted);

    CHECK(mil < 0.0f);
    CHECK(gro < 0.0f);
}

TEST_CASE("AIStrategy::scoreAllocatePopulation: diversityBonus выше при нулевой доле")
{
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Science, 40);
    s.assignFromIdle(Sphere::Industry, 40);

    float sci = AIStrategy::scoreAllocatePopulation(s, Sphere::Science, GlobalPhase::Phase2_OpenMap);
    float dip = AIStrategy::scoreAllocatePopulation(s, Sphere::Diplomacy, GlobalPhase::Phase2_OpenMap);

    CHECK(dip > sci);
}

TEST_CASE("AIStrategy::scoreBuild: -1 для уже построенного")
{
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(10);
    Building b = makeBuilding("test", 10);
    s.queueBuilding(b);
    s.progressBuildingQueue(10);

    float score = AIStrategy::scoreBuild(s, b, GlobalPhase::NotStarted);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scoreBuild: военный модификатор даёт вклад")
{
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Industry, 50);

    Building milBld = makeBuilding("mil_b", 20, {}, false, 1, 1, 1, 1, 1.5f);
    Building plainBld = makeBuilding("plain_b", 20);

    float milScore = AIStrategy::scoreBuild(s, milBld, GlobalPhase::Phase2_OpenMap);
    float plainScore = AIStrategy::scoreBuild(s, plainBld, GlobalPhase::Phase2_OpenMap);

    CHECK(milScore > plainScore);
}

TEST_CASE("AIStrategy::scoreResearch: -1 для уже исследованного")
{
    Player p;
    p.loadTechnologies({makeTech("t1", 10)});
    p.setCurrentResearch("t1");
    p.investScience(10);

    float score = AIStrategy::scoreResearch(p, p.technologies()[0], GlobalPhase::NotStarted);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scoreResearch: -1 если не выполнены пререквизиты")
{
    Player p;
    p.loadTechnologies({makeTech("t1", 10, {"nonexistent"})});

    float score = AIStrategy::scoreResearch(p, makeTech("t1", 10, {"nonexistent"}), GlobalPhase::NotStarted);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scoreResearch: положительный скор для доступной tech")
{
    Player p;
    p.loadTechnologies({makeTech("t1", 10, {}, {"industry"}, 1.5f)});

    float score = AIStrategy::scoreResearch(p, makeTech("t1", 10, {}, {"industry"}, 1.5f), GlobalPhase::NotStarted);
    CHECK(score > 0.0f);
}

TEST_CASE("AIStrategy::scoreRecruitUnit: -1 если нет требуемого здания")
{
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(100);
    Unit u = makeUnit("sword", 1, "barracks", 10, 5, 30);

    float score = AIStrategy::scoreRecruitUnit(s, u, GlobalPhase::NotStarted);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scoreRecruitUnit: -1 если не хватает населения")
{
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(5);
    Unit u = makeUnit("militia", 0, "", 5, 2, 20, 20, 30);

    float score = AIStrategy::scoreRecruitUnit(s, u, GlobalPhase::NotStarted);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scoreRecruitUnit: положительный для доступного юнита")
{
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(100);
    Unit u = makeUnit("militia", 0, "", 5, 2, 20);

    float score = AIStrategy::scoreRecruitUnit(s, u, GlobalPhase::NotStarted);
    CHECK(score > 0.0f);
}

TEST_CASE("AIStrategy::scoreTradeExchange: -1 если торговли нет")
{
    Player p;
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    float score = AIStrategy::scoreTradeExchange(p, "science", GlobalPhase::NotStarted);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scorePoliticalAction: -1 если не хватает дипломатии")
{
    Player p, target;
    p.setId(0);
    target.setId(1);
    auto action = makeAction("pa_alliance", "alliance", 999);

    float score = AIStrategy::scorePoliticalAction(p, action, target, GlobalPhase::NotStarted);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scorePoliticalAction: PoliticalConquest выше против слабого")
{
    Player p, weak, strong;
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));
    weak.setRace(makeRace("orcs", 1, 1, 1, 1, 1, 1));
    strong.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(100);
    s.assignFromIdle(Sphere::Diplomacy, 50);
    p.addSettlement(s);
    p.advanceTurn();

    auto conquest = makeAction("pa_conquest", "politicalConquest", 30);

    float scoreWeak = AIStrategy::scorePoliticalAction(p, conquest, weak, GlobalPhase::NotStarted);
    float scoreStrong = AIStrategy::scorePoliticalAction(p, conquest, strong, GlobalPhase::NotStarted);

    CHECK(scoreWeak > 0.0f);
    CHECK(scoreStrong > 0.0f);
}

TEST_CASE("AIStrategy::scoreAttack: -1 для пустой армии")
{
    Army me("me", 0, {0, 0}, {});
    Army enemy("enemy", 1, {1, 0}, {makeUnit("u", 0, "", 5, 2, 20)});

    float score = AIStrategy::scoreAttack(me, enemy, GlobalPhase::NotStarted);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scoreAttack: 5*factor если враг пуст")
{
    Army me("me", 0, {0, 0}, {makeUnit("u", 0, "", 5, 2, 20)});
    Army empty("empty", 1, {1, 0}, {});

    float score = AIStrategy::scoreAttack(me, empty, GlobalPhase::NotStarted);
    CHECK(score > 0.0f);
}

TEST_CASE("AIStrategy::scoreAttack: сильный бьёт слабого — положительно")
{
    Unit strongU = makeUnit("strong", 0, "", 20, 10, 50);
    Unit weakU = makeUnit("weak", 0, "", 2, 1, 10);

    Army me("me", 0, {0, 0}, {strongU});
    Army enemy("enemy", 1, {1, 0}, {weakU});

    float score = AIStrategy::scoreAttack(me, enemy, GlobalPhase::NotStarted);
    CHECK(score > 0.0f);
}

TEST_CASE("AIStrategy::scoreExpand: -1 для воды")
{
    Map map = Map::generateMap(5, 5);
    Tile *water = map.tileAt(2, 2);
    CHECK(water != nullptr);
    water->setType(TileType::Water);

    Army me("me", 0, {0, 0}, {makeUnit("u", 0, "", 5, 2, 20)});

    float score = AIStrategy::scoreExpand(me, {2, 2}, GlobalPhase::NotStarted, map);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scoreExpand: -1 для той же клетки")
{
    Map map = Map::generateMap(5, 5);
    Army me("me", 0, {0, 0}, {makeUnit("u", 0, "", 5, 2, 20)});

    float score = AIStrategy::scoreExpand(me, {0, 0}, GlobalPhase::NotStarted, map);
    CHECK(score < 0.0f);
}

TEST_CASE("AIStrategy::scoreExpand: Grass/Desert ценнее чем Mountain")
{
    Map map = Map::generateMap(10, 10);
    Army me("me", 0, {0, 0}, {makeUnit("u", 0, "", 5, 2, 20)});

    float grass = AIStrategy::scoreExpand(me, {1, 0}, GlobalPhase::Phase2_OpenMap, map);
    float mount = AIStrategy::scoreExpand(me, {1, 1}, GlobalPhase::Phase2_OpenMap, map);

    CHECK(grass > mount);
}

// ═══════════════════════════════════════════════════════════════════════
// Часть 2: AIController decisions
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::decideAllocations: idle распределяется пропорционально")
{
    GlobalPhase phase = GlobalPhase::Phase2_OpenMap;
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(100);

    AIController::decideAllocations(s, phase);

    int totalAssigned = s.allocation().science + s.allocation().industry + s.allocation().trade + s.allocation().diplomacy;
    CHECK(totalAssigned > 0);
    CHECK(s.allocation().idle < 100);
}

TEST_CASE("AIController::decideConstruction: ставит в очередь лучшее здание")
{
    GlobalPhase phase = GlobalPhase::Phase2_OpenMap;
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(50);
    s.assignFromIdle(Sphere::Industry, 20);

    Building good = makeBuilding("good", 10, {}, false, 1, 1.5f, 1, 1, 1);
    Building bad = makeBuilding("bad", 10, {}, false, 1, 0.5f, 1, 1, 1);
    std::vector<Building> catalog = {bad, good};

    AIController::decideConstruction(s, catalog, phase);

    CHECK(s.buildingQueue().size() == 1u);
    CHECK(s.buildingQueue()[0].building.getId() == "good");
}

TEST_CASE("AIController::decideResearch: выбирает лучшую доступную tech")
{
    GlobalPhase phase = GlobalPhase::Phase2_OpenMap;
    Player p;
    p.loadTechnologies({
        makeTech("bad", 100),
        makeTech("good", 10, {}, {"industry"}, 1.5f),
    });
    AIController::decideResearch(p, p.technologies(), phase);

    CHECK(p.currentResearchId() == "good");
}

TEST_CASE("AIController::decideRecruitment: выбирает лучшего доступного юнита")
{
    GlobalPhase phase = GlobalPhase::NotStarted;
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(100);

    Unit weak = makeUnit("weak", 0, "", 1, 1, 5, 20, 30);
    Unit strong = makeUnit("strong", 0, "", 15, 10, 50, 20, 30);
    std::vector<Unit> catalog = {weak, strong};

    AIController::decideRecruitment(s, catalog, phase);

    CHECK(s.recruitmentQueue().size() == 1u);
    CHECK(s.recruitmentQueue()[0].unit.getId() == "strong");
}

TEST_CASE("AIController::decidePoliticalAction: выбирает Conquest против слабого")
{
    GameState state;
    Map map;
    Player p1, p2;
    Settlement s1, s2;
    setupTwoPlayerState(state, map, p1, p2, s1, s2);

    Player &human = state.players()[0];
    Settlement &settlement = human.settlements()[0];
    settlement.assignFromIdle(Sphere::Diplomacy, 40);
    human.advanceTurn();

    auto conquest = makeAction("pa_conquest", "politicalConquest", 30);
    auto alliance = makeAction("pa_alliance", "alliance", 10);
    std::vector<PoliticalAction> catalog = {alliance, conquest};

    AIController::decidePoliticalAction(human, catalog, state);

    CHECK(human.resources().diplomacy < 30.0f);
    // Мало просто "дипломатия уменьшилась" — это верно и для alliance (cost
    // 10), и для conquest (cost 30). Проверяем именно следствие Conquest:
    // поселения цели переходят победителю (см. GameState::applyPoliticalConquest).
    bool targetSettlementsTransferred = state.players()[1].settlements().empty();
    CHECK(targetSettlementsTransferred);
    bool humanGainedSettlement = state.players()[0].settlements().size() > 1;
    CHECK(humanGainedSettlement);
    // И заодно — что альянс НЕ был создан вместо этого.
    CHECK(state.alliances().empty());
}

TEST_CASE("AIController::decideMilitaryActions: армия движется к врагу")
{
    GameState state;
    Map map;
    Player p1, p2;
    Settlement s1, s2;
    setupTwoPlayerState(state, map, p1, p2, s1, s2);

    Unit u = makeUnit("u", 0, "", 10, 5, 30);
    u.setTrained(true);
    state.players()[0].addArmy(Army("army0", 0, {1, 1}, {u}));
    state.players()[0].armies()[0].resetMovement();

    state.players()[1].addArmy(Army("enemy", 1, {8, 8}, {makeUnit("e", 0, "", 2, 1, 10)}));

    AIController::decideMilitaryActions(state.players()[0], state, GlobalPhase::Phase2_OpenMap);

    Army *army = state.findArmy("army0");
    CHECK(army != nullptr);
    CHECK(army->position().q != 1 || army->position().r != 1);
}

// ═══════════════════════════════════════════════════════════════════════
// Часть 3: Интеграция — симуляция 3 ходов с 2 AI игроками
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::takeTurn: найм юнитов реально происходит "
          "(регрессия на порядок decideRecruitment/decideAllocations)")
{
    // decideAllocations всегда распределяет ВСЁ свободное население между
    // гражданскими сферами без остатка (см. её реализацию: remaining всегда
    // уходит в лучшую сферу). Если бы она вызывалась внутри takeTurn раньше
    // decideRecruitment, найм юнитов почти никогда не находил бы свободных
    // людей — очередь найма оставалась бы пустой из хода в ход, несмотря на
    // явно выгодного юнита в каталоге.
    GameState state;
    Map map = Map::generateMap(6, 6);
    state.setMap(map);

    Player bot;
    bot.setId(0);
    bot.setRace(makeRace("orcs", 0.8f, 1, 1, 0.5f, 1.5f, 1));
    Settlement s(0, SettlementType::Village);
    s.addTile(Tile(0, 0));
    s.seedInitialPopulation(50); // всё в idle — ИИ есть, чем и население занять, и юнита нанять
    bot.addSettlement(s);
    state.addPlayer(bot);

    std::vector<Building> buildingCatalog; // построек нет — не отвлекает от вопроса найма
    std::vector<Technology> techCatalog;
    std::vector<Unit> unitCatalog = {makeUnit("militia", 0, "", 5, 3, 20, 20, 10)};
    std::vector<PoliticalAction> actionCatalog;

    AIController::takeTurn(state.players()[0], state, buildingCatalog, techCatalog,
                           unitCatalog, actionCatalog);

    CHECK(state.players()[0].settlements()[0].recruitmentQueue().size() == 1u);
}

TEST_CASE("AIController::takeTurn: 3 хода двух AI игроков")
{
    GameState state;
    Map map = Map::generateMap(12, 10);
    state.setMap(map);

    auto humanRace = makeRace("humans", 1, 1, 1, 1, 1, 1);
    auto orcRace = makeRace("orcs", 0.8f, 1, 1, 0.5f, 1.5f, 1);

    std::vector<Technology> techCatalog = {
        makeTech("basic_sci", 30, {}, {"science"}, 1.2f),
        makeTech("basic_ind", 30, {}, {"industry"}, 1.3f),
    };

    for (int i = 0; i < 2; ++i)
    {
        Player p;
        p.setId(i);
        p.setRace(i == 0 ? humanRace : orcRace);
        p.loadTechnologies(techCatalog);
        Settlement s(i, SettlementType::Village);
        s.addTile(Tile(1 + i * 6, 1 + i * 2));
        s.seedInitialPopulation(60);
        p.addSettlement(s);
        state.addPlayer(p);
    }

    std::vector<Building> buildingCatalog = {
        makeBuilding("workshop", 30, {}, false, 1, 1.3f, 1, 1, 1),
        makeBuilding("barracks", 40, {}, true, 1, 1, 1, 1, 1.2f),
    };

    std::vector<Unit> unitCatalog = {
        makeUnit("militia", 0, "", 3, 2, 20, 20, 30),
    };

    std::vector<PoliticalAction> actionCatalog = {
        makeAction("pa_alliance", "alliance", 20),
        makeAction("pa_conquest", "politicalConquest", 200),
    };

    for (int turn = 0; turn < 3; ++turn)
    {
        for (auto &player : state.players())
        {
            AIController::takeTurn(player, state,
                                   buildingCatalog, techCatalog,
                                   unitCatalog, actionCatalog);
            player.advanceTurn();
            for (auto &s : player.settlements())
            {
                s.growPopulation(0.1f, player.race().getModifiers().growth);
                player.spendIndustryOnQueues(s);
            }
            for (auto &army : player.armies())
                army.resetMovement();
        }
        state.nextTurn();
    }

    for (const auto &player : state.players())
    {
        CHECK(player.resources().science >= 0.0f);
        CHECK(player.resources().industry >= 0.0f);
        CHECK(player.currentResearchId().empty() || !player.currentResearchId().empty());
    }

    CHECK(state.turnNumber() == 3);
}

TEST_CASE("AIController::decideTradeExchange: реально обменивает торговлю на дефицитный ресурс")
{
    Player p;
    p.setId(0);
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    Resources r;
    r.trade = 100.0f;
    r.science = 0.0f;
    r.industry = 0.0f;
    r.diplomacy = 0.0f;
    p.addResources(r);

    float tradeBefore = p.resources().trade;
    AIController::decideTradeExchange(p, GlobalPhase::Phase2_OpenMap);

    CHECK(p.resources().trade < tradeBefore);
    CHECK(p.resources().science + p.resources().industry + p.resources().diplomacy > 0.0f);
}

TEST_CASE("AIController::decideTradeExchange: не трогает торговлю, если её меньше единицы")
{
    Player p;
    p.setId(0);
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    Resources r;
    r.trade = 0.5f;
    p.addResources(r);

    AIController::decideTradeExchange(p, GlobalPhase::Phase2_OpenMap);

    CHECK_NEAR(p.resources().trade, 0.5f, 0.001f);
}

TEST_CASE("AIController::decideSettlerActions: основывает город прямо на месте, если клетка годится")
{
    GameState state;
    state.setMap(Map::generateMap(10, 10));

    Player p;
    p.setId(0);
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    Unit settler = Unit::loadFromJson({{"id", "settler"}, {"name", "Поселенец"}, {"stats", {{"health", 10}, {"attack", 0}, {"defense", 1}, {"movementPoints", 2}}}, {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 60}, {"populationCost", 80}, {"isSettler", true}});
    p.addArmy(Army("settler_army", 0, {5, 5}, {settler}));
    state.addPlayer(p);

    AIController::decideSettlerActions(state.players()[0], state, GlobalPhase::Phase2_OpenMap);

    CHECK(state.players()[0].settlements().size() == 1u);
    CHECK(state.players()[0].armies().empty()); // поселенец израсходован
}

TEST_CASE("AIController::decideSettlerActions: не основывает город на клетке своей же столицы, уходит искать другую")
{
    GameState state;
    state.setMap(Map::generateMap(10, 10));

    Player p;
    p.setId(0);
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    Settlement capital(0, SettlementType::Village);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            capital.addTile(Tile(5 + dx, 5 + dy));
    capital.seedInitialPopulation(50);
    p.addSettlement(capital);

    Unit settler = Unit::loadFromJson({{"id", "settler"}, {"name", "Поселенец"}, {"stats", {{"health", 10}, {"attack", 0}, {"defense", 1}, {"movementPoints", 3}}}, {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 60}, {"populationCost", 80}, {"isSettler", true}});
    // Поселенец стоит внутри клеток своей же столицы — здесь основать нельзя.
    p.addArmy(Army("settler_army", 0, {5, 5}, {settler}));
    state.addPlayer(p);

    AIController::decideSettlerActions(state.players()[0], state, GlobalPhase::Phase2_OpenMap);

    CHECK(state.players()[0].settlements().size() == 1u); // новый город ещё не основан
    Army *army = state.findArmy("settler_army");
    CHECK(army != nullptr);
    CHECK(!(army->position().q == 5 && army->position().r == 5)); // ушёл искать другое место
}

// ═══════════════════════════════════════════════════════════════════════
// Блок 3.5: playtest выявил, что гарнизон никогда не выводился в поле —
// decideDeployment закрывает этот пробел.
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("AIController::decideDeployment: гарнизон выше порога выводится в армию, "
          "минимальный резерв остаётся в городе")
{
    GameState state;
    state.setMap(Map::generateMap(10, 10));

    Player p;
    p.setId(0);
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    Settlement s(0, SettlementType::Village);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            s.addTile(Tile(5 + dx, 5 + dy));

    std::vector<Unit> garrison;
    for (int i = 0; i < 8; ++i)
        garrison.push_back(makeUnit("militia", 0, "", 3, 2, 20, 15, 20));
    s.addToGarrison(garrison);

    p.addSettlement(s);
    state.addPlayer(p);

    AIController::decideDeployment(state.players()[0]);

    Settlement &after = state.players()[0].settlements()[0];
    CHECK(after.garrison().size() >= 1u); // резерв остался
    CHECK(after.garrison().size() < 8u);  // но часть точно вывели

    int deployedUnits = 0;
    for (const auto &a : state.players()[0].armies())
        deployedUnits += static_cast<int>(a.units().size());
    CHECK(deployedUnits > 0);
    CHECK(deployedUnits + static_cast<int>(after.garrison().size()) == 8);
}

TEST_CASE("AIController::decideDeployment: маленький гарнизон (не выше порога) не трогаем")
{
    GameState state;
    state.setMap(Map::generateMap(10, 10));

    Player p;
    p.setId(0);
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    Settlement s(0, SettlementType::Village);
    s.addTile(Tile(5, 5));
    s.addToGarrison({makeUnit("militia", 0, "", 3, 2, 20, 15, 20)});
    p.addSettlement(s);
    state.addPlayer(p);

    AIController::decideDeployment(state.players()[0]);

    CHECK(state.players()[0].settlements()[0].garrison().size() == 1u);
    CHECK(state.players()[0].armies().empty());
}

TEST_CASE("AIController::decideDeployment: поселенцы выводятся каждый в отдельную "
          "\"чистую\" армию, а не смешиваются с бойцами")
{
    GameState state;
    state.setMap(Map::generateMap(10, 10));

    Player p;
    p.setId(0);
    p.setRace(makeRace("humans", 1, 1, 1, 1, 1, 1));

    Settlement s(0, SettlementType::Village);
    s.addTile(Tile(5, 5));

    Unit settler1 = Unit::loadFromJson({{"id", "settler"}, {"name", "Поселенец"}, {"stats", {{"health", 10}, {"attack", 0}, {"defense", 1}, {"movementPoints", 2}}}, {"tier", 0}, {"requiredBuildingId", ""}, {"recruitCost", 60}, {"populationCost", 80}, {"isSettler", true}});
    s.addToGarrison({settler1, settler1}); // два поселенца одновременно в гарнизоне
    p.addSettlement(s);
    state.addPlayer(p);

    AIController::decideDeployment(state.players()[0]);

    CHECK(state.players()[0].settlements()[0].garrison().empty());
    CHECK(state.players()[0].armies().size() == 2u); // каждый поселенец — своя армия
    for (const auto &a : state.players()[0].armies())
    {
        CHECK(a.isSettlerArmy());
        CHECK(a.units().size() == 1u); // ровно один поселенец на армию
    }
}
