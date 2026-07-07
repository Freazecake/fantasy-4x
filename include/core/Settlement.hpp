#pragma once

#include <vector>

#include "Tile.hpp"

// struct SettlementModifiers
// {
//     float science = 1.0f;
//     float industry = 1.0f;
//     float trade = 1.0f;
//     float diplomacy = 1.0f;
//     float military = 1.0f;
//     float growth = 1.0f;
// };

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
    int m_population;

public:
    Settlement();
    ~Settlement() = default;
};