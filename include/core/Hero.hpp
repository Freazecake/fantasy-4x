#pragma once

#include <string>

#include "Race.hpp"

class Hero
{
private:
    std::string m_id;
    std::string m_name;
    Sphere m_sphere;
    Modifiers m_bonusMultiplier;

public:
    Hero();
    Hero(const json &obj) { *this = Hero::loadFromJson(obj); }
    ~Hero() = default;
    static Hero loadFromJson(const json &obj);
    static std::vector<Hero> loadAllFronJson(const json &arr);

    std::string getName() const { return m_name; }
    Modifiers getModifiers() const { return m_bonusMultiplier; }
    std::string getId() const { return m_id; }
};