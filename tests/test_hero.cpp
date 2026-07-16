#include "test_framework.hpp"

#include "core/Hero.hpp"

namespace {

json makeHeroJson(const std::string &id, const std::string &raceId, Sphere specialty, float bonus) {
    json obj = {
        {"id", id},
        {"name", id + "_name"},
        {"raceId", raceId},
        {"modifiers",
         {{"science", 1.08}, {"industry", 1.08}, {"trade", 1.08}, {"diplomacy", 1.08},
          {"military", 1.08}, {"growth", 1.0}}},
    };
    obj["modifiers"][sphereToString(specialty)] = bonus;
    return obj;
}

}  // namespace

TEST_CASE("Hero::loadFromJson читает raceId") {
    Hero h = Hero::loadFromJson(makeHeroJson("hero_x", "dwarves", Sphere::Industry, 1.3f));

    CHECK(h.getRaceId() == "dwarves");
}

TEST_CASE("Hero::loadFromJson: sphere вычисляется как сфера с максимальным модификатором") {
    Hero h = Hero::loadFromJson(makeHeroJson("hero_x", "orcs", Sphere::Military, 1.3f));

    CHECK(h.getSphere() == Sphere::Military);
}

TEST_CASE("Hero: конструктор из JSON присваивает результат loadFromJson (регресс теста бага из недели 2)") {
    Hero h(makeHeroJson("hero_x", "gnomes", Sphere::Science, 1.3f));

    CHECK(h.getId() == "hero_x");
    CHECK(h.getRaceId() == "gnomes");
}

TEST_CASE("Hero::loadAllFromJson загружает весь массив героев") {
    json arr = json::array({
        makeHeroJson("hero_a", "humans", Sphere::Diplomacy, 1.3f),
        makeHeroJson("hero_b", "elves", Sphere::Trade, 1.3f),
    });

    std::vector<Hero> heroes = Hero::loadAllFromJson(arr);

    CHECK(heroes.size() == 2u);
    CHECK(heroes[0].getRaceId() == "humans");
    CHECK(heroes[1].getRaceId() == "elves");
}
