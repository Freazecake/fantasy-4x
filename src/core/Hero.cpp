#include "core/Hero.hpp"

Hero::Hero() {}

Hero Hero::loadFromJson(const json &obj)
{
    Hero r;
    r.m_name = obj["name"].get<std::string>();
    r.m_id = obj["id"].get<std::string>();
    auto m = obj["modifiers"];
    r.m_bonusMultiplier = {m["science"], m["industry"], m["trade"],
                           m["diplomacy"], m["military"], m["growth"]};
    r.m_sphere = r.m_bonusMultiplier.findMaxModifier(r.m_bonusMultiplier).first;
    return r;
}

std::vector<Hero> Hero::loadAllFromJson(const json &arr)
{
    std::vector<Hero> heroes;
    for (const auto &obj : arr)
    {
        heroes.push_back(Hero::loadFromJson(obj));
    }
    return heroes;
}

std::string Hero::toString() const
{
    std::string result = "Hero: " +
                         m_name + " (ID: " + m_id + ", Sphere: " +
                         sphereToString(m_sphere) + ")";
    result += "\n" + m_bonusMultiplier.toString();
    return result;
}