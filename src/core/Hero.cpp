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