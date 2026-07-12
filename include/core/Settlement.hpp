#pragma once

#include <vector>

#include "Tile.hpp"

struct PopulationAllocation
{
    int science = 0;
    int industry = 0;
    int trade = 0;
    int diplomacy = 0;
    int idle = 0;

    int total() const { return science + industry + trade + diplomacy + idle; }
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
    SettlementType m_type;
    std::vector<Tile> m_tiles;
    int m_ownerId;
    PopulationAllocation m_population;

public:
    Settlement();
    ~Settlement() = default;
};