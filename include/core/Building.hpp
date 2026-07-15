#pragma once

#include <string>
#include <vector>

#include "Common.hpp"
#include "external/json.hpp"

using json = nlohmann::json;

class Building
{
private:
    std::string m_id;
    std::string m_name;
    std::string m_category;
    Modifiers m_modifiers;
    int m_cost = 0;
    bool m_isunlocked = false;
    std::vector<std::string> m_unlocksUnitIds;
    bool m_removesUntrainedPenalty = false;

public:
    Building();
    Building(const json &obj);
    static Building loadFromJson(const json &obj);
    static std::vector<Building> loadAllFromJson(const json &arrayNode);

    std::string getId() const { return m_id; }
    std::string getName() const { return m_name; }
    std::string getCategory() const { return m_category; }
    Modifiers getModifiers() const { return m_modifiers; }
    int getCost() const { return m_cost; }
    bool isUnlocked() const { return m_isunlocked; }
    void setUnlocked(bool unlocked) { m_isunlocked = unlocked; }
    const std::vector<std::string> &getUnlocksUnitIds() const { return m_unlocksUnitIds; }
    bool removesUntrainedPenalty() const { return m_removesUntrainedPenalty; }

    std::string toString() const;
};