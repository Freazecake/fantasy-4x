#include "test_framework.hpp"

#include "core/Building.hpp"

namespace {

json makeBuildingJson(const std::string& id, const std::string& category,
                       const std::string& resourceType, float bonusMultiplier, int cost,
                       std::vector<std::string> unlocksUnits, bool removesPenalty) {
    return json{
        {"id", id},
        {"name", id + "_name"},
        {"category", category},
        {"resourceType", resourceType},
        {"bonusMultiplier", bonusMultiplier},
        {"industryCost", cost},
        {"unlocksUnitIds", unlocksUnits},
        {"removesUntrainedPenalty", removesPenalty},
    };
}

}  // namespace

TEST_CASE("Building::loadFromJson читает id, категорию и стоимость") {
    json obj = makeBuildingJson("library", "economic", "science", 1.15f, 40, {}, false);

    Building b = Building::loadFromJson(obj);

    CHECK(b.getId() == "library");
    CHECK(b.getCategory() == "economic");
    CHECK(b.getCost() == 40);
}

TEST_CASE("Building::loadFromJson с resourceType=science пишет bonusMultiplier "
          "именно в Modifiers.science, остальные поля остаются 1.0") {
    json obj = makeBuildingJson("observatory", "economic", "science", 1.3f, 90, {}, false);

    Building b = Building::loadFromJson(obj);
    Modifiers m = b.getModifiers();

    CHECK_NEAR(m.science, 1.3f, 0.001f);
    CHECK_NEAR(m.industry, 1.0f, 0.001f);
    CHECK_NEAR(m.trade, 1.0f, 0.001f);
    CHECK_NEAR(m.diplomacy, 1.0f, 0.001f);
}

TEST_CASE("Building::loadFromJson с resourceType=trade пишет bonusMultiplier в "
          "Modifiers.trade") {
    json obj = makeBuildingJson("market", "economic", "trade", 1.15f, 40, {}, false);

    Building b = Building::loadFromJson(obj);

    CHECK_NEAR(b.getModifiers().trade, 1.15f, 0.001f);
}

TEST_CASE("Building::loadFromJson: военное здание с unlocksUnitIds и removesUntrainedPenalty") {
    json obj =
        makeBuildingJson("barracks", "military", "", 0.0f, 60, {"swordsman", "archer"}, true);

    Building b = Building::loadFromJson(obj);

    CHECK(b.getCategory() == "military");
    CHECK(b.removesUntrainedPenalty());
    CHECK(b.getUnlocksUnitIds().size() == 2u);
    CHECK(b.getUnlocksUnitIds()[0] == "swordsman");
}

TEST_CASE("Building::loadFromJson: здание без юнитов имеет пустой unlocksUnitIds") {
    json obj = makeBuildingJson("library", "economic", "science", 1.15f, 40, {}, false);

    Building b = Building::loadFromJson(obj);

    CHECK(b.getUnlocksUnitIds().empty());
}

TEST_CASE("Building::isUnlocked по умолчанию false, setUnlocked переключает его") {
    json obj = makeBuildingJson("library", "economic", "science", 1.15f, 40, {}, false);
    Building b = Building::loadFromJson(obj);

    CHECK(!b.isUnlocked());
    b.setUnlocked(true);
    CHECK(b.isUnlocked());
}

TEST_CASE("Building::loadAllFromJson загружает весь массив построек") {
    json arr = json::array({
        makeBuildingJson("library", "economic", "science", 1.15f, 40, {}, false),
        makeBuildingJson("workshop", "economic", "industry", 1.15f, 40, {}, false),
    });

    std::vector<Building> buildings = Building::loadAllFromJson(arr);

    CHECK(buildings.size() == 2u);
    CHECK(buildings[0].getId() == "library");
    CHECK(buildings[1].getId() == "workshop");
}

// ВАЖНО: как и у Race (см. test_race.cpp), у Building тоже есть конструктор
// из JSON, который отбрасывает результат Building::loadFromJson(...) вместо
// того, чтобы присвоить его *this. Сейчас этот конструктор нигде не
// используется в проекте (используется только статический loadFromJson),
// но он молча создаёт "пустое" здание, если кто-то его вызовет напрямую.
TEST_CASE("Building: конструктор из JSON (баг: не присваивает результат loadFromJson)") {
    json obj = makeBuildingJson("library", "economic", "science", 1.15f, 40, {}, false);

    Building b(obj);

    CHECK(b.getId() == "library");
}
