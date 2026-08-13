// Диагностический GUI-репро краша с армиями-поселенцами.
// Собирается отдельным таргетом (EXCLUDE_FROM_ALL): cmake --build build --target gui_repro
//
// ВАЖНО про #define private public: макрос определён ТОЛЬКО вокруг инклуда
// заголовков проекта. Все STL-заголовки, которые тянет json.hpp (и Qt), обязаны
// быть включены ДО макроса, иначе они компилируются под макросом и потом
// переопределяются ("redeclared with different access" в <sstream>/<any>).

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <any>
#include <variant>
#include <optional>
#include <tuple>
#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>
#include <exception>
#include <type_traits>
#include <initializer_list>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <atomic>
#include <new>
#include <locale>
#include <charconv>
#include <numeric>
#include <deque>
#include <queue>
#include <random>

#include <QApplication>

#define private public
#include "gui/MainWindow.hpp"
#include "gui/SettlementPanel.hpp"
#undef private

static const Unit *findSettler(const std::vector<Unit> &catalog)
{
    for (const auto &u : catalog)
        if (u.getId() == "settler")
            return &u;
    return nullptr;
}

static void freeAllIdle(Settlement &s)
{
    const auto &a = s.allocation();
    s.unassignToIdle(Sphere::Science, a.science);
    s.unassignToIdle(Sphere::Industry, a.industry);
    s.unassignToIdle(Sphere::Trade, a.trade);
    s.unassignToIdle(Sphere::Diplomacy, a.diplomacy);
}

static void recruitSettler(Player &human, const Unit &settler)
{
    freeAllIdle(human.settlements()[0]);
    human.settlements()[0].queueUnitRecruitment(settler);
    human.settlements()[0].addToGarrison({settler});
}

// Сценарий 1: два поселенца -> две армии (сценарий из отчёта пользователя).
static bool scenarioTwoSettlerArmies(MainWindow &w)
{
    GameEngine &eng = w.m_engine;
    Player &human = eng.state().players()[0];
    const Unit *settler = findSettler(eng.unitCatalog());
    if (!settler)
    {
        std::cout << "[1] no settler unit in catalog\n";
        return false;
    }

    recruitSettler(human, *settler);
    w.refreshAllPanels();
    w.onMarchRequested();
    std::cout << "[1] armies after march1: " << human.armies().size() << "\n";

    recruitSettler(human, *settler);
    w.refreshAllPanels();
    w.onMarchRequested();
    std::cout << "[1] armies after march2: " << human.armies().size() << "\n";

    return human.armies().size() == 2;
}

// Сценарий 2: у игрока 0 поселений (все захвачены ИИ). Панель поселения
// обязана очиститься (clearSettlement), а не хранить протухший указатель на
// элемент std::vector<Settlement>. Клики "Выступить в поход"/"Нанять" не должны
// валить приложение.
static bool scenarioEmptySettlementsNoCrash(MainWindow &w)
{
    GameEngine &eng = w.m_engine;
    Player &human = eng.state().players()[0];

    human.settlements().clear(); // имитация захвата всех поселений
    w.refreshAllPanels();

    w.onMarchRequested();
    w.m_settlementPanel->onQueueRecruitment();
    w.m_settlementPanel->onMarch();
    w.m_settlementPanel->onApplyAllocation();

    return w.m_settlementPanel->currentSettlement() == nullptr;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    int failures = 0;

    MainWindow w;
    w.show();
    w.onNewGame();

    if (!scenarioTwoSettlerArmies(w))
    {
        std::cout << "[1] FAILED\n";
        ++failures;
    }
    else
        std::cout << "[1] OK\n";

    w.onNewGame(); // свежее состояние

    if (!scenarioEmptySettlementsNoCrash(w))
    {
        std::cout << "[2] FAILED\n";
        ++failures;
    }
    else
        std::cout << "[2] OK\n";

    if (failures)
    {
        std::cout << "GUI-REPRO: " << failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "GUI-REPRO: all scenarios passed\n";
    return 0;
}
