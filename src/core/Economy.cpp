#include "core/Economy.hpp"

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

Economy::Economy() {}

Economy::Economy(const Modifiers &mods)
{
    Resources res(1);
    m_res = res * mods;
}

Resources Economy::computeSettlementOutput(const Settlement &s, const Modifiers &mods)
{
    Resources res(1);
    return res * mods;
}

void Economy::accumulate(const Resources &res)
{
    this->m_res += res;
}