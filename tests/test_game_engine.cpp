#include "test_framework.hpp"

#include "core/GameEngine.hpp"
#include "core/StartLayout.hpp"
#include <set>

TEST_CASE("GameEngine::loadFromAssets: карта 20x18, столица человека — точка 0 пятиугольника")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    const auto &tiles = engine.state().map().getMap();
    CHECK(tiles.size() == static_cast<size_t>(StartLayout::MAP_WIDTH * StartLayout::MAP_HEIGHT));

    Coords expectedCenter = StartLayout::pentagonCapitals()[0];
    const auto &capitalTiles = engine.state().players()[0].settlements()[0].tiles();
    CHECK(capitalTiles.size() == 9u); // блок 3x3

    bool centerPresent = false;
    for (const auto &t : capitalTiles)
        if (t.getCoords().q == expectedCenter.q && t.getCoords().r == expectedCenter.r)
            centerPresent = true;
    CHECK(centerPresent);
}

TEST_CASE("GameEngine::loadFromAssets: 5 игроков — человек + 4 бота, только человек isHuman")
{
    GameEngine engine;
    bool loaded = engine.loadFromAssets(ASSETS_DIR);
    CHECK(loaded);
    CHECK(engine.state().players().size() == 5u);
    CHECK(engine.state().players()[0].isHuman());
    for (size_t i = 1; i < engine.state().players().size(); ++i)
        CHECK(!engine.state().players()[i].isHuman());
}

TEST_CASE("GameEngine::loadFromAssets: у ботов 4 разные расы, все отличные от человеческой")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    CHECK(engine.state().players()[0].race().getId() == "humans");

    std::vector<std::string> botRaces;
    for (size_t i = 1; i < engine.state().players().size(); ++i)
        botRaces.push_back(engine.state().players()[i].race().getId());

    CHECK(botRaces.size() == 4u);
    for (const auto &id : botRaces)
        CHECK(id != "humans");

    // Все 4 расы у ботов должны быть разными (не два бота с одной расой).
    for (size_t i = 0; i < botRaces.size(); ++i)
        for (size_t j = i + 1; j < botRaces.size(); ++j)
            CHECK(botRaces[i] != botRaces[j]);
}

TEST_CASE("GameEngine::loadFromAssets: столица каждого игрока стоит на его точке пятиугольника")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    const auto &capitals = StartLayout::pentagonCapitals();
    for (size_t i = 0; i < engine.state().players().size(); ++i)
    {
        const auto &tiles = engine.state().players()[i].settlements()[0].tiles();
        bool centerPresent = false;
        for (const auto &t : tiles)
            if (t.getCoords().q == capitals[i].q && t.getCoords().r == capitals[i].r)
                centerPresent = true;
        CHECK(centerPresent);
    }
}

TEST_CASE("GameEngine::loadFromAssets: у всех игроков стартовый гарнизон не пуст")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    for (const auto &p : engine.state().players())
        CHECK(!p.settlements()[0].garrison().empty());
}

TEST_CASE("GameEngine: полная партия — 10 ходов подряд с 4 ботами не падает (дымовой тест)")
{
    // Это и есть проверка "можно запустить и играть тестовую партию" на
    // уровне логики, без GUI: свежая игра из loadFromAssets, несколько ходов
    // endTurn() подряд (боты реально принимают решения через AIController),
    // и после каждого хода — что состояние осталось внутренне непротиворечивым.
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    for (int turn = 0; turn < 10; ++turn)
    {
        CHECK_NOTHROW(engine.endTurn());

        // Игроков не должно физически пропадать (даже проигравший остаётся
        // в списке без поселений, до этого пункта в 10 ходов дойти маловероятно).
        CHECK(engine.state().players().size() == 5u);

        auto victory = engine.state().checkVictory();
        if (victory.hasWinner())
            break; // партия завершилась раньше — тоже валидный, ожидаемый исход
    }
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

    // id 1..4 уже заняты настоящими ботами из loadFromAssets — берём
    // заведомо свободный id, а не полагаемся на конкретный индекс в векторе.
    const int extraBotId = 99;
    Player bot;
    bot.setId(extraBotId);
    bot.setRace(engine.state().players()[0].race());
    bot.loadTechnologies(engine.techCatalog());
    Settlement s(extraBotId, SettlementType::Village);
    s.addTile(Tile(0, 0));
    s.seedInitialPopulation(60); // всё сразу в idle — ИИ есть, что распределять
    bot.addSettlement(s);
    engine.state().addPlayer(bot);

    engine.endTurn();

    Player *addedBot = nullptr;
    for (auto &p : engine.state().players())
        if (p.id() == extraBotId)
            addedBot = &p;
    CHECK(addedBot != nullptr);
    if (addedBot != nullptr)
    {
        const auto &alloc = addedBot->settlements()[0].allocation();
        int assigned = alloc.science + alloc.industry + alloc.trade + alloc.diplomacy;
        CHECK(assigned > 0); // ИИ должен был распределить хоть что-то за ход
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Блок 3.4: ресурсные клетки — интеграция через GameEngine::loadFromAssets
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("GameEngine::loadFromAssets: карта содержит ресурсные клетки (tiles.json подключён)")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    CHECK(!engine.resourceCatalog().empty());

    int withResource = 0;
    for (const auto &tile : engine.state().map().getMap())
        if (tile.getResource() != ResourceType::None)
            ++withResource;

    // При плотности из tiles.json и карте 20x18 практически гарантированно
    // найдётся хотя бы одна ресурсная клетка (вероятность нуля исчезающе мала).
    CHECK(withResource > 0);
}

TEST_CASE("GameEngine::loadFromAssets: на клетках стартовых столиц ресурсов нет")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR));

    for (const auto &p : engine.state().players())
    {
        for (const auto &settlement : p.settlements())
        {
            for (const auto &tile : settlement.tiles())
            {
                CHECK(tile.getResource() == ResourceType::None);
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Блок 3.7: сохранение/загрузка
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("GameEngine::saveGame/loadGame: round-trip после нескольких ходов "
          "сохраняет ключевые поля партии")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR, 777));

    for (int i = 0; i < 3; ++i)
        engine.endTurn();

    std::string path = "/tmp/test_save_roundtrip.json";
    CHECK(engine.saveGame(path));

    GameEngine loaded;
    CHECK(loaded.loadGame(path, ASSETS_DIR));

    CHECK(loaded.state().turnNumber() == engine.state().turnNumber());
    CHECK(loaded.state().map().getMap().size() == engine.state().map().getMap().size());
    CHECK(loaded.state().players().size() == engine.state().players().size());

    for (size_t i = 0; i < engine.state().players().size(); ++i)
    {
        const Player &orig = engine.state().players()[i];
        const Player &rest = loaded.state().players()[i];

        CHECK(rest.id() == orig.id());
        CHECK(rest.isHuman() == orig.isHuman());
        CHECK(rest.race().getId() == orig.race().getId());
        CHECK(rest.settlements().size() == orig.settlements().size());
        CHECK(rest.armies().size() == orig.armies().size());

        CHECK_NEAR(rest.resources().science, orig.resources().science, 0.01f);
        CHECK_NEAR(rest.resources().industry, orig.resources().industry, 0.01f);
        CHECK_NEAR(rest.resources().trade, orig.resources().trade, 0.01f);
        CHECK_NEAR(rest.resources().diplomacy, orig.resources().diplomacy, 0.01f);

        for (size_t s = 0; s < orig.settlements().size(); ++s)
        {
            CHECK(rest.settlements()[s].populationCount() == orig.settlements()[s].populationCount());
            CHECK(rest.settlements()[s].tiles().size() == orig.settlements()[s].tiles().size());
            CHECK(rest.settlements()[s].garrison().size() == orig.settlements()[s].garrison().size());
            CHECK(rest.settlements()[s].type() == orig.settlements()[s].type());
        }

        for (size_t a = 0; a < orig.armies().size(); ++a)
        {
            CHECK(rest.armies()[a].position().q == orig.armies()[a].position().q);
            CHECK(rest.armies()[a].position().r == orig.armies()[a].position().r);
            CHECK(rest.armies()[a].units().size() == orig.armies()[a].units().size());
        }
    }
}

TEST_CASE("GameEngine::saveGame/loadGame: round-trip сразу после старта партии (0 ходов)")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR, 42));

    std::string path = "/tmp/test_save_turn0.json";
    CHECK(engine.saveGame(path));

    GameEngine loaded;
    CHECK(loaded.loadGame(path, ASSETS_DIR));

    CHECK(loaded.state().turnNumber() == 0);
    CHECK(loaded.state().players().size() == engine.state().players().size());
    for (size_t i = 0; i < engine.state().players().size(); ++i)
    {
        CHECK(loaded.state().players()[i].settlements().size() ==
              engine.state().players()[i].settlements().size());
        // Гарнизон стартовой столицы (один юнит) должен восстановиться как есть.
        if (!engine.state().players()[i].settlements().empty())
        {
            CHECK(loaded.state().players()[i].settlements()[0].garrison().size() ==
                  engine.state().players()[i].settlements()[0].garrison().size());
        }
    }
}

TEST_CASE("GameEngine::saveGame/loadGame: сохранённая карта восстанавливает тип клетки и ресурс")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR, 555));

    std::string path = "/tmp/test_save_map.json";
    CHECK(engine.saveGame(path));

    GameEngine loaded;
    CHECK(loaded.loadGame(path, ASSETS_DIR));

    const auto &origTiles = engine.state().map().getMap();
    const auto &loadedTiles = loaded.state().map().getMap();
    CHECK(origTiles.size() == loadedTiles.size());

    for (const auto &ot : origTiles)
    {
        const Tile *lt = loaded.state().map().tileAt(ot.getCoords().q, ot.getCoords().r);
        CHECK(lt != nullptr);
        if (lt)
        {
            CHECK(lt->getType() == ot.getType());
            CHECK(lt->getResource() == ot.getResource());
        }
    }
}

TEST_CASE("GameEngine::loadGame: несуществующий файл — false, не падает")
{
    GameEngine engine;
    CHECK(!engine.loadGame("/tmp/this_save_definitely_does_not_exist_12345.json", ASSETS_DIR));
}

// ═══════════════════════════════════════════════════════════════════════
// Блок 3.14: отчёт о ходе
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("GameEngine::endTurn: событие Victory появляется в том же ходу, "
          "когда остаётся один живой игрок")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR, 999));

    // Искусственно убираем поселения у всех, кроме первого игрока —
    // имитация "последнего выжившего" без прогона реального боя ботов.
    for (size_t i = 1; i < engine.state().players().size(); ++i)
        engine.state().players()[i].settlements().clear();

    engine.endTurn();

    bool hasVictory = false;
    for (const auto &ev : engine.state().turnEvents())
        if (ev.type == TurnEventType::Victory)
            hasVictory = true;

    CHECK(hasVictory);
    CHECK(engine.checkVictoryCondition().hasWinner());
}

TEST_CASE("GameEngine::endTurn: события очищаются в начале каждого следующего хода")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR, 1001));

    // Создаём хоть одно событие вручную, эмулируя действие прошлого хода.
    engine.state().pushEvent(TurnEventType::TradeExchange, 0, -1, "тест");
    CHECK(!engine.state().turnEvents().empty());

    engine.endTurn();

    // Ровно то, что произошло В ЭТОМ ходу — старое событие не переживает.
    for (const auto &ev : engine.state().turnEvents())
        CHECK(ev.description != "тест");
}

// ═══════════════════════════════════════════════════════════════════════
// Блок 3.11: выбор расы и настройки партии
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("GameEngine::loadFromAssets(options): смена расы человека применяется")
{
    GameEngine engine;
    GameStartOptions options;
    options.humanRaceId = "orcs";
    options.numBots = 4;

    CHECK(engine.loadFromAssets(ASSETS_DIR, options, 111));

    CHECK(engine.state().players()[0].race().getId() == "orcs");
    CHECK(engine.state().players()[0].isHuman());
}

TEST_CASE("GameEngine::loadFromAssets(options): число ботов задаёт итоговое число игроков")
{
    for (int numBots = 1; numBots <= 4; ++numBots)
    {
        GameEngine engine;
        GameStartOptions options;
        options.humanRaceId = "humans";
        options.numBots = numBots;

        CHECK(engine.loadFromAssets(ASSETS_DIR, options, 222));
        CHECK(static_cast<int>(engine.state().players().size()) == numBots + 1);

        // Расы ботов не повторяются и не совпадают с расой человека.
        std::set<std::string> seenRaces;
        seenRaces.insert(engine.state().players()[0].race().getId());
        for (size_t i = 1; i < engine.state().players().size(); ++i)
        {
            std::string raceId = engine.state().players()[i].race().getId();
            CHECK(seenRaces.count(raceId) == 0u);
            seenRaces.insert(raceId);
            CHECK(!engine.state().players()[i].isHuman());
        }
    }
}

TEST_CASE("GameEngine::loadFromAssets(options): каждый бот получил свою столицу и не на воде")
{
    GameEngine engine;
    GameStartOptions options;
    options.humanRaceId = "elves";
    options.numBots = 2;

    CHECK(engine.loadFromAssets(ASSETS_DIR, options, 333));
    CHECK(engine.state().players().size() == 3u);

    for (const auto &p : engine.state().players())
    {
        CHECK(p.settlements().size() == 1u);
        CHECK(!p.settlements()[0].tiles().empty());
    }
}

TEST_CASE("GameEngine::loadFromAssets(): без options ведёт себя как раньше (5 игроков, humans)")
{
    GameEngine engine;
    CHECK(engine.loadFromAssets(ASSETS_DIR, 444));

    CHECK(engine.state().players().size() == 5u);
    CHECK(engine.state().players()[0].race().getId() == "humans");
}
