#include "ai/CombatSimulator.hpp"

CombatPrediction CombatSimulator::predict(const std::vector<Unit> &attackers,
                                          const std::vector<Unit> &defenders,
                                          const std::vector<Hero> &attackerHeroes,
                                          const std::vector<Hero> &defenderHeroes,
                                          float defenderTerrainBonus,
                                          const std::vector<Building> &defenderBuildings,
                                          float attackerRaceModifier,
                                          float defenderRaceModifier)
{
    CombatPrediction pred;
    pred.attackerUnitsBefore = static_cast<int>(attackers.size());
    pred.defenderUnitsBefore = static_cast<int>(defenders.size());

    if (attackers.empty())
        return pred;

    if (defenders.empty())
    {
        // Пустой гарнизон/армия — бой не состоится, атакующий просто входит.
        pred.attackerWins = true;
        pred.attackerSurvivors = pred.attackerUnitsBefore;
        pred.attackerLossFraction = 0.0f;
        return pred;
    }

    // Copies passed by value into Combat::resolve — не трогает исходные армии/гарнизон.
    CombatResult result = Combat::resolve(attackers, defenders,
                                          attackerHeroes, defenderHeroes,
                                          defenderTerrainBonus, defenderBuildings,
                                          attackerRaceModifier, defenderRaceModifier);

    pred.attackerWins = result.attackerWon;
    pred.isDraw = result.isDraw;
    pred.attackerSurvivors = static_cast<int>(result.attackerSurvivors.size());
    pred.defenderSurvivors = static_cast<int>(result.defenderSurvivors.size());
    pred.attackerLossFraction =
        pred.attackerUnitsBefore > 0
            ? 1.0f - static_cast<float>(pred.attackerSurvivors) / static_cast<float>(pred.attackerUnitsBefore)
            : 0.0f;

    return pred;
}
