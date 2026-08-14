// Симулирует человека, который намеренно играет в раш: максимум промышленности
// с первого хода, казарма как только доступна, непрерывный найм военных без
// оборонительного резерва, армия атакует ближайшего соседа при первой
// возможности — БЕЗ "ранней осторожности", которая есть у ботов (см.
// AIController::earlyCaution). Вопрос: успевают ли боты защититься от игрока,
// который специально играет агрессивнее, чем позволяет себе ИИ?
//
// Запуск: ./human_rush_test [numGames per race] [maxTurns] [baseSeed]

#include "core/GameEngine.hpp"

#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{

void rushTurn(Player &human, GameState &state, const std::vector<Building> &buildingCatalog,
              const std::vector<Unit> &unitCatalog)
{
    for (auto &s : human.settlements())
    {
        int idle = s.allocation().idle;
        if (idle > 0)
            s.assignFromIdle(Sphere::Industry, idle);
    }

    for (auto &s : human.settlements())
    {
        bool hasBarracks = false;
        for (const auto &b : s.builtBuildings())
            if (b.getId() == "barracks")
                hasBarracks = true;
        for (const auto &item : s.buildingQueue())
            if (item.building.getId() == "barracks")
                hasBarracks = true; // уже в очереди — не дублируем
        if (!hasBarracks)
        {
            for (const auto &b : buildingCatalog)
                if (b.getId() == "barracks")
                {
                    s.queueBuilding(b);
                    break;
                }
        }
    }

    for (auto &s : human.settlements())
    {
        const Unit *best = nullptr;
        for (const auto &u : unitCatalog)
        {
            if (u.isSettler())
                continue;
            if (!u.getRequiredBuildingId().empty())
            {
                bool has = false;
                for (const auto &b : s.builtBuildings())
                    if (b.getId() == u.getRequiredBuildingId())
                        has = true;
                if (!has)
                    continue;
            }
            if (!best || u.getStats().attack > best->getStats().attack)
                best = &u;
        }
        if (best)
        {
            try
            {
                s.queueUnitRecruitment(*best);
            }
            catch (const std::exception &)
            {
                // Недостаточно свободного населения в этот ход — пропускаем,
                // рашер попробует снова на следующем ходу.
            }
        }
    }

    for (auto &s : human.settlements())
    {
        std::vector<std::string> combatIds;
        for (const auto &u : s.garrison())
            if (!u.isSettler())
                combatIds.push_back(u.getId());
        if (combatIds.size() >= 3)
        {
            Army *existing = nullptr;
            for (auto &a : human.armies())
                for (const auto &t : s.tiles())
                    if (a.position().q == t.getCoords().q && a.position().r == t.getCoords().r)
                        existing = &a;

            if (existing)
            {
                auto units = s.removeFromGarrison(combatIds);
                existing->addUnits(units);
            }
            else
            {
                human.detachArmy(s, combatIds);
            }
        }
    }

    std::vector<std::string> armyIds;
    for (const auto &a : human.armies())
        armyIds.push_back(a.getId());

    for (const auto &armyId : armyIds)
    {
        Army *mine = nullptr;
        for (auto &a : human.armies())
            if (a.getId() == armyId)
                mine = &a;
        if (!mine || mine->units().empty() || mine->movementPointsRemaining() <= 0)
            continue;

        const Settlement *nearestTarget = nullptr;
        int nearestDist = -1;
        for (const auto &enemy : state.players())
        {
            if (enemy.id() == human.id())
                continue;
            for (const auto &s : enemy.settlements())
            {
                if (s.tiles().empty())
                    continue;
                int d = mine->position().distanceTo(s.tiles().front().getCoords());
                if (nearestDist < 0 || d < nearestDist)
                {
                    nearestDist = d;
                    nearestTarget = &s;
                }
            }
        }
        if (!nearestTarget)
            continue;

        Coords dest = nearestTarget->tiles().front().getCoords();
        int cost = state.map().movementCostTo(mine->position(), dest, mine->movementPointsRemaining());
        if (cost >= 0)
        {
            state.moveArmy(armyId, dest);
        }
        else
        {
            Coords bestStep{};
            int bestCost = -1;
            bool found = false;
            for (const auto &d : state.map().reachableTiles(mine->position(), mine->movementPointsRemaining()))
            {
                int c = state.map().movementCostTo(d, dest, 100000);
                if (c < 0)
                    continue;
                if (!found || c < bestCost)
                {
                    bestCost = c;
                    bestStep = d;
                    found = true;
                }
            }
            if (found)
                state.moveArmy(armyId, bestStep);
        }
    }
}

struct RunResult
{
    bool decided = false;
    bool humanEliminated = false;
    bool humanWon = false;
    int turn = -1;
};

RunResult playOneGame(const std::string &humanRace, unsigned int seed, int maxTurns)
{
    GameEngine engine;
    GameStartOptions options;
    options.humanRaceId = humanRace;
    options.numBots = 4;

    RunResult result;
    if (!engine.loadFromAssets(ASSETS_DIR, options, seed))
        return result;

    for (auto &p : engine.state().players())
        p.setIsHuman(false); // управляем ходом игрока 0 сами, боты — AIController

    const auto &buildingCatalog = engine.buildingCatalog();
    const auto &unitCatalog = engine.unitCatalog();

    for (int turn = 1; turn <= maxTurns; ++turn)
    {
        Player &human = engine.state().players()[0];
        if (!human.settlements().empty())
            rushTurn(human, engine.state(), buildingCatalog, unitCatalog);

        engine.endTurn();

        bool humanAlive = false;
        for (const auto &p : engine.state().players())
            if (p.id() == 0 && !p.settlements().empty())
                humanAlive = true;

        if (!humanAlive)
        {
            result.decided = true;
            result.humanEliminated = true;
            result.turn = turn;
            return result;
        }

        auto v = engine.checkVictoryCondition();
        if (v.hasWinner())
        {
            result.decided = true;
            result.humanWon = (v.winnerId == 0);
            result.turn = turn;
            return result;
        }
    }

    return result; // не решилось за maxTurns
}

} // namespace

int main(int argc, char **argv)
{
    int gamesPerRace = argc > 1 ? std::atoi(argv[1]) : 20;
    int maxTurns = argc > 2 ? std::atoi(argv[2]) : 150;
    unsigned int baseSeed = argc > 3 ? static_cast<unsigned int>(std::atoi(argv[3])) : 7000u;

    std::vector<std::string> races = {"humans", "dwarves", "gnomes", "elves", "orcs"};

    std::cout << "Раш-игрок против 4 умных ботов, " << gamesPerRace
              << " партий на расу, maxTurns=" << maxTurns << "\n\n";

    int totalGames = 0, humanWins = 0, humanEliminated = 0, undecided = 0;
    std::map<int, int> eliminationTurnHistogram; // ход поражения человека / 10

    for (const auto &race : races)
    {
        int wins = 0, eliminated = 0, undec = 0;
        int minEliminationTurn = -1;
        for (int g = 0; g < gamesPerRace; ++g)
        {
            unsigned int seed = baseSeed + static_cast<unsigned int>(g);
            RunResult r = playOneGame(race, seed, maxTurns);
            ++totalGames;
            if (!r.decided)
            {
                ++undec;
                ++undecided;
                continue;
            }
            if (r.humanWon)
            {
                ++wins;
                ++humanWins;
            }
            else if (r.humanEliminated)
            {
                ++eliminated;
                ++humanEliminated;
                if (minEliminationTurn < 0 || r.turn < minEliminationTurn)
                    minEliminationTurn = r.turn;
                eliminationTurnHistogram[r.turn / 10]++;
            }
        }
        std::cout << "раса рашера=" << race << ": побед человека=" << wins
                  << "/" << gamesPerRace << ", человек уничтожен=" << eliminated
                  << "/" << gamesPerRace << ", не решилось=" << undec;
        if (minEliminationTurn > 0)
            std::cout << ", самое раннее поражение на ходу " << minEliminationTurn;
        std::cout << "\n";
    }

    std::cout << "\n========== ИТОГО ==========\n";
    std::cout << "Партий: " << totalGames << "\n";
    std::cout << "Раш-человек победил: " << humanWins << " (" << (100.0 * humanWins / totalGames) << "%)\n";
    std::cout << "Раш-человек уничтожен: " << humanEliminated << " (" << (100.0 * humanEliminated / totalGames) << "%)\n";
    std::cout << "Не решилось за " << maxTurns << " ходов: " << undecided << "\n";

    std::cout << "\nРаспределение хода поражения человека (десятки ходов):\n";
    for (const auto &[bucket, count] : eliminationTurnHistogram)
        std::cout << "  ход " << bucket * 10 << "-" << bucket * 10 + 9 << ": " << count << " раз\n";

    return 0;
}
