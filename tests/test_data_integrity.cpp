// Неделя 2, день 7 роадмапа:
// "Консольный тест-раннер: загрузить все JSON, вывести в лог — убедиться,
// что нет ошибок парсинга и что граф зависимостей техов не содержит циклов."
//
// В отличие от остальных test_*.cpp (которые тестируют классы на
// синтетических json-объектах, собранных прямо в коде), этот файл грузит
// НАСТОЯЩИЕ файлы из assets/ — races.json, buildings.json,
// technologies.json, units.json, heroes.json, political_actions.json — и
// проверяет:
//   1) что все они парсятся без исключений;
//   2) что все ссылки между файлами (unlocksBuildingIds -> buildings,
//      unlocksUnitIds -> units, requiredBuildingId -> buildings,
//      prerequisiteIds -> technologies, heroes[].raceId -> races) указывают
//      на существующие id;
//   3) что граф зависимостей технологий (prerequisiteIds) не содержит циклов.
//
// Путь к assets/ передаётся через макрос ASSETS_DIR (см. tests/CMakeLists.txt),
// чтобы тесты не зависели от текущей рабочей директории при запуске.

#include "test_framework.hpp"

#include "core/Building.hpp"
#include "core/Race.hpp"
#include "core/Technology.hpp"
#include "core/Hero.hpp"
#include "core/PoliticalAction.hpp"

#include <fstream>
#include <unordered_map>
#include <unordered_set>

#ifndef ASSETS_DIR
#define ASSETS_DIR "../assets"
#endif

namespace
{

    json loadJsonFile(const std::string &relativePath)
    {
        std::ifstream file(std::string(ASSETS_DIR) + "/" + relativePath);
        if (!file.is_open())
        {
            throw std::runtime_error("Не удалось открыть файл assets/" + relativePath +
                                     " (ожидался путь: " + std::string(ASSETS_DIR) + "/" +
                                     relativePath + ")");
        }
        json assets;
        file >> assets; // бросит nlohmann::json::parse_error при синтаксической ошибке
        return assets;
    }

    // DFS-детектор циклов с тремя цветами (белый/серый/чёрный) —
    // стандартный алгоритм для поиска цикла в ориентированном графе.
    bool hasCycleDFS(const std::string &node,
                     const std::unordered_map<std::string, std::vector<std::string>> &graph,
                     std::unordered_set<std::string> &visiting,
                     std::unordered_set<std::string> &visited)
    {
        if (visited.count(node))
            return false;
        if (visiting.count(node))
            return true; // встретили узел, который сейчас в стеке -> цикл

        visiting.insert(node);
        auto it = graph.find(node);
        if (it != graph.end())
        {
            for (const auto &dep : it->second)
            {
                if (hasCycleDFS(dep, graph, visiting, visited))
                    return true;
            }
        }
        visiting.erase(node);
        visited.insert(node);
        return false;
    }

} // namespace

TEST_CASE("assets/races.json парсится и содержит ровно 5 рас с уникальными id")
{
    json assets = loadJsonFile("races.json");
    const auto &races = assets.at("races");

    CHECK(races.size() == 5u);

    std::unordered_set<std::string> ids;
    for (const auto &r : races)
        ids.insert(r.at("id").get<std::string>());
    CHECK(ids.size() == races.size()); // все id уникальны
}

TEST_CASE("assets/races.json: каждая раса грузится в Race без исключений и с непустым именем")
{
    json assets = loadJsonFile("races.json");

    for (const auto &r : assets.at("races"))
    {
        Race race = Race::loadFromJson(r);
        CHECK(!race.getName().empty());
    }
}

TEST_CASE("assets/buildings.json парсится и у всех построек уникальные id")
{
    json assets = loadJsonFile("buildings.json");
    const auto &buildings = assets.at("buildings");

    std::unordered_set<std::string> ids;
    for (const auto &b : buildings)
        ids.insert(b.at("id").get<std::string>());
    CHECK(ids.size() == buildings.size());
}

TEST_CASE("assets/technologies.json парсится и у всех технологий уникальные id")
{
    json assets = loadJsonFile("technologies.json");
    const auto &techs = assets.at("technologies");

    std::unordered_set<std::string> ids;
    for (const auto &t : techs)
        ids.insert(t.at("id").get<std::string>());
    CHECK(ids.size() == techs.size());
}

TEST_CASE("assets/units.json парсится и у всех юнитов уникальные id")
{
    json assets = loadJsonFile("units.json");
    const auto &units = assets.at("units");

    std::unordered_set<std::string> ids;
    for (const auto &u : units)
        ids.insert(u.at("id").get<std::string>());
    CHECK(ids.size() == units.size());
}

TEST_CASE("Technology.prerequisiteIds: все ссылки указывают на существующие id технологий")
{
    json assets = loadJsonFile("technologies.json");
    const auto &techs = assets.at("technologies");

    std::unordered_set<std::string> allIds;
    for (const auto &t : techs)
        allIds.insert(t.at("id").get<std::string>());

    for (const auto &t : techs)
    {
        for (const auto &prereq : t.at("prerequisiteIds"))
        {
            std::string prereqId = prereq.get<std::string>();
            if (allIds.count(prereqId) == 0)
            {
                testfw::reportFailure("technologies.json: '" + t.at("id").get<std::string>() +
                                          "' ссылается на несуществующую prerequisiteId '" +
                                          prereqId + "'",
                                      __FILE__, __LINE__);
            }
        }
    }
}

TEST_CASE("Дерево технологий (prerequisiteIds) не содержит циклов")
{
    json assets = loadJsonFile("technologies.json");
    const auto &techs = assets.at("technologies");

    std::unordered_map<std::string, std::vector<std::string>> graph;
    for (const auto &t : techs)
    {
        std::vector<std::string> prereqs;
        for (const auto &p : t.at("prerequisiteIds"))
            prereqs.push_back(p.get<std::string>());
        graph[t.at("id").get<std::string>()] = std::move(prereqs);
    }

    std::unordered_set<std::string> visiting, visited;
    for (const auto &[id, _] : graph)
    {
        CHECK(!hasCycleDFS(id, graph, visiting, visited));
    }
}

TEST_CASE("Technology.unlocksBuildingIds: все ссылки указывают на существующие id построек")
{
    json techassets = loadJsonFile("technologies.json");
    json buildingassets = loadJsonFile("buildings.json");

    std::unordered_set<std::string> buildingIds;
    for (const auto &b : buildingassets.at("buildings"))
        buildingIds.insert(b.at("id").get<std::string>());

    for (const auto &t : techassets.at("technologies"))
    {
        for (const auto &unlocked : t.at("unlocksBuildingIds"))
        {
            std::string buildingId = unlocked.get<std::string>();
            if (buildingIds.count(buildingId) == 0)
            {
                testfw::reportFailure("technologies.json: '" + t.at("id").get<std::string>() +
                                          "' открывает несуществующее здание '" + buildingId +
                                          "'",
                                      __FILE__, __LINE__);
            }
        }
    }
}

// Раньше этот тест падал: buildings.json -> "barracks" -> unlocksUnitIds
// содержал несуществующий id "unit_veteran_warrior". Баг исправлен (теперь
// там ["swordsman", "archer"]) — тест оставлен как регресс-проверка.
TEST_CASE("Building.unlocksUnitIds: все ссылки указывают на существующие id юнитов")
{
    json buildingassets = loadJsonFile("buildings.json");
    json unitassets = loadJsonFile("units.json");

    std::unordered_set<std::string> unitIds;
    for (const auto &u : unitassets.at("units"))
        unitIds.insert(u.at("id").get<std::string>());

    for (const auto &b : buildingassets.at("buildings"))
    {
        for (const auto &unlocked : b.at("unlocksUnitIds"))
        {
            std::string unitId = unlocked.get<std::string>();
            if (unitIds.count(unitId) == 0)
            {
                testfw::reportFailure("buildings.json: '" + b.at("id").get<std::string>() +
                                          "' открывает несуществующий юнит '" + unitId + "'",
                                      __FILE__, __LINE__);
            }
        }
    }
}

TEST_CASE("Unit.requiredBuildingId: непустые ссылки указывают на существующие id построек")
{
    json unitassets = loadJsonFile("units.json");
    json buildingassets = loadJsonFile("buildings.json");

    std::unordered_set<std::string> buildingIds;
    for (const auto &b : buildingassets.at("buildings"))
        buildingIds.insert(b.at("id").get<std::string>());

    for (const auto &u : unitassets.at("units"))
    {
        std::string requiredBuildingId = u.at("requiredBuildingId").get<std::string>();
        if (requiredBuildingId.empty())
            continue; // tier 0 — без требования, это нормально
        if (buildingIds.count(requiredBuildingId) == 0)
        {
            testfw::reportFailure("units.json: '" + u.at("id").get<std::string>() +
                                      "' требует несуществующее здание '" + requiredBuildingId +
                                      "'",
                                  __FILE__, __LINE__);
        }
    }
}

TEST_CASE("assets/technologies.json: каждая технология грузится в Technology без исключений")
{
    json assets = loadJsonFile("technologies.json");

    for (const auto &t : assets.at("technologies"))
    {
        Technology tech = Technology::loadFromJson(t);
        CHECK(!tech.getId().empty());
        CHECK(tech.getCost() >= 0);
    }
}

TEST_CASE("assets/buildings.json: каждая постройка грузится в Building без исключений")
{
    json assets = loadJsonFile("buildings.json");

    for (const auto &b : assets.at("buildings"))
    {
        Building building = Building::loadFromJson(b);
        CHECK(!building.getId().empty());
        CHECK(building.getCost() >= 0);
    }
}

TEST_CASE("assets/heroes.json парсится, содержит ровно 5 героев с уникальными id")
{
    json assets = loadJsonFile("heroes.json");
    const auto &heroesArr = assets.at("heroes");

    CHECK(heroesArr.size() == 5u);

    std::unordered_set<std::string> ids;
    for (const auto &h : heroesArr)
        ids.insert(h.at("id").get<std::string>());
    CHECK(ids.size() == heroesArr.size());
}

TEST_CASE("assets/heroes.json: raceId у каждого героя указывает на существующую расу, все расы покрыты по одному разу")
{
    json racesAssets = loadJsonFile("races.json");
    json heroesAssets = loadJsonFile("heroes.json");

    std::unordered_set<std::string> raceIds;
    for (const auto &r : racesAssets.at("races"))
        raceIds.insert(r.at("id").get<std::string>());

    std::unordered_set<std::string> heroRaceIds;
    for (const auto &h : heroesAssets.at("heroes"))
    {
        std::string raceId = h.at("raceId").get<std::string>();
        if (raceIds.count(raceId) == 0)
        {
            testfw::reportFailure("heroes.json: '" + h.at("id").get<std::string>() +
                                      "' ссылается на несуществующую расу '" + raceId + "'",
                                  __FILE__, __LINE__);
        }
        heroRaceIds.insert(raceId);
    }

    // "1 герой = 1 раса": пять героев должны покрывать все пять рас без повторов.
    CHECK(heroRaceIds.size() == raceIds.size());
}

TEST_CASE("assets/heroes.json: каждый герой грузится в Hero без исключений")
{
    json assets = loadJsonFile("heroes.json");

    std::vector<Hero> heroes = Hero::loadAllFromJson(assets.at("heroes"));
    CHECK(heroes.size() == assets.at("heroes").size());
}

TEST_CASE("assets/political_actions.json парсится, содержит 4 действия с уникальными id, включая politicalConquest")
{
    json assets = loadJsonFile("political_actions.json");
    const auto &actionsArr = assets.at("politicalActions");

    CHECK(actionsArr.size() == 4u);

    std::unordered_set<std::string> ids;
    bool hasConquest = false;
    for (const auto &a : actionsArr)
    {
        ids.insert(a.at("id").get<std::string>());
        if (a.at("type").get<std::string>() == "politicalConquest")
            hasConquest = true;
    }
    CHECK(ids.size() == actionsArr.size());
    CHECK(hasConquest);  // раздел 2.5 гайда: политическое поглощение — обязательный путь победы
}

TEST_CASE("assets/political_actions.json: каждое действие грузится в PoliticalAction без исключений")
{
    json assets = loadJsonFile("political_actions.json");

    std::vector<PoliticalAction> actions = PoliticalAction::loadAllFromJson(assets.at("politicalActions"));
    CHECK(actions.size() == assets.at("politicalActions").size());
    for (const auto &a : actions)
        CHECK(a.getCost() >= 0);
}
