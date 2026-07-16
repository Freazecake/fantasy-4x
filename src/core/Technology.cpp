#include "core/Technology.hpp"

Technology::Technology() {}

Technology Technology::loadFromJson(const json &obj)
{
    Technology t;
    t.m_id = obj["id"].get<std::string>();
    t.m_name = obj["name"].get<std::string>();
    t.m_category = obj["category"].get<std::string>();
    t.m_scienceCost = obj["scienceCost"].get<int>();

    for (const auto &str : obj["prerequisiteIds"])
    {
        t.m_prerequisiteIds.push_back(str.get<std::string>());
    }
    for (const auto &str : obj["unlocksBuildingIds"])
    {
        t.m_unlocksBuildingIds.push_back(str.get<std::string>());
    }
    std::vector<std::string> temp;
    for (const auto &str : obj["bonusCategories"])
    {
        temp.push_back(str.get<std::string>());
    }
    float mod = obj["globalBonusMultiplier"].get<float>();
    t.m_modifiers = Modifiers();
    for (const auto &type : temp)
    {
        if (type == "science")
        {
            t.m_modifiers.science = mod;
        }
        else if (type == "industry")
        {
            t.m_modifiers.industry = mod;
        }
        else if (type == "trade")
        {
            t.m_modifiers.trade = mod;
        }
        else if (type == "diplomacy")
        {
            t.m_modifiers.diplomacy = mod;
        }
        else if (type == "military")
        {
            t.m_modifiers.military = mod;
        }
        else if (type == "growth")
        {
            t.m_modifiers.growth = mod;
        }
    }
    return t;
}

std::vector<Technology> Technology::loadAllFromJson(const json &TechNode)
{
    std::vector<Technology> result;
    for (const auto &obj : TechNode)
    {
        result.push_back(Technology::loadFromJson(obj));
    }
    return result;
}

bool Technology::canResearch(const std::unordered_set<std::string> &ResearchedTechIds)
{
    for (const auto &prereq : m_prerequisiteIds)
    {
        if (ResearchedTechIds.count(prereq) == 0)
            return false;
    }
    return true;
}

int Technology::invest(int points)
{
    if (m_isResearched)
        return points;

    int needed = m_scienceCost - m_scienceInvested;
    if (points < needed)
    {
        m_scienceInvested += points;
        return 0;
    }

    m_scienceInvested = m_scienceCost;
    m_isResearched = true;
    return points - needed;
}