#pragma once

#include <string>

#include "external/json.hpp"
#include "Common.hpp"

using json = nlohmann::json;

class Race
{
private:
    std::string m_name;
    Modifiers m_modifiers;
    std::string m_startingSettlementType;

public:
    Race();
    Race(const json &obj) { *this = Race::loadFromJson(obj); }
    ~Race() = default;
    static Race loadFromJson(const json &obj);
    std::string getName() const { return m_name; }
    Modifiers getModifiers() const { return m_modifiers; }
};