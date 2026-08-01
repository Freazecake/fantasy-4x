#include "ai/AIStrategy.hpp"

#include <algorithm>
#include <cmath>

float AIStrategy::phaseWeightMilitary(GlobalPhase phase)
{
    switch (phase)
    {
    case GlobalPhase::NotStarted:
        return 0.6f;
    case GlobalPhase::Phase2_OpenMap:
        return 1.3f;
    case GlobalPhase::Phase3_Final:
        return 1.0f;
    }
    return 1.0f;
}

float AIStrategy::phaseWeightEconomy(GlobalPhase phase)
{
    switch (phase)
    {
    case GlobalPhase::NotStarted:
        return 1.0f;
    case GlobalPhase::Phase2_OpenMap:
        return 0.8f;
    case GlobalPhase::Phase3_Final:
        return 1.4f;
    }
    return 1.0f;
}

float AIStrategy::phaseWeightDiplomacy(GlobalPhase phase)
{
    switch (phase)
    {
    case GlobalPhase::NotStarted:
        return 0.4f;
    case GlobalPhase::Phase2_OpenMap:
        return 1.1f;
    case GlobalPhase::Phase3_Final:
        return 1.0f;
    }
    return 1.0f;
}

float AIStrategy::scoreAllocatePopulation(const Settlement &s, Sphere target, GlobalPhase phase)
{
    if (target == Sphere::Military || target == Sphere::Growth)
        return -1.0f;

    const auto &alloc = s.allocation();
    int currentInSphere = alloc.get(target);
    int assignedTotal = alloc.total() - alloc.idle;

    float share = assignedTotal > 0 ? static_cast<float>(currentInSphere) / static_cast<float>(assignedTotal) : 0.0f;
    float diversityBonus = 1.0f - share;

    float idleBonus = alloc.idle > 0 ? 1.0f : 0.3f;

    float phaseWeight = 1.0f;
    switch (target)
    {
    case Sphere::Science:
    case Sphere::Trade:
        phaseWeight = phaseWeightEconomy(phase);
        break;
    case Sphere::Diplomacy:
        phaseWeight = phaseWeightDiplomacy(phase);
        break;
    case Sphere::Industry:
        phaseWeight = 1.1f;
        break;
    default:
        break;
    }

    return diversityBonus * idleBonus * phaseWeight;
}

float AIStrategy::scoreBuild(const Settlement &s, const Building &b, GlobalPhase phase)
{
    if (s.hasBuilt(b.getId()))
        return -1.0f;

    const auto &mods = b.getModifiers();
    const auto &alloc = s.allocation();

    float economicValue =
        (mods.science - 1.0f) * static_cast<float>(alloc.science) * phaseWeightEconomy(phase) +
        (mods.trade - 1.0f) * static_cast<float>(alloc.trade) * phaseWeightEconomy(phase) +
        (mods.diplomacy - 1.0f) * static_cast<float>(alloc.diplomacy) * phaseWeightDiplomacy(phase) +
        (mods.industry - 1.0f) * static_cast<float>(alloc.industry);

    float militaryValue = (mods.military - 1.0f) * phaseWeightMilitary(phase) * 5.0f;

    float unlockBonus = 0.0f;
    if (!b.getUnlocksUnitIds().empty())
        unlockBonus += 2.0f * phaseWeightMilitary(phase);
    if (b.removesUntrainedPenalty())
        unlockBonus += 1.5f * phaseWeightMilitary(phase);

    float rawValue = economicValue + militaryValue + unlockBonus;
    int cost = std::max(1, b.getCost());

    return rawValue / static_cast<float>(cost);
}

float AIStrategy::scoreResearch(const Player &p, const Technology &t, GlobalPhase phase)
{
    if (t.isResearched())
        return -1.0f;

    if (!t.canResearch(p.researchedTechIds()))
        return -1.0f;

    const auto &mods = t.getModifiers();
    float boostValue = (mods.science - 1.0f) + (mods.industry - 1.0f) +
                       (mods.trade - 1.0f) + (mods.diplomacy - 1.0f) +
                       (mods.military - 1.0f) * phaseWeightMilitary(phase);

    float unlockBonus = static_cast<float>(t.getUnloksBuildingsIds().size()) * 0.5f;

    int cost = std::max(1, t.remainingCost());

    return (boostValue * 3.0f + unlockBonus) * phaseWeightEconomy(phase) / static_cast<float>(cost);
}

float AIStrategy::scoreRecruitUnit(const Settlement &s, const Unit &u, GlobalPhase phase)
{
    if (u.requiresBuilding() && !s.hasBuilt(u.getRequiredBuildingId()))
        return -1.0f;

    if (u.getPopulationCost() > s.allocation().idle)
        return -1.0f;

    if (u.isSettler())
    {
        float desire;
        switch (phase)
        {
        case GlobalPhase::NotStarted:
            desire = 1.5f;
            break;
        case GlobalPhase::Phase2_OpenMap:
            desire = 1.2f;
            break;
        case GlobalPhase::Phase3_Final:
        default:
            desire = 0.3f;
            break;
        }
        int cost = std::max(1, u.getRecruitCost());
        return (desire * 20.0f) / static_cast<float>(cost);
    }

    const auto &stats = u.getStats();
    float power = stats.attack + stats.defense + stats.health * 0.1f;

    int cost = std::max(1, u.getRecruitCost());
    float tierBonus = 1.0f + static_cast<float>(u.getTier()) * 0.2f; // юниты старших tier ценнее при прочих равных

    return (power * tierBonus / static_cast<float>(cost)) * phaseWeightMilitary(phase);
}

float AIStrategy::scoreTradeExchange(const Player &p, const std::string &targetResource, GlobalPhase phase)
{
    const auto &r = p.resources();
    if (r.trade < 1.0f)
        return -1.0f;

    float targetAmount;
    float phaseWeight;
    if (targetResource == "science")
    {
        targetAmount = r.science;
        phaseWeight = phaseWeightEconomy(phase);
    }
    else if (targetResource == "industry")
    {
        targetAmount = r.industry;
        phaseWeight = 1.0f;
    }
    else if (targetResource == "diplomacy")
    {
        targetAmount = r.diplomacy;
        phaseWeight = phaseWeightDiplomacy(phase);
    }
    else
    {
        return -1.0f;
    }

    float totalOther = r.science + r.industry + r.diplomacy;
    float deficitBonus = totalOther > 0.0f ? 1.0f - (targetAmount / totalOther) : 1.0f;
    deficitBonus = std::max(0.0f, deficitBonus);

    float rate = p.effectiveTradeMultiplier();

    return deficitBonus * rate * phaseWeight;
}

float AIStrategy::scorePoliticalAction(const Player &p, const PoliticalAction &a,
                                       const Player &target, GlobalPhase phase)
{
    if (p.resources().diplomacy < static_cast<float>(a.getCost()))
        return -1.0f;

    float base = 1.0f;
    switch (a.getType())
    {
    case PoliticalActionType::PoliticalConquest:
        base = target.resources().diplomacy < p.resources().diplomacy * 0.7f ? 3.0f : 0.2f;
        break;
    case PoliticalActionType::Vassalize:
        base = 2.0f;
        break;
    case PoliticalActionType::DemandTribute:
        base = 1.2f;
        break;
    case PoliticalActionType::Alliance:
        base = 1.0f;
        break;
    }

    return base * phaseWeightDiplomacy(phase);
}

float AIStrategy::scoreAttack(const Army &mine, const Army &enemyArmyOrGarrisonOwner, GlobalPhase phase)
{
    if (mine.units().empty())
        return -1.0f;

    if (enemyArmyOrGarrisonOwner.units().empty())
        return 5.0f * phaseWeightMilitary(phase);

    float myPower = 0.0f, enemyPower = 0.0f;
    for (const auto &u : mine.units())
        myPower += u.getStats().attack + u.getStats().defense + u.getCurrentHealth() * 0.1f;
    for (const auto &u : enemyArmyOrGarrisonOwner.units())
        enemyPower += u.getStats().attack + u.getStats().defense + u.getCurrentHealth() * 0.1f;

    if (enemyPower <= 0.0f)
        return 5.0f * phaseWeightMilitary(phase);

    float ratio = myPower / enemyPower;

    float advantage = ratio - 1.3f;

    int distance = mine.position().distanceTo(enemyArmyOrGarrisonOwner.position());
    float distancePenalty = 1.0f / (1.0f + static_cast<float>(distance) * 0.3f);

    return advantage * phaseWeightMilitary(phase) * distancePenalty;
}

float AIStrategy::scoreExpand(const Army &mine, const Coords &candidateDestination,
                              GlobalPhase phase, const Map &map)
{
    if (mine.movementPointsRemaining() <= 0)
        return -1.0f;
    if (mine.position().q == candidateDestination.q && mine.position().r == candidateDestination.r)
        return -1.0f;

    const Tile *dest = map.tileAt(candidateDestination.q, candidateDestination.r);
    if (!dest || dest->getType() == TileType::Water)
        return -1.0f;

    const int LONG_TERM_BUDGET = 50;
    int cost = map.movementCostTo(mine.position(), candidateDestination, LONG_TERM_BUDGET);
    if (cost < 0)
        return -1.0f;

    bool reachableNow = cost <= mine.movementPointsRemaining();
    float reachPenalty = 1.0f / (1.0f + static_cast<float>(cost) * 0.4f);
    if (!reachableNow)
        reachPenalty *= 0.5f;

    float terrainBonus = 1.0f;
    switch (dest->getType())
    {
    case TileType::Grass:
    case TileType::Desert:
        terrainBonus = 1.2f;
        break;
    case TileType::Forest:
        terrainBonus = 1.0f;
        break;
    case TileType::Mountain:
        terrainBonus = 0.8f;
        break;
    default:
        break;
    }

    float resourceBonus = dest->getResource() != ResourceType::None ? 1.5f : 1.0f;

    return reachPenalty * terrainBonus * resourceBonus * phaseWeightMilitary(phase);
}