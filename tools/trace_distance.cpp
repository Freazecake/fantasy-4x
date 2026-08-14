#include "core/GameEngine.hpp"

#include <iostream>

int main(int argc, char **argv)
{
    unsigned int seed = argc > 1 ? static_cast<unsigned int>(std::atoi(argv[1])) : 5006u;
    int maxTurns = argc > 2 ? std::atoi(argv[2]) : 200;

    GameEngine engine;
    GameStartOptions options;
    if (!engine.loadFromAssets(ASSETS_DIR, options, seed))
        return 1;
    for (auto &p : engine.state().players())
        p.setIsHuman(false);

    for (int turn = 1; turn <= maxTurns; ++turn)
    {
        engine.endTurn();

        if (turn % 10 == 0 || turn <= 5)
        {
            std::cout << "ход " << turn << ":\n";
            for (auto &pl : engine.state().players())
            {
                if (pl.armies().empty())
                    continue;
                for (auto &a : pl.armies())
                {
                    // ближайший враг (город или армия)
                    int bestDist = -1;
                    for (auto &other : engine.state().players())
                    {
                        if (other.id() == pl.id())
                            continue;
                        for (auto &s : other.settlements())
                        {
                            if (s.tiles().empty())
                                continue;
                            int d = a.position().distanceTo(s.tiles().front().getCoords());
                            if (bestDist < 0 || d < bestDist)
                                bestDist = d;
                        }
                        for (auto &oa : other.armies())
                        {
                            int d = a.position().distanceTo(oa.position());
                            if (bestDist < 0 || d < bestDist)
                                bestDist = d;
                        }
                    }
                    std::cout << "  игрок " << pl.id() << " армия @(" << a.position().q << ","
                              << a.position().r << ") юнитов=" << a.units().size()
                              << " ОД=" << a.movementPointsRemaining() << "/" << a.movementPointsMax()
                              << " дист.до ближайшего врага=" << bestDist << "\n";
                }
            }
        }

        auto v = engine.checkVictoryCondition();
        if (v.hasWinner())
        {
            std::cout << "Победа на ходу " << turn << "\n";
            return 0;
        }
    }

    std::cout << "Таймаут\n";
    return 0;
}
