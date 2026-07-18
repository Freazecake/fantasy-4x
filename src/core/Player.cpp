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

        int bankScience = static_cast<int>(m_economy.totals().science);
        if (bankScience > 0 && m_economy.trySpendScience(bankScience))
        {
            int leftover = tech.invest(bankScience);
            int spent = bankScience - leftover;
            if (leftover > 0)
                m_economy.addResource("science", leftover);

            if (tech.isResearched())
            {
                m_researchedTechIds.insert(tech.getId());
                m_currentResearchId.clear();
            }
        }

        return true;
    }
    return false;
}

int Player::investScience(int points)
{
    if (m_currentResearchId.empty())
        return 0;

    for (auto &tech : m_technologies)
    {
        if (tech.getId() != m_currentResearchId)
            continue;

        int leftover = tech.invest(points);
        int spent = points - leftover;
        if (tech.isResearched())
        {
            m_researchedTechIds.insert(tech.getId());
            m_currentResearchId.clear();
        }
        return spent;
    }
    return 0;
}

void Player::spendIndustryOnQueues(Settlement& settlement)
{
    int bankIndustry = static_cast<int>(m_economy.totals().industry);
    if (bankIndustry <= 0)
        return;

    if (!m_economy.trySpendIndustry(bankIndustry))
        return;

    auto buildResult = settlement.progressBuildingQueue(bankIndustry);
    int spent = buildResult.spentIndustry;
    int leftover = bankIndustry - spent;

    auto recruitResult = settlement.progressRecruitmentQueue(leftover);
    leftover -= recruitResult.spentIndustry;

    if (leftover > 0)
        m_economy.addResource("industry", leftover);
}

void Player::advanceTurn()
{
    Resources totalIncome;

    for (auto &settlement : m_settlements)
    {
        Resources income = Economy::computeSettlementIncome(
            settlement, m_race.getModifiers(), settlement.builtBuildings(), m_heroes);

        int currentIndustry = static_cast<int>(income.industry);

        auto buildResult = settlement.progressBuildingQueue(currentIndustry);
        currentIndustry -= buildResult.spentIndustry;

        auto recruitResult = settlement.progressRecruitmentQueue(currentIndustry);
        currentIndustry -= recruitResult.spentIndustry;

        Resources incomeToBank = income;
        incomeToBank.industry = static_cast<float>(currentIndustry);
        totalIncome += incomeToBank;
    }

    int scienceBudget = static_cast<int>(totalIncome.science);
    int scienceSpent = investScience(scienceBudget);
    totalIncome.science = static_cast<float>(scienceBudget - scienceSpent);

    m_economy.accumulate(totalIncome);
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
