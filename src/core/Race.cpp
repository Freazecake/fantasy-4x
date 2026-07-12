#include "core/Race.hpp"

Race::Race() {}

Race Race::loadFromJson(const json &obj)
{
    Race r;
    r.m_name = obj["name"].get<std::string>();
    r.m_startingSettlementType = obj["startingSettlementType"].get<std::string>();
    auto m = obj["modifiers"];
    r.m_modifiers = {m["science"], m["industry"], m["trade"],
                     m["diplomacy"], m["military"], m["growth"]};

    return r;
}
