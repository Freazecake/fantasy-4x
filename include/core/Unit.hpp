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
    float m_currentHealth = 0.0f;
    bool m_trained = false;
    bool m_isSettler = false;

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
    int getMovementPoints() const { return m_stats.movementPoints; }

    float getCurrentHealth() const { return m_currentHealth; }
    bool isAlive() const { return m_currentHealth > 0.0f; }
    void setCurrentHealth(float hp) { m_currentHealth = hp; }

    bool isTrained() const { return m_trained; }
    void setTrained(bool trained) { m_trained = trained; }

    bool requiresBuilding() const { return !m_requiredBuildingId.empty(); }
    bool isSettler() const { return m_isSettler; }

    std::string toString() const;
};