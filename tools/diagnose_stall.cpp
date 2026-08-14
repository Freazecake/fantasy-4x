// Диагностика таймаутов: для каждой партии в диапазоне сидов проверяет,
// доходит ли она до победителя за maxTurns; для первой найденной "зависшей"
// печатает состояние на нескольких контрольных ходах — сколько армий у
// каждого игрока, сколько городов, было ли движение армий за последние N
// ходов (по изменению позиций), чтобы понять, реально ли идёт борьба или
// всё замерло.
#include "core/GameEngine.hpp"
#include "core/StartLayout.hpp"

#include <iostream>
#include <map>
#include <set>
#include <string>

int main(int argc, char **argv)
{
    unsigned int baseSeed = argc > 1 ? static_cast<unsigned int>(std::atoi(argv[1])) : 5000u;
    int numGames = argc > 2 ? std::atoi(argv[2]) : 60;
    int maxTurns = argc > 3 ? std::atoi(argv[3]) : 200;

    for (int i = 0; i < numGames; ++i)
    {
        unsigned int seed = baseSeed + static_cast<unsigned int>(i);
        GameEngine engine;
        GameStartOptions options;
        if (!engine.loadFromAssets(ASSETS_DIR, options, seed))
            continue;
        for (auto &p : engine.state().players())
            p.setIsHuman(false);

        std::map<int, Coords> lastPositions; // "любая армия игрока" сигнатура позиции
        int turnsWithoutAnyMovement = 0;

        bool won = false;
        for (int turn = 1; turn <= maxTurns; ++turn)
        {
            std::map<int, size_t> posHashBefore;
            for (auto &pl : engine.state().players())
            {
                size_t h = 0;
                for (auto &a : pl.armies())
                    h = h * 1000003u + static_cast<size_t>(a.position().q * 1000 + a.position().r);
                posHashBefore[pl.id()] = h;
            }

            engine.endTurn();

            bool anyMoved = false;
            for (auto &pl : engine.state().players())
            {
                size_t h = 0;
                for (auto &a : pl.armies())
                    h = h * 1000003u + static_cast<size_t>(a.position().q * 1000 + a.position().r);
                if (h != posHashBefore[pl.id()])
                    anyMoved = true;
            }
            if (!anyMoved)
                ++turnsWithoutAnyMovement;
            else
                turnsWithoutAnyMovement = 0;

            auto v = engine.checkVictoryCondition();
            if (v.hasWinner())
            {
                won = true;
                break;
            }
        }

        if (!won)
        {
            std::cout << "=== ЗАВИСШАЯ ПАРТИЯ seed=" << seed << " ===\n";
            for (auto &pl : engine.state().players())
            {
                std::cout << "  игрок " << pl.id() << " (" << pl.race().getId() << "): "
                          << pl.settlements().size() << " городов, "
                          << pl.armies().size() << " армий, "
                          << "воен.сила=" << pl.militaryPower() << "\n";
                for (auto &a : pl.armies())
                    std::cout << "    армия @(" << a.position().q << "," << a.position().r
                              << ") юнитов=" << a.units().size()
                              << " ОД=" << a.movementPointsRemaining() << "/" << a.movementPointsMax()
                              << (a.isSettlerArmy() ? " [поселенец]" : "") << "\n";
                for (auto &s : pl.settlements())
                    std::cout << "    город: гарнизон=" << s.garrison().size() << "\n";
            }
            std::cout << "  ходов без движения армий подряд (в конце): " << turnsWithoutAnyMovement << "\n";
            return 0; // остановиться на первой найденной
        }
    }

    std::cout << "Зависших партий не найдено в этом диапазоне.\n";
    return 0;
}
