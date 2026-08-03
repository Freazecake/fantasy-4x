#include "core/GameState.hpp"

#include <algorithm>

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

bool GameState::updatePhase()
{
    if (m_globalPhase == GlobalPhase::NotStarted)
    {
        bool someoneHasSecondSettlement = false;
        for (const auto &p : m_players)
        {
            if (p.settlements().size() >= 2)
            {
                someoneHasSecondSettlement = true;
                break;
            }
        }

        if (someoneHasSecondSettlement || m_turnNumber >= PHASE2_TURN_THRESHOLD)
        {
            nextPhase();
            return true;
        }
        return false;
    }

    if (m_globalPhase == GlobalPhase::Phase2_OpenMap)
    {
        int aliveCount = 0;
        for (const auto &p : m_players)
            if (!p.settlements().empty())
                ++aliveCount;

        if (aliveCount <= PHASE3_MAX_ALIVE_PLAYERS || m_turnNumber >= PHASE3_TURN_THRESHOLD)
        {
            nextPhase();
            return true;
        }
        return false;
    }

    return false;
}

bool GameState::isAllied(const GameState &state, int a, int b)
{
    if (a == b)
        return true;
    for (const auto &alliance : state.m_alliances)
    {
        if ((alliance.playerA == a && alliance.playerB == b) ||
            (alliance.playerA == b && alliance.playerB == a))
            return true;
    }
    return false;
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

    Resources taken = target->takeCappedFractionFromEconomy(0.3f, TRIBUTE_STOCKPILE_CAP);
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
            Resources taken = vassal->takeCappedFractionFromEconomy(0.3f, TRIBUTE_STOCKPILE_CAP);
            master->addResources(taken);
        }
    }
}

bool GameState::canPoliticalConquer(int conquerorId, int targetId) const
{
    const Player *conqueror = nullptr, *target = nullptr;
    for (const auto &p : m_players)
    {
        if (p.id() == conquerorId)
            conqueror = &p;
        if (p.id() == targetId)
            target = &p;
    }
    if (!conqueror || !target)
        return false;
    if (target->settlements().empty())
        return false;
    if (conqueror->resources().diplomacy <= target->resources().diplomacy)
        return false;
    return true;
}

int GameState::politicalConquestCost(int conquerorId, int targetId) const
{
    const Player *conqueror = nullptr, *target = nullptr;
    for (const auto &p : m_players)
    {
        if (p.id() == conquerorId)
            conqueror = &p;
        if (p.id() == targetId)
            target = &p;
    }
    if (!target)
        return POLITICAL_CONQUEST_BASE_COST;

    int conquerorExtra = conqueror ? static_cast<int>(conqueror->settlements().size()) - 1 : 0;
    if (conquerorExtra < 0)
        conquerorExtra = 0;

    int extra = static_cast<int>(target->settlements().size()) - 1;
    if (extra < 0)
        extra = 0;
    int militaryCost = static_cast<int>(target->militaryPower());
    if (militaryCost > 200)
        militaryCost = 200;
    return POLITICAL_CONQUEST_BASE_COST +
           POLITICAL_CONQUEST_COST_PER_EXTRA_SETTLEMENT * extra +
           POLITICAL_CONQUEST_COST_PER_EXTRA_SETTLEMENT * conquerorExtra + militaryCost;
}

bool GameState::applyPoliticalConquest(int conquerorId, int targetId)
{
    if (!canPoliticalConquer(conquerorId, targetId))
        return false;

    Player *conqueror = nullptr, *target = nullptr;
    for (auto &p : m_players)
    {
        if (p.id() == conquerorId)
            conqueror = &p;
        if (p.id() == targetId)
            target = &p;
    }
    if (!conqueror || !target)
        return false;

    for (auto &s : target->settlements())
    {
        s.sufferCaptureLoss(0.5f);
        s.setOwnerId(conquerorId);
        conqueror->addSettlement(s);
    }
    target->settlements().clear();

    Resources taken =
        target->takeCappedFractionFromEconomy(0.5f, POLITICAL_CONQUEST_STOCKPILE_CAP);
    conqueror->addResources(taken);

    int aliveCount = 0;
    for (const auto &p : m_players)
        if (!p.settlements().empty())
            ++aliveCount;
    if (aliveCount <= 1)
        m_politicalConquestWinnerId = conquerorId;

    return true;
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
        if (isAllied(*this, attacker.getOwnerId(), p.id()))
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
        if (isAllied(*this, attacker.getOwnerId(), p.id()))
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
                s.sufferCaptureLoss(0.5f);
                s.clearGarrison();
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
        if (!isAllied(*this, army->getOwnerId(), p.id()))
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
        if (isAllied(*this, army->getOwnerId(), p.id()))
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

std::string GameState::settlementFoundingBlockReason(const Coords &center) const
{
    const Tile *centerTile = m_map.tileAt(center.q, center.r);
    if (!centerTile)
        return "Клетка за пределами карты";
    if (centerTile->getType() == TileType::Water)
        return "Нельзя основать город на воде";

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            int q = center.q + dx, r = center.r + dy;
            const Tile *t = m_map.tileAt(q, r);
            if (!t)
                continue; // за краем карты — просто не войдёт в площадку города

            for (const auto &p : m_players)
            {
                for (const auto &s : p.settlements())
                {
                    for (const auto &st : s.tiles())
                    {
                        Coords c = st.getCoords();
                        if (c.q == q && c.r == r)
                            return "Клетка уже занята другим поселением";
                    }
                }
            }
        }
    }

    return "";
}

bool GameState::canFoundSettlementAt(const Coords &center) const
{
    return settlementFoundingBlockReason(center).empty();
}

FoundSettlementResult GameState::foundSettlementFromArmy(const std::string &armyId)
{
    FoundSettlementResult result;

    Army *army = findArmy(armyId);
    if (!army || army->units().empty())
    {
        result.errorMessage = "Армия не найдена";
        return result;
    }

    if (!army->isSettlerArmy())
    {
        result.errorMessage = "Основать город может только армия, состоящая из одного поселенца";
        return result;
    }

    Player *owner = findPlayer(m_players, army->getOwnerId());
    if (!owner)
    {
        result.errorMessage = "Владелец армии не найден";
        return result;
    }

    Coords center = army->position();
    std::string blockReason = settlementFoundingBlockReason(center);
    if (!blockReason.empty())
    {
        result.errorMessage = blockReason;
        return result;
    }

    Settlement founded(owner->id(), SettlementType::Village);
    founded.seedInitialPopulation(SETTLER_FOUNDED_INITIAL_POPULATION);

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            int q = center.q + dx, r = center.r + dy;
            if (m_map.tileAt(q, r))
                founded.addTile(Tile(q, r));
        }
    }

    owner->addSettlement(founded);
    owner->removeArmy(armyId);

    result.success = true;
    return result;
}
