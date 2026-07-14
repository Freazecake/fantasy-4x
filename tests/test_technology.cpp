#include "test_framework.hpp"

#include "core/Technology.hpp"

#include <unordered_set>

namespace {

json makeTechJson(const std::string& id, const std::string& category, int scienceCost,
                   std::vector<std::string> prereqs, std::vector<std::string> unlocksBuildings,
                   std::vector<std::string> bonusCategories, float globalBonusMultiplier) {
    return json{
        {"id", id},
        {"name", id + "_name"},
        {"category", category},
        {"scienceCost", scienceCost},
        {"prerequisiteIds", prereqs},
        {"unlocksBuildingIds", unlocksBuildings},
        {"bonusCategories", bonusCategories},
        {"globalBonusMultiplier", globalBonusMultiplier},
    };
}

}  // namespace

TEST_CASE("Technology::loadFromJson читает id, категорию, стоимость и зависимости") {
    json obj =
        makeTechJson("tech_science_2", "science", 100, {"tech_science_1"}, {}, {"science"}, 1.1f);

    Technology t = Technology::loadFromJson(obj);

    CHECK(t.getId() == "tech_science_2");
    CHECK(t.getCategory() == "science");
    CHECK(t.getCost() == 100);
    CHECK(t.getPrerequisiteIds().size() == 1u);
    CHECK(t.getPrerequisiteIds()[0] == "tech_science_1");
}

TEST_CASE("Technology::loadFromJson без пререквизитов — доступна для исследования сразу") {
    json obj = makeTechJson("tech_science_1", "science", 50, {}, {"observatory"}, {"science"}, 1.0f);
    Technology t = Technology::loadFromJson(obj);

    std::unordered_set<std::string> researched;  // пусто

    CHECK(t.canResearch(researched));
}

TEST_CASE("Technology::canResearch: false, пока не все пререквизиты изучены") {
    json obj = makeTechJson("tech_synergy_1", "science", 80, {"tech_science_1", "tech_trade_1"},
                             {}, {"science", "trade"}, 1.1f);
    Technology t = Technology::loadFromJson(obj);

    std::unordered_set<std::string> researched = {"tech_science_1"};  // trade ещё нет

    CHECK(!t.canResearch(researched));
}

TEST_CASE("Technology::canResearch: true, когда изучены все пререквизиты (в т.ч. синергия из двух веток)") {
    json obj = makeTechJson("tech_synergy_1", "science", 80, {"tech_science_1", "tech_trade_1"},
                             {}, {"science", "trade"}, 1.1f);
    Technology t = Technology::loadFromJson(obj);

    std::unordered_set<std::string> researched = {"tech_science_1", "tech_trade_1"};

    CHECK(t.canResearch(researched));
}

TEST_CASE("Technology::loadFromJson: globalBonusMultiplier применяется ко всем "
          "категориям из bonusCategories (синергетическая технология)") {
    json obj = makeTechJson("tech_synergy_2", "industry", 80, {"tech_industry_1", "tech_diplomacy_1"},
                             {}, {"industry", "diplomacy"}, 1.1f);

    Technology t = Technology::loadFromJson(obj);
    Modifiers m = t.getModifiers();

    CHECK_NEAR(m.industry, 1.1f, 0.001f);
    CHECK_NEAR(m.diplomacy, 1.1f, 0.001f);
    // Науку/торговлю эта технология не бонусит.
    CHECK_NEAR(m.science, 1.0f, 0.001f);
    CHECK_NEAR(m.trade, 1.0f, 0.001f);
}

TEST_CASE("Technology::invest: очков меньше стоимости — накапливается, не исследована") {
    json obj = makeTechJson("tech_science_1", "science", 50, {}, {}, {"science"}, 1.0f);
    Technology t = Technology::loadFromJson(obj);

    int leftover = t.invest(30);

    CHECK(leftover == 0);
    CHECK(!t.isResearched());
    CHECK(t.remainingCost() == 20);
}

TEST_CASE("Technology::invest: очков ровно достаточно — технология исследована, остаток 0") {
    json obj = makeTechJson("tech_science_1", "science", 50, {}, {}, {"science"}, 1.0f);
    Technology t = Technology::loadFromJson(obj);

    int leftover = t.invest(50);

    CHECK(leftover == 0);
    CHECK(t.isResearched());
    CHECK(t.remainingCost() == 0);
}

TEST_CASE("Technology::invest: очков больше стоимости — возвращает остаток, технология исследована") {
    json obj = makeTechJson("tech_science_1", "science", 50, {}, {}, {"science"}, 1.0f);
    Technology t = Technology::loadFromJson(obj);

    int leftover = t.invest(70);

    CHECK(leftover == 20);
    CHECK(t.isResearched());
}

TEST_CASE("Technology::invest на уже исследованную технологию просто возвращает все очки обратно") {
    json obj = makeTechJson("tech_science_1", "science", 50, {}, {}, {"science"}, 1.0f);
    Technology t = Technology::loadFromJson(obj);
    t.invest(50);  // исследовали

    int leftover = t.invest(40);  // повторное вложение

    CHECK(leftover == 40);
}

TEST_CASE("Technology::progress отражает долю вложенных очков от стоимости") {
    json obj = makeTechJson("tech_science_2", "science", 100, {}, {}, {"science"}, 1.0f);
    Technology t = Technology::loadFromJson(obj);

    t.invest(25);

    CHECK_NEAR(t.progress(), 0.25f, 0.001f);
}

// См. аналогичный комментарий в test_race.cpp/test_building.cpp — тот же баг
// присутствует и в Technology(const json&).
TEST_CASE("Technology: конструктор из JSON (баг: не присваивает результат loadFromJson)") {
    json obj = makeTechJson("tech_science_1", "science", 50, {}, {"observatory"}, {"science"}, 1.0f);

    Technology t(obj);

    CHECK(t.getId() == "tech_science_1");
}
