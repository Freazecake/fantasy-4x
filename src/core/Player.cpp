#include "core/Player.hpp"
#include "core/Map.hpp"

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

void Player::spendIndustryOnQueues(Settlement &settlement)
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

void Player::advanceTurn(const std::vector<ResourceDef> &resourceCatalog)
{
    Resources totalIncome;

    for (auto &settlement : m_settlements)
    {
        Resources income = Economy::computeSettlementIncome(
            settlement, m_race.getModifiers(), settlement.builtBuildings(), m_heroes, resourceCatalog);

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

    if (m_allianceBonus)
        multiplier *= 1.2;

    float buildingBonus = 0.0f;
    for (const auto &settlement : m_settlements)
        for (const auto &building : settlement.builtBuildings())
            buildingBonus += (building.getModifiers().trade - 1.0f);

    return multiplier * (1.0f + buildingBonus);
}

bool Player::exchangeTrade(const std::string &targetResource, int tradeToSpend)
{
    if (!m_economy.trySpendTrade(tradeToSpend))
        return false;

    TradeExchangeResult result = TradeMarket::exchange(effectiveTradeMultiplier(), targetResource, tradeToSpend);
    m_economy.addResource(result.receivedResourceType, result.received);
    return true;
}

Resources Player::takeFractionFromEconomy(float fraction)
{
    Resources total = m_economy.totals();
    Resources taken = total * fraction;
    m_economy.accumulate(taken * (-1.0f));
    return taken;
}

Resources Player::takeCappedFractionFromEconomy(float fraction, float capPerResource)
{
    Resources total = m_economy.totals();
    Resources taken = total * fraction;
    taken.science = std::min(taken.science, capPerResource);
    taken.industry = std::min(taken.industry, capPerResource);
    taken.trade = std::min(taken.trade, capPerResource);
    taken.diplomacy = std::min(taken.diplomacy, capPerResource);
    taken.growth = 0.0f;
    taken.military = 0.0f;
    m_economy.accumulate(taken * (-1.0f));
    return taken;
}

void Player::addResources(const Resources &res)
{
    m_economy.accumulate(res);
}

bool Player::trySpendOnPoliticalAction(const PoliticalAction &action)
{
    return m_economy.trySpendDiplomacy(action.getCost());
}

bool Player::trySpendDiplomacy(int amount)
{
    return m_economy.trySpendDiplomacy(amount);
}

float Player::militaryPower() const
{
    float power = 0.0f;
    for (const auto &s : m_settlements)
        for (const auto &u : s.garrison())
            power += u.getStats().attack + u.getStats().defense + u.getCurrentHealth() * 0.1f;
    for (const auto &a : m_armies)
        for (const auto &u : a.units())
            power += u.getStats().attack + u.getStats().defense + u.getCurrentHealth() * 0.1f;
    return power;
}

Army Player::detachArmy(Settlement &from, const std::vector<std::string> &unitIds)
{
    std::vector<Unit> taken = from.removeFromGarrison(unitIds);
    static int armyCounter = 0;
    Coords pos = from.tiles().empty() ? Coords(0, 0) : from.tiles()[0].getCoords();
    std::string aid = "army_p" + std::to_string(m_id) + "_" + std::to_string(armyCounter++);
    Army army(aid, m_id, pos, taken);
    m_armies.push_back(army);
    return army;
}

void Player::disbandArmyIntoSettlement(Army &army, Settlement &target)
{
    target.addToGarrison(army.units());
    removeArmy(army.getId());
}

void Player::removeArmy(const std::string &id)
{
    for (auto it = m_armies.begin(); it != m_armies.end(); ++it)
    {
        if (it->getId() == id)
        {
            m_armies.erase(it);
            return;
        }
    }
}

void Player::recomputeVisibility(const Map &map, int visionRadius)
{
    m_visibleTiles.clear();

    std::vector<Coords> centers;
    for (const auto &s : m_settlements)
        centers.push_back(s.anchorCoords());
    for (const auto &a : m_armies)
        centers.push_back(a.position());

    if (centers.empty())
        return;

    for (const auto &tile : map.getMap())
    {
        Coords c = tile.getCoords();
        for (const auto &center : centers)
        {
            if (center.distanceTo(c) <= visionRadius)
            {
                m_visibleTiles.insert({c.q, c.r});
                break;
            }
        }
    }

    for (const auto &v : m_visibleTiles)
        m_exploredTiles.insert(v);
}