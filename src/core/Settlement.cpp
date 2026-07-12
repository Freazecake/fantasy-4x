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

void Settlement::seedInitialPopulation(int amount)
{
    if (amount < 0)
    {
        throw std::invalid_argument("Settlement::seedInitialPopulation: amount не может быть отрицательным");
    }
    m_populationCount += amount;
    m_population.idle += amount;
}

void Settlement::growPopulation(float baseGrowthRate, float raceGrowthModifier)
{
    int growth = static_cast<int>(m_populationCount * baseGrowthRate * raceGrowthModifier);
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