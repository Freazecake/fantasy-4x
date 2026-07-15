#pragma once

#include <string>
#include <vector>

#include "external/json.hpp"

using json = nlohmann::json;

enum class PoliticalActionType
{
    Alliance,
    Vassalize,
    DemandTribute,
    PoliticalConquest,
};

PoliticalActionType politicalActionTypeFromString(const std::string &s);
std::string politicalActionTypeToString(PoliticalActionType t);

class PoliticalAction
{
private:
    std::string m_id;
    std::string m_name;
    int m_diplomacyCost = 0;
    PoliticalActionType m_type = PoliticalActionType::Alliance;

public:
    PoliticalAction() = default;
    PoliticalAction(const json &obj) { *this = PoliticalAction::loadFromJson(obj); }
    ~PoliticalAction() = default;

    static PoliticalAction loadFromJson(const json &obj);
    static std::vector<PoliticalAction> loadAllFromJson(const json &arr);

    std::string getId() const { return m_id; }
    std::string getName() const { return m_name; }
    int getCost() const { return m_diplomacyCost; }
    PoliticalActionType getType() const { return m_type; }

    bool isVictoryAction() const { return m_type == PoliticalActionType::PoliticalConquest; }

    std::string toString() const;
};
