#pragma once

#include "core/Combat.hpp"
#include "core/Unit.hpp"
#include "core/Hero.hpp"
#include "core/Building.hpp"

#include <vector>

// Combat::resolve детерминирован (без случайности, см. src/core/Combat.cpp) и
// принимает юниты ПО ЗНАЧЕНИЮ — то есть он уже безопасен для "сухого прогона"
// без побочных эффектов на реальную армию/гарнизон. CombatSimulator просто
// даёт ИИ удобный, читаемый интерфейс поверх него: не "а что если", а точный
// прогноз результата боя, который дальше используют военные score-функции,
// чтобы не гадать по грубой эвристике (сумма атаки/защиты), а знать заранее,
// кто выживет и какой ценой.
struct CombatPrediction
{
    bool attackerWins = false;
    bool isDraw = false;
    int attackerUnitsBefore = 0;
    int defenderUnitsBefore = 0;
    int attackerSurvivors = 0;
    int defenderSurvivors = 0;
    // Доля УНИЧТОЖЕННЫХ юнитов атакующего (0 = никого не потерял, 1 = вся армия легла).
    float attackerLossFraction = 0.0f;
};

class CombatSimulator
{
public:
    // defenderBuildings пустой для полевого боя (армия vs армия), непустой —
    // для осады города (гарнизон получает бонусы построек), см.
    // GameState::resolveMovementCombat, который CombatSimulator зеркалит.
    static CombatPrediction predict(const std::vector<Unit> &attackers,
                                    const std::vector<Unit> &defenders,
                                    const std::vector<Hero> &attackerHeroes,
                                    const std::vector<Hero> &defenderHeroes,
                                    float defenderTerrainBonus,
                                    const std::vector<Building> &defenderBuildings,
                                    float attackerRaceModifier,
                                    float defenderRaceModifier);
};
