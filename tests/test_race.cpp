#include "test_framework.hpp"

#include "core/Race.hpp"

namespace {

json makeRaceJson(const std::string& id, const std::string& name,
                   const std::string& startType, float science, float industry,
                   float trade, float diplomacy, float military, float growth) {
    return json{
        {"id", id},
        {"name", name},
        {"startingSettlementType", startType},
        {"modifiers",
         {
             {"science", science},
             {"industry", industry},
             {"trade", trade},
             {"diplomacy", diplomacy},
             {"military", military},
             {"growth", growth},
         }},
    };
}

}  // namespace

TEST_CASE("Race::loadFromJson читает имя и стартовый тип поселения") {
    json obj = makeRaceJson("humans", "Люди", "village", 1.0f, 1.0f, 1.3f, 1.4f, 1.0f, 1.1f);

    Race r = Race::loadFromJson(obj);

    CHECK(r.getName() == "Люди");
}

TEST_CASE("Race::loadFromJson читает все 6 модификаторов из таблицы гайда (Дворфы)") {
    // Значения из раздела 1 основного гайда: Дворфы 90/180/90/40/150/70
    json obj = makeRaceJson("dwarves", "Дворфы", "mountain_fortress", 0.9f, 1.8f, 0.9f,
                             0.4f, 1.5f, 0.7f);

    Race r = Race::loadFromJson(obj);
    Modifiers m = r.getModifiers();

    CHECK_NEAR(m.science, 0.9f, 0.001f);
    CHECK_NEAR(m.industry, 1.8f, 0.001f);
    CHECK_NEAR(m.trade, 0.9f, 0.001f);
    CHECK_NEAR(m.diplomacy, 0.4f, 0.001f);
    CHECK_NEAR(m.military, 1.5f, 0.001f);
    CHECK_NEAR(m.growth, 0.7f, 0.001f);
}

TEST_CASE("Race: люди без явных провалов (все модификаторы >= 1.0)") {
    // Люди задуманы как универсалы без слабых сторон (см. гайд, раздел 1) —
    // если кто-то по ошибке поставит человеку модификатор < 1.0, это ломает
    // задуманный дизайн расы, а не просто баланс.
    json obj = makeRaceJson("humans", "Люди", "village", 1.0f, 1.0f, 1.3f, 1.4f, 1.0f, 1.1f);

    Race r = Race::loadFromJson(obj);
    Modifiers m = r.getModifiers();

    CHECK(m.science >= 1.0f);
    CHECK(m.industry >= 1.0f);
    CHECK(m.trade >= 1.0f);
    CHECK(m.diplomacy >= 1.0f);
    CHECK(m.military >= 1.0f);
    CHECK(m.growth >= 1.0f);
}

// ВАЖНО: этот тест сейчас падает и указывает на реальный баг в коде, а не в тесте.
//
// Race::Race(const json& obj) { Race::loadFromJson(obj); }
//
// loadFromJson — статический метод, который возвращает НОВЫЙ объект Race,
// а не заполняет текущий (*this). Результат вызова в конструкторе никуда
// не присваивается и отбрасывается — сконструированный объект остаётся
// с полями по умолчанию (пустое имя, нейтральные модификаторы).
//
// Сейчас нигде в проекте этот конструктор не используется (используется
// только статический Race::loadFromJson(...)), поэтому баг пока не влияет
// на работающий код — но если кто-то в будущем напишет `Race r(obj);`
// вместо `Race r = Race::loadFromJson(obj);`, он получит пустую расу молча.
//
// Исправление: либо `Race(const json& obj) { *this = loadFromJson(obj); }`,
// либо вообще убрать этот конструктор, раз есть статическая фабрика.
TEST_CASE("Race: конструктор из JSON (баг: не присваивает результат loadFromJson)") {
    json obj = makeRaceJson("humans", "Люди", "village", 1.0f, 1.0f, 1.3f, 1.4f, 1.0f, 1.1f);

    Race r(obj);

    CHECK(r.getName() == "Люди");
}
