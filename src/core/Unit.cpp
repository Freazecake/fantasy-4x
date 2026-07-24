#include "core/Unit.hpp"

#include <sstream>

Unit Unit::loadFromJson(const json &obj)
{
    Unit u;
    u.m_id = obj["id"].get<std::string>();
    u.m_name = obj["name"].get<std::string>();

    const auto &statsJson = obj["stats"];
    u.m_stats.health = statsJson["health"].get<float>();
    u.m_stats.attack = statsJson["attack"].get<float>();
    u.m_stats.defense = statsJson["defense"].get<float>();
    u.m_stats.movementPoints = statsJson.value("movementPoints", 2);
    u.m_currentHealth = u.m_stats.health;

    u.m_tier = obj["tier"].get<int>();
    u.m_requiredBuildingId = obj["requiredBuildingId"].get<std::string>();
    u.m_recruitCost = obj["recruitCost"].get<int>();
    u.m_populationCost = obj["populationCost"].get<int>();

    return u;
}

std::vector<Unit> Unit::loadAllFromJson(const json &arr)
{
    std::vector<Unit> units;
    units.reserve(arr.size());
    for (const auto &unitJson : arr)
        units.push_back(Unit::loadFromJson(unitJson));
    return units;
}

std::string Unit::toString() const
{
    std::ostringstream oss;
    oss << m_id << " | " << m_name << " (tier " << m_tier << ")"
        << "| hp=" << m_currentHealth << "/" << m_stats.health
        << ", atk=" << m_stats.attack
        << ", def=" << m_stats.defense
        << ", recruitCost=" << m_recruitCost
        << ", populationCost=" << m_populationCost;
    if (requiresBuilding())
        oss << ", requires building: " << m_requiredBuildingId;
    return oss.str();
}