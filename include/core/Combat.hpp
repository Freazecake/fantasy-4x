#pragma once

#include "Military.hpp"

struct CombatRoundLog
{
    int roundNumber;
    std::string attackerUnitId, defenderUnitId;
    float damageDealt;
    bool defenderUnitDied;
};

struct CombatResult
{
    bool attackerWon = false;
    bool isDraw = false;
    bool settlementCaptured = false;
    std::vector<Unit> attackerSurvivors;
    std::vector<Unit> defenderSurvivors;
    std::vector<CombatRoundLog> log;
};

class Combat
{
public:
    static CombatResult resolve(std::vector<Unit> attackers,
                                std::vector<Unit> defenders,
                                std::vector<Hero> attackerHeroes,
                                std::vector<Hero> defenderHeroes,
                                float defenderTerrainBonus,
                                const std::vector<Building> &builtBuildings,
                                float attackerRaceModifier,
                                float defenderRaceModifier);
};