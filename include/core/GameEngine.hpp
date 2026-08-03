#pragma once

#include <string>
#include <vector>

#include "GameState.hpp"
#include "Building.hpp"
#include "Technology.hpp"
#include "Unit.hpp"
#include "PoliticalAction.hpp"
#include "ResourceCatalog.hpp"
#include "ai/AIController.hpp"

class GameEngine
{
private:
    GameState m_state;
    std::vector<Building> m_buildingCatalog;
    std::vector<Technology> m_techCatalog;
    std::vector<Unit> m_unitCatalog;
    std::vector<PoliticalAction> m_politicalActionCatalog;
    std::vector<ResourceDef> m_resourceCatalog;

public:
    bool loadFromAssets(const std::string &assetsDir, unsigned int mapSeed = 0);
    void endTurn();
    std::string applyPoliticalAction(int playerId, const std::string &actionId, int targetId);
    VictoryResult checkVictoryCondition() const { return m_state.checkVictory(); }

    const GameState &state() const { return m_state; }
    GameState &state() { return m_state; }

    const std::vector<Building> &buildingCatalog() const { return m_buildingCatalog; }
    const std::vector<Technology> &techCatalog() const { return m_techCatalog; }
    const std::vector<Unit> &unitCatalog() const { return m_unitCatalog; }
    const std::vector<PoliticalAction> &politicalActionCatalog() const { return m_politicalActionCatalog; }
    const std::vector<ResourceDef> &resourceCatalog() const { return m_resourceCatalog; }
};
