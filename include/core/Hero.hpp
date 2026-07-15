#pragma once

#include <string>
#include <vector>

#include "Common.hpp"
#include "external/json.hpp"

using json = nlohmann::json;

class Hero
{
private:
    std::string m_id;
    std::string m_name;
    std::string m_raceId;
    Sphere m_sphere;
    Modifiers m_bonusMultiplier;

public:
    Hero();
    Hero(const json &obj) { *this = Hero::loadFromJson(obj); }
    ~Hero() = default;
    static Hero loadFromJson(const json &obj);
    static std::vector<Hero> loadAllFromJson(const json &arr);

    std::string getName() const { return m_name; }
    Modifiers getModifiers() const { return m_bonusMultiplier; }
    std::string getId() const { return m_id; }
    std::string getRaceId() const { return m_raceId; }
    Sphere getSphere() const { return m_sphere; }

    std::string toString() const;
};