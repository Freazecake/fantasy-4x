#include "core/GameEngine.hpp"
#include "core/Hero.hpp"
#include "core/Race.hpp"
#include "core/MapGenerator.hpp"
#include "core/StartLayout.hpp"
#include "external/json.hpp"

#include <fstream>
#include <string>
#include <algorithm>

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

bool GameEngine::loadFromAssets(const std::string &assetsDir, const GameStartOptions &options,
                                unsigned int mapSeed)
{
    m_assetsDir = assetsDir;

    json racesJson = loadFile(assetsDir + "/races.json");
    json buildingsJson = loadFile(assetsDir + "/buildings.json");
    json techJson = loadFile(assetsDir + "/technologies.json");
    json heroesJson = loadFile(assetsDir + "/heroes.json");
    json unitsJson = loadFile(assetsDir + "/units.json");
    json politicsJson = loadFile(assetsDir + "/political_actions.json");
    json tilesJson = loadFile(assetsDir + "/tiles.json");

    if (racesJson.is_null() || buildingsJson.is_null() ||
        techJson.is_null() || heroesJson.is_null() || unitsJson.is_null() ||
        politicsJson.is_null())
        return false;

    m_buildingCatalog = Building::loadAllFromJson(buildingsJson["buildings"]);
    m_techCatalog = Technology::loadAllFromJson(techJson["technologies"]);
    m_unitCatalog = Unit::loadAllFromJson(unitsJson["units"]);
    m_politicalActionCatalog = PoliticalAction::loadAllFromJson(politicsJson["politicalActions"]);
    float resourceDensity = 0.0f;
    if (!tilesJson.is_null())
    {
        m_resourceCatalog = ResourceCatalog::loadAllFromJson(tilesJson["resources"]);
        resourceDensity = tilesJson.value("resourceDensity", 0.0f);
    }

    std::vector<Race> allRaces;
    for (const auto &r : racesJson["races"])
        allRaces.push_back(Race::loadFromJson(r));

    auto raceById = [&allRaces](const std::string &id) -> const Race &
    {
        for (const auto &r : allRaces)
            if (r.getId() == id)
                return r;
        return allRaces.front();
    };

    auto allHeroes = Hero::loadAllFromJson(heroesJson["heroes"]);
    m_raceCatalog = allRaces;
    m_heroCatalog = allHeroes;

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
    mapParams.width = options.mapWidth;
    mapParams.height = options.mapHeight;
    int numPlayers = std::clamp(options.numBots, 1, 4) + 1;
    auto capitals = StartLayout::capitalsForPlayerCount(numPlayers, options.mapWidth, options.mapHeight);
    mapParams.guaranteedLandCenters = capitals;
    mapParams.guaranteedLandRadius = StartLayout::CAPITAL_LAND_RADIUS;
    mapParams.resourceCatalog = m_resourceCatalog;
    mapParams.resourceDensity = resourceDensity;
    mapParams.seed = mapSeed;
    Map gameMap = MapGenerator::generate(mapParams);

    std::vector<std::string> botRaceIds;
    for (const auto &r : allRaces)
        if (r.getId() != options.humanRaceId)
            botRaceIds.push_back(r.getId());
    int numBots = std::min(static_cast<int>(botRaceIds.size()), std::clamp(options.numBots, 1, 4));

    Player human = createPlayer(0, true, raceById(options.humanRaceId), allHeroes,
                                m_techCatalog, startingUnit, capitals[0],
                                options.mapWidth, options.mapHeight);
    m_state.addPlayer(human);

    for (int i = 0; i < numBots; ++i)
    {
        Player bot = createPlayer(i + 1, false,
                                  raceById(botRaceIds[i]), allHeroes,
                                  m_techCatalog, startingUnit, capitals[i + 1],
                                  options.mapWidth, options.mapHeight);
        m_state.addPlayer(bot);
    }

    m_state.setMap(gameMap);
    m_state.recomputeVisibility();

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

        if (a.getType() == PoliticalActionType::PoliticalConquest)
        {
            if (!target)
                return "Цель не найдена";
            if (!m_state.canPoliticalConquer(playerId, targetId))
                return "Цель слишком сильна для политического поглощения";
            if (!player->trySpendDiplomacy(m_state.politicalConquestCost(playerId, targetId)))
                return "Недостаточно дипломатии";
            if (!m_state.applyPoliticalConquest(playerId, targetId))
                return "Политическое поглощение не удалось";
            if (m_state.checkVictory().hasWinner())
                return "ПОЛИТИЧЕСКАЯ ПОБЕДА!";
            return "Политическое поглощение проведено";
        }

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
            return "Политическое поглощение не удалось";
        }
    }
    return "Действие не найдено";
}

void GameEngine::endTurn()
{
    m_state.clearTurnEvents();

    for (auto &player : m_state.players())
    {
        for (auto &settlement : player.settlements())
        {
            int foodBonus = 0;
            for (const auto &tile : settlement.tiles())
            {
                if (tile.getResource() == ResourceType::None)
                    continue;
                const ResourceDef *def = ResourceCatalog::find(m_resourceCatalog, tile.getResource());
                if (def && def->sphere == "growth")
                    foodBonus += static_cast<int>(def->bonus);
            }

            settlement.growPopulation(DEFAULT_BASE_GROWTH_RATE,
                                      player.race().getModifiers().growth, foodBonus);
        }
    }

    for (auto &player : m_state.players())
    {
        if (!player.isHuman())
        {
            AIController::takeTurn(player, m_state, m_buildingCatalog,
                                   m_techCatalog, m_unitCatalog,
                                   m_politicalActionCatalog);
        }
        player.advanceTurn(m_resourceCatalog);

        for (auto &settlement : player.settlements())
            player.spendIndustryOnQueues(settlement);
    }

    m_state.processVassalage();
    m_state.checkAlliances();

    for (auto &player : m_state.players())
        for (auto &army : player.armies())
            army.resetMovement();

    m_state.updatePhase();
    m_state.nextTurn();
    m_state.recomputeVisibility();

    VictoryResult v = m_state.checkVictory();
    if (v.hasWinner())
    {
        m_state.pushEvent(TurnEventType::Victory, v.winnerId, -1,
                          "Игрок " + std::to_string(v.winnerId) + " одержал победу (" +
                              v.type + ")");
    }
}

bool GameEngine::saveGame(const std::string &path) const
{
    json j = m_state.toJson();
    std::ofstream file(path);
    if (!file.is_open())
        return false;
    file << j.dump(2);
    return file.good();
}

bool GameEngine::loadGame(const std::string &path, const std::string &assetsDir)
{
    if (!loadFromAssets(assetsDir))
        return false;

    json j = loadFile(path);
    if (j.is_null())
        return false;

    m_state = GameState::fromJson(j, m_raceCatalog, m_heroCatalog, m_techCatalog,
                                  m_buildingCatalog, m_unitCatalog);
    return true;
}
