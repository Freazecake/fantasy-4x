#pragma once

#include <string>

#include "external/json.hpp"

using json = nlohmann::json;

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
    std::string m_name;
    RaceModifiers m_modifiers;
    std::string m_startingSettlementType; // "village", "cave", "fortress"...
public:
    Race(const json &obj) { Race::loadFromJson(obj); }
    ~Race() = default;
    static Race loadFromJson(const json &obj);
    std::string getName() const { return m_name; }
    RaceModifiers getModifiers() const { return m_modifiers; }
};