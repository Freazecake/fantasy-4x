#include "core/GameEngine.hpp"
#include "core/Hero.hpp"
#include "core/Race.hpp"
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

    Race humanRace;
    for (const auto &r : racesJson["races"])
    {
        Race race = Race::loadFromJson(r);
        if (race.getId() == "humans")
            humanRace = race;
    }

    auto allHeroes = Hero::loadAllFromJson(heroesJson["heroes"]);

    Player human;
    human.setId(0);
    human.setRace(humanRace);
    human.loadTechnologies(m_techCatalog);

    for (const auto &h : allHeroes)
    {
        if (h.getRaceId() == "humans")
            human.addHero(h);
    }

    Settlement capital(0, SettlementType::Village);
    capital.seedInitialPopulation(80);
    capital.assignFromIdle(Sphere::Science, 15);
    capital.assignFromIdle(Sphere::Industry, 25);
    capital.assignFromIdle(Sphere::Trade, 10);
    capital.assignFromIdle(Sphere::Diplomacy, 5);

    Map gameMap = Map::generateMap(16, 12);

    int cx = 7, cy = 5;
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            int q = cx + dx, r = cy + dy;
            if (q >= 0 && q < 16 && r >= 0 && r < 12)
                capital.addTile(Tile(q, r));
        }
    }

    human.addSettlement(capital);

    for (auto &t : m_techCatalog)
    {
        if (t.getPrerequisiteIds().empty())
        {
            human.setCurrentResearch(t.getId());
            break;
        }
    }

    m_state.addPlayer(human);
    m_state.setMap(gameMap);

    return true;
}

void GameEngine::endTurn()
{
    for (auto &player : m_state.players())
    {
        player.advanceTurn();
        for (auto &settlement : player.settlements())
        {
            settlement.growPopulation(DEFAULT_BASE_GROWTH_RATE,
                                      player.race().getModifiers().growth);
            player.spendIndustryOnQueues(settlement);
        }
    }
    m_state.checkAlliances();
    m_state.nextTurn();
}
