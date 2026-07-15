// Неделя 3, день 7 роадмапа: "Консольная симуляция 5-10 ходов одного
// игрока: распределить население, построить здание, исследовать тех,
// нанять юнита без казармы и с казармой — свериться, что штраф
// применяется корректно."
//
// В отличие от остальных test_*.cpp этот файл не проверяет отдельный
// класс, а прогоняет весь цикл разом — ровно то, что просит роадмап.
// Печатает лог по ходам (запусти tests/game_tests и посмотри вывод), а
// также проверяет через CHECK, что итоговое состояние осмысленно, чтобы
// регресс был виден автоматически, а не только глазами.

#include "test_framework.hpp"

#include "core/Player.hpp"

#include <fstream>

#ifndef ASSETS_DIR
#define ASSETS_DIR "../assets"
#endif

namespace {

json loadAssetFile(const std::string &relativePath) {
    std::ifstream file(std::string(ASSETS_DIR) + "/" + relativePath);
    json data;
    file >> data;
    return data;
}

}  // namespace

TEST_CASE("Неделя 3, день 7: симуляция 8 ходов дворфов — экономика, стройка, наука, найм со штрафом и без") {
    // --- Загрузка данных (races/buildings/technologies/heroes/units) -------
    json racesJson = loadAssetFile("races.json");
    json buildingsJson = loadAssetFile("buildings.json");
    json techJson = loadAssetFile("technologies.json");
    json heroesJson = loadAssetFile("heroes.json");
    json unitsJson = loadAssetFile("units.json");

    std::vector<Building> buildingCatalog = Building::loadAllFromJson(buildingsJson["buildings"]);
    std::vector<Technology> techCatalog = Technology::loadAllFromJson(techJson["technologies"]);
    std::vector<Hero> heroes = Hero::loadAllFromJson(heroesJson["heroes"]);
    std::vector<Unit> units = Unit::loadAllFromJson(unitsJson["units"]);

    Race dwarves;
    for (const auto &r : racesJson["races"])
    {
        Race race = Race::loadFromJson(r);
        if (race.getId() == "dwarves")
            dwarves = race;
    }
    CHECK(dwarves.getId() == "dwarves");

    Hero dwarfHero;
    for (const auto &h : heroes)
        if (h.getRaceId() == "dwarves")
            dwarfHero = h;
    CHECK(dwarfHero.getRaceId() == "dwarves");

    auto findBuilding = [&](const std::string &id) {
        for (const auto &b : buildingCatalog)
            if (b.getId() == id)
                return b;
        throw std::runtime_error("building not found: " + id);
    };
    auto findUnit = [&](const std::string &id) {
        for (const auto &u : units)
            if (u.getId() == id)
                return u;
        throw std::runtime_error("unit not found: " + id);
    };

    // --- Игрок: раса, герой, поселение, стартовое распределение -----------
    Player player;
    player.setId(1);
    player.setRace(dwarves);
    player.addHero(dwarfHero);
    player.loadTechnologies(techCatalog);

    Settlement capital(1, SettlementType::Village);
    capital.seedInitialPopulation(100);
    capital.assignFromIdle(Sphere::Industry, 40);
    capital.assignFromIdle(Sphere::Science, 30);
    capital.assignFromIdle(Sphere::Trade, 20);
    player.addSettlement(capital);
    player.settlements()[0].queueBuilding(findBuilding("workshop"));

    std::string firstTechId;
    for (const auto &t : techCatalog)
        if (t.getPrerequisiteIds().empty())
        {
            firstTechId = t.getId();
            break;
        }
    CHECK(player.setCurrentResearch(firstTechId));

    int populationBeforeTurns = player.settlements()[0].populationCount();

    std::cout << "    [demo] старт: раса=" << dwarves.getName() << ", герой=" << dwarfHero.getName()
              << ", исследуем=" << firstTechId << "\n";

    for (int turn = 1; turn <= 8; ++turn)
    {
        player.advanceTurn();
        player.settlements()[0].growPopulation(DEFAULT_BASE_GROWTH_RATE, dwarves.getModifiers().growth);

        std::cout << "    [demo] ход " << turn << ": " << player.resources().toString()
                  << ", население=" << player.settlements()[0].populationCount()
                  << ", построено=" << player.settlements()[0].builtBuildings().size() << "\n";
    }

    // Население должно вырасти (Дворфы: growth=0.7, но 8 ходов роста на базе 100+ population — рост точно есть).
    CHECK(player.settlements()[0].populationCount() > populationBeforeTurns);
    // Мастерская (industryCost=40) должна была достроиться — доход industry
    // у дворфов (модификатор 1.8) с 40 населения даёт 72 очка уже за первый ход.
    CHECK(player.settlements()[0].hasBuilt("workshop"));
    CHECK(player.settlements()[0].buildingQueue().empty());

    // --- Найм юнита без казармы: должен получить штраф необученности -------
    Unit militia = findUnit("militia");
    Unit swordsman = findUnit("swordsman");

    CHECK(!player.settlements()[0].hasBuilt("barracks"));
    player.settlements()[0].queueUnitRecruitment(militia);
    std::vector<Unit> recruitedUntrained = player.settlements()[0].progressRecruitmentQueue(militia.getRecruitCost());
    CHECK(recruitedUntrained.size() == 1u);

    UnitStats untrainedStats = player.effectiveUnitStats(player.settlements()[0], recruitedUntrained[0]);
    UnitStats baseStats = militia.getStats();

    // "Сырые" цифры из JSON — не подходящая база для сравнения: у дворфов
    // бонус расы+героя к military (1.5×1.08=1.62) перекрывает штраф
    // необученности (×0.7), так что даже штрафованный юнит сильнее "сырых"
    // цифр. Правильное сравнение — с эффективными статами ТОГО ЖЕ юнита,
    // но если бы штрафа не было (например, если бы казарма уже стояла):
    // штраф должен урезать именно относительно этого, а не голого JSON.
    Building hypotheticalBarracks = findBuilding("barracks");
    UnitStats trainedReference =
        Military::effectiveStats(recruitedUntrained[0], {hypotheticalBarracks},
                                  dwarves.getModifiers().military, player.heroes());

    CHECK(untrainedStats.attack < trainedReference.attack);
    CHECK(untrainedStats.defense < trainedReference.defense);
    // А относительно "сырых" цифр из JSON проверяем более слабое, но
    // содержательное свойство: штраф действительно был применён, то есть
    // untrained ровно в UNTRAINED_PENALTY_MULTIPLIER раз меньше trainedReference.
    CHECK_NEAR(untrainedStats.attack / trainedReference.attack,
               Military::UNTRAINED_PENALTY_MULTIPLIER, 0.01f);

    // tier 1+ юнит недоступен без казармы:
    CHECK_THROWS(player.settlements()[0].queueUnitRecruitment(swordsman));

    // --- Строим казарму, нанимаем мечника — штрафа быть не должно ----------
    player.settlements()[0].queueBuilding(findBuilding("barracks"));
    player.settlements()[0].progressBuildingQueue(findBuilding("barracks").getCost());
    CHECK(player.settlements()[0].hasBuilt("barracks"));

    // Освобождаем населения на найм мечника (populationCost=50), если не хватает idle.
    if (player.settlements()[0].allocation().idle < swordsman.getPopulationCost())
    {
        int missing = swordsman.getPopulationCost() - player.settlements()[0].allocation().idle;
        player.settlements()[0].unassignToIdle(Sphere::Trade, missing);
    }

    player.settlements()[0].queueUnitRecruitment(swordsman);
    std::vector<Unit> recruitedTrained = player.settlements()[0].progressRecruitmentQueue(swordsman.getRecruitCost());
    CHECK(recruitedTrained.size() == 1u);

    UnitStats trainedStats = player.effectiveUnitStats(player.settlements()[0], recruitedTrained[0]);
    Unit rawSwordsman = findUnit("swordsman");
    // Без штрафа необученности, но с бонусом расы/героя к military — статы
    // должны быть строго БОЛЬШЕ "сырых" (не искажены штрафом в минус).
    CHECK(trainedStats.attack >= rawSwordsman.getStats().attack);
    CHECK(trainedStats.defense >= rawSwordsman.getStats().defense);

    CHECK(player.settlements()[0].garrison().size() == 2u);  // militia + swordsman

    std::cout << "    [demo] необученный ополченец: atk=" << untrainedStats.attack
              << " (база " << baseStats.attack << ")\n";
    std::cout << "    [demo] обученный мечник:      atk=" << trainedStats.attack
              << " (база " << rawSwordsman.getStats().attack << ")\n";
}
