#include "test_framework.hpp"

#include "core/Unit.hpp"

#include <fstream>

#ifndef ASSETS_DIR
#define ASSETS_DIR "../data"
#endif

namespace
{

    json makeUnitJson(const std::string &id, float hp, float atk, float def, int tier,
                      const std::string &requiredBuildingId, int recruitCost, int populationCost)
    {
        return json{
            {"id", id},
            {"name", id + "_name"},
            {"stats", {{"health", hp}, {"attack", atk}, {"defense", def}}},
            {"tier", tier},
            {"requiredBuildingId", requiredBuildingId},
            {"recruitCost", recruitCost},
            {"populationCost", populationCost},
        };
    }

} // namespace

TEST_CASE("Unit::loadFromJson читает id, tier и стоимости")
{
    json obj = makeUnitJson("militia", 20.0f, 3.0f, 2.0f, 0, "", 15, 30);

    Unit u = Unit::loadFromJson(obj);

    CHECK(u.getId() == "militia");
    CHECK(u.getTier() == 0);
    CHECK(u.getRecruitCost() == 15);
    CHECK(u.getPopulationCost() == 30);
}

TEST_CASE("Unit::loadFromJson читает боевые характеристики (health/attack/defense)")
{
    json obj = makeUnitJson("swordsman", 35.0f, 7.0f, 5.0f, 1, "barracks", 30, 50);

    Unit u = Unit::loadFromJson(obj);

    CHECK_NEAR(u.getStats().health, 35.0f, 0.001f);
    CHECK_NEAR(u.getStats().attack, 7.0f, 0.001f);
    CHECK_NEAR(u.getStats().defense, 5.0f, 0.001f);
}

TEST_CASE("Unit::requiresBuilding: false для юнита tier 0 с пустым requiredBuildingId")
{
    json obj = makeUnitJson("militia", 20.0f, 3.0f, 2.0f, 0, "", 15, 30);

    Unit u = Unit::loadFromJson(obj);

    CHECK(!u.requiresBuilding());
}

TEST_CASE("Unit::requiresBuilding: true для юнита tier 1+ с указанной казармой")
{
    json obj = makeUnitJson("swordsman", 35.0f, 7.0f, 5.0f, 1, "barracks", 30, 50);

    Unit u = Unit::loadFromJson(obj);

    CHECK(u.requiresBuilding());
    CHECK(u.getRequiredBuildingId() == "barracks");
}

TEST_CASE("Unit::loadAllFromJson загружает весь массив юнитов в исходном порядке")
{
    json arr = json::array({
        makeUnitJson("militia", 20.0f, 3.0f, 2.0f, 0, "", 15, 30),
        makeUnitJson("swordsman", 35.0f, 7.0f, 5.0f, 1, "barracks", 30, 50),
        makeUnitJson("archer", 22.0f, 8.0f, 2.0f, 1, "barracks", 28, 50),
    });

    std::vector<Unit> units = Unit::loadAllFromJson(arr);

    CHECK(units.size() == 3u);
    CHECK(units[0].getId() == "militia");
    CHECK(units[1].getId() == "swordsman");
    CHECK(units[2].getId() == "archer");
}

TEST_CASE("Unit::toString не падает и содержит id юнита")
{
    json obj = makeUnitJson("archer", 22.0f, 8.0f, 2.0f, 1, "barracks", 28, 50);
    Unit u = Unit::loadFromJson(obj);

    std::string s = u.toString();

    CHECK(s.find("archer") != std::string::npos);
}

// Часть проверки дня 7 роадмапа ("загрузить все JSON... убедиться, что нет
// ошибок парсинга") для units.json — специально здесь, а не в
// test_data_integrity.cpp, потому что там core/Unit.hpp не подключается
// (см. комментарий в начале test_data_integrity.cpp про дублирование
// struct UnitStats между Common.hpp и Unit.hpp).
TEST_CASE("assets/units.json: реальный файл грузится в Unit без исключений")
{
    std::ifstream file(std::string(ASSETS_DIR) + "/units.json");
    CHECK(file.is_open());
    if (!file.is_open())
        return;

    json data;
    file >> data;

    std::vector<Unit> units = Unit::loadAllFromJson(data.at("units"));

    CHECK(units.size() == data.at("units").size());
    CHECK(!units.empty());
}
