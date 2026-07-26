#pragma once

#include <string>
#include <vector>
#include <unordered_set>

#include "Common.hpp"
#include "external/json.hpp"

using json = nlohmann::json;

class Technology
{
private:
    std::string m_id;
    std::string m_name;
    std::string m_category;
    int m_scienceCost = 0;
    std::vector<std::string> m_prerequisiteIds;
    std::vector<std::string> m_unlocksBuildingIds;
    Modifiers m_modifiers;
    bool m_isResearched = false;
    int m_scienceInvested = 0;

public:
    Technology();
    Technology(const json &obj) { *this = Technology::loadFromJson(obj); };
    ~Technology() = default;
    static Technology loadFromJson(const json &);
    static std::vector<Technology> loadAllFromJson(const json &);

    std::string getId() const { return m_id; }
    std::string getName() const { return m_name; }
    std::string getCategory() const { return m_category; }
    Modifiers getModifiers() const { return m_modifiers; }
    int getCost() const { return m_scienceCost; }
    const std::vector<std::string> &getPrerequisiteIds() const { return m_prerequisiteIds; }
    const std::vector<std::string> &getUnloksBuildingsIds() const { return m_unlocksBuildingIds; }
    bool isResearched() const { return m_isResearched; }

    bool canResearch(const std::unordered_set<std::string> &ResearchedTechIds);
    bool canResearch(const std::unordered_set<std::string> &ResearchedTechIds) const;
    int invest(int points);
    int remainingCost() const { return std::max(0, m_scienceCost - m_scienceInvested); }
    float progress() const { return m_scienceCost > 0 ? float(m_scienceInvested) / m_scienceCost : 1.0f; }
};