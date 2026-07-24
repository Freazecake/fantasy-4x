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

static Player *findPlayer(std::vector<Player> &players, int id)
{
    for (auto &p : players)
        if (p.id() == id)
            return &p;
    return nullptr;
}

static Settlement *findSettlementAt(std::vector<Player> &players, int q, int r)
{
    for (auto &p : players)
    {
        for (auto &s : p.settlements())
        {
            for (const auto &t : s.tiles())
            {
                auto c = t.getCoords();
                if (c.q == q && c.r == r)
                    return &s;
            }
        }
    }
    return nullptr;
}

static bool isAllied(const std::vector<ActiveAlliance> &alliances, int a, int b)
{
    if (a == b)
        return true;
    for (const auto &al : alliances)
    {
        if ((al.playerA == a && al.playerB == b) || (al.playerA == b && al.playerB == a))
            return true;
    }
    return false;
}

static float terrainBonus(TileType type)
{
    switch (type)
    {
    case TileType::Forest:
        return 1.2f;
    case TileType::Mountain:
        return 1.5f;
    default:
        return 1.0f;
    }
}

CombatResult GameState::resolveMovementCombat(Army &attacker, const Coords &destination)
{
    Player *attackerPlayer = findPlayer(m_players, attacker.getOwnerId());
    if (!attackerPlayer)
    {
        CombatResult err;
        return err;
    }

    float attackerRace = attackerPlayer->race().getModifiers().military;

    const Tile *destTile = m_map.tileAt(destination.q, destination.r);
    float defTerrain = destTile ? terrainBonus(destTile->getType()) : 1.0f;

    for (auto &p : m_players)
    {
        if (p.id() == attacker.getOwnerId())
            continue;
        if (isAllied(m_alliances, attacker.getOwnerId(), p.id()))
            continue;

        for (auto &a : p.armies())
        {
            auto pos = a.position();
            if (pos.q == destination.q && pos.r == destination.r)
            {
                std::vector<Unit> atkUnits = attacker.units();
                std::vector<Unit> defUnits = a.units();
                std::vector<Building> emptyBld;

                CombatResult result = Combat::resolve(
                    atkUnits, defUnits,
                    attackerPlayer->heroes(), p.heroes(),
                    defTerrain, emptyBld, attackerRace,
                    p.race().getModifiers().military);

                if (result.attackerWon)
                {
                    attacker.applyCasualties(result.attackerSurvivors);
                    p.removeArmy(a.getId());
                }
                else if (result.isDraw)
                {
                    attacker.applyCasualties(result.attackerSurvivors);
                    a.applyCasualties(result.defenderSurvivors);
                }
                else
                {
                    a.applyCasualties(result.defenderSurvivors);
                    attackerPlayer->removeArmy(attacker.getId());
                }
                return result;
            }
        }
    }

    for (auto &p : m_players)
    {
        if (p.id() == attacker.getOwnerId())
            continue;
        if (isAllied(m_alliances, attacker.getOwnerId(), p.id()))
            continue;

        for (auto it = p.settlements().begin(); it != p.settlements().end(); ++it)
        {
            Settlement &s = *it;
            bool tileMatch = false;
            for (const auto &t : s.tiles())
            {
                auto c = t.getCoords();
                if (c.q == destination.q && c.r == destination.r)
                {
                    tileMatch = true;
                    break;
                }
            }
            if (!tileMatch)
                continue;

            CombatResult result = Combat::resolve(
                attacker.units(), s.garrison(),
                attackerPlayer->heroes(), p.heroes(),
                defTerrain, s.builtBuildings(),
                attackerRace, p.race().getModifiers().military);

            if (result.attackerWon)
            {
                attacker.applyCasualties(result.attackerSurvivors);
                s.setOwnerId(attacker.getOwnerId());
                Settlement captured = s;
                p.settlements().erase(it);
                attackerPlayer->addSettlement(captured);
                result.settlementCaptured = true;
            }
            else if (result.isDraw)
            {
                attacker.applyCasualties(result.attackerSurvivors);
                s.addToGarrison(result.defenderSurvivors);
            }
            else
            {
                s.addToGarrison(result.defenderSurvivors);
                attackerPlayer->removeArmy(attacker.getId());
            }
            return result;
        }
    }

    CombatResult err;
    return err;
}

MoveResult GameState::moveArmy(const std::string &armyId, const Coords &destination)
{
    MoveResult result;

    Army *army = findArmy(armyId);
    if (!army)
    {
        result.errorMessage = "Армия не найдена";
        return result;
    }

    const Tile *destTile = m_map.tileAt(destination.q, destination.r);
    if (!destTile)
    {
        result.errorMessage = "Клетка за пределами карты";
        return result;
    }

    Player *owner = findPlayer(m_players, army->getOwnerId());
    if (!owner)
    {
        result.errorMessage = "Владелец армии не найден";
        return result;
    }

    for (auto &a : owner->armies())
    {
        if (&a == army)
            continue;
        auto pos = a.position();
        if (pos.q == destination.q && pos.r == destination.r)
        {
            result.errorMessage = "На этой клетке уже стоит ваша армия";
            return result;
        }
    }

    for (auto &p : m_players)
    {
        if (p.id() == army->getOwnerId())
            continue;
        if (!isAllied(m_alliances, army->getOwnerId(), p.id()))
            continue;

        for (auto &a : p.armies())
        {
            auto pos = a.position();
            if (pos.q == destination.q && pos.r == destination.r)
            {
                result.errorMessage = "На этой клетке армия союзника";
                return result;
            }
        }

        for (auto &s : p.settlements())
        {
            for (const auto &t : s.tiles())
            {
                auto c = t.getCoords();
                if (c.q == destination.q && c.r == destination.r)
                {
                    result.errorMessage = "На этой клетке поселение союзника";
                    return result;
                }
            }
        }
    }

    int cost = m_map.movementCostTo(army->position(), destination, army->movementPointsRemaining());
    if (cost < 0)
    {
        result.errorMessage = "Недостаточно очков движения";
        return result;
    }

    for (auto &p : m_players)
    {
        if (p.id() == army->getOwnerId())
            continue;
        if (isAllied(m_alliances, army->getOwnerId(), p.id()))
            continue;

        for (auto &a : p.armies())
        {
            auto pos = a.position();
            if (pos.q == destination.q && pos.r == destination.r)
            {
                result.combatOccurred = true;
                result.combat = resolveMovementCombat(*army, destination);
                result.success = true;
                if (result.combat.attackerWon)
                    army->moveTo(destination, cost);
                return result;
            }
        }

        for (auto &s : p.settlements())
        {
            for (const auto &t : s.tiles())
            {
                auto c = t.getCoords();
                if (c.q == destination.q && c.r == destination.r)
                {
                    result.combatOccurred = true;
                    result.combat = resolveMovementCombat(*army, destination);
                    result.settlementCaptured = result.combat.settlementCaptured;
                    result.success = true;
                    if (result.combat.attackerWon)
                        army->moveTo(destination, cost);
                    return result;
                }
            }
        }
    }

    army->moveTo(destination, cost);
    result.success = true;
    return result;
}
