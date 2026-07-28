#include "test_framework.hpp"

#include "core/GameEngine.hpp"

TEST_CASE("GameEngine::loadFromAssets: человек помечен как isHuman")
{
    GameEngine engine;
    bool loaded = engine.loadFromAssets(ASSETS_DIR);
    CHECK(loaded);
    CHECK(engine.state().players().size() == 1u);
    CHECK(engine.state().players()[0].isHuman());
}

TEST_CASE("GameEngine::endTurn: не разыгрывает ход человека автоматически (регрессия)")
{
    // Player::m_isHuman по умолчанию false. Раньше ничто в loadFromAssets не
    // вызывало setIsHuman(true) для человека, и endTurn() (который вызывает
    // AIController::takeTurn для всех !isHuman() игроков) отдавал ход
    // человека ИИ так же, как и ботам — распределение населения, очередь
    // построек/найма, дипломатия менялись бы без ведома игрока.
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    Player &human = engine.state().players()[0];
    CHECK(human.settlements().size() == 1u);

    const auto &allocBefore = human.settlements()[0].allocation();
    int science = allocBefore.science;
    int industry = allocBefore.industry;
    int trade = allocBefore.trade;
    int diplomacy = allocBefore.diplomacy;
    // loadFromAssets вручную назначает 15/25/10/5 — если это не так,
    // тест ниже проверяет не то, что задумано.
    CHECK(science == 15);
    CHECK(industry == 25);
    CHECK(trade == 10);
    CHECK(diplomacy == 5);

    engine.endTurn();

    // Рост населения увеличивает idle, но НЕ имеет права трогать уже
    // распределённых по сферам людей — если AIController::decideAllocations
    // ошибочно отработал бы по человеку, эти четыре числа изменились бы.
    const auto &allocAfter = engine.state().players()[0].settlements()[0].allocation();
    CHECK(allocAfter.science == science);
    CHECK(allocAfter.industry == industry);
    CHECK(allocAfter.trade == trade);
    CHECK(allocAfter.diplomacy == diplomacy);
}

TEST_CASE("GameEngine::endTurn: боту (isHuman=false) реально разыгрывает ход ИИ")
{
    // Контрольная проверка к предыдущему тесту: убеждаемся, что endTurn
    // действительно вызывает AIController хоть для кого-то, а не просто
    // ничего не делает для всех игроков подряд.
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    Player bot;
    bot.setId(1);
    bot.setRace(engine.state().players()[0].race());
    bot.loadTechnologies(engine.techCatalog());
    Settlement s(1, SettlementType::Village);
    s.addTile(Tile(0, 0));
    s.seedInitialPopulation(60); // всё сразу в idle — ИИ есть, что распределять
    bot.addSettlement(s);
    engine.state().addPlayer(bot);

    engine.endTurn();

    const auto &alloc = engine.state().players()[1].settlements()[0].allocation();
    int assigned = alloc.science + alloc.industry + alloc.trade + alloc.diplomacy;
    CHECK(assigned > 0); // ИИ должен был распределить хоть что-то за ход
}
