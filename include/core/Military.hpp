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

// Military — набор правил найма/боевых характеристик, без собственного
// состояния (все методы static). Состояние (очередь, гарнизон) живёт в
// Settlement — см. Settlement::queueUnitRecruitment/progressRecruitmentQueue.
class Military
{
public:
    static bool canRecruit(const Unit &unit, const std::vector<Building> &builtBuildings);
    static RecruitmentItem queueUnit(const Unit &unit, const std::vector<Building> &builtBuildings);
    static constexpr float UNTRAINED_PENALTY_MULTIPLIER = 0.7f;
    static UnitStats effectiveStats(const Unit &unit,
                                    const std::vector<Building> &builtBuildings,
                                    float raceMilitaryModifier,
                                    const std::vector<Hero> &heroes);

private:
    static bool hasUntrainedPenaltyRemoved(const std::vector<Building> &builtBuildings);
};
