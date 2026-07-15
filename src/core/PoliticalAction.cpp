#include "core/PoliticalAction.hpp"

#include <stdexcept>

PoliticalActionType politicalActionTypeFromString(const std::string &s)
{
    if (s == "alliance")
        return PoliticalActionType::Alliance;
    if (s == "vassalize")
        return PoliticalActionType::Vassalize;
    if (s == "demandTribute")
        return PoliticalActionType::DemandTribute;
    if (s == "politicalConquest")
        return PoliticalActionType::PoliticalConquest;

    throw std::invalid_argument("politicalActionTypeFromString: неизвестный тип '" + s + "'");
}

std::string politicalActionTypeToString(PoliticalActionType t)
{
    switch (t)
    {
    case PoliticalActionType::Alliance:
        return "alliance";
    case PoliticalActionType::Vassalize:
        return "vassalize";
    case PoliticalActionType::DemandTribute:
        return "demandTribute";
    case PoliticalActionType::PoliticalConquest:
        return "politicalConquest";
    }
    return "unknown";
}

PoliticalAction PoliticalAction::loadFromJson(const json &obj)
{
    PoliticalAction action;
    action.m_id = obj["id"].get<std::string>();
    action.m_name = obj["name"].get<std::string>();
    action.m_diplomacyCost = obj["diplomacyCost"].get<int>();
    action.m_type = politicalActionTypeFromString(obj["type"].get<std::string>());
    return action;
}

std::vector<PoliticalAction> PoliticalAction::loadAllFromJson(const json &arr)
{
    std::vector<PoliticalAction> result;
    for (const auto &obj : arr)
    {
        result.push_back(PoliticalAction::loadFromJson(obj));
    }
    return result;
}

std::string PoliticalAction::toString() const
{
    return "PoliticalAction: " + m_name + " (ID: " + m_id +
           ", Type: " + politicalActionTypeToString(m_type) +
           ", Cost: " + std::to_string(m_diplomacyCost) + ")";
}
