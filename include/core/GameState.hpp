#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Player.hpp"
#include "Map.hpp"
#include "Army.hpp"
#include "Combat.hpp"

enum class GlobalPhase
{
    NotStarted,
    Phase2_OpenMap,
    Phase3_Final,
};

inline constexpr int PHASE2_TURN_THRESHOLD = 20;
inline constexpr int PHASE3_TURN_THRESHOLD = 50;
inline constexpr int PHASE3_MAX_ALIVE_PLAYERS = 3;

struct ActiveAlliance
{
    int playerA, playerB;
    int expiryTurn;
};

struct VictoryResult
{
    int winnerId = -1;
    std::string type;
    bool hasWinner() const { return winnerId >= 0; }
};

struct MoveResult
{
    bool success = false;
    bool combatOccurred = false;
    bool settlementCaptured = false;
    std::string errorMessage;
    CombatResult combat;
};

struct FoundSettlementResult
{
    bool success = false;
    std::string errorMessage;
};

inline constexpr int SETTLER_FOUNDED_INITIAL_POPULATION = 20;

class GameState
{
private:
    int m_turnNumber = 0;
    std::vector<Player> m_players;
    std::vector<ActiveAlliance> m_alliances;
    std::unordered_map<int, int> m_vassalizations;
    int m_politicalConquestWinnerId = -1;
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
    bool updatePhase();

    static bool isAllied(const GameState &state, int a, int b);
    void checkAlliances();
    void applyDemandTribute(int sourceId, int targetId);
    void applyVassalize(int masterId, int vassalId);
    void processVassalage();
    const std::unordered_map<int, int> &vassalizations() const { return m_vassalizations; }
    void applyPoliticalConquest(int conquerorId, int targetId);
    VictoryResult checkVictory() const;

    Army *findArmy(const std::string &id);
    MoveResult moveArmy(const std::string &armyId, const Coords &destination);
    CombatResult resolveMovementCombat(Army &attacker, const Coords &destination);

    bool canFoundSettlementAt(const Coords &center) const;
    FoundSettlementResult foundSettlementFromArmy(const std::string &armyId);

private:
    std::string settlementFoundingBlockReason(const Coords &center) const;
};
