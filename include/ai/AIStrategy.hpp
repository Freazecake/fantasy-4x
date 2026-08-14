#pragma once

#include <string>

#include "core/GameState.hpp"

class AIStrategy
{
public:
    static float scoreAllocatePopulation(const Settlement &s, Sphere target, GlobalPhase phase);
    static float scoreBuild(const Settlement &s, const Building &b, GlobalPhase phase);
    static float scoreResearch(const Player &p, const Technology &t, GlobalPhase phase);
    static float scoreRecruitUnit(const Settlement &s, const Unit &u, GlobalPhase phase);
    static float scoreTradeExchange(const Player &p, const std::string &targetResource, GlobalPhase phase);
    static float scorePoliticalAction(const Player &p, const PoliticalAction &a,
                                      const Player &target, GlobalPhase phase, float trust = 0.0f);
    static float scoreAttack(const Army &mine, const Army &enemyArmyOrGarrisonOwner,
                             GlobalPhase phase, int enemySettlementCount);
    static float scoreExpand(const Army &mine, const Coords &candidateDestination,
                             GlobalPhase phase, const Map &map);

private:
    static float phaseWeightMilitary(GlobalPhase phase);
    static float phaseWeightEconomy(GlobalPhase phase);
    static float phaseWeightDiplomacy(GlobalPhase phase);
};
