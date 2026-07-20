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
        if (p.id() == sourceId)
            source = &p;
        if (p.id() == targetId)
            target = &p;
    }
    if (!source || !target)
        return;

    Resources taken = target->takeFractionFromEconomy(0.3f);
    source->addResources(taken);
}

void GameState::applyVassalize(int masterId, int vassalId)
{
    m_vassalizations[vassalId] = masterId;
}

void GameState::processVassalage()
{
    for (const auto &[vassalId, masterId] : m_vassalizations)
    {
        Player *vassal = nullptr, *master = nullptr;
        for (auto &p : m_players)
        {
            if (p.id() == vassalId)
                vassal = &p;
            if (p.id() == masterId)
                master = &p;
        }
        if (vassal && master)
        {
            Resources taken = vassal->takeFractionFromEconomy(0.3f);
            master->addResources(taken);
        }
    }
}

void GameState::applyPoliticalConquest(int conquerorId, int targetId)
{
    Player *conqueror = nullptr, *target = nullptr;
    for (auto &p : m_players)
    {
        if (p.id() == conquerorId)
            conqueror = &p;
        if (p.id() == targetId)
            target = &p;
    }
    if (!conqueror || !target)
        return;

    for (auto &s : target->settlements())
    {
        s.setOwnerId(conquerorId);
        conqueror->addSettlement(s);
    }
    target->settlements().clear();

    Resources taken = target->takeFractionFromEconomy(1.0f);
    conqueror->addResources(taken);

    int aliveCount = 0;
    for (const auto &p : m_players)
        if (!p.settlements().empty())
            ++aliveCount;
    if (aliveCount <= 1)
        m_politicalConquestWinnerId = conquerorId;
}

VictoryResult GameState::checkVictory() const
{
    if (m_politicalConquestWinnerId >= 0)
        return {m_politicalConquestWinnerId, "political"};

    int aliveId = -1, aliveCount = 0;
    for (const auto &p : m_players)
    {
        if (!p.settlements().empty())
        {
            ++aliveCount;
            aliveId = p.id();
        }
    }
    if (aliveCount == 1 && m_players.size() > 1)
        return {aliveId, "military"};

    return {};
}

Army *GameState::findArmy(const std::string &id)
{
    for (auto &p : m_players)
    {
        for (auto &a : p.armies())
        {
            if (a.getId() == id)
                return &a;
        }
    }
    return nullptr;
}
