# tests/ — консольные тесты core/ (недели 2-3 роадмапа)

Лёгкие тесты без внешних зависимостей (GTest и т.п. намеренно не тянем —
см. правило "не тащить тяжёлые библиотеки без необходимости" в инструкциях
проекта). Фреймворк — один header (`test_framework.hpp`, ~120 строк):
`TEST_CASE`, `CHECK`, `CHECK_NEAR`, `CHECK_THROWS`, `CHECK_NOTHROW`.

136 тестов, все зелёные. Покрыта неделя 2 (`Race`, `Tile`/`Coords`, `Map`,
`Settlement`, `Building`, `Technology`, `Unit`) и неделя 3 (`Hero`,
`Economy`, `Military`, `PoliticalAction`, `TradeMarket`, `Player`), плюс
`test_data_integrity.cpp` — грузит настоящие JSON из `assets/` и проверяет
перекрёстные ссылки между файлами (`unlocksBuildingIds`, `unlocksUnitIds`,
`requiredBuildingId`, `prerequisiteIds`, `heroes[].raceId`) и отсутствие
циклов в дереве технологий (день 7 обеих недель).

## Сборка

Положи эту папку в корень проекта (рядом с `src/`, `include/`, `assets/`).

**Вариант А — отдельно, без изменения корневого CMakeLists.txt:**

```
cd tests
cmake -S . -B build
cmake --build build
./build/game_tests          # Linux/macOS
build\game_tests.exe        # Windows (Debug/Release смотри в build/)
```

**Вариант Б — из корневого CMakeLists.txt:** добавь одну строку после
`add_executable(${PROJECT_NAME} ...)`:

```cmake
add_subdirectory(tests)
```

и пересобери проект — появится ещё один target, `game_tests`.

Тесты по-прежнему не глобят `src/core/*.cpp` целиком: `GameEngine` и
`GameState` пока пустые заглушки (недели 4-5, там будет игровой цикл и
условие победы), а `main.cpp` требует `<Windows.h>`. Когда допишешь их —
добавь соответствующие `.cpp` и тестовые файлы в `tests/CMakeLists.txt`.

## Неделя 3: что добавилось и какие баги нашлись по пути

- **`Race`** — добавлено поле `id` (раньше была только `name`, что мешало
  сопоставить расу с героем по `raceId`).
- **`Hero`** — добавлено поле `raceId`: теперь 5 героев = 5 рас, один герой
  на расу, специализация — по топ-стату расы (см. `heroes.json`).
- **`Economy::computeSettlementIncome`** — переписан. Раньше метод
  игнорировал `PopulationAllocation` поселения и всегда считал доход как
  `Resources(1) * raceModifiers`, то есть реального дохода не было вообще
  (баг, не просто недоделанная заглушка). Теперь: население по сфере ×
  раса × постройки поселения × герои игрока (все множители — раздел 2.3
  гайда).
- **`Settlement`** — добавлены очередь построек (`queueBuilding` /
  `progressBuildingQueue`, с переносом излишка очков на следующее здание в
  очереди) и очередь найма юнитов (`queueUnitRecruitment` /
  `progressRecruitmentQueue`, гарнизон). Найм сразу списывает
  `populationCost` из `idle`, а не только в момент завершения.
- **`Military`** — новый класс: `canRecruit` (tier 1+ юнит нужен
  соответствующее построенное здание), `queueUnit`, `effectiveStats`
  (штраф необученности ×0.7, если нет здания с
  `removesUntrainedPenalty()`, плюс бонус расы/героя к `military`).
- **`PoliticalAction`** + `political_actions.json` — 4 действия
  (`alliance`/`vassalize`/`demandTribute`/`politicalConquest`).
- **`TradeMarket`** — обмен очков по курсу, зависящему только от
  покупателя (раздел 2.6 гайда); нельзя купить `trade` за `trade`.
- **`Player`** — теперь полноценный: раса, герои, личная копия дерева
  технологий с `setCurrentResearch`/`investScience`, `advanceTurn()`
  (полный ход: доход по всем поселениям → industry каждого поселения идёт
  в его собственную очередь построек → наука идёт в текущее исследование),
  `exchangeTrade`, `trySpendOnPoliticalAction`, `effectiveUnitStats`.

Один нюанс дизайна стоит держать в голове: `Military::effectiveStats`
домножает `attack`/`defense`/`health` на расовый + геройский модификатор
`military`, а не только на штраф необученности. У сильно военных рас (орки,
дворфы) это может перекрыть штраф с запасом — "необученный" юнит всё равно
может быть сильнее "сырых" цифр из `units.json`. Это не баг: штраф всё
равно применяется (юнит слабее, чем был бы с той же расой/героем, но без
штрафа) — просто сравнивать нужно с "обученным" вариантом того же юнита,
а не с голыми числами из JSON. См. `test_week3_simulation.cpp` — там ровно
это сравнение и приведено как пример.

## Структура

```
tests/
├── CMakeLists.txt            — standalone или add_subdirectory(tests)
├── test_framework.hpp        — TEST_CASE/CHECK, без внешних зависимостей
├── main.cpp                  — testfw::runAll()
├── test_race.cpp
├── test_map_tile.cpp         — Coords + Tile + Map
├── test_settlement.cpp       — население, распределение по сферам, рост
├── test_settlement_queues.cpp — очередь построек и найма юнитов (неделя 3)
├── test_building.cpp
├── test_technology.cpp       — canResearch, invest, дерево зависимостей
├── test_unit.cpp
├── test_hero.cpp             — raceId, специализация по максимальному модификатору
├── test_economy.cpp          — доход: население × раса × постройки × герои
├── test_military.cpp         — canRecruit, effectiveStats, штраф необученности
├── test_political_action.cpp
├── test_trade_market.cpp
├── test_player.cpp           — раса/герой, исследования, advanceTurn, торговля, политика
├── test_data_integrity.cpp   — реальные assets/*.json, перекрёстные ссылки, циклы
└── test_week3_simulation.cpp — консольная симуляция 8 ходов (день 7, неделя 3)
```
