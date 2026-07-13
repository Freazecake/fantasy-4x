#include "core/Building.hpp"

Building::Building()
    : m_id(""), m_name(""), m_category(""), m_modifiers(), m_cost(0), m_isunlocked(false), m_unlocksUnitIds(), m_removesUntrainedPenalty(false)
{
}

Building::Building(const json &obj) { Building::loadFromJson(obj); }

Building Building::loadFromJson(const json &obj)
{
    Building building;
    building.m_id = obj["id"].get<std::string>();
    building.m_name = obj["name"].get<std::string>();
    building.m_category = obj["category"].get<std::string>();
    building.m_modifiers = Modifiers();
    std::string type = obj["resourceType"].get<std::string>();
    if (type == "science")
    {
        building.m_modifiers.science = obj["bonusMultiplier"].get<float>();
    }
    else if (type == "industry")
    {
        building.m_modifiers.industry = obj["bonusMultiplier"].get<float>();
    }
    else if (type == "trade")
    {
        building.m_modifiers.trade = obj["bonusMultiplier"].get<float>();
    }
    else if (type == "diplomacy")
    {
        building.m_modifiers.diplomacy = obj["bonusMultiplier"].get<float>();
    }
    else if (type == "military")
    {
        building.m_modifiers.military = obj["bonusMultiplier"].get<float>();
    }
    else if (type == "growth")
    {
        building.m_modifiers.growth = obj["bonusMultiplier"].get<float>();
    }
    building.m_cost = obj["industryCost"].get<int>();
    for (const auto &unitId : obj["unlocksUnitIds"])
    {
        building.m_unlocksUnitIds.push_back(unitId.get<std::string>());
    }
    building.m_removesUntrainedPenalty = obj["removesUntrainedPenalty"].get<bool>();
    return building;
}

std::string Building::toString() const
{
    std::string result = "Building: " +
                         m_name + " (ID: " + m_id + ", Category: " +
                         m_category + ", Cost: " + std::to_string(m_cost) +
                         ", Researched: " + (m_isunlocked ? "True" : "False") +
                         ", Removes Untrained Penalty: " + (m_removesUntrainedPenalty ? "True" : "False") + ")";
    result += "\n" + m_modifiers.toString();
    result += "\nUnlocks Units: ";
    for (const auto &unitId : m_unlocksUnitIds)
    {
        result += unitId + ", ";
    }
    return result;
}

std::vector<Building> Building::loadAllFromJson(const json &arrayNode)
{
    std::vector<Building> result;
    for (const auto &obj : arrayNode)
    {
        result.push_back(Building::loadFromJson(obj));
    }
    return result;
}