#pragma once

#include <vector>

#include "Tile.hpp"
#include "Common.hpp"

struct PopulationAllocation
{
    int science = 0;
    int industry = 0;
    int trade = 0;
    int diplomacy = 0;
    int idle = 0;

    int total() const { return science + industry + trade + diplomacy + idle; }

    int get(Sphere s) const
    {
        switch (s)
        {
        case Sphere::Science:
            return science;
        case Sphere::Industry:
            return industry;
        case Sphere::Trade:
            return trade;
        case Sphere::Diplomacy:
            return diplomacy;
        }
        return 0;
    }

    void set(Sphere s, int value)
    {
        switch (s)
        {
        case Sphere::Science:
            science = value;
            break;
        case Sphere::Industry:
            industry = value;
            break;
        case Sphere::Trade:
            trade = value;
            break;
        case Sphere::Diplomacy:
            diplomacy = value;
            break;
        }
    }
};

enum class SettlementType
{
    PreSettlement,
    Village,
    Town,
    City,
    Capital,
};

class Settlement
{
private:
    SettlementType m_type = SettlementType::PreSettlement;
    std::vector<Tile> m_tiles;
    int m_ownerId;
    int m_populationCount;
    PopulationAllocation m_population;

public:
    Settlement();
    Settlement(int ownerId, SettlementType type);
    ~Settlement() = default;

    SettlementType type() const { return m_type; }
    int ownerId() const { return m_ownerId; }
    int populationCount() const { return m_populationCount; }
    const PopulationAllocation &allocation() const { return m_population; }
    const std::vector<Tile> &tiles() const { return m_tiles; }

    void setType(SettlementType type) { m_type = type; }
    void setOwnerId(int ownerId) { m_ownerId = ownerId; }

    void addTile(const Tile &tile);

    void seedInitialPopulation(int amount);
    void growPopulation(float baseGrowthRate, float raceGrowthModifier);
    void reallocate(Sphere from, Sphere to, int amount);
    void assignFromIdle(Sphere sphere, int amount);
    void unassignToIdle(Sphere sphere, int amount);
    void removePopulationPermanently(int amount);

    std::string debugString() const;
};
