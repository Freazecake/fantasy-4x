#include "test_framework.hpp"

#include "core/Settlement.hpp"

TEST_CASE("Settlement: конструктор по умолчанию — нулевое население, тип PreSettlement") {
    Settlement s;

    CHECK(s.type() == SettlementType::PreSettlement);
    CHECK(s.populationCount() == 0);
    CHECK(s.allocation().total() == 0);
}

TEST_CASE("Settlement: конструктор с ownerId/type сохраняет их") {
    Settlement s(7, SettlementType::Village);

    CHECK(s.ownerId() == 7);
    CHECK(s.type() == SettlementType::Village);
}

TEST_CASE("Settlement::seedInitialPopulation увеличивает и population, и idle") {
    Settlement s(1, SettlementType::Village);

    s.seedInitialPopulation(10);

    CHECK(s.populationCount() == 10);
    CHECK(s.allocation().idle == 10);
    CHECK(s.allocation().total() == 10);
}

TEST_CASE("Settlement::seedInitialPopulation с отрицательным числом бросает исключение") {
    Settlement s(1, SettlementType::Village);

    CHECK_THROWS(s.seedInitialPopulation(-5));
}

TEST_CASE("Settlement::assignFromIdle переносит людей из idle в сферу") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(10);

    s.assignFromIdle(Sphere::Science, 4);

    CHECK(s.allocation().idle == 6);
    CHECK(s.allocation().science == 4);
    // Общее население не меняется — распределение, а не трата (раздел 2.2 гайда).
    CHECK(s.allocation().total() == 10);
    CHECK(s.populationCount() == 10);
}

TEST_CASE("Settlement::assignFromIdle больше, чем есть в idle — бросает исключение") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(5);

    CHECK_THROWS(s.assignFromIdle(Sphere::Science, 6));
    // Состояние не должно было измениться после неудачной попытки.
    CHECK(s.allocation().idle == 5);
    CHECK(s.allocation().science == 0);
}

TEST_CASE("Settlement::assignFromIdle с отрицательным amount бросает исключение") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(5);

    CHECK_THROWS(s.assignFromIdle(Sphere::Trade, -1));
}

TEST_CASE("Settlement::unassignToIdle возвращает людей из сферы обратно в idle") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(10);
    s.assignFromIdle(Sphere::Industry, 7);

    s.unassignToIdle(Sphere::Industry, 3);

    CHECK(s.allocation().industry == 4);
    CHECK(s.allocation().idle == 6);
    CHECK(s.allocation().total() == 10);
}

TEST_CASE("Settlement::unassignToIdle больше, чем назначено в сфере — бросает исключение") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(10);
    s.assignFromIdle(Sphere::Trade, 2);

    CHECK_THROWS(s.unassignToIdle(Sphere::Trade, 3));
}

TEST_CASE("Settlement::reallocate переносит людей между двумя сферами напрямую") {
    // Раздел 2.2 гайда: "население можно свободно перераспределять между
    // сферами в любой ход (мгновенно, без штрафа), включая ранее назначенное".
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(10);
    s.assignFromIdle(Sphere::Science, 6);

    s.reallocate(Sphere::Science, Sphere::Diplomacy, 4);

    CHECK(s.allocation().science == 2);
    CHECK(s.allocation().diplomacy == 4);
    CHECK(s.allocation().idle == 4);
    CHECK(s.allocation().total() == 10);
}

TEST_CASE("Settlement::growPopulation увеличивает и population, и idle по формуле расы") {
    // growthPerTurn = population * baseGrowthRate * race.modifiers.growth (раздел 2.2 гайда)
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);

    s.growPopulation(0.1f, 1.5f);  // ожидаемый прирост: 100 * 0.1 * 1.5 = 15

    CHECK(s.populationCount() == 115);
    CHECK(s.allocation().idle == 115);
}

TEST_CASE("Settlement::growPopulation с нулевым населением не даёт роста") {
    Settlement s(1, SettlementType::Village);

    s.growPopulation(0.1f, 1.5f);

    CHECK(s.populationCount() == 0);
}

TEST_CASE("Settlement::growPopulation не уходит в отрицательный прирост даже при "
          "странных входных данных") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(50);

    s.growPopulation(-0.5f, 1.0f);  // некорректный baseGrowthRate

    CHECK(s.populationCount() >= 50);
}

TEST_CASE("Settlement::removePopulationPermanently уменьшает population и idle") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(20);

    s.removePopulationPermanently(5);

    CHECK(s.populationCount() == 15);
    CHECK(s.allocation().idle == 15);
}

TEST_CASE("Settlement::removePopulationPermanently требует свободных людей в idle") {
    // Нельзя списать назначенных в сферу людей напрямую — сначала unassignToIdle
    // (см. комментарий в Settlement.cpp).
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(10);
    s.assignFromIdle(Sphere::Science, 10);  // весь idle занят

    CHECK_THROWS(s.removePopulationPermanently(1));
}

TEST_CASE("Settlement::addTile добавляет клетку в список") {
    Settlement s(1, SettlementType::Village);

    s.addTile(Tile(2, 3));

    CHECK(s.tiles().size() == 1u);
    CHECK(s.tiles()[0].getCoords().q == 2);
}

TEST_CASE("PopulationAllocation::get/set работают согласованно для всех 4 сфер") {
    PopulationAllocation alloc;
    alloc.set(Sphere::Science, 1);
    alloc.set(Sphere::Industry, 2);
    alloc.set(Sphere::Trade, 3);
    alloc.set(Sphere::Diplomacy, 4);

    CHECK(alloc.get(Sphere::Science) == 1);
    CHECK(alloc.get(Sphere::Industry) == 2);
    CHECK(alloc.get(Sphere::Trade) == 3);
    CHECK(alloc.get(Sphere::Diplomacy) == 4);
    CHECK(alloc.total() == 10);  // idle остался 0
}
