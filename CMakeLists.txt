cmake_minimum_required(VERSION 3.14)
project("game")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "include/*.hpp"
)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    include
    src
    assets
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

enable_testing()
add_subdirectory(tests)

add_executable(playtest
    tools/playtest.cpp

    src/core/Race.cpp
    src/core/Tile.cpp
    src/core/Map.cpp
    src/core/MapGenerator.cpp
    src/core/ResourceCatalog.cpp
    src/core/Settlement.cpp
    src/core/Building.cpp
    src/core/Technology.cpp
    src/core/Unit.cpp
    src/core/Hero.cpp
    src/core/Economy.cpp
    src/core/Military.cpp
    src/core/PoliticalAction.cpp
    src/core/TradeMarket.cpp
    src/core/Player.cpp
    src/core/GameState.cpp
    src/core/Army.cpp
    src/core/Combat.cpp
    src/core/GameEngine.cpp
    src/ai/AIController.cpp
    src/ai/AIStrategy.cpp
)

target_include_directories(playtest PRIVATE
    include
    src
)

target_compile_definitions(playtest PRIVATE
    ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)