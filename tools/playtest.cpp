// Блок 3.5 роадмапа: прогон N партий подряд (все игроки — AIController,
// включая слот "человека") с фиксированными seed-ами карты, чтобы получить
// численную оценку баланса: длительность партий, доля военных/политических
// побед, размер державы победителя, доля "зависших" партий (нет победителя
// за MAX_TURNS ходов).
//
// Запуск:
//   ./playtest [numGames] [maxTurns] [baseSeed]
// Все параметры необязательны (значения по умолчанию: 30 партий, 200 ходов,
// seed 1000). Отчёт печатается в консоль.

#include "core/GameEngine.hpp"
#include "core/StartLayout.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace
{

struct GameStats
{
    int turns = 0;
    std::string victoryType; // "military" | "political" | "timeout"
    int winnerId = -1;
    int winnerSettlementCount = 0;
    int aliveAtTimeout = -1;
};

GameStats playOneGame(unsigned int mapSeed, int maxTurns, int mapWidth, int mapHeight)
{
    GameEngine engine;
    GameStats stats;

    GameStartOptions options;
    options.mapWidth = mapWidth;
    options.mapHeight = mapHeight;

    if (!engine.loadFromAssets(ASSETS_DIR, options, mapSeed))
    {
        stats.victoryType = "load_error";
        return stats;
    }

    // Playtest не различает "человека" — все 5 слотов ведёт ИИ, чтобы
    // партии были воспроизводимы и не требовали ручного ввода.
    for (auto &p : engine.state().players())
        p.setIsHuman(false);

    for (int turn = 1; turn <= maxTurns; ++turn)
    {
        engine.endTurn();

        VictoryResult v = engine.checkVictoryCondition();
        if (v.hasWinner())
        {
            stats.turns = turn;
            stats.victoryType = v.type;
            stats.winnerId = v.winnerId;
            for (const auto &p : engine.state().players())
                if (p.id() == v.winnerId)
                    stats.winnerSettlementCount = static_cast<int>(p.settlements().size());
            return stats;
        }
    }

    stats.turns = maxTurns;
    stats.victoryType = "timeout";

    int aliveCount = 0;
    for (const auto &p : engine.state().players())
    {
        if (!p.settlements().empty())
            ++aliveCount;
    }
    stats.aliveAtTimeout = aliveCount;

    return stats;
}

double median(std::vector<int> values)
{
    if (values.empty())
        return 0.0;
    std::sort(values.begin(), values.end());
    size_t n = values.size();
    if (n % 2 == 1)
        return static_cast<double>(values[n / 2]);
    return (values[n / 2 - 1] + values[n / 2]) / 2.0;
}

} // namespace

int main(int argc, char **argv)
{
    int numGames = argc > 1 ? std::atoi(argv[1]) : 30;
    int maxTurns = argc > 2 ? std::atoi(argv[2]) : 200;
    unsigned int baseSeed = argc > 3 ? static_cast<unsigned int>(std::atoi(argv[3])) : 1000u;
    int mapWidth = argc > 4 ? std::atoi(argv[4]) : StartLayout::MAP_WIDTH;
    int mapHeight = argc > 5 ? std::atoi(argv[5]) : StartLayout::MAP_HEIGHT;

    std::vector<GameStats> results;
    results.reserve(numGames);

    std::cout << "Прогон " << numGames << " партий (maxTurns=" << maxTurns
              << ", baseSeed=" << baseSeed << ", карта=" << mapWidth << "x" << mapHeight << ")...\n";

    auto startTime = std::chrono::steady_clock::now();

    for (int i = 0; i < numGames; ++i)
    {
        GameStats stats = playOneGame(baseSeed + static_cast<unsigned int>(i), maxTurns,
                                      mapWidth, mapHeight);
        results.push_back(stats);
        std::cout << "  партия " << (i + 1) << "/" << numGames
                  << ": ход=" << stats.turns
                  << " тип=" << stats.victoryType;
        if (stats.winnerId >= 0)
            std::cout << " победитель=" << stats.winnerId
                       << " городов=" << stats.winnerSettlementCount;
        if (stats.aliveAtTimeout >= 0)
            std::cout << " живых_игроков=" << stats.aliveAtTimeout;
        std::cout << "\n";
    }

    int militaryWins = 0, politicalWins = 0, timeouts = 0, loadErrors = 0;
    std::vector<int> finishedTurns;
    std::vector<int> winnerSettlementCounts;

    for (const auto &r : results)
    {
        if (r.victoryType == "military")
        {
            ++militaryWins;
            finishedTurns.push_back(r.turns);
            winnerSettlementCounts.push_back(r.winnerSettlementCount);
        }
        else if (r.victoryType == "political")
        {
            ++politicalWins;
            finishedTurns.push_back(r.turns);
            winnerSettlementCounts.push_back(r.winnerSettlementCount);
        }
        else if (r.victoryType == "timeout")
        {
            ++timeouts;
        }
        else
        {
            ++loadErrors;
        }
    }

    double avgTurns = finishedTurns.empty()
                           ? 0.0
                           : std::accumulate(finishedTurns.begin(), finishedTurns.end(), 0.0) /
                                 finishedTurns.size();
    double medTurns = median(finishedTurns);

    double avgWinnerSettlements =
        winnerSettlementCounts.empty()
            ? 0.0
            : std::accumulate(winnerSettlementCounts.begin(), winnerSettlementCounts.end(), 0.0) /
                  winnerSettlementCounts.size();

    int finished = militaryWins + politicalWins;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - startTime)
                        .count();

    std::cout << "\n========== ОТЧЁТ О БАЛАНСЕ ==========\n";
    std::cout << "Всего партий:              " << numGames << "\n";
    std::cout << "Завершились победой:       " << finished
              << " (" << std::fixed << std::setprecision(1)
              << (numGames > 0 ? 100.0 * finished / numGames : 0.0) << "%)\n";
    std::cout << "  из них военных побед:    " << militaryWins
              << " (" << (finished > 0 ? 100.0 * militaryWins / finished : 0.0) << "%)\n";
    std::cout << "  из них политических:     " << politicalWins
              << " (" << (finished > 0 ? 100.0 * politicalWins / finished : 0.0) << "%)\n";
    std::cout << "Застряли (таймаут):        " << timeouts
              << " (" << (numGames > 0 ? 100.0 * timeouts / numGames : 0.0) << "%)\n";
    if (loadErrors > 0)
        std::cout << "Ошибок загрузки assets:    " << loadErrors << "\n";
    std::cout << "Средняя длительность:      " << std::setprecision(1) << avgTurns << " ходов\n";
    std::cout << "Медианная длительность:    " << medTurns << " ходов\n";
    std::cout << "Средний размер победителя: " << avgWinnerSettlements << " городов\n";
    std::cout << "Время прогона:             " << elapsed << " мс (" << std::fixed
              << std::setprecision(1) << (numGames > 0 ? static_cast<double>(elapsed) / numGames : 0.0)
              << " мс/партия)\n";
    std::cout << "======================================\n";

    return 0;
}
