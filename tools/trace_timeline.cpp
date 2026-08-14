#include "core/GameEngine.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    unsigned int seed = argc > 1 ? static_cast<unsigned int>(std::atoi(argv[1])) : 5000u;
    int maxTurns = argc > 2 ? std::atoi(argv[2]) : 60;

    GameEngine engine;
    GameStartOptions options;
    engine.loadFromAssets(ASSETS_DIR, options, seed);
    for (auto &p : engine.state().players())
        p.setIsHuman(false);

    for (int turn = 1; turn <= maxTurns; ++turn)
    {
        engine.endTurn();

        for (const auto &ev : engine.state().turnEvents())
        {
            if (ev.type == TurnEventType::Combat || ev.type == TurnEventType::CityCaptured ||
                ev.type == TurnEventType::Absorbed || ev.type == TurnEventType::Vassalized ||
                ev.type == TurnEventType::AllianceFormed)
                std::cout << "ход " << turn << ": " << ev.description << "\n";
        }

        if (turn % 5 == 0)
        {
            std::cout << "  [срез] ";
            for (auto &p : engine.state().players())
                std::cout << p.race().getId() << "=" << p.settlements().size() << "г/"
                          << p.militaryPower() << "вс "; 
            std::cout << "\n";
        }

        auto v = engine.checkVictoryCondition();
        if (v.hasWinner())
        {
            std::cout << "=== Победа на ходу " << turn << ", тип=" << v.type << " ===\n";
            return 0;
        }
    }
    std::cout << "Не доиграно за " << maxTurns << "\n";
    return 0;
}
