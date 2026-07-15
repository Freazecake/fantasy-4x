#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Race.hpp"
#include "Settlement.hpp"
#include "Economy.hpp"
#include "Hero.hpp"
#include "Technology.hpp"
#include "PoliticalAction.hpp"
#include "TradeMarket.hpp"
#include "Military.hpp"

inline constexpr float DEFAULT_BASE_GROWTH_RATE = 0.1f;

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
    int m_id = -1;

    std::vector<Technology> m_technologies;
    std::string m_currentResearchId;
    std::unordered_set<std::string> m_researchedTechIds;

public:
    Player();
    ~Player() = default;

    PersonalPhase personalPhase() const
    {
        return hasFoundedFirstSettlement() ? PersonalPhase::Phase2_PreState
                                           : PersonalPhase::Phase1_PreSettlement;
    }

    void setId(int id) { m_id = id; }
    int id() const { return m_id; }

    void setRace(const Race &race) { m_race = race; }
    const Race &race() const { return m_race; }

    void addHero(const Hero &hero) { m_heroes.push_back(hero); }
    const std::vector<Hero> &heroes() const { return m_heroes; }

    void addSettlement(const Settlement &settlement) { m_settlements.push_back(settlement); }
    const std::vector<Settlement> &settlements() const { return m_settlements; }
    std::vector<Settlement> &settlements() { return m_settlements; }

    const Economy &economy() const { return m_economy; }
    const Resources &resources() const { return m_economy.totals(); }

    void loadTechnologies(const std::vector<Technology> &catalog) { m_technologies = catalog; }
    const std::vector<Technology> &technologies() const { return m_technologies; }
    const std::string &currentResearchId() const { return m_currentResearchId; }
    const std::unordered_set<std::string> &researchedTechIds() const { return m_researchedTechIds; }
    bool hasResearched(const std::string &techId) const { return m_researchedTechIds.count(techId) > 0; }

    bool setCurrentResearch(const std::string &techId);
    void investScience(int points);

    void advanceTurn();

    float effectiveTradeMultiplier() const;
    bool exchangeTrade(const std::string &targetResource, int tradeToSpend);

    bool trySpendOnPoliticalAction(const PoliticalAction &action);

    UnitStats effectiveUnitStats(const Settlement &settlement, const Unit &unit) const
    {
        return Military::effectiveStats(unit, settlement.builtBuildings(),
                                        m_race.getModifiers().military, m_heroes);
    }
};