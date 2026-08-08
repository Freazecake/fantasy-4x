#include "core/Settlement.hpp"

#include <sstream>

Settlement::Settlement()
    : m_type(SettlementType::PreSettlement), m_ownerId(-1), m_populationCount(0)
{
}

Settlement::Settlement(int ownerId, SettlementType type)
    : m_type(type), m_ownerId(ownerId), m_populationCount(0)
{
}

void Settlement::addTile(const Tile &tile)
{
    m_tiles.push_back(tile);
}

Coords Settlement::anchorCoords() const
{
    if (m_tiles.empty())
        return Coords(-1, -1);

    Coords best = m_tiles.front().getCoords();
    for (const auto &t : m_tiles)
    {
        Coords c = t.getCoords();
        if (c.q < best.q || (c.q == best.q && c.r < best.r))
            best = c;
    }
    return best;
}

void Settlement::seedInitialPopulation(int amount)
{
    if (amount < 0)
    {
        throw std::invalid_argument("Settlement::seedInitialPopulation: amount не может быть отрицательным");
    }
    m_populationCount += amount;
    m_population.idle += amount;
}

void Settlement::growPopulation(float baseGrowthRate, float raceGrowthModifier, int extraFlatGrowth)
{
    float capFactor = 1.0f - static_cast<float>(m_populationCount) / static_cast<float>(POPULATION_SOFT_CAP);
    if (capFactor < 0.0f)
        capFactor = 0.0f;

    float rawGrowth = m_populationCount * baseGrowthRate * raceGrowthModifier + extraFlatGrowth;
    int growth = static_cast<int>(rawGrowth * capFactor);
    if (growth < 0)
        growth = 0;

    m_populationCount += growth;
    m_population.idle += growth;
}

void Settlement::assignFromIdle(Sphere sphere, int amount)
{
    if (amount < 0 || amount > m_population.idle)
    {
        throw std::invalid_argument("Settlement::assignFromIdle: недостаточно людей в idle");
    }
    m_population.idle -= amount;
    m_population.set(sphere, m_population.get(sphere) + amount);
}

void Settlement::unassignToIdle(Sphere sphere, int amount)
{
    int current = m_population.get(sphere);
    if (amount < 0 || amount > current)
    {
        throw std::invalid_argument("Settlement::unassignToIdle: недостаточно людей в указанной сфере");
    }
    m_population.set(sphere, current - amount);
    m_population.idle += amount;
}

void Settlement::reallocate(Sphere from, Sphere to, int amount)
{
    unassignToIdle(from, amount);
    assignFromIdle(to, amount);
}

void Settlement::removePopulationPermanently(int amount)
{
    if (amount < 0 || amount > m_population.idle)
    {
        throw std::invalid_argument(
            "Settlement::removePopulationPermanently: недостаточно людей в idle "
            "(освободите людей из сфер через unassignToIdle перед списанием)");
    }
    m_population.idle -= amount;
    m_populationCount -= amount;
}

void Settlement::sufferCaptureLoss(float fraction)
{
    int loss = static_cast<int>(m_populationCount * fraction);
    if (loss <= 0)
    {
        return;
    }
    int oldCount = m_populationCount;
    m_populationCount -= loss;
    float scale = static_cast<float>(m_populationCount) / static_cast<float>(oldCount);
    m_population.science = static_cast<int>(m_population.science * scale);
    m_population.industry = static_cast<int>(m_population.industry * scale);
    m_population.trade = static_cast<int>(m_population.trade * scale);
    m_population.diplomacy = static_cast<int>(m_population.diplomacy * scale);
    m_population.idle = m_populationCount - (m_population.science + m_population.industry +
                                             m_population.trade + m_population.diplomacy);
    if (m_population.idle < 0)
    {
        m_population.idle = 0;
    }
}

std::string Settlement::debugString() const
{
    std::ostringstream out;
    const char *typeNames[] = {"PreSettlement", "Village", "Town", "City", "State"};

    out << "Settlement(owner=" << m_ownerId
        << ", type=" << typeNames[static_cast<int>(m_type)]
        << ", tiles=" << m_tiles.size()
        << ", population=" << m_populationCount << ")\n"
        << "  allocation: science=" << m_population.science
        << " industry=" << m_population.industry
        << " trade=" << m_population.trade
        << " diplomacy=" << m_population.diplomacy
        << " idle=" << m_population.idle
        << " (total=" << m_population.total() << ")";

    return out.str();
}

void Settlement::queueBuilding(const Building &building)
{
    BuildingQueueItem item;
    item.building = building;
    m_buildingQueue.push_back(item);
}

BuildingProgressResult Settlement::progressBuildingQueue(int industryPoints)
{
    BuildingProgressResult result;
    int remaining = industryPoints;

    while (remaining > 0 && !m_buildingQueue.empty())
    {
        BuildingQueueItem &item = m_buildingQueue.front();
        int need = item.remainingCost();

        if (remaining < need)
        {
            item.investedIndustry += remaining;
            result.spentIndustry += remaining;
            remaining = 0;
        }
        else
        {
            remaining -= need;
            item.investedIndustry += need;
            result.spentIndustry += need;
            m_builtBuildings.push_back(item.building);
            result.completed.push_back(item.building);
            m_buildingQueue.erase(m_buildingQueue.begin());
        }
    }

    return result;
}

bool Settlement::hasBuilt(const std::string &buildingId) const
{
    for (const auto &b : m_builtBuildings)
    {
        if (b.getId() == buildingId)
            return true;
    }
    return false;
}

void Settlement::queueUnitRecruitment(const Unit &unit)
{
    RecruitmentItem item = Military::queueUnit(unit, m_builtBuildings);

    if (Military::hasUntrainedPenaltyRemoved(m_builtBuildings))
        item.unit.setTrained(true);

    if (unit.getPopulationCost() > m_population.idle)
    {
        throw std::runtime_error(
            "Settlement::queueUnitRecruitment: недостаточно свободного населения для найма '" +
            unit.getId() + "' (нужно " + std::to_string(unit.getPopulationCost()) +
            ", доступно " + std::to_string(m_population.idle) + ")");
    }
    removePopulationPermanently(unit.getPopulationCost());

    m_recruitmentQueue.push_back(item);
}

RecruitmentProgressResult Settlement::progressRecruitmentQueue(int industryPoints)
{
    RecruitmentProgressResult result;
    int remaining = industryPoints;

    while (remaining > 0 && !m_recruitmentQueue.empty())
    {
        RecruitmentItem &item = m_recruitmentQueue.front();
        int need = item.remainingCost();

        if (remaining < need)
        {
            item.investedPoints += remaining;
            result.spentIndustry += remaining;
            remaining = 0;
        }
        else
        {
            remaining -= need;
            item.investedPoints += need;
            result.spentIndustry += need;
            m_garrison.push_back(item.unit);
            result.completed.push_back(item.unit);
            m_recruitmentQueue.erase(m_recruitmentQueue.begin());
        }
    }

    return result;
}

std::vector<Unit> Settlement::removeFromGarrison(const std::vector<std::string> &unitIds)
{
    std::vector<Unit> removed;
    for (const auto &id : unitIds)
    {
        for (auto it = m_garrison.begin(); it != m_garrison.end(); ++it)
        {
            if (it->getId() == id)
            {
                removed.push_back(*it);
                m_garrison.erase(it);
                break;
            }
        }
    }
    return removed;
}

void Settlement::addToGarrison(const std::vector<Unit> &units)
{
    for (const auto &u : units)
        m_garrison.push_back(u);
}
