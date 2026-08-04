#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Player.hpp"
#include "Map.hpp"
#include "Army.hpp"
#include "Combat.hpp"
#include "external/json.hpp"

using json = nlohmann::json;

enum class GlobalPhase
{
    NotStarted,
    Phase2_OpenMap,
    Phase3_Final,
};

inline constexpr int PHASE2_TURN_THRESHOLD = 20;
inline constexpr int PHASE3_TURN_THRESHOLD = 35;
inline constexpr int PHASE3_MAX_ALIVE_PLAYERS = 3;

inline constexpr int POLITICAL_CONQUEST_BASE_COST = 200;
inline constexpr int POLITICAL_CONQUEST_COST_PER_EXTRA_SETTLEMENT = 150;
inline constexpr float POLITICAL_CONQUEST_STOCKPILE_CAP = 150.0f;
inline constexpr float TRIBUTE_STOCKPILE_CAP = 60.0f;

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
    bool canPoliticalConquer(int conquerorId, int targetId) const;
    int politicalConquestCost(int conquerorId, int targetId) const;
    bool applyPoliticalConquest(int conquerorId, int targetId);
    VictoryResult checkVictory() const;

    Army *findArmy(const std::string &id);
    MoveResult moveArmy(const std::string &armyId, const Coords &destination);
    void recomputeVisibility(int visionRadius = FOG_OF_WAR_VISION_RADIUS);
    CombatResult resolveMovementCombat(Army &attacker, const Coords &destination);

    bool canFoundSettlementAt(const Coords &center) const;
    FoundSettlementResult foundSettlementFromArmy(const std::string &armyId);

    json toJson() const;
    static GameState fromJson(const json &j,
                              const std::vector<Race> &raceCatalog,
                              const std::vector<Hero> &heroCatalog,
                              const std::vector<Technology> &techCatalog,
                              const std::vector<Building> &buildingCatalog,
                              const std::vector<Unit> &unitCatalog);

    void setTurnNumber(int turn) { m_turnNumber = turn; }
    void setPhase(GlobalPhase phase) { m_globalPhase = phase; }
    void setVassalizations(const std::unordered_map<int, int> &v) { m_vassalizations = v; }
    void setPoliticalConquestWinnerId(int id) { m_politicalConquestWinnerId = id; }

private:
    std::string settlementFoundingBlockReason(const Coords &center) const;
};
