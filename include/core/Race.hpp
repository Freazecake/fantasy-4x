#pragma once

#include <string>

#include "external/json.hpp"

struct RaceModifiers
{
    float science = 1.0f;
    float industry = 1.0f;
    float trade = 1.0f;
    float diplomacy = 1.0f;
    float military = 1.0f;
    float growth = 1.0f;
};

class Race
{
private:
    std::string name;
    RaceModifiers modifiers;
    std::string startingSettlementType; // "village", "cave", "fortress"...
public:
    Race();
    ~Race() = default;
    static Race loadFromJson(const json &obj);
};