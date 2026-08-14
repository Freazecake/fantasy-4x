#include "core/GameEngine.hpp"
#include "core/StartLayout.hpp"

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>

namespace
{
const std::array<std::string, 5> kBaseOrder = {"humans", "dwarves", "gnomes", "elves", "orcs"};

struct RunResult
{
    bool hasWinner = false;
    int winnerPosition = -1;
    std::string winnerRace;
};

RunResult playOneGame(const std::vector<std::string> &raceOrder, unsigned int mapSeed, int maxTurns)
{
    GameEngine engine;
    RunResult result;

    GameStartOptions options;
    options.numBots = 4;
    options.raceIdsByPosition = raceOrder;

    if (!engine.loadFromAssets(ASSETS_DIR, options, mapSeed))
        return result;

    for (auto &p : engine.state().players())
        p.setIsHuman(false);

    for (int turn = 1; turn <= maxTurns; ++turn)
    {
        engine.endTurn();
        auto v = engine.checkVictoryCondition();
        if (v.hasWinner())
        {
            result.hasWinner = true;
            result.winnerPosition = v.winnerId;
            result.winnerRace = raceOrder[v.winnerId];
            return result;
        }
    }
    return result;
}
}

int main(int argc, char **argv)
{
    int gamesPerRotation = argc > 1 ? std::atoi(argv[1]) : 40;
    int maxTurns = argc > 2 ? std::atoi(argv[2]) : 200;
    unsigned int baseSeed = argc > 3 ? static_cast<unsigned int>(std::atoi(argv[3])) : 5000u;

    std::map<std::string, int> winsByRace;
    std::map<int, int> winsByPosition;
    int total = 0, withWinner = 0;

    auto start = std::chrono::steady_clock::now();

    for (int rotation = 0; rotation < 5; ++rotation)
    {
        std::vector<std::string> order(5);
        for (int i = 0; i < 5; ++i)
            order[i] = kBaseOrder[(i + rotation) % 5];

        for (int g = 0; g < gamesPerRotation; ++g)
        {
            unsigned int seed = baseSeed + static_cast<unsigned int>(rotation * 10000 + g);
            RunResult r = playOneGame(order, seed, maxTurns);
            ++total;
            if (r.hasWinner)
            {
                ++withWinner;
                ++winsByRace[r.winnerRace];
                ++winsByPosition[r.winnerPosition];
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "\n========== ПОБЕДЫ ПО РАСЕ ==========\n";
    for (const auto &r : kBaseOrder)
    {
        int wins = winsByRace.count(r) ? winsByRace[r] : 0;
        double pct = withWinner > 0 ? 100.0 * wins / withWinner : 0.0;
        std::cout << std::left << std::setw(10) << r << wins << " побед  ("
                  << std::fixed << std::setprecision(1) << pct << "%)\n";
    }

    std::cout << "\n========== ПОБЕДЫ ПО ПОЗИЦИИ ==========\n";
    for (int pos = 0; pos < 5; ++pos)
    {
        int wins = winsByPosition.count(pos) ? winsByPosition[pos] : 0;
        double pct = withWinner > 0 ? 100.0 * wins / withWinner : 0.0;
        std::cout << "позиция " << pos << ": " << wins << " побед  ("
                  << std::fixed << std::setprecision(1) << pct << "%)\n";
    }

    std::cout << "\nВсего: " << total << ", с победителем: " << withWinner
              << " (" << (100.0 * withWinner / total) << "%)\n";
    std::cout << "Время: " << static_cast<int>(ms) << " мс\n";
    return 0;
}
