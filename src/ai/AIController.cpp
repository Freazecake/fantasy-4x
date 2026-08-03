#include "ai/AIController.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace
{
    struct Candidate
    {
        Coords destination;
        float score;
    };

    Player *findPlayerById(GameState &state, int id)
    {
        for (auto &p : state.players())
            if (p.id() == id)
                return &p;
        return nullptr;
    }
}

void AIController::takeTurn(Player &bot, GameState &state,
                            const std::vector<Building> &buildingCatalog,
                            const std::vector<Technology> &techCatalog,
                            const std::vector<Unit> &unitCatalog,
                            const std::vector<PoliticalAction> &politicalActionCatalog)
{
    decideTradeExchange(bot, state.CurPhase());
    decideResearch(bot, techCatalog, state.CurPhase());
    for (auto &s : bot.settlements())
    {
        decideRecruitment(s, unitCatalog, state.CurPhase());
        decideConstruction(s, buildingCatalog, state.CurPhase());
        decideAllocations(s, state.CurPhase());
    }
    decideDeployment(bot);
    decideMilitaryActions(bot, state, state.CurPhase());
    decideSettlerActions(bot, state, state.CurPhase());
    decidePoliticalAction(bot, politicalActionCatalog, state);
}

void AIController::decideAllocations(Settlement &s, GlobalPhase phase)
{
    struct SphereScore
    {
        Sphere sphere;
        float score;
    };

    SphereScore scores[4] = {
        {Sphere::Science, AIStrategy::scoreAllocatePopulation(s, Sphere::Science, phase)},
        {Sphere::Industry, AIStrategy::scoreAllocatePopulation(s, Sphere::Industry, phase)},
        {Sphere::Diplomacy, AIStrategy::scoreAllocatePopulation(s, Sphere::Diplomacy, phase)},
        {Sphere::Trade, AIStrategy::scoreAllocatePopulation(s, Sphere::Trade, phase)},
    };

    float total = 0.0f;
    for (const auto &ss : scores)
        total += ss.score;

    if (total <= 0.0f)
        return;

    int idle = s.allocation().idle;
    if (idle <= 0)
        return;

    // Keep idle population reserved for settler recruitment during expansion
    // phases, so the AI can actually grow new settlements instead of sinking
    // every spare citizen into the four economic spheres.
    constexpr int SETTLER_POP_RESERVE = 40;
    int reserve = 0;
    if (phase != GlobalPhase::Phase3_Final)
        reserve = std::min(idle, SETTLER_POP_RESERVE);

    int allocatable = idle - reserve;
    if (allocatable <= 0)
        return;

    int assigned = 0;
    for (const auto &ss : scores)
    {
        int amount = static_cast<int>(std::floor(ss.score / total * static_cast<float>(allocatable)));
        amount = std::min(amount, allocatable - assigned);
        if (amount > 0)
        {
            s.assignFromIdle(ss.sphere, amount);
            assigned += amount;
        }
    }

    int remaining = allocatable - assigned;
    if (remaining > 0)
    {
        auto best = std::max_element(scores, scores + 4,
                                     [](const SphereScore &a, const SphereScore &b)
                                     { return a.score < b.score; });
        s.assignFromIdle(best->sphere, remaining);
    }
}

void AIController::decideConstruction(Settlement &s, const std::vector<Building> &catalog, GlobalPhase phase)
{
    std::vector<std::pair<float, const Building *>> sc;
    for (const auto &b : catalog)
        sc.push_back({AIStrategy::scoreBuild(s, b, phase), &b});

    std::sort(sc.begin(), sc.end(), [](const auto &a, const auto &b)
              { return a.first > b.first; });

    if (!sc.empty() && sc[0].first > 0.0f)
        s.queueBuilding(*sc[0].second);
}

void AIController::decideResearch(Player &p, const std::vector<Technology> &catalog, GlobalPhase phase)
{
    std::vector<std::pair<float, const Technology *>> sc;
    for (const auto &t : catalog)
        sc.push_back({AIStrategy::scoreResearch(p, t, phase), &t});

    std::sort(sc.begin(), sc.end(), [](const auto &a, const auto &b)
              { return a.first > b.first; });

    if (!sc.empty() && sc[0].first > 0.0f)
        p.setCurrentResearch(sc[0].second->getId());
}

void AIController::decideTradeExchange(Player &p, GlobalPhase phase)
{
    static const std::vector<std::string> targets = {"science", "industry"};

    const std::string *best = nullptr;
    float bestScore = 0.0f;
    for (const auto &target : targets)
    {
        float score = AIStrategy::scoreTradeExchange(p, target, phase);
        if (score > bestScore)
        {
            bestScore = score;
            best = &target;
        }
    }

    if (!best)
        return;

    int tradeToSpend = static_cast<int>(p.resources().trade * 0.8f);
    if (tradeToSpend < 1)
        return;

    p.exchangeTrade(*best, tradeToSpend);
}

void AIController::decideRecruitment(Settlement &s, const std::vector<Unit> &catalog, GlobalPhase phase)
{
    constexpr size_t GARRISON_CAP = 5;
    constexpr size_t GARRISON_FLOOR = 3;
    bool hasCombatUnits = false;
    size_t combatCount = 0;
    for (const auto &u : s.garrison())
        if (!u.isSettler())
        {
            hasCombatUnits = true;
            ++combatCount;
        }

    std::vector<std::pair<float, const Unit *>> sc;
    for (const auto &u : catalog)
        sc.push_back({AIStrategy::scoreRecruitUnit(s, u, phase), &u});

    std::sort(sc.begin(), sc.end(), [](const auto &a, const auto &b)
              { return a.first > b.first; });

    if (sc.empty() || sc[0].first <= 0.0f)
        return;

    const Unit *best = sc[0].second;
    if (best->isSettler() && combatCount < GARRISON_FLOOR)
        return;

    if (best->isSettler() || s.garrison().size() < GARRISON_CAP || !hasCombatUnits)
        s.queueUnitRecruitment(*best);
}

void AIController::decidePoliticalAction(Player &p, const std::vector<PoliticalAction> &catalog, GameState &state)
{
    struct ActionCandidate
    {
        const PoliticalAction *action;
        int targetId;
        float score;
        int cost;
    };

    const PoliticalAction *conquestAction = nullptr;
    for (const auto &a : catalog)
        if (a.getType() == PoliticalActionType::PoliticalConquest)
            conquestAction = &a;

    // When a political conquest is viable, commit the AI to saving for it:
    // no tribute/vassal spending that would drain the stockpile before the annexation.
    if (conquestAction)
    {
        ActionCandidate bestConquest{nullptr, -1, -1.0f, -1};
        for (const auto &pl : state.players())
        {
            if (pl.id() == p.id())
                continue;
            if (pl.settlements().empty())
                continue;
            if (!state.canPoliticalConquer(p.id(), pl.id()))
                continue;

            int cost = state.politicalConquestCost(p.id(), pl.id());
            float s = AIStrategy::scorePoliticalAction(p, *conquestAction, pl, state.CurPhase());
            if (s <= 0.0f)
                continue;
            if (s > bestConquest.score)
                bestConquest = {conquestAction, pl.id(), s, cost};
        }

        if (bestConquest.action != nullptr)
        {
            if (p.resources().diplomacy < static_cast<float>(bestConquest.cost))
                return;
            if (!p.trySpendDiplomacy(bestConquest.cost))
                return;
            state.applyPoliticalConquest(p.id(), bestConquest.targetId);
            return;
        }
    }

    std::vector<ActionCandidate> candidates;

    for (const auto &a : catalog)
    {
        if (a.getType() == PoliticalActionType::PoliticalConquest)
            continue;

        for (const auto &pl : state.players())
        {
            if (pl.id() == p.id())
                continue;

            if (pl.settlements().empty())
                continue;

            if (a.getType() == PoliticalActionType::Alliance &&
                GameState::isAllied(state, p.id(), pl.id()))
                continue;

            int cost = a.getCost();
            if (p.resources().diplomacy < static_cast<float>(cost))
                continue;

            float s = AIStrategy::scorePoliticalAction(p, a, pl, state.CurPhase());
            if (s > 0.0f)
                candidates.push_back({&a, pl.id(), s, cost});
        }
    }

    if (candidates.empty())
        return;

    auto best = std::max_element(candidates.begin(), candidates.end(),
                                 [](const ActionCandidate &a, const ActionCandidate &b)
                                 { return a.score < b.score; });

    if (best->score <= 0.0f)
        return;

    if (!p.trySpendOnPoliticalAction(*best->action))
        return;

    switch (best->action->getType())
    {
    case PoliticalActionType::Alliance:
    {
        Player *target = findPlayerById(state, best->targetId);
        if (target)
            state.newAlliance(p, *target, state.turnNumber() + 5);
        break;
    }
    case PoliticalActionType::DemandTribute:
        state.applyDemandTribute(p.id(), best->targetId);
        break;
    case PoliticalActionType::Vassalize:
        state.applyVassalize(p.id(), best->targetId);
        break;
    case PoliticalActionType::PoliticalConquest:
        break;
    }
}

void AIController::decideMilitaryActions(Player &p, GameState &state, GlobalPhase phase)
{
    std::vector<std::string> armyIds;
    for (const auto &a : p.armies())
        armyIds.push_back(a.getId());

    for (const auto &armyId : armyIds)
    {
        Army *mine = state.findArmy(armyId);
        if (!mine || mine->units().empty())
            continue;

        if (mine->isSettlerArmy())
            continue;

        if (mine->movementPointsRemaining() <= 0)
            continue;

        std::vector<Candidate> candidates;

        Coords lastCityTarget{};
        bool hasLastCityTarget = false;
        if (phase == GlobalPhase::Phase3_Final)
        {
            for (const auto &enemy : state.players())
            {
                if (GameState::isAllied(state, p.id(), enemy.id()))
                    continue;
                if (enemy.settlements().size() != 1)
                    continue;
                for (const auto &s : enemy.settlements())
                {
                    if (s.tiles().empty())
                        continue;
                    lastCityTarget = s.tiles().front().getCoords();
                    hasLastCityTarget = true;
                    break;
                }
                if (hasLastCityTarget)
                    break;
            }
        }

        for (const auto &enemy : state.players())
        {
            if (GameState::isAllied(state, p.id(), enemy.id()))
                continue;

            for (const auto &s : enemy.settlements())
            {
                if (s.tiles().empty())
                    continue;

                Coords target = s.tiles().front().getCoords();

                if (hasLastCityTarget &&
                    !(target.q == lastCityTarget.q && target.r == lastCityTarget.r))
                    continue;

                if (state.map().movementCostTo(mine->position(), target,
                                               mine->movementPointsRemaining()) < 0)
                    continue;

                Army garrisonAsArmy("enemy_garrison", s.ownerId(), target, s.garrison());

                float score = AIStrategy::scoreAttack(*mine, garrisonAsArmy, phase,
                                                      static_cast<int>(enemy.settlements().size()));
                candidates.push_back({target, score});
            }

            for (const auto &enemyArmy : enemy.armies())
            {
                if (hasLastCityTarget)
                    continue;

                Coords target = enemyArmy.position();

                if (state.map().movementCostTo(mine->position(), target,
                                               mine->movementPointsRemaining()) < 0)
                    continue;

                float score = AIStrategy::scoreAttack(*mine, enemyArmy, phase,
                                                      static_cast<int>(enemy.settlements().size()));
                candidates.push_back({target, score});
            }
        }

        std::vector<Coords> occupied;
        for (const auto &pl : state.players())
        {
            for (const auto &s : pl.settlements())
                for (const auto &t : s.tiles())
                    occupied.push_back(t.getCoords());
            if (pl.id() == p.id() || GameState::isAllied(state, p.id(), pl.id()))
            {
                for (const auto &ar : pl.armies())
                    occupied.push_back(ar.position());
            }
        }

        if (candidates.empty())
        {
            pursueNearestEnemy(p, state, *mine, armyId, occupied);
            continue;
        }

        auto best = std::max_element(candidates.begin(), candidates.end(),
                                     [](const Candidate &a, const Candidate &b)
                                     { return a.score < b.score; });

        if (best->score <= 0.0f)
        {
            pursueNearestEnemy(p, state, *mine, armyId, occupied);
            continue;
        }

        state.moveArmy(armyId, best->destination);
    }
}

void AIController::pursueNearestEnemy(Player &p, GameState &state, Army &mine,
                                      const std::string &armyId, const std::vector<Coords> &occupied)
{
    const Settlement *nearestEnemySettlement = nullptr;
    int nearestDist = -1;

    for (const auto &enemy : state.players())
    {
        if (enemy.id() == p.id() || GameState::isAllied(state, p.id(), enemy.id()))
            continue;

        for (const auto &s : enemy.settlements())
        {
            if (s.tiles().empty())
                continue;

            Coords target = s.tiles().front().getCoords();
            int dist = mine.position().distanceTo(target);
            if (nearestDist < 0 || dist < nearestDist)
            {
                nearestDist = dist;
                nearestEnemySettlement = &s;
            }
        }
    }

    if (!nearestEnemySettlement)
        return;

    Coords targetCoords = nearestEnemySettlement->tiles().front().getCoords();

    Coords bestStep{};
    int bestDist = nearestDist;
    bool found = false;

    for (const auto &dest : state.map().reachableTiles(mine.position(), mine.movementPointsRemaining()))
    {
        bool blocked = false;
        for (const auto &o : occupied)
        {
            if (o.q == dest.q && o.r == dest.r)
            {
                blocked = true;
                break;
            }
        }
        if (blocked)
            continue;

        int d = dest.distanceTo(targetCoords);
        if (d < bestDist)
        {
            bestDist = d;
            bestStep = dest;
            found = true;
        }
    }

    if (found)
        state.moveArmy(armyId, bestStep);
}

void AIController::decideSettlerActions(Player &p, GameState &state, GlobalPhase phase)
{
    std::vector<std::string> settlerArmyIds;
    for (const auto &a : p.armies())
        if (a.isSettlerArmy())
            settlerArmyIds.push_back(a.getId());

    for (const auto &armyId : settlerArmyIds)
    {
        Army *mine = state.findArmy(armyId);
        if (!mine)
            continue;

        if (state.canFoundSettlementAt(mine->position()))
        {
            state.foundSettlementFromArmy(armyId);
            continue;
        }

        if (mine->movementPointsRemaining() <= 0)
            continue;

        std::vector<Coords> occupied;
        for (const auto &pl : state.players())
        {
            for (const auto &s : pl.settlements())
                for (const auto &t : s.tiles())
                    occupied.push_back(t.getCoords());
            for (const auto &ar : pl.armies())
                occupied.push_back(ar.position());
        }
        auto isOccupied = [&occupied](const Coords &c)
        {
            for (const auto &o : occupied)
                if (o.q == c.q && o.r == c.r)
                    return true;
            return false;
        };

        std::vector<Candidate> candidates;
        for (const auto &dest : state.map().reachableTiles(mine->position(), mine->movementPointsRemaining()))
        {
            if (isOccupied(dest))
                continue;
            if (!state.canFoundSettlementAt(dest))
                continue;

            float score = AIStrategy::scoreExpand(*mine, dest, phase, state.map());
            candidates.push_back({dest, score});
        }

        if (candidates.empty())
            continue;

        auto best = std::max_element(candidates.begin(), candidates.end(),
                                     [](const Candidate &a, const Candidate &b)
                                     { return a.score < b.score; });

        if (best->score <= 0.0f)
            continue;

        state.moveArmy(armyId, best->destination);
    }
}

void AIController::decideDeployment(Player &p)
{
    constexpr size_t MIN_GARRISON_RESERVE = 2;
    constexpr size_t DEPLOY_THRESHOLD = 4;

    for (auto &s : p.settlements())
    {
        while (true)
        {
            std::string settlerId;
            for (const auto &u : s.garrison())
            {
                if (u.isSettler())
                {
                    settlerId = u.getId();
                    break;
                }
            }
            if (settlerId.empty())
                break;
            p.detachArmy(s, {settlerId});
        }

        const auto &garrison = s.garrison();
        if (garrison.size() <= DEPLOY_THRESHOLD)
            continue;

        std::vector<std::string> toDeploy;
        for (size_t i = 0; i + MIN_GARRISON_RESERVE < garrison.size(); ++i)
            toDeploy.push_back(garrison[i].getId());
        if (toDeploy.empty())
            continue;

        Army *existing = nullptr;
        for (auto &a : p.armies())
        {
            if (a.isSettlerArmy())
                continue;
            for (const auto &t : s.tiles())
            {
                if (a.position().q == t.getCoords().q && a.position().r == t.getCoords().r)
                {
                    existing = &a;
                    break;
                }
            }
            if (existing)
                break;
        }

        if (existing)
        {
            std::vector<Unit> units = s.removeFromGarrison(toDeploy);
            existing->addUnits(units);
        }
        else
        {
            p.detachArmy(s, toDeploy);
        }
    }
}
