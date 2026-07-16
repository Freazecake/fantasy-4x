#include "core/Military.hpp"

#include <algorithm>
#include <stdexcept>

bool Military::hasUntrainedPenaltyRemoved(const std::vector<Building> &builtBuildings)
{
    for (const auto &b : builtBuildings)
    {
        if (b.removesUntrainedPenalty())
            return true;
    }
    return false;
}

bool Military::canRecruit(const Unit &unit, const std::vector<Building> &builtBuildings)
{
    if (!unit.requiresBuilding())
        return true;

    for (const auto &b : builtBuildings)
    {
        const auto &unlocked = b.getUnlocksUnitIds();
        if (std::find(unlocked.begin(), unlocked.end(), unit.getId()) != unlocked.end())
            return true;
    }
    return false;
}

RecruitmentItem Military::queueUnit(const Unit &unit, const std::vector<Building> &builtBuildings)
{
    if (!canRecruit(unit, builtBuildings))
    {
        throw std::invalid_argument("Military::queueUnit: '" + unit.getId() +
                                     "' требует здание (" + unit.getRequiredBuildingId() +
                                     "), которого нет среди построек этого поселения");
    }

    RecruitmentItem item;
    item.unit = unit;
    return item;
}

UnitStats Military::effectiveStats(const Unit &unit,
                                    const std::vector<Building> &builtBuildings,
                                    float raceMilitaryModifier,
                                    const std::vector<Hero> &heroes)
{
    float militaryMultiplier = raceMilitaryModifier;
    for (const auto &hero : heroes)
        militaryMultiplier *= hero.getModifiers().military;
    for (const auto &building : builtBuildings)
        militaryMultiplier *= building.getModifiers().military;

    if (!hasUntrainedPenaltyRemoved(builtBuildings))
        militaryMultiplier *= UNTRAINED_PENALTY_MULTIPLIER;

    UnitStats stats = unit.getStats();
    stats.attack *= militaryMultiplier;
    stats.defense *= militaryMultiplier;
    stats.health *= militaryMultiplier;
    return stats;
}
