cmake_minimum_required(VERSION 3.14)

# Два способа сборки:
#   1) Standalone, без изменения корневого CMakeLists.txt:
#        cd tests
#        cmake -S . -B build
#        cmake --build build
#        ./build/game_tests            (на Windows: build\game_tests.exe или через IDE)
#
#   2) Из корневого CMakeLists.txt проекта — просто добавь строку
#        add_subdirectory(tests)
#      после add_executable(${PROJECT_NAME} ...). project() ниже тогда не
#      выполнится повторно (см. if(NOT DEFINED PROJECT_NAME)).
if(NOT DEFINED PROJECT_NAME)
    project("game_tests" CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Корень проекта — на уровень выше tests/. Если структура папок другая,
# поправь этот путь.
set(GAME_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Намеренно НЕ используется file(GLOB ...) по всему src/, чтобы явно
# видеть, что реально протестировано в этой папке (main.cpp вообще требует
# <Windows.h> и не годится для консольных тестов).
add_executable(game_tests
    main.cpp
    test_race.cpp
    test_map_tile.cpp
    test_settlement.cpp
    test_settlement_queues.cpp
    test_building.cpp
    test_technology.cpp
    test_unit.cpp
    test_hero.cpp
    test_economy.cpp
    test_military.cpp
    test_political_action.cpp
    test_political_effects.cpp
    test_trade_market.cpp
    test_player.cpp
    test_data_integrity.cpp
    test_week3_simulation.cpp
    test_coords_army.cpp
    test_combat.cpp
    test_ai.cpp
    test_ai_smart.cpp
    test_game_engine.cpp
    test_map_generator.cpp

    ${GAME_ROOT}/src/core/Race.cpp
    ${GAME_ROOT}/src/core/Tile.cpp
    ${GAME_ROOT}/src/core/Map.cpp
    ${GAME_ROOT}/src/core/MapGenerator.cpp
    ${GAME_ROOT}/src/core/ResourceCatalog.cpp
    ${GAME_ROOT}/src/core/Settlement.cpp
    ${GAME_ROOT}/src/core/Building.cpp
    ${GAME_ROOT}/src/core/Technology.cpp
    ${GAME_ROOT}/src/core/Unit.cpp
    ${GAME_ROOT}/src/core/Hero.cpp
    ${GAME_ROOT}/src/core/Economy.cpp
    ${GAME_ROOT}/src/core/Military.cpp
    ${GAME_ROOT}/src/core/PoliticalAction.cpp
    ${GAME_ROOT}/src/core/TradeMarket.cpp
    ${GAME_ROOT}/src/core/Player.cpp
    ${GAME_ROOT}/src/core/GameState.cpp
    ${GAME_ROOT}/src/core/Army.cpp
    ${GAME_ROOT}/src/core/Combat.cpp
    ${GAME_ROOT}/src/core/GameEngine.cpp
    ${GAME_ROOT}/src/ai/AIController.cpp
    ${GAME_ROOT}/src/ai/AIStrategy.cpp
    ${GAME_ROOT}/src/ai/CombatSimulator.cpp
)

target_include_directories(game_tests PRIVATE
    ${GAME_ROOT}/include
    ${GAME_ROOT}/src
)

# Абсолютный путь к assets/, чтобы test_data_integrity.cpp находил
# реальные races.json/buildings.json/technologies.json/units.json
# независимо от того, откуда запущен исполняемый файл.
target_compile_definitions(game_tests PRIVATE
    ASSETS_DIR="${GAME_ROOT}/assets"
)

enable_testing()
add_test(NAME core_tests COMMAND game_tests)
