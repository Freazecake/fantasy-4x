#include "core/Economy.hpp"

#include <stdexcept>

Resources::Resources() {}

Resources::Resources(float num)
{
    this->science = num;
    this->diplomacy = num;
    this->growth = num;
    this->industry = num;
    this->military = num;
    this->trade = num;
}

Resources::Resources(const Resources &other)
{
    this->science = other.science;
    this->diplomacy = other.diplomacy;
    this->growth = other.growth;
    this->industry = other.industry;
    this->military = other.military;
    this->trade = other.trade;
}

void Resources::operator+=(const Resources &other)
{
    this->science += other.science;
    this->diplomacy += other.diplomacy;
    this->growth += other.growth;
    this->industry += other.industry;
    this->military += other.military;
    this->trade += other.trade;
}

Resources Resources::operator*(float num)
{
    Resources res(*this);
    res.science *= num;
    res.diplomacy *= num;
    res.growth *= num;
    res.industry *= num;
    res.military *= num;
    res.trade *= num;

    return res;
}

Resources Resources::operator*(const Modifiers &mods)
{
    Resources res(*this);
    res.science *= mods.science;
    res.diplomacy *= mods.diplomacy;
    res.growth *= mods.growth;
    res.industry *= mods.industry;
    res.military *= mods.military;
    res.trade *= mods.trade;

    return res;
}

Resources &Resources::operator=(Resources &&other)
{
    this->science = other.science;
    this->diplomacy = other.diplomacy;
    this->growth = other.growth;
    this->industry = other.industry;
    this->military = other.military;
    this->trade = other.trade;

    return *this;
}

Resources &Resources::operator=(const Resources &other)
{
    this->science = other.science;
    this->diplomacy = other.diplomacy;
    this->growth = other.growth;
    this->industry = other.industry;
    this->military = other.military;
    this->trade = other.trade;

    return *this;
}

std::string Resources::toString() const
{
    return "Resources: science=" + std::to_string(science) +
           " industry=" + std::to_string(industry) +
           " trade=" + std::to_string(trade) +
           " diplomacy=" + std::to_string(diplomacy);
}

Economy::Economy() {}

Economy::Economy(const Modifiers &mods)
{
    Resources res(1);
    m_res = res * mods;
}

Economy::Economy(const Economy &other) : m_res(other.m_res) {}

Resources Economy::computeSettlementIncome(const Settlement &settlement,
                                           const Modifiers &raceModifiers,
                                           const std::vector<Building> &builtBuildings,
                                           const std::vector<Hero> &heroes)
{
    Resources income;
    income.science = static_cast<float>(settlement.allocation().science);
    income.industry = static_cast<float>(settlement.allocation().industry);
    income.trade = static_cast<float>(settlement.allocation().trade);
    income.diplomacy = static_cast<float>(settlement.allocation().diplomacy);
    income.military = 0.0f;
    income.growth = 0.0f;

    income = income * raceModifiers;

    for (const auto &building : builtBuildings)
        income = income * building.getModifiers();

    for (const auto &hero : heroes)
        income = income * hero.getModifiers();

    return income;
}

void Economy::accumulate(const Resources &res)
{
    this->m_res += res;
}

bool Economy::trySpendTrade(int amount)
{
    if (amount < 0)
        throw std::invalid_argument("Economy::trySpendTrade: amount не может быть отрицательным");
    if (m_res.trade < amount)
        return false;
    m_res.trade -= amount;
    return true;
}

bool Economy::trySpendDiplomacy(int amount)
{
    if (amount < 0)
        throw std::invalid_argument("Economy::trySpendDiplomacy: amount не может быть отрицательным");
    if (m_res.diplomacy < amount)
        return false;
    m_res.diplomacy -= amount;
    return true;
}

void Economy::addResource(const std::string &resourceType, int amount)
{
    if (resourceType == "science")
        m_res.science += amount;
    else if (resourceType == "industry")
        m_res.industry += amount;
    else if (resourceType == "trade")
        m_res.trade += amount;
    else if (resourceType == "diplomacy")
        m_res.diplomacy += amount;
    else
        throw std::invalid_argument("Economy::addResource: неизвестный resourceType '" + resourceType + "'");
}
