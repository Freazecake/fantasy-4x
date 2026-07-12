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

    Resources();
    Resources(float);
    Resources(const Resources &);
    void operator+=(const Resources &);
    Resources operator*(float);
    Resources operator*(const Modifiers &);
    Resources &operator=(Resources &&);
};

class Economy
{
private:
    Resources m_res;

public:
    Economy();
    Economy(const Modifiers &);
    Economy(const Economy &);
    static Resources computeSettlementOutput(const Settlement &, const Modifiers &);
    void accumulate(const Resources &);
};