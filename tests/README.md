# tests/ — консольные тесты core/ (неделя 2, день 7 роадмапа)

Лёгкие тесты без внешних зависимостей (GTest и т.п. намеренно не тянем —
см. правило "не тащить тяжёлые библиотеки без необходимости" в инструкциях
проекта). Фреймворк — один header (`test_framework.hpp`, ~120 строк):
`TEST_CASE`, `CHECK`, `CHECK_NEAR`, `CHECK_THROWS`, `CHECK_NOTHROW`.

Тестируется всё, что сделано на неделе 2: `Race`, `Tile`/`Coords`, `Map`,
`Settlement`, `Building`, `Technology`, `Unit`, плюс отдельный файл
`test_data_integrity.cpp`, который грузит настоящие JSON из `assets/` и
проверяет перекрёстные ссылки (`unlocksBuildingIds`, `unlocksUnitIds`,
`requiredBuildingId`, `prerequisiteIds`) и отсутствие циклов в дереве
технологий — это как раз задача дня 7 из роадмапа.

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

Тесты собирают только `Race.cpp`, `Tile.cpp`, `Map.cpp`, `Settlement.cpp`,
`Building.cpp`, `Technology.cpp`, `Unit.cpp` — то, что реально протестировано
здесь. `GameEngine`/`GameState`/`Player`/`Economy`/`Hero` пока пустые
заглушки (недели 3+), поэтому в сборку тестов не включены. По мере
реализации экономики (неделя 3) стоит добавить для них `test_economy.cpp`,
`test_hero.cpp` и дописать соответствующие `.cpp` в `tests/CMakeLists.txt`.

## Что показывает прогон (69 тестов, из них 4 сейчас падают — это ожидаемо)

Тесты нашли три настоящих бага в текущем коде проекта, не в самих тестах:

1. **`Race`, `Building`, `Technology`: конструктор из JSON ничего не грузит.**
   ```cpp
   Race::Race(const json &obj) { Race::loadFromJson(obj); }
   ```
   `loadFromJson` — статический метод, возвращающий **новый** объект, а не
   заполняющий `*this`. Результат отбрасывается, и `Race r(obj);` тихо даёт
   пустой объект (аналогично для `Building` и `Technology`). Пока нигде в
   проекте эти конструкторы не используются (используется только
   `Race::loadFromJson(...)` напрямую), поэтому баг ни на что ещё не влияет —
   но лучше поправить сейчас, пока не забылось: либо
   `Race(const json& obj) { *this = loadFromJson(obj); }`, либо вообще убрать
   конструктор, раз есть статическая фабрика.
   → тесты: `test_race.cpp`, `test_building.cpp`, `test_technology.cpp`
   (последний TEST_CASE в каждом файле).

2. **`buildings.json`: `barracks.unlocksUnitIds` ссылается на несуществующий
   юнит.** Там указано `"unit_veteran_warrior"`, а в `units.json` только
   `militia` / `swordsman` / `archer`. Похоже на старый id, оставшийся от
   более ранней версии данных. Скорее всего нужно поставить
   `["swordsman", "archer"]`.
   → тест: `test_data_integrity.cpp`, `"Building.unlocksUnitIds: все ссылки
   указывают на существующие id юнитов"`.

3. **`struct UnitStats` определена дважды** — в `core/Common.hpp` и заново,
   независимо, в `core/Unit.hpp`. Сейчас это неопасно, потому что оба
   заголовка нигде не подключаются вместе в одной единице трансляции — но
   стоит попробовать (например, при написании `test_data_integrity.cpp`
   изначально) — компиляция падает с `redefinition of struct UnitStats`.
   Исправление — одна строка: убрать повторное объявление из `Unit.hpp` и
   подключить туда `Common.hpp`, как это уже сделано в `Building.hpp` и
   `Technology.hpp`.
   → сам этот файл README и комментарий в начале `test_data_integrity.cpp`
   объясняют, почему там нет `#include "core/Unit.hpp"` — это обход, а не
   исправление; после починки `Unit.hpp` обход можно убрать.

Всё остальное (65 из 69 тестов) — зелёное. Как только поправишь три пункта
выше, оставшиеся 4 теста тоже станут зелёными без изменения самих тестов.

## Структура

```
tests/
├── CMakeLists.txt        — standalone или add_subdirectory(tests)
├── test_framework.hpp    — TEST_CASE/CHECK, без внешних зависимостей
├── main.cpp               — testfw::runAll()
├── test_race.cpp
├── test_map_tile.cpp      — Coords + Tile + Map
├── test_settlement.cpp    — население, распределение по сферам, рост
├── test_building.cpp
├── test_technology.cpp    — canResearch, invest, дерево зависимостей
├── test_unit.cpp
└── test_data_integrity.cpp — реальные assets/*.json, перекрёстные ссылки,
                               поиск циклов в дереве технологий (день 7)
```
