#pragma once

#include <vector>

#include "GameState.hpp"

class GameEngine
{
private:
    GameState m_state;

public:
    void nextTurn();
    bool checkVictoryCondition();
    GlobalPhase currentPhase() const;
};