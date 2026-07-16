#pragma once

#include <vector>

#include "Player.hpp"
#include "Map.hpp"

enum class GlobalPhase
{
    NotStarted,
    Phase2_OpenMap,
    Phase3_Final,
};

class GameState
{
private:
    int m_turnNumber = 0;
    std::vector<Player> m_players;
    Map m_map;
    GlobalPhase m_globalPhase = GlobalPhase::NotStarted;

public:
    GlobalPhase CurPhase() { return m_globalPhase; }
    const std::vector<Player> &players() const { return m_players; }
    const Map &map() const { return m_map; }
    int turnNumber() const { return m_turnNumber; }
};