// Неделя 2, день 7 роадмапа:
// "Консольный тест-раннер: загрузить все JSON, вывести в лог — убедиться,
// что нет ошибок парсинга и что граф зависимостей техов не содержит циклов."
//
// В отличие от остальных test_*.cpp (которые тестируют классы на
// синтетических json-объектах, собранных прямо в коде), этот файл грузит
// НАСТОЯЩИЕ файлы из assets/ — races.json, buildings.json,
// technologies.json, units.json — и проверяет:
//   1) что все они парсятся без исключений;
//   2) что все ссылки между файлами (unlocksBuildingIds -> buildings,
//      unlocksUnitIds -> units, requiredBuildingId -> buildings,
//      prerequisiteIds -> technologies) указывают на существующие id;
//   3) что граф зависимостей технологий (prerequisiteIds) не содержит циклов.
//
// Путь к assets/ передаётся через макрос ASSETS_DIR (см. tests/CMakeLists.txt),
// чтобы тесты не зависели от текущей рабочей директории при запуске.
//
// ВАЖНО: этот файл намеренно НЕ подключает core/Unit.hpp. В проекте сейчас
// структура UnitStats определена дважды — один раз в core/Common.hpp,
// второй раз (полностью независимо) в core/Unit.hpp. Пока Unit.hpp не
// подключался вместе с файлами, которые тянут Common.hpp (Building.hpp,
// Race.hpp, Technology.hpp), это было незаметно. Но стоит подключить их в
// одной единице трансляции — компиляция падает с "redefinition of struct
// UnitStats". Проверки по юнитам в этом файле поэтому читают units.json
// напрямую как JSON (без создания объектов Unit), а round-trip тест
// Unit::loadAllFromJson вынесен в test_unit.cpp, где Building.hpp не
// подключается. Исправление в проекте — одна строка: убрать повторное
// объявление struct UnitStats из Unit.hpp и подключить туда Common.hpp.

#include "test_framework.hpp"

#include "core/Building.hpp"
#include "core/Race.hpp"
#include "core/Technology.hpp"

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

// Этот тест сейчас ПАДАЕТ и указывает на реальное рассогласование данных:
// buildings.json -> "barracks" -> unlocksUnitIds содержит "unit_veteran_warrior",
// но такого id нет ни в одном юните в units.json (там только militia/swordsman/archer).
// Похоже на неактуальный id, оставшийся от более раннего варианта units.json —
// стоит поправить на реальные id, например ["swordsman", "archer"], либо
// завести юнита unit_veteran_warrior, если он планировался отдельно.
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
