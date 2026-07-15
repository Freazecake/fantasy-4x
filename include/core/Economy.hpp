#pragma once

#include <string>
#include <vector>

#include "Settlement.hpp"
#include "Building.hpp"
#include "Hero.hpp"
#include "Common.hpp"

struct Resources
{
    float science = 0;
    float industry = 0;
    float trade = 0;
    float diplomacy = 0;
    float military = 0;
    float growth = 0;

    Resources();
    Resources(float);
    Resources(const Resources &);
    void operator+=(const Resources &);
    Resources operator*(float);
    Resources operator*(const Modifiers &);
    Resources &operator=(Resources &&);
    Resources &operator=(const Resources &);

    std::string toString() const;
};

class Economy
{
private:
    Resources m_res;

public:
    Economy();
    Economy(const Modifiers &);
    Economy(const Economy &);

    static Resources computeSettlementIncome(const Settlement &settlement,
                                             const Modifiers &raceModifiers,
                                             const std::vector<Building> &builtBuildings,
                                             const std::vector<Hero> &heroes);

    void accumulate(const Resources &);
    const Resources &totals() const { return m_res; }

    bool trySpendTrade(int amount);
    bool trySpendDiplomacy(int amount);
    void addResource(const std::string &resourceType, int amount);
};