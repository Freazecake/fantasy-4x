#include "core/GameState.hpp"

void GameState::addPlayer(const Player &player)
{
    m_players.push_back(player);
}

void GameState::newAlliance(Player &player_1, Player &player_2, int expTurn)
{
    ActiveAlliance all;
    all.playerA = player_1.id();
    all.playerB = player_2.id();
    all.expiryTurn = expTurn;
    m_alliances.push_back(all);

    player_1.setAllianceBonus(true);
    player_2.setAllianceBonus(true);
}

void GameState::setMap(const Map &map)
{
    m_map = map;
}

void GameState::nextTurn()
{
    ++m_turnNumber;
}

void GameState::checkAlliances()
{
    std::vector<ActiveAlliance> active;
    for (const auto &a : m_alliances)
        if (a.expiryTurn > m_turnNumber)
            active.push_back(a);
    m_alliances = std::move(active);

    for (auto &p : m_players)
    {
        bool has = false;
        for (const auto &a : m_alliances)
            if (a.playerA == p.id() || a.playerB == p.id())
            {
                has = true;
                break;
            }
        p.setAllianceBonus(has);
    }
}

void GameState::applyDemandTribute(int sourceId, int targetId)
{
    Player *source = nullptr, *target = nullptr;
    for (auto &p : m_players)
    {
        if (p.id() == sourceId) source = &p;
        if (p.id() == targetId) target = &p;
    }
    if (!source || !target) return;

    Resources taken = target->takeFractionFromEconomy(0.3f);
    source->addResources(taken);
}
