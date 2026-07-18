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

struct ActiveAlliance
{
    int playerA, playerB;
    int expiryTurn;
};

class GameState
{
private:
    int m_turnNumber = 0;
    std::vector<Player> m_players;
    std::vector<ActiveAlliance> m_alliances;
    Map m_map;
    GlobalPhase m_globalPhase = GlobalPhase::NotStarted;

public:
    GlobalPhase CurPhase() { return m_globalPhase; }
    const std::vector<Player> &players() const { return m_players; }
    std::vector<Player> &players() { return m_players; }
    std::vector<ActiveAlliance> &alliances() { return m_alliances; }
    const std::vector<ActiveAlliance> &alliances() const { return m_alliances; }
    const Map &map() const { return m_map; }
    int turnNumber() const { return m_turnNumber; }

    void addPlayer(const Player &player);
    void newAlliance(Player &player_1, Player &player_2, int expTurn);
    void setMap(const Map &map);
    void nextPhase() { m_globalPhase = static_cast<GlobalPhase>(static_cast<int>(m_globalPhase) + 1); }
    void nextTurn();

    void checkAlliances();
};
