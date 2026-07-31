#include "core/GameEngine.hpp"
#include "core/Hero.hpp"
#include "core/Race.hpp"
#include "core/MapGenerator.hpp"
#include "core/StartLayout.hpp"
#include "external/json.hpp"

#include <fstream>

static json loadFile(const std::string &path)
{
    std::ifstream file(path);
    json data;
    if (file.is_open())
        file >> data;
    return data;
}

static Player createPlayer(int id, bool isHuman, const Race &race,
                           const std::vector<Hero> &allHeroes,
                           const std::vector<Technology> &techCatalog,
                           const Unit &startingUnitPrototype,
                           Coords capitalCenter, int mapWidth, int mapHeight)
{
    Player p;
    p.setId(id);
    p.setIsHuman(isHuman);
    p.setRace(race);
    p.loadTechnologies(techCatalog);

    for (const auto &h : allHeroes)
        if (h.getRaceId() == race.getId())
            p.addHero(h);

    Settlement capital(id, SettlementType::Village);
    capital.seedInitialPopulation(80);
    capital.assignFromIdle(Sphere::Science, 15);
    capital.assignFromIdle(Sphere::Industry, 25);
    capital.assignFromIdle(Sphere::Trade, 10);
    capital.assignFromIdle(Sphere::Diplomacy, 5);

    Unit garrisonUnit = startingUnitPrototype;
    garrisonUnit.setTrained(true);
    capital.addToGarrison({garrisonUnit});

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            int q = capitalCenter.q + dx, r = capitalCenter.r + dy;
            if (q >= 0 && q < mapWidth && r >= 0 && r < mapHeight)
                capital.addTile(Tile(q, r));
        }
    }

    p.addSettlement(capital);

    for (const auto &t : techCatalog)
    {
        if (t.getPrerequisiteIds().empty())
        {
            p.setCurrentResearch(t.getId());
            break;
        }
    }

    return p;
}

bool GameEngine::loadFromAssets(const std::string &assetsDir)
{
    json racesJson = loadFile(assetsDir + "/races.json");
    json buildingsJson = loadFile(assetsDir + "/buildings.json");
    json techJson = loadFile(assetsDir + "/technologies.json");
    json heroesJson = loadFile(assetsDir + "/heroes.json");
    json unitsJson = loadFile(assetsDir + "/units.json");
    json politicsJson = loadFile(assetsDir + "/political_actions.json");

    if (racesJson.is_null() || buildingsJson.is_null() ||
        techJson.is_null() || heroesJson.is_null() || unitsJson.is_null() ||
        politicsJson.is_null())
        return false;

    m_buildingCatalog = Building::loadAllFromJson(buildingsJson["buildings"]);
    m_techCatalog = Technology::loadAllFromJson(techJson["technologies"]);
    m_unitCatalog = Unit::loadAllFromJson(unitsJson["units"]);
    m_politicalActionCatalog = PoliticalAction::loadAllFromJson(politicsJson["politicalActions"]);

    std::vector<Race> allRaces;
    for (const auto &r : racesJson["races"])
        allRaces.push_back(Race::loadFromJson(r));

    auto raceById = [&allRaces](const std::string &id) -> const Race &
    {
        for (const auto &r : allRaces)
            if (r.getId() == id)
                return r;
        return allRaces.front(); // не должно случиться при исправных данных
    };

    auto allHeroes = Hero::loadAllFromJson(heroesJson["heroes"]);

    Unit startingUnit = m_unitCatalog.front();
    for (const auto &u : m_unitCatalog)
    {
        if (u.getRequiredBuildingId().empty())
        {
            startingUnit = u;
            break;
        }
    }

    MapGenerationParams mapParams;
    mapParams.width = StartLayout::MAP_WIDTH;
    mapParams.height = StartLayout::MAP_HEIGHT;
    mapParams.guaranteedLandCenters = StartLayout::pentagonCapitals();
    mapParams.guaranteedLandRadius = StartLayout::CAPITAL_LAND_RADIUS;
    Map gameMap = MapGenerator::generate(mapParams);

    const std::vector<std::string> botRaceIds = {"dwarves", "gnomes", "elves", "orcs"};
    const auto &capitals = StartLayout::pentagonCapitals();

    Player human = createPlayer(0, true, raceById("humans"), allHeroes,
                                m_techCatalog, startingUnit, capitals[0],
                                StartLayout::MAP_WIDTH, StartLayout::MAP_HEIGHT);
    m_state.addPlayer(human);

    for (size_t i = 0; i < botRaceIds.size(); ++i)
    {
        Player bot = createPlayer(static_cast<int>(i) + 1, false,
                                  raceById(botRaceIds[i]), allHeroes,
                                  m_techCatalog, startingUnit, capitals[i + 1],
                                  StartLayout::MAP_WIDTH, StartLayout::MAP_HEIGHT);
        m_state.addPlayer(bot);
    }

    m_state.setMap(gameMap);

    return true;
}

std::string GameEngine::applyPoliticalAction(int playerId, const std::string &actionId, int targetId)
{
    Player *player = nullptr, *target = nullptr;
    for (auto &p : m_state.players())
    {
        if (p.id() == playerId)
            player = &p;
        if (p.id() == targetId)
            target = &p;
    }
    if (!player)
        return "Игрок не найден";

    for (const auto &a : m_politicalActionCatalog)
    {
        if (a.getId() != actionId)
            continue;

        if (!player->trySpendOnPoliticalAction(a))
            return "Недостаточно дипломатии";

        switch (a.getType())
        {
        case PoliticalActionType::Alliance:
            if (!target)
                return "Цель не найдена";
            m_state.newAlliance(*player, *target, m_state.turnNumber() + 5);
            return "Союз заключён";

        case PoliticalActionType::DemandTribute:
            if (!target)
                return "Цель не найдена";
            m_state.applyDemandTribute(playerId, targetId);
            return "Дань потребована";

        case PoliticalActionType::Vassalize:
            if (!target)
                return "Цель не найдена";
            m_state.applyVassalize(playerId, targetId);
            return "Вассализация проведена";

        case PoliticalActionType::PoliticalConquest:
            if (!target)
                return "Цель не найдена";
            m_state.applyPoliticalConquest(playerId, targetId);
            if (m_state.checkVictory().hasWinner())
                return "ПОЛИТИЧЕСКАЯ ПОБЕДА!";
            return "Политическое поглощение проведено";
        }
    }
    return "Действие не найдено";
}

void GameEngine::endTurn()
{
    for (auto &player : m_state.players())
    {
        if (!player.isHuman())
        {
            AIController::takeTurn(player, m_state, m_buildingCatalog,
                                   m_techCatalog, m_unitCatalog,
                                   m_politicalActionCatalog);
        }
        player.advanceTurn();
    }

    m_state.processVassalage();

    for (auto &player : m_state.players())
    {
        for (auto &settlement : player.settlements())
        {
            settlement.growPopulation(DEFAULT_BASE_GROWTH_RATE,
                                      player.race().getModifiers().growth);
            player.spendIndustryOnQueues(settlement);
        }
    }
    m_state.checkAlliances();

    for (auto &player : m_state.players())
        for (auto &army : player.armies())
            army.resetMovement();

    m_state.nextTurn();
}
