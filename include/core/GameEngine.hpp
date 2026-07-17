#pragma once

#include <string>
#include <vector>

#include "GameState.hpp"
#include "Building.hpp"
#include "Technology.hpp"
#include "Unit.hpp"

class GameEngine
{
private:
    GameState m_state;
    std::vector<Building> m_buildingCatalog;
    std::vector<Technology> m_techCatalog;
    std::vector<Unit> m_unitCatalog;

public:
    bool loadFromAssets(const std::string &assetsDir);
    void endTurn();

    const GameState &state() const { return m_state; }
    GameState &state() { return m_state; }

    const std::vector<Building> &buildingCatalog() const { return m_buildingCatalog; }
    const std::vector<Technology> &techCatalog() const { return m_techCatalog; }
    const std::vector<Unit> &unitCatalog() const { return m_unitCatalog; }
};
