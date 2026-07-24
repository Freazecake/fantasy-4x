#pragma once

#include <vector>

#include "Unit.hpp"
#include "Building.hpp"
#include "Hero.hpp"

struct RecruitmentItem
{
    Unit unit;
    int investedPoints = 0;

    int remainingCost() const
    {
        int need = unit.getRecruitCost() - investedPoints;
        return need > 0 ? need : 0;
    }
};

class Military
{
public:
    static bool canRecruit(const Unit &unit, const std::vector<Building> &builtBuildings);
    static RecruitmentItem queueUnit(const Unit &unit, const std::vector<Building> &builtBuildings);
    static constexpr float UNTRAINED_PENALTY_MULTIPLIER = 0.7f;
    static bool hasUntrainedPenaltyRemoved(const std::vector<Building> &builtBuildings);
    static UnitStats effectiveStats(const Unit &unit,
                                    const std::vector<Building> &builtBuildings,
                                    float raceMilitaryModifier,
                                    const std::vector<Hero> &heroes);
};
