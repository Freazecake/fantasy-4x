#pragma once

#include <vector>

#include "Tile.hpp"
#include "Common.hpp"
#include "Building.hpp"
#include "Military.hpp"

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

struct BuildingQueueItem
{
    Building building;
    int investedIndustry = 0;

    int remainingCost() const
    {
        int need = building.getCost() - investedIndustry;
        return need > 0 ? need : 0;
    }
};

struct BuildingProgressResult
{
    std::vector<Building> completed;
    int spentIndustry = 0;
};

struct RecruitmentProgressResult
{
    std::vector<Unit> completed;
    int spentIndustry = 0;
};

class Settlement
{
private:
    SettlementType m_type = SettlementType::PreSettlement;
    std::vector<Tile> m_tiles;
    int m_ownerId;
    int m_populationCount;
    PopulationAllocation m_population;

    std::vector<Building> m_builtBuildings;
    std::vector<BuildingQueueItem> m_buildingQueue;

    std::vector<Unit> m_garrison;
    std::vector<RecruitmentItem> m_recruitmentQueue;

public:
    Settlement();
    Settlement(int ownerId, SettlementType type);
    ~Settlement() = default;

    SettlementType type() const { return m_type; }
    int ownerId() const { return m_ownerId; }
    int populationCount() const { return m_populationCount; }
    const PopulationAllocation &allocation() const { return m_population; }
    const std::vector<Tile> &tiles() const { return m_tiles; }

    Coords anchorCoords() const;

    void setType(SettlementType type) { m_type = type; }
    void setOwnerId(int ownerId) { m_ownerId = ownerId; }

    void addTile(const Tile &tile);

    void seedInitialPopulation(int amount);
    void growPopulation(float baseGrowthRate, float raceGrowthModifier, int extraFlatGrowth = 0);
    void reallocate(Sphere from, Sphere to, int amount);
    void assignFromIdle(Sphere sphere, int amount);
    void unassignToIdle(Sphere sphere, int amount);
    void removePopulationPermanently(int amount);

    void queueBuilding(const Building &building);
    BuildingProgressResult progressBuildingQueue(int industryPoints);
    const std::vector<Building> &builtBuildings() const { return m_builtBuildings; }
    const std::vector<BuildingQueueItem> &buildingQueue() const { return m_buildingQueue; }
    bool hasBuilt(const std::string &buildingId) const;

    void queueUnitRecruitment(const Unit &unit);
    RecruitmentProgressResult progressRecruitmentQueue(int industryPoints);
    const std::vector<Unit> &garrison() const { return m_garrison; }
    std::vector<Unit> removeFromGarrison(const std::vector<std::string> &unitIds);
    void addToGarrison(const std::vector<Unit> &units);
    const std::vector<RecruitmentItem> &recruitmentQueue() const { return m_recruitmentQueue; }

    std::string debugString() const;
    bool operator==(const Settlement &other) const
    {
        return this == &other;
    }
};
