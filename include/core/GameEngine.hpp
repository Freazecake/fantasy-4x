#pragma once

#include <vector>

#include "Map.hpp"
#include "GameEngine.hpp"
#include "Player.hpp"

enum class GamePhase
{
    Phase1,
    Phase2,
};

class GameEngine
{
private:
    std::vector<Player> m_players;
    Map m_map;
    int m_turnNumber = 0;

public:
    void nextTurn();                // применяет ход для всех игроков
    bool checkVictoryCondition();   // "остался один игрок"
    GamePhase currentPhase() const; // Phase1_Village, Phase2_State, ...
};