#pragma once

#include <string>
#include <vector>

#include "external/json.hpp"
#include "Common.hpp"

using json = nlohmann::json;

class Unit
{
private:
    std::string m_id;
    std::string m_name;
    UnitStats m_stats;
    int m_tier = 0;
    std::string m_requiredBuildingId;
    int m_recruitCost = 0;
    int m_populationCost = 0;

public:
    Unit() = default;
    Unit(const json &obj) { *this = Unit::loadFromJson(obj); }
    ~Unit() = default;

    static Unit loadFromJson(const json &obj);
    static std::vector<Unit> loadAllFromJson(const json &arr);

    std::string getId() const { return m_id; }
    std::string getName() const { return m_name; }
    const UnitStats &getStats() const { return m_stats; }
    int getTier() const { return m_tier; }
    const std::string &getRequiredBuildingId() const { return m_requiredBuildingId; }
    int getRecruitCost() const { return m_recruitCost; }
    int getPopulationCost() const { return m_populationCost; }

    bool requiresBuilding() const { return !m_requiredBuildingId.empty(); }

    std::string toString() const;
};