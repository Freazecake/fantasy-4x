#pragma once

#include <vector>

#include "Race.hpp"
#include "Settlement.hpp"
#include "Economy.hpp"
#include "Hero.hpp"

enum class PersonalPhase
{
    Phase1_PreSettlement,
    Phase2_PreState,
    Phase3_Global,
};

class Player
{
private:
    bool hasFoundedFirstSettlement() const { return !m_settlements.empty(); }
    std::vector<Settlement> m_settlements;
    std::vector<Hero> m_heroes;
    Economy m_economy;
    Race m_race;
    int m_id;

public:
    Player();
    ~Player() = default;
    PersonalPhase personalPhase() const
    {
        return hasFoundedFirstSettlement() ? PersonalPhase::Phase2_PreState
                                           : PersonalPhase::Phase1_PreSettlement;
    }
    void collectIncome(const Modifiers &mods)
    {
        Resources turnTotal;
        for (const auto &s : m_settlements)
            turnTotal += Economy::computeSettlementOutput(s, mods);
        m_economy.accumulate(turnTotal);
    }
};