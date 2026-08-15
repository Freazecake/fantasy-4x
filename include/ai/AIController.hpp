#pragma once

#include "AIStrategy.hpp"
#include "BotMemory.hpp"

class AIController
{
public:
    static void takeTurn(Player &bot, GameState &state, BotMemory &memory,
                         const std::vector<Building> &buildingCatalog,
                         const std::vector<Technology> &techCatalog,
                         const std::vector<Unit> &unitCatalog,
                         const std::vector<PoliticalAction> &politicalActionCatalog);

    static void observeEvents(Player &bot, const GameState &state, BotMemory &memory);

    static void decideAllocations(Settlement &s, GlobalPhase phase, BotGoal goal = BotGoal::Grow,
                                  RushSignal rushSignal = {});
    static void decideConstruction(Settlement &s, const std::vector<Building> &catalog, GlobalPhase phase,
                                   BotGoal goal = BotGoal::Grow);
    static void decideResearch(Player &p, const std::vector<Technology> &catalog, GlobalPhase phase);
    static void decideRecruitment(Settlement &s, const std::vector<Unit> &catalog, GlobalPhase phase,
                                  BotGoal goal = BotGoal::Grow);
    static void decidePoliticalAction(Player &p, const std::vector<PoliticalAction> &catalog, GameState &state,
                                      BotMemory &memory);
    static void decideMilitaryActions(Player &p, GameState &state, GlobalPhase phase, BotMemory &memory);
    static void decideDeployment(Player &p, GameState &state);
    static void decideTradeExchange(Player &p, GameState &state, GlobalPhase phase);
    static void decideSettlerActions(Player &p, GameState &state, GlobalPhase phase);

    static void maybeReassessGoal(Player &p, GameState &state, BotMemory &memory);

    static RushSignal detectRivalRush(const Player &p, const GameState &state);

    static bool worthAttacking(const Player &attacker, const Army &mine,
                               const Player &defenderOwner, const std::vector<Unit> &defenderUnits,
                               float defenderTerrainBonus, const std::vector<Building> &defenderBuildings,
                               bool isEliminationTarget, int turnNumber, bool isDefensive = false);

private:
    static void pursueNearestEnemy(Player &p, GameState &state, Army &mine,
                                   const std::string &armyId, const std::vector<Coords> &occupied);

    static bool tryRallyOrMerge(Player &p, GameState &state, const std::string &armyId,
                                const std::vector<Coords> &occupied);
};