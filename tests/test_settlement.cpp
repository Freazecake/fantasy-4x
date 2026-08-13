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

TEST_CASE("Settlement::growPopulation увеличивает и population, и idle по формуле расы, "
          "с учётом мягкого предела населения") {
    // growthPerTurn = population * baseGrowthRate * race.modifiers.growth,
    // дополнительно затухающий по мере приближения к POPULATION_SOFT_CAP
    // (см. комментарий в Settlement::growPopulation).
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);

    // capFactor = 1 - 100/1000 = 0.9; прирост = 100*0.1*1.5 * 0.9 = 13 (после floor)
    s.growPopulation(0.1f, 1.5f);

    CHECK(s.populationCount() == 113);
    CHECK(s.allocation().idle == 113);
}

TEST_CASE("Settlement::growPopulation с нулевым населением не даёт роста") {
    Settlement s(1, SettlementType::Village);

    s.growPopulation(0.1f, 1.5f);

    CHECK(s.populationCount() == 0);
}

TEST_CASE("Settlement::growPopulation: extraFlatGrowth (бонус ресурса еды) добавляется поверх процентного роста") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(100);

    // (100*0.1*1.5 + 6) * capFactor(0.9) = 21 * 0.9 = 18 (после floor)
    s.growPopulation(0.1f, 1.5f, 6);

    CHECK(s.populationCount() == 118);
    CHECK(s.allocation().idle == 118);
}

TEST_CASE("Settlement::growPopulation: extraFlatGrowth работает даже при нулевом населении") {
    Settlement s(1, SettlementType::Village);

    s.growPopulation(0.1f, 1.5f, 6);

    CHECK(s.populationCount() == 6);
}

TEST_CASE("Settlement::growPopulation: прирост затухает почти до нуля у POPULATION_SOFT_CAP "
          "(регрессионный тест на баг из блока 3.5 — раньше население росло экспоненциально "
          "без ограничений и за ~200 ходов ломало float-арифметику ИИ)") {
    Settlement s(1, SettlementType::Village);
    s.seedInitialPopulation(Settlement::POPULATION_SOFT_CAP - 5);

    for (int i = 0; i < 20; ++i)
        s.growPopulation(0.1f, 1.5f);

    // Население не может уйти существенно выше мягкого предела.
    CHECK(s.populationCount() < Settlement::POPULATION_SOFT_CAP + 10);
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

TEST_CASE("Settlement::anchorCoords: минимальная координата по (q, затем r) среди тайлов") {
    Settlement s(0, SettlementType::Village);
    s.addTile(Tile(5, 5));
    s.addTile(Tile(4, 9));
    s.addTile(Tile(4, 2));
    s.addTile(Tile(6, 0));

    Coords anchor = s.anchorCoords();
    CHECK(anchor.q == 4);
    CHECK(anchor.r == 2);
}

TEST_CASE("Settlement::anchorCoords: не зависит от порядка добавления тайлов") {
    Settlement a(0, SettlementType::Village);
    a.addTile(Tile(1, 1));
    a.addTile(Tile(0, 0));
    a.addTile(Tile(2, 2));

    Settlement b(0, SettlementType::Village);
    b.addTile(Tile(2, 2));
    b.addTile(Tile(0, 0));
    b.addTile(Tile(1, 1));

    CHECK(a.anchorCoords().q == b.anchorCoords().q);
    CHECK(a.anchorCoords().r == b.anchorCoords().r);
}

TEST_CASE("Settlement::anchorCoords: (-1, -1) для поселения без тайлов") {
    Settlement s(0, SettlementType::Village);
    Coords anchor = s.anchorCoords();
    CHECK(anchor.q == -1);
    CHECK(anchor.r == -1);
}
