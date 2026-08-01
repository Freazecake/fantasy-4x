#pragma once

#include "AIStrategy.hpp"

class AIController
{
public:
    static void takeTurn(Player &bot, GameState &state,
                         const std::vector<Building> &buildingCatalog,
                         const std::vector<Technology> &techCatalog,
                         const std::vector<Unit> &unitCatalog,
                         const std::vector<PoliticalAction> &politicalActionCatalog);

    static void decideAllocations(Settlement &s, GlobalPhase phase);
    static void decideConstruction(Settlement &s, const std::vector<Building> &catalog, GlobalPhase phase);
    static void decideResearch(Player &p, const std::vector<Technology> &catalog, GlobalPhase phase);
    static void decideRecruitment(Settlement &s, const std::vector<Unit> &catalog, GlobalPhase phase);
    static void decidePoliticalAction(Player &p, const std::vector<PoliticalAction> &catalog, GameState &state);
    static void decideMilitaryActions(Player &p, GameState &state, GlobalPhase phase);
    static void decideTradeExchange(Player &p, GlobalPhase phase);
};