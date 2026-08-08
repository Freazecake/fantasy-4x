#include "core/GameState.hpp"
#include <set>
#include <utility>
#include <string>

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

    pushEvent(TurnEventType::AllianceFormed, player_1.id(), player_2.id(),
              "Игроки " + std::to_string(player_1.id()) + " и " + std::to_string(player_2.id()) +
                  " заключили союз");
}

void GameState::setMap(const Map &map)
{
    m_map = map;
}

void GameState::nextTurn()
{
    ++m_turnNumber;
}

void GameState::recomputeVisibility(int visionRadius)
{
    for (auto &p : m_players)
        p.recomputeVisibility(m_map, visionRadius);
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

    pushEvent(TurnEventType::TributeDemanded, sourceId, targetId,
              "Игрок " + std::to_string(sourceId) + " потребовал дань у игрока " +
                  std::to_string(targetId));
}

void GameState::applyVassalize(int masterId, int vassalId)
{
    m_vassalizations[vassalId] = masterId;

    pushEvent(TurnEventType::Vassalized, masterId, vassalId,
              "Игрок " + std::to_string(vassalId) + " стал вассалом игрока " +
                  std::to_string(masterId));
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

    pushEvent(TurnEventType::Absorbed, conquerorId, targetId,
              "Игрок " + std::to_string(conquerorId) + " политически поглотил игрока " +
                  std::to_string(targetId));

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

    int totalAliveSettlements = 0;
    for (const auto &p : m_players)
        totalAliveSettlements += static_cast<int>(p.settlements().size());

    if (totalAliveSettlements > 0 && m_players.size() > 1)
    {
        for (const auto &p : m_players)
        {
            if (m_vassalizations.count(p.id()) > 0)
                continue;

            bool hasVassals = false;
            int controlled = static_cast<int>(p.settlements().size());
            for (const auto &kv : m_vassalizations)
            {
                if (kv.second != p.id())
                    continue;
                hasVassals = true;
                for (const auto &vassal : m_players)
                    if (vassal.id() == kv.first)
                        controlled += static_cast<int>(vassal.settlements().size());
            }

            if (hasVassals && controlled == totalAliveSettlements)
                return {p.id(), "political"};
        }
    }

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

static std::string formatCombatLog(const std::vector<CombatRoundLog> &log)
{
    std::string result;
    for (const auto &r : log)
    {
        if (!result.empty())
            result += "; ";
        result += "Р" + std::to_string(r.roundNumber) + ": " + r.attackerUnitId + " → " +
                  r.defenderUnitId + ", урон " + std::to_string(static_cast<int>(r.damageDealt));
        if (r.defenderUnitDied)
            result += " (погиб)";
    }
    return result;
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

                {
                    std::string outcome = result.attackerWon ? "победа атакующего"
                                          : result.isDraw    ? "ничья"
                                                             : "победа защищающегося";
                    std::string roundsLog = formatCombatLog(result.log);
                    pushEvent(TurnEventType::Combat, attacker.getOwnerId(), p.id(),
                              "Бой у (" + std::to_string(destination.q) + ", " +
                                  std::to_string(destination.r) + "): игрок " +
                                  std::to_string(attacker.getOwnerId()) + " атаковал армию игрока " +
                                  std::to_string(p.id()) + " — " + outcome +
                                  (roundsLog.empty() ? "" : " [" + roundsLog + "]"),
                              destination);
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

            {
                std::string outcome = result.attackerWon ? "город взят"
                                      : result.isDraw    ? "ничья, город устоял"
                                                         : "атака отбита";
                std::string roundsLog = formatCombatLog(result.log);
                pushEvent(TurnEventType::Combat, attacker.getOwnerId(), p.id(),
                          "Осада у (" + std::to_string(destination.q) + ", " +
                              std::to_string(destination.r) + "): игрок " +
                              std::to_string(attacker.getOwnerId()) + " атаковал город игрока " +
                              std::to_string(p.id()) + " — " + outcome +
                              (roundsLog.empty() ? "" : " [" + roundsLog + "]"),
                          destination);
            }

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

                pushEvent(TurnEventType::CityCaptured, attacker.getOwnerId(), p.id(),
                          "Игрок " + std::to_string(attacker.getOwnerId()) +
                              " захватил город игрока " + std::to_string(p.id()),
                          destination);
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

    if (owner->isHuman() && !owner->isExplored(destination))
    {
        result.errorMessage = "Клетка не разведана";
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

    pushEvent(TurnEventType::CityFounded, owner->id(), -1,
              "Игрок " + std::to_string(owner->id()) + " основал город у (" +
                  std::to_string(center.q) + ", " + std::to_string(center.r) + ")",
              center);

    result.success = true;
    return result;
}

namespace
{

    std::string tileTypeToString(TileType t)
    {
        switch (t)
        {
        case TileType::Water:
            return "water";
        case TileType::Mountain:
            return "mountain";
        case TileType::Desert:
            return "desert";
        case TileType::Forest:
            return "forest";
        case TileType::Grass:
        default:
            return "grass";
        }
    }

    TileType tileTypeFromString(const std::string &s)
    {
        if (s == "water")
            return TileType::Water;
        if (s == "mountain")
            return TileType::Mountain;
        if (s == "desert")
            return TileType::Desert;
        if (s == "forest")
            return TileType::Forest;
        return TileType::Grass;
    }

    json tileToJson(const Tile &t)
    {
        return json{
            {"q", t.getCoords().q},
            {"r", t.getCoords().r},
            {"type", tileTypeToString(t.getType())},
            {"resource", ResourceCatalog::resourceTypeToString(t.getResource())},
        };
    }

    Tile tileFromJson(const json &j)
    {
        Tile t(j.at("q").get<int>(), j.at("r").get<int>());
        t.setType(tileTypeFromString(j.value("type", std::string("grass"))));
        t.setResource(ResourceCatalog::parseResourceType(j.value("resource", std::string("none"))));
        return t;
    }

    std::string settlementTypeToString(SettlementType t)
    {
        switch (t)
        {
        case SettlementType::PreSettlement:
            return "presettlement";
        case SettlementType::Town:
            return "town";
        case SettlementType::City:
            return "city";
        case SettlementType::Capital:
            return "capital";
        case SettlementType::Village:
        default:
            return "village";
        }
    }

    SettlementType settlementTypeFromString(const std::string &s)
    {
        if (s == "presettlement")
            return SettlementType::PreSettlement;
        if (s == "town")
            return SettlementType::Town;
        if (s == "city")
            return SettlementType::City;
        if (s == "capital")
            return SettlementType::Capital;
        return SettlementType::Village;
    }

    const Unit *findUnitTemplate(const std::vector<Unit> &catalog, const std::string &id)
    {
        for (const auto &u : catalog)
            if (u.getId() == id)
                return &u;
        return nullptr;
    }

    const Building *findBuildingTemplate(const std::vector<Building> &catalog, const std::string &id)
    {
        for (const auto &b : catalog)
            if (b.getId() == id)
                return &b;
        return nullptr;
    }

    const Race *findRaceTemplate(const std::vector<Race> &catalog, const std::string &id)
    {
        for (const auto &r : catalog)
            if (r.getId() == id)
                return &r;
        return nullptr;
    }

    const Hero *findHeroTemplate(const std::vector<Hero> &catalog, const std::string &id)
    {
        for (const auto &h : catalog)
            if (h.getId() == id)
                return &h;
        return nullptr;
    }

    json unitInstanceToJson(const Unit &u)
    {
        return json{
            {"id", u.getId()},
            {"currentHealth", u.getCurrentHealth()},
            {"trained", u.isTrained()},
        };
    }

    Unit unitInstanceFromJson(const json &j, const std::vector<Unit> &catalog)
    {
        std::string id = j.value("id", std::string());
        const Unit *tmpl = findUnitTemplate(catalog, id);
        Unit u = tmpl ? *tmpl : Unit();
        u.setCurrentHealth(j.value("currentHealth", u.getCurrentHealth()));
        u.setTrained(j.value("trained", false));
        return u;
    }

    json settlementToJson(const Settlement &s)
    {
        json tilesJson = json::array();
        for (const auto &t : s.tiles())
            tilesJson.push_back(tileToJson(t));

        json builtJson = json::array();
        for (const auto &b : s.builtBuildings())
            builtJson.push_back(b.getId());

        json queueJson = json::array();
        for (const auto &item : s.buildingQueue())
            queueJson.push_back(json{{"buildingId", item.building.getId()},
                                     {"investedIndustry", item.investedIndustry}});

        json garrisonJson = json::array();
        for (const auto &u : s.garrison())
            garrisonJson.push_back(unitInstanceToJson(u));

        json recruitJson = json::array();
        for (const auto &item : s.recruitmentQueue())
            recruitJson.push_back(json{{"unitId", item.unit.getId()},
                                       {"investedPoints", item.investedPoints}});

        const auto &a = s.allocation();
        return json{
            {"type", settlementTypeToString(s.type())},
            {"ownerId", s.ownerId()},
            {"populationCount", s.populationCount()},
            {"allocation", json{{"science", a.science},
                                {"industry", a.industry},
                                {"trade", a.trade},
                                {"diplomacy", a.diplomacy},
                                {"idle", a.idle}}},
            {"tiles", tilesJson},
            {"builtBuildingIds", builtJson},
            {"buildingQueue", queueJson},
            {"garrison", garrisonJson},
            {"recruitmentQueue", recruitJson},
        };
    }

    Settlement settlementFromJson(const json &j, const std::vector<Building> &buildingCatalog,
                                  const std::vector<Unit> &unitCatalog)
    {
        Settlement s(j.value("ownerId", -1), settlementTypeFromString(j.value("type", std::string("village"))));

        for (const auto &tj : j.value("tiles", json::array()))
            s.addTile(tileFromJson(tj));

        PopulationAllocation alloc;
        if (j.contains("allocation"))
        {
            const auto &aj = j.at("allocation");
            alloc.science = aj.value("science", 0);
            alloc.industry = aj.value("industry", 0);
            alloc.trade = aj.value("trade", 0);
            alloc.diplomacy = aj.value("diplomacy", 0);
            alloc.idle = aj.value("idle", 0);
        }

        std::vector<Building> built;
        for (const auto &bid : j.value("builtBuildingIds", json::array()))
        {
            const Building *tmpl = findBuildingTemplate(buildingCatalog, bid.get<std::string>());
            if (tmpl)
                built.push_back(*tmpl);
        }

        std::vector<BuildingQueueItem> queue;
        for (const auto &qj : j.value("buildingQueue", json::array()))
        {
            const Building *tmpl = findBuildingTemplate(buildingCatalog, qj.value("buildingId", std::string()));
            if (!tmpl)
                continue;
            BuildingQueueItem item;
            item.building = *tmpl;
            item.investedIndustry = qj.value("investedIndustry", 0);
            queue.push_back(item);
        }

        std::vector<Unit> garrison;
        for (const auto &gj : j.value("garrison", json::array()))
            garrison.push_back(unitInstanceFromJson(gj, unitCatalog));

        std::vector<RecruitmentItem> recruitQueue;
        for (const auto &rj : j.value("recruitmentQueue", json::array()))
        {
            const Unit *tmpl = findUnitTemplate(unitCatalog, rj.value("unitId", std::string()));
            if (!tmpl)
                continue;
            RecruitmentItem item;
            item.unit = *tmpl;
            item.investedPoints = rj.value("investedPoints", 0);
            recruitQueue.push_back(item);
        }

        s.restoreFromSave(j.value("populationCount", 0), alloc, built, queue, garrison, recruitQueue);
        return s;
    }

    json armyToJson(const Army &a)
    {
        json unitsJson = json::array();
        for (const auto &u : a.units())
            unitsJson.push_back(unitInstanceToJson(u));

        return json{
            {"id", a.getId()},
            {"ownerId", a.getOwnerId()},
            {"position", json{{"q", a.position().q}, {"r", a.position().r}}},
            {"movementPointsRemaining", a.movementPointsRemaining()},
            {"units", unitsJson},
        };
    }

    Army armyFromJson(const json &j, const std::vector<Unit> &unitCatalog)
    {
        std::vector<Unit> units;
        for (const auto &uj : j.value("units", json::array()))
            units.push_back(unitInstanceFromJson(uj, unitCatalog));

        Coords pos(j.at("position").value("q", 0), j.at("position").value("r", 0));
        Army army(j.value("id", std::string()), j.value("ownerId", -1), pos, units);
        army.setMovementPointsRemaining(j.value("movementPointsRemaining", army.movementPointsMax()));
        return army;
    }

    json playerToJson(const Player &p)
    {
        json heroIds = json::array();
        for (const auto &h : p.heroes())
            heroIds.push_back(h.getId());

        json researchedIds = json::array();
        for (const auto &id : p.researchedTechIds())
            researchedIds.push_back(id);

        // Прогресс по КАЖДОЙ технологии с ненулевым вложением науки или уже
        // завершённой — иначе после загрузки частично вложенная наука сгорит.
        json techProgress = json::array();
        for (const auto &t : p.technologies())
        {
            if (t.isResearched() || t.scienceInvested() > 0)
            {
                techProgress.push_back(json{{"id", t.getId()},
                                            {"researched", t.isResearched()},
                                            {"scienceInvested", t.scienceInvested()}});
            }
        }

        json settlementsJson = json::array();
        for (const auto &s : p.settlements())
            settlementsJson.push_back(settlementToJson(s));

        json armiesJson = json::array();
        for (const auto &a : p.armies())
            armiesJson.push_back(armyToJson(a));

        json exploredJson = json::array();
        for (const auto &c : p.exploredTiles())
            exploredJson.push_back(json{{"q", c.first}, {"r", c.second}});

        json visibleJson = json::array();
        for (const auto &c : p.visibleTiles())
            visibleJson.push_back(json{{"q", c.first}, {"r", c.second}});

        const Resources &res = p.resources();

        return json{
            {"id", p.id()},
            {"isHuman", p.isHuman()},
            {"raceId", p.race().getId()},
            {"heroIds", heroIds},
            {"allianceBonus", p.allianceBonus()},
            {"currentResearchId", p.currentResearchId()},
            {"researchedTechIds", researchedIds},
            {"technologyProgress", techProgress},
            {"resources", json{{"science", res.science},
                               {"industry", res.industry},
                               {"trade", res.trade},
                               {"diplomacy", res.diplomacy},
                               {"military", res.military},
                               {"growth", res.growth}}},
            {"settlements", settlementsJson},
            {"armies", armiesJson},
            {"exploredTiles", exploredJson},
            {"visibleTiles", visibleJson},
        };
    }

    Player playerFromJson(const json &j, const std::vector<Race> &raceCatalog,
                          const std::vector<Hero> &heroCatalog,
                          const std::vector<Technology> &techCatalog,
                          const std::vector<Building> &buildingCatalog,
                          const std::vector<Unit> &unitCatalog)
    {
        Player p;
        p.setId(j.value("id", -1));
        p.setIsHuman(j.value("isHuman", false));

        const Race *race = findRaceTemplate(raceCatalog, j.value("raceId", std::string()));
        if (race)
            p.setRace(*race);

        p.loadTechnologies(techCatalog);

        for (const auto &hid : j.value("heroIds", json::array()))
        {
            const Hero *h = findHeroTemplate(heroCatalog, hid.get<std::string>());
            if (h)
                p.addHero(*h);
        }

        p.setAllianceBonus(j.value("allianceBonus", false));

        std::unordered_set<std::string> researched;
        for (const auto &id : j.value("researchedTechIds", json::array()))
            researched.insert(id.get<std::string>());
        p.restoreResearchState(j.value("currentResearchId", std::string()), researched);

        for (const auto &tj : j.value("technologyProgress", json::array()))
        {
            std::string techId = tj.value("id", std::string());
            for (auto &t : p.technologies())
            {
                if (t.getId() == techId)
                {
                    t.restoreProgress(tj.value("researched", false), tj.value("scienceInvested", 0));
                    break;
                }
            }
        }

        if (j.contains("resources"))
        {
            const auto &rj = j.at("resources");
            Resources res;
            res.science = rj.value("science", 0.0f);
            res.industry = rj.value("industry", 0.0f);
            res.trade = rj.value("trade", 0.0f);
            res.diplomacy = rj.value("diplomacy", 0.0f);
            res.military = rj.value("military", 0.0f);
            res.growth = rj.value("growth", 0.0f);
            p.addResources(res);
        }

        for (const auto &sj : j.value("settlements", json::array()))
            p.addSettlement(settlementFromJson(sj, buildingCatalog, unitCatalog));

        for (const auto &aj : j.value("armies", json::array()))
            p.addArmy(armyFromJson(aj, unitCatalog));

        std::set<std::pair<int, int>> explored, visible;
        for (const auto &cj : j.value("exploredTiles", json::array()))
            explored.insert({cj.value("q", 0), cj.value("r", 0)});
        for (const auto &cj : j.value("visibleTiles", json::array()))
            visible.insert({cj.value("q", 0), cj.value("r", 0)});
        p.restoreVisibility(explored, visible);

        return p;
    }

    std::string globalPhaseToString(GlobalPhase p)
    {
        switch (p)
        {
        case GlobalPhase::Phase2_OpenMap:
            return "phase2";
        case GlobalPhase::Phase3_Final:
            return "phase3";
        case GlobalPhase::NotStarted:
        default:
            return "notstarted";
        }
    }

    GlobalPhase globalPhaseFromString(const std::string &s)
    {
        if (s == "phase2")
            return GlobalPhase::Phase2_OpenMap;
        if (s == "phase3")
            return GlobalPhase::Phase3_Final;
        return GlobalPhase::NotStarted;
    }

} // namespace

json GameState::toJson() const
{
    json playersJson = json::array();
    for (const auto &p : m_players)
        playersJson.push_back(playerToJson(p));

    json alliancesJson = json::array();
    for (const auto &a : m_alliances)
        alliancesJson.push_back(
            json{{"playerA", a.playerA}, {"playerB", a.playerB}, {"expiryTurn", a.expiryTurn}});

    json vassalJson = json::array();
    for (const auto &kv : m_vassalizations)
        vassalJson.push_back(json{{"vassalId", kv.first}, {"masterId", kv.second}});

    json tilesJson = json::array();
    for (const auto &t : m_map.getMap())
        tilesJson.push_back(tileToJson(t));

    return json{
        {"turnNumber", m_turnNumber},
        {"globalPhase", globalPhaseToString(m_globalPhase)},
        {"politicalConquestWinnerId", m_politicalConquestWinnerId},
        {"alliances", alliancesJson},
        {"vassalizations", vassalJson},
        {"map", json{{"tiles", tilesJson}}},
        {"players", playersJson},
    };
}

GameState GameState::fromJson(const json &j,
                              const std::vector<Race> &raceCatalog,
                              const std::vector<Hero> &heroCatalog,
                              const std::vector<Technology> &techCatalog,
                              const std::vector<Building> &buildingCatalog,
                              const std::vector<Unit> &unitCatalog)
{
    GameState state;

    Map map;
    for (const auto &tj : j.at("map").value("tiles", json::array()))
        map.getMap().push_back(tileFromJson(tj));
    state.setMap(map);

    for (const auto &pj : j.value("players", json::array()))
        state.addPlayer(playerFromJson(pj, raceCatalog, heroCatalog, techCatalog, buildingCatalog, unitCatalog));

    for (const auto &aj : j.value("alliances", json::array()))
    {
        ActiveAlliance a;
        a.playerA = aj.value("playerA", -1);
        a.playerB = aj.value("playerB", -1);
        a.expiryTurn = aj.value("expiryTurn", 0);
        state.alliances().push_back(a);
    }

    std::unordered_map<int, int> vassalizations;
    for (const auto &vj : j.value("vassalizations", json::array()))
        vassalizations[vj.value("vassalId", -1)] = vj.value("masterId", -1);
    state.setVassalizations(vassalizations);

    state.setTurnNumber(j.value("turnNumber", 0));
    state.setPhase(globalPhaseFromString(j.value("globalPhase", std::string("notstarted"))));
    state.setPoliticalConquestWinnerId(j.value("politicalConquestWinnerId", -1));

    return state;
}
