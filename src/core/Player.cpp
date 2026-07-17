#include "core/Player.hpp"

Player::Player() {}

bool Player::setCurrentResearch(const std::string &techId)
{
    for (auto &tech : m_technologies)
    {
        if (tech.getId() != techId)
            continue;

        if (tech.isResearched())
            return false;
        if (!tech.canResearch(m_researchedTechIds))
            return false;

        m_currentResearchId = techId;
        return true;
    }
    return false;
}

void Player::investScience(int points)
{
    if (m_currentResearchId.empty())
        return;

    for (auto &tech : m_technologies)
    {
        if (tech.getId() != m_currentResearchId)
            continue;

        tech.invest(points);
        if (tech.isResearched())
        {
            m_researchedTechIds.insert(tech.getId());
            m_currentResearchId.clear();
        }
        return;
    }
}

void Player::advanceTurn()
{
    Resources totalIncome;

    for (auto &settlement : m_settlements)
    {
        Resources income = Economy::computeSettlementIncome(
            settlement, m_race.getModifiers(), settlement.builtBuildings(), m_heroes);

        int industryBudget = static_cast<int>(income.industry);

        auto buildResult = settlement.progressBuildingQueue(industryBudget);
        industryBudget -= buildResult.spentIndustry;

        auto recruitResult = settlement.progressRecruitmentQueue(industryBudget);
        industryBudget -= recruitResult.spentIndustry;

        Resources incomeToBank = income;
        incomeToBank.industry = static_cast<float>(industryBudget);
        totalIncome += incomeToBank;
    }

    m_economy.accumulate(totalIncome);
    investScience(static_cast<int>(totalIncome.science));
}

float Player::effectiveTradeMultiplier() const
{
    float multiplier = m_race.getModifiers().trade;

    for (const auto &hero : m_heroes)
        multiplier *= hero.getModifiers().trade;

    for (const auto &settlement : m_settlements)
        for (const auto &building : settlement.builtBuildings())
            multiplier *= building.getModifiers().trade;

    return multiplier;
}

bool Player::exchangeTrade(const std::string &targetResource, int tradeToSpend)
{
    if (!m_economy.trySpendTrade(tradeToSpend))
        return false;

    TradeExchangeResult result = TradeMarket::exchange(effectiveTradeMultiplier(), targetResource, tradeToSpend);
    m_economy.addResource(result.receivedResourceType, result.received);
    return true;
}

bool Player::trySpendOnPoliticalAction(const PoliticalAction &action)
{
    return m_economy.trySpendDiplomacy(action.getCost());
}
