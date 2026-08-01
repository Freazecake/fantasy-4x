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
    decideMilitaryActions(bot, state, state.CurPhase());
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

    int assigned = 0;
    for (const auto &ss : scores)
    {
        int amount = static_cast<int>(std::floor(ss.score / total * static_cast<float>(idle)));
        if (amount > 0)
        {
            s.assignFromIdle(ss.sphere, amount);
            assigned += amount;
        }
    }

    int remaining = idle - assigned;
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
    static const std::vector<std::string> targets = {"science", "industry", "diplomacy"};

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

    // Тратим большую часть накопленной торговли, но не всю — оставляем
    // небольшой запас, чтобы бот не оголял торговлю подчистую за один ход.
    int tradeToSpend = static_cast<int>(p.resources().trade * 0.8f);
    if (tradeToSpend < 1)
        return;

    p.exchangeTrade(*best, tradeToSpend);
}

void AIController::decideRecruitment(Settlement &s, const std::vector<Unit> &catalog, GlobalPhase phase)
{
    std::vector<std::pair<float, const Unit *>> sc;
    for (const auto &u : catalog)
        sc.push_back({AIStrategy::scoreRecruitUnit(s, u, phase), &u});

    std::sort(sc.begin(), sc.end(), [](const auto &a, const auto &b)
              { return a.first > b.first; });

    if (!sc.empty() && sc[0].first > 0.0f)
        s.queueUnitRecruitment(*sc[0].second);
}

void AIController::decidePoliticalAction(Player &p, const std::vector<PoliticalAction> &catalog, GameState &state)
{
    struct ActionCandidate
    {
        const PoliticalAction *action;
        int targetId;
        float score;
    };
    std::vector<ActionCandidate> candidates;

    for (const auto &a : catalog)
    {
        if (p.resources().diplomacy < static_cast<float>(a.getCost()))
            continue;

        for (const auto &pl : state.players())
        {
            if (pl.id() == p.id())
                continue;

            // Уже союзники — не предлагаем заключить союз ещё раз, это
            // просто трата очков без какого-либо нового эффекта.
            if (a.getType() == PoliticalActionType::Alliance &&
                GameState::isAllied(state, p.id(), pl.id()))
                continue;

            float s = AIStrategy::scorePoliticalAction(p, a, pl, state.CurPhase());
            if (s > 0.0f)
                candidates.push_back({&a, pl.id(), s});
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
        state.applyPoliticalConquest(p.id(), best->targetId);
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

        if (mine->movementPointsRemaining() <= 0)
            continue;

        std::vector<Candidate> candidates;

        for (const auto &enemy : state.players())
        {
            if (GameState::isAllied(state, p.id(), enemy.id()))
                continue;

            for (const auto &s : enemy.settlements())
            {
                if (s.tiles().empty())
                    continue;

                Coords target = s.tiles().front().getCoords();

                if (state.map().movementCostTo(mine->position(), target,
                                               mine->movementPointsRemaining()) < 0)
                    continue;

                Army garrisonAsArmy("enemy_garrison", s.ownerId(), target, s.garrison());

                float score = AIStrategy::scoreAttack(*mine, garrisonAsArmy, phase);
                candidates.push_back({target, score});
            }

            for (const auto &enemyArmy : enemy.armies())
            {
                Coords target = enemyArmy.position();

                if (state.map().movementCostTo(mine->position(), target,
                                               mine->movementPointsRemaining()) < 0)
                    continue;

                float score = AIStrategy::scoreAttack(*mine, enemyArmy, phase);
                candidates.push_back({target, score});
            }
        }

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

        for (const auto &dest : state.map().reachableTiles(mine->position(), mine->movementPointsRemaining()))
        {
            if (isOccupied(dest))
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
