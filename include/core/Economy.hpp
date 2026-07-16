#pragma once

#include "Settlement.hpp"
#include "Race.hpp"

struct Resources
{
    float science = 0;
    float industry = 0;
    float trade = 0;
    float diplomacy = 0;
    float military = 0;
    float growth = 0;

    void operator+=(const Resources &);
    Resources operator*(float);
};

class Economy
{
private:
    Resources m_res;

public:
    static Resources computeSettlementOutput(const Settlement &s, const RaceModifiers &mods);
    void accumulate(const Resources &);
};