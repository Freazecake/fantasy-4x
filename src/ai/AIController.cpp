#include "ai/AIController.hpp"
#include "ai/CombatSimulator.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <string>

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

    // Зеркалит GameState.cpp::terrainBonus (файл-локальная там, недоступна
    // отсюда) — держим то же самое правило: Forest 1.2, Mountain 1.5.
    float terrainBonusFor(TileType type)
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

    // Партия не должна вязнуть в вечном позиционном стоянии: чем дольше она
    // идёт без развязки, тем охотнее боты рискуют — растущий множитель
    // допустимых потерь и убывающий порог "неприкосновенного" гарнизона.
    // К ходу ESCALATION_FULL готовы почти на любую цену, лишь бы решить исход.
    constexpr int ESCALATION_START = 30;
    constexpr int ESCALATION_FULL = 80;

    float aggressionEscalation(int turnNumber)
    {
        if (turnNumber <= ESCALATION_START)
            return 0.0f;
        float t = static_cast<float>(turnNumber - ESCALATION_START) /
                  static_cast<float>(ESCALATION_FULL - ESCALATION_START);
        return std::clamp(t, 0.0f, 1.0f);
    }

    // Обратная сторона медали: после того как честная симуляция боя и
    // осмысленное преследование перестали быть багом (см. историю правок),
    // партии стали решаться почти исключительно и почти мгновенно войной —
    // экономика/дипломатия просто не успевали ничего сделать за ~25 ходов.
    // EARLY_CAUTION даёт первым ходам "запас мирного времени": в начале
    // партии бот резко разборчивее насчёт потерь, ближе к EARLY_CAUTION_END
    // выходит на базовый уровень. Окно расширено (было 20) — армии успевают
    // дойти до врага только к ~20-25 ходу, и более короткое окно вообще не
    // успевало сработать (см. историю правок: playtest показывал 99% войн
    // при окне в 20 ходов).
    constexpr int EARLY_CAUTION_END = 45;

    float earlyCaution(int turnNumber)
    {
        if (turnNumber >= EARLY_CAUTION_END)
            return 0.0f;
        return 1.0f - static_cast<float>(turnNumber) / static_cast<float>(EARLY_CAUTION_END);
    }

    size_t computeSafeGarrisonFloor(const Player &p, const GameState &state, const Settlement &s)
    {
        constexpr size_t BASE_MIN_FLOOR = 2;
        float escalation = aggressionEscalation(state.turnNumber());
        // Поздняя партия — держим меньше "неприкосновенного" гарнизона,
        // чтобы у бота физически было чем решать исход в поле, а не только
        // сидеть за стенами.
        size_t minFloor = escalation > 0.85f ? 0 : (escalation > 0.5f ? 1 : BASE_MIN_FLOOR);

        if (s.tiles().empty())
            return minFloor;
        Coords anchor = s.tiles().front().getCoords();

        const Army *nearestThreat = nullptr;
        const Player *threatOwner = nullptr;
        int nearestDist = -1;
        for (const auto &pl : state.players())
        {
            if (pl.id() == p.id() || GameState::isAllied(state, p.id(), pl.id()))
                continue;
            for (const auto &a : pl.armies())
            {
                int d = anchor.distanceTo(a.position());
                if (nearestDist < 0 || d < nearestDist)
                {
                    nearestDist = d;
                    nearestThreat = &a;
                    threatOwner = &pl;
                }
            }
        }

        constexpr int THREAT_RADIUS = 5;
        if (!nearestThreat || nearestDist > THREAT_RADIUS)
            return minFloor;

        CombatPrediction pred = CombatSimulator::predict(
            nearestThreat->units(), s.garrison(),
            threatOwner->heroes(), p.heroes(),
            1.0f, s.builtBuildings(),
            threatOwner->race().getModifiers().military,
            p.race().getModifiers().military);

        bool weHoldWithMargin =
            !pred.attackerWins && !pred.isDraw &&
            pred.defenderSurvivors >= static_cast<int>(s.garrison().size() / 2);

        if (weHoldWithMargin)
            return minFloor;

        if (!pred.attackerWins)
            return std::max(minFloor, s.garrison().size() > 1 ? s.garrison().size() - 1 : s.garrison().size());

        return s.garrison().size();
    }
}

void AIController::takeTurn(Player &bot, GameState &state, BotMemory &memory,
                            const std::vector<Building> &buildingCatalog,
                            const std::vector<Technology> &techCatalog,
                            const std::vector<Unit> &unitCatalog,
                            const std::vector<PoliticalAction> &politicalActionCatalog)
{
    maybeReassessGoal(bot, state, memory);

    RushSignal rushSignal = detectRivalRush(bot, state);

    decideTradeExchange(bot, state, state.CurPhase());
    decideResearch(bot, techCatalog, state.CurPhase());
    for (auto &s : bot.settlements())
    {
        decideRecruitment(s, unitCatalog, state.CurPhase(), memory.goal);
        decideConstruction(s, buildingCatalog, state.CurPhase(), memory.goal);
        decideAllocations(s, state.CurPhase(), memory.goal, rushSignal);
    }
    decideDeployment(bot, state);
    decideMilitaryActions(bot, state, state.CurPhase(), memory);
    decideSettlerActions(bot, state, state.CurPhase());
    decidePoliticalAction(bot, politicalActionCatalog, state, memory);
}

void AIController::observeEvents(Player &bot, const GameState &state, BotMemory &memory)
{
    for (const auto &ev : state.turnEvents())
    {
        switch (ev.type)
        {
        case TurnEventType::Combat:
            if (ev.targetId == bot.id() && ev.actorId != bot.id() && ev.actorId >= 0)
            {
                auto &rel = memory.relationship(ev.actorId);
                rel.trust -= 25.0f;
                rel.lastHostileTurn = state.turnNumber();
            }
            break;
        case TurnEventType::TributeDemanded:
            if (ev.targetId == bot.id() && ev.actorId != bot.id())
            {
                auto &rel = memory.relationship(ev.actorId);
                rel.trust -= 15.0f;
                rel.lastHostileTurn = state.turnNumber();
            }
            break;
        case TurnEventType::Vassalized:
        case TurnEventType::Absorbed:
            if (ev.targetId == bot.id() && ev.actorId != bot.id())
            {
                auto &rel = memory.relationship(ev.actorId);
                rel.trust -= 60.0f;
                rel.lastHostileTurn = state.turnNumber();
                rel.everBetrayedAlliance = true;
            }
            else if (ev.actorId != bot.id() && ev.targetId != bot.id() && ev.actorId >= 0)
            {
                auto &rel = memory.relationship(ev.actorId);
                rel.trust -= 5.0f;
            }
            break;
        case TurnEventType::AllianceFormed:
            if (ev.actorId == bot.id() || ev.targetId == bot.id())
            {
                int other = ev.actorId == bot.id() ? ev.targetId : ev.actorId;
                if (other >= 0)
                    memory.relationship(other).trust += 20.0f;
            }
            break;
        case TurnEventType::AllianceBroken:
            if (ev.actorId == bot.id() || ev.targetId == bot.id())
            {
                int other = ev.actorId == bot.id() ? ev.targetId : ev.actorId;
                if (other >= 0)
                {
                    // Кулдаун на повторный союз с тем же соперником — иначе
                    // два бота перезаключают его бесконечно и никогда не
                    // воюют (см. комментарий у allianceCooldownUntilTurn).
                    memory.relationship(other).allianceCooldownUntilTurn = state.turnNumber() + 10;
                }
            }
            break;
        default:
            break;
        }
    }

    for (auto &kv : memory.relationships)
    {
        auto &rel = kv.second;
        if (rel.trust > 0.0f)
            rel.trust = std::max(0.0f, rel.trust - 0.5f);
        else if (rel.trust < 0.0f)
            rel.trust = std::min(0.0f, rel.trust + 0.5f);
        rel.trust = std::clamp(rel.trust, -100.0f, 100.0f);
    }
}

RushSignal AIController::detectRivalRush(const Player &p, const GameState &state)
{
    // Порог, начиная с которого доля населения соперника в одной сфере
    // считается "перекошенной" (не нормальным экономическим развитием, а
    // целенаправленным рашем). Обычная сбалансированная экономика — даже с
    // сильным расовым модификатором в одну сторону — не толкает ОДНУ сферу
    // выше половины всего распределяемого населения через обычный score-based
    // decideAllocations (см. там же): это осознанный перекос, а не то, что
    // выбрала бы сама scoring-функция.
    constexpr float RUSH_SHARE_THRESHOLD = 0.55f;

    RushSignal signal;

    for (const auto &rival : state.players())
    {
        if (rival.id() == p.id() || rival.settlements().empty())
            continue;
        if (GameState::isAllied(state, p.id(), rival.id()))
            continue;

        float totalAllocated = 0.0f;
        float bySphere[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // science, industry, diplomacy, trade
        for (const auto &s : rival.settlements())
        {
            const auto &a = s.allocation();
            bySphere[0] += static_cast<float>(a.science);
            bySphere[1] += static_cast<float>(a.industry);
            bySphere[2] += static_cast<float>(a.diplomacy);
            bySphere[3] += static_cast<float>(a.trade);
        }
        totalAllocated = bySphere[0] + bySphere[1] + bySphere[2] + bySphere[3];
        if (totalAllocated <= 0.0f)
            continue;

        float shareScience = bySphere[0] / totalAllocated;
        float shareIndustry = bySphere[1] / totalAllocated;
        float shareDiplomacy = bySphere[2] / totalAllocated;
        float shareTrade = bySphere[3] / totalAllocated;

        signal.science = std::max(signal.science, shareScience - RUSH_SHARE_THRESHOLD);
        signal.industry = std::max(signal.industry, shareIndustry - RUSH_SHARE_THRESHOLD);
        signal.diplomacy = std::max(signal.diplomacy, shareDiplomacy - RUSH_SHARE_THRESHOLD);
        signal.trade = std::max(signal.trade, shareTrade - RUSH_SHARE_THRESHOLD);
    }

    signal.science = std::max(0.0f, signal.science);
    signal.industry = std::max(0.0f, signal.industry);
    signal.diplomacy = std::max(0.0f, signal.diplomacy);
    signal.trade = std::max(0.0f, signal.trade);
    return signal;
}

void AIController::maybeReassessGoal(Player &p, GameState &state, BotMemory &memory)
{
    if (p.settlements().empty())
        return;

    bool forceReassess = false;
    if (memory.goal == BotGoal::WarWith && memory.goalTargetId >= 0)
    {
        bool targetAlive = false;
        for (const auto &pl : state.players())
            if (pl.id() == memory.goalTargetId && !pl.settlements().empty())
                targetAlive = true;
        if (!targetAlive)
            forceReassess = true;
    }

    // Угроза у границ — немедленный пересмотр цели, не дожидаясь планового
    // REASSESS_INTERVAL. Без этого бот может продолжать мирный курс (Grow)
    // ещё несколько ходов, пока вражеская армия уже на подходе к столице —
    // именно так раш-игрок (см. human_rush_test) успевал застать ботов
    // врасплох в первые ~20-30 ходов.
    int threatenedTargetId = -1;
    {
        constexpr int THREAT_TRIGGER_RADIUS = 5;
        for (const auto &mySettlement : p.settlements())
        {
            if (mySettlement.tiles().empty())
                continue;
            Coords anchor = mySettlement.tiles().front().getCoords();
            for (const auto &pl : state.players())
            {
                if (pl.id() == p.id() || GameState::isAllied(state, p.id(), pl.id()))
                    continue;
                for (const auto &a : pl.armies())
                {
                    if (anchor.distanceTo(a.position()) <= THREAT_TRIGGER_RADIUS)
                    {
                        threatenedTargetId = pl.id();
                        forceReassess = true;
                    }
                }
            }
        }
    }

    int turnsSinceSet = state.turnNumber() - memory.goalSetOnTurn;
    if (!forceReassess && turnsSinceSet < BotMemory::REASSESS_INTERVAL)
        return;

    struct GoalOption
    {
        BotGoal goal;
        int targetId;
        float score;
    };
    std::vector<GoalOption> options;

    options.push_back({BotGoal::Grow, -1, 1.0f});

    float myMilitary = p.militaryPower();
    float avgEnemyMilitary = 0.0f;
    int aliveEnemies = 0;
    for (const auto &pl : state.players())
    {
        if (pl.id() == p.id() || pl.settlements().empty())
            continue;
        avgEnemyMilitary += pl.militaryPower();
        ++aliveEnemies;
    }
    if (aliveEnemies > 0)
        avgEnemyMilitary /= static_cast<float>(aliveEnemies);

    if (avgEnemyMilitary > 0.0f && myMilitary < avgEnemyMilitary * 0.8f)
        options.push_back({BotGoal::BuildMilitary, -1, 1.3f});

    int bestWarTarget = -1;
    float bestWarScore = 0.0f;
    for (const auto &pl : state.players())
    {
        if (pl.id() == p.id() || pl.settlements().empty())
            continue;
        if (GameState::isAllied(state, p.id(), pl.id()))
            continue;

        float theirMilitary = pl.militaryPower();
        float ratio = theirMilitary > 0.0f ? myMilitary / theirMilitary : 3.0f;
        float trust = memory.relationships.count(pl.id()) ? memory.relationships.at(pl.id()).trust : 0.0f;
        float score = (ratio - 1.0f) * 2.0f - trust / 50.0f;
        // В начале партии (пока армии и экономика ещё не развернулись)
        // сильно приглушаем военную цель — иначе бот берёт курс на войну
        // буквально с первой переоценки (ход ~4) и вся партия дальше просто
        // разыгрывает этот ранний выбор, дипломатия/экономика не успевают.
        // Исключение — тот, кто сейчас реально угрожает нашим границам
        // (см. threatenedTargetId выше): на него приглушение не действует,
        // иначе бот "мирно" проигрывает рашеру, честно ожидая своего часа.
        if (state.turnNumber() < 25 && pl.id() != threatenedTargetId)
            score *= 0.25f;
        if (pl.id() == threatenedTargetId)
            score += 3.0f; // прямая угроза перевешивает почти любые другие соображения
        if (score > bestWarScore)
        {
            bestWarScore = score;
            bestWarTarget = pl.id();
        }
    }
    if (bestWarTarget >= 0)
        options.push_back({BotGoal::WarWith, bestWarTarget, bestWarScore});

    const auto &res = p.resources();
    float totalRes = res.science + res.industry + res.trade + res.diplomacy + 1.0f;
    float dipRatio = res.diplomacy / totalRes;
    if (dipRatio > 0.3f)
        options.push_back({BotGoal::PoliticalDominance, -1, 1.0f + dipRatio});

    if (forceReassess && memory.goal == BotGoal::WarWith)
        options.push_back({BotGoal::Consolidate, -1, 1.5f});

    for (auto &opt : options)
        if (opt.goal == memory.goal)
            opt.score *= 1.2f;

    auto best = std::max_element(options.begin(), options.end(),
                                 [](const GoalOption &a, const GoalOption &b)
                                 { return a.score < b.score; });

    memory.goal = best->goal;
    memory.goalTargetId = best->targetId;
    memory.goalSetOnTurn = state.turnNumber();
}

void AIController::decideAllocations(Settlement &s, GlobalPhase phase, BotGoal goal, RushSignal rushSignal)
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

    for (auto &ss : scores)
    {
        switch (goal)
        {
        case BotGoal::BuildMilitary:
        case BotGoal::WarWith:
            if (ss.sphere == Sphere::Industry)
                ss.score *= 1.6f;
            else if (ss.sphere == Sphere::Science)
                ss.score *= 0.8f;
            break;
        case BotGoal::PoliticalDominance:
            if (ss.sphere == Sphere::Diplomacy)
                ss.score *= 1.6f;
            break;
        case BotGoal::Consolidate:
            if (ss.sphere == Sphere::Industry)
                ss.score *= 1.3f;
            break;
        case BotGoal::Grow:
        default:
            break;
        }
    }

    float total = 0.0f;
    for (const auto &ss : scores)
        total += ss.score;

    if (total <= 0.0f)
        return;

    int idle = s.allocation().idle;
    if (idle <= 0)
        return;

    constexpr int SETTLER_POP_RESERVE = 40;
    int reserve = 0;
    if (phase != GlobalPhase::Phase3_Final)
    {
        // Раньше резерв мог "съедать" ВЕСЬ свободный пул, пока население ещё
        // маленькое (idle < 40) — это оставляло 0 на промышленность/науку на
        // добрый десяток первых ходов, и бот физически не мог начать
        // наращивать войска. Ограничиваем резерв половиной свободного пула —
        // колонист будет собираться чуть дольше, зато экономика/армия не
        // простаивают с самого начала.
        reserve = std::min({idle, SETTLER_POP_RESERVE, idle / 2});
    }

    int allocatable = idle - reserve;
    if (allocatable <= 0)
        return;

    // Ответ на перекос соперника: если кто-то из соперников вкладывает
    // непропорционально много в одну сферу (рашит промышленность под армию,
    // или науку/дипломатию), мы отвечаем усилением ТОЙ ЖЕ сферы у себя —
    // не слепым таймером "первые N ходов всегда осторожны", а реакцией на
    // конкретный наблюдаемый сигнал. В мирных партиях, где никто так не
    // делает, это НЕ срабатывает вообще, и экономика распределяется как
    // обычно (наука/торговля/дипломатия остаются жизнеспособны — не мешаем
    // политическому пути к победе там, где реальной угрозы раша нет).
    auto floorFor = [&](Sphere sphere) -> int
    {
        float excess = 0.0f;
        switch (sphere)
        {
        case Sphere::Science:
            excess = rushSignal.science;
            break;
        case Sphere::Industry:
            excess = rushSignal.industry;
            break;
        case Sphere::Diplomacy:
            excess = rushSignal.diplomacy;
            break;
        case Sphere::Trade:
            excess = rushSignal.trade;
            break;
        }
        if (excess <= 0.0f)
            return 0;
        // Превышение порога напрямую конвертируется в долю нашего ответа —
        // чем сильнее соперник перекошен, тем решительнее мы отвечаем.
        float responseShare = std::clamp(excess * 1.5f, 0.0f, 0.6f);
        return static_cast<int>(std::ceil(allocatable * responseShare));
    };

    int assigned = 0;
    int remainingAfterFloor = allocatable;
    for (const auto &ss : scores)
    {
        int floorAmount = floorFor(ss.sphere);
        if (floorAmount <= 0)
            continue;
        int amount = std::min(floorAmount, remainingAfterFloor);
        if (amount > 0)
        {
            s.assignFromIdle(ss.sphere, amount);
            assigned += amount;
            remainingAfterFloor -= amount;
        }
    }

    for (const auto &ss : scores)
    {
        if (remainingAfterFloor <= 0)
            break;
        int amount = static_cast<int>(std::floor(ss.score / total * static_cast<float>(allocatable)));
        int alreadyFromFloor = floorFor(ss.sphere);
        amount = std::max(0, amount - alreadyFromFloor);
        amount = std::min(amount, remainingAfterFloor);
        if (amount > 0)
        {
            s.assignFromIdle(ss.sphere, amount);
            assigned += amount;
            remainingAfterFloor -= amount;
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

void AIController::decideConstruction(Settlement &s, const std::vector<Building> &catalog, GlobalPhase phase,
                                      BotGoal goal)
{
    std::vector<std::pair<float, const Building *>> sc;
    for (const auto &b : catalog)
    {
        float score = AIStrategy::scoreBuild(s, b, phase);
        if ((goal == BotGoal::BuildMilitary || goal == BotGoal::WarWith) &&
            b.getModifiers().military > 1.0f)
            score *= 1.5f;
        sc.push_back({score, &b});
    }

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

void AIController::decideTradeExchange(Player &p, GameState &state, GlobalPhase phase)
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

    if (p.exchangeTrade(*best, tradeToSpend))
    {
        state.pushEvent(TurnEventType::TradeExchange, p.id(), -1,
                        "Игрок " + std::to_string(p.id()) + " обменял " +
                            std::to_string(tradeToSpend) + " торговли на " + *best);
    }
}

void AIController::decideRecruitment(Settlement &s, const std::vector<Unit> &catalog, GlobalPhase phase,
                                     BotGoal goal)
{
    size_t garrisonCap = 5;
    constexpr size_t GARRISON_FLOOR = 3;
    if (goal == BotGoal::BuildMilitary || goal == BotGoal::WarWith)
        garrisonCap = 9;

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
    {
        float score = AIStrategy::scoreRecruitUnit(s, u, phase);
        if (!u.isSettler() && (goal == BotGoal::BuildMilitary || goal == BotGoal::WarWith))
            score *= 1.4f;
        sc.push_back({score, &u});
    }

    std::sort(sc.begin(), sc.end(), [](const auto &a, const auto &b)
              { return a.first > b.first; });

    if (sc.empty() || sc[0].first <= 0.0f)
        return;

    const Unit *best = sc[0].second;
    if (best->isSettler() && combatCount < GARRISON_FLOOR)
        return;

    if (best->isSettler() || s.garrison().size() < garrisonCap || !hasCombatUnits)
        s.queueUnitRecruitment(*best);
}

void AIController::decidePoliticalAction(Player &p, const std::vector<PoliticalAction> &catalog, GameState &state,
                                         BotMemory &memory)
{
    struct ActionCandidate
    {
        const PoliticalAction *action;
        int targetId;
        float score;
        int cost;
    };

    auto trustFor = [&](int rivalId) -> float
    {
        auto it = memory.relationships.find(rivalId);
        return it != memory.relationships.end() ? it->second.trust : 0.0f;
    };

    const PoliticalAction *conquestAction = nullptr;
    for (const auto &a : catalog)
        if (a.getType() == PoliticalActionType::PoliticalConquest)
            conquestAction = &a;

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
            float s = AIStrategy::scorePoliticalAction(p, *conquestAction, pl, state.CurPhase(), trustFor(pl.id()));
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

            if (a.getType() == PoliticalActionType::Alliance && trustFor(pl.id()) < -50.0f)
                continue;

            if (a.getType() == PoliticalActionType::Alliance)
            {
                auto it = memory.relationships.find(pl.id());
                if (it != memory.relationships.end() &&
                    state.turnNumber() < it->second.allianceCooldownUntilTurn)
                    continue;
            }

            int cost = a.getCost();
            if (p.resources().diplomacy < static_cast<float>(cost))
                continue;

            float s = AIStrategy::scorePoliticalAction(p, a, pl, state.CurPhase(), trustFor(pl.id()));
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

bool AIController::worthAttacking(const Player &attacker, const Army &mine,
                                  const Player &defenderOwner, const std::vector<Unit> &defenderUnits,
                                  float defenderTerrainBonus, const std::vector<Building> &defenderBuildings,
                                  bool isEliminationTarget, int turnNumber, bool isDefensive)
{
    CombatPrediction pred = CombatSimulator::predict(
        mine.units(), defenderUnits,
        attacker.heroes(), defenderOwner.heroes(),
        defenderTerrainBonus, defenderBuildings,
        attacker.race().getModifiers().military,
        defenderOwner.race().getModifiers().military);

    float escalation = aggressionEscalation(turnNumber);
    // Оборонительный удар — враг уже стоит у наших границ, угрожая
    // поселению. Здесь не действует "ранняя осторожность": игрок может
    // намеренно рашить, не дожидаясь, пока боты "созреют" по расписанию
    // (см. human_rush_test — без этого исключения раш-игрок выносил ботов
    // почти всегда, потому что они терпеливо ждали своего часа, пока враг
    // уже стоял у ворот).
    float caution = isDefensive ? 0.0f : earlyCaution(turnNumber);

    // Партия затянулась — на ничью тоже соглашаемся (взаимное истощение
    // всё равно продвигает партию к развязке), а не только на чистую победу.
    if (!pred.attackerWins && !(pred.isDraw && escalation > 0.5f))
        return false;

    float baseMaxLoss = isEliminationTarget ? 0.85f : 0.55f;
    // Ранняя осторожность (первые ~45 ходов) снижает планку — не лезем в
    // драку без явного перевеса, пока экономика/дипломатия только
    // разворачиваются. Поздняя эскалация, наоборот, поднимает планку, чтобы
    // партия не зависала без развязки (см. earlyCaution/aggressionEscalation).
    float maxAcceptableLoss = baseMaxLoss - caution * 0.45f + escalation * 0.4f;
    maxAcceptableLoss = std::clamp(maxAcceptableLoss, 0.10f, 0.98f);
    return pred.attackerLossFraction <= maxAcceptableLoss;
}

bool AIController::tryRallyOrMerge(Player &p, GameState &state, const std::string &armyId,
                                   const std::vector<Coords> &occupied)
{
    Army *mine = state.findArmy(armyId);
    if (!mine)
        return false;

    Army *bestAlly = nullptr;
    int bestDist = -1;
    for (auto &a : p.armies())
    {
        if (a.getId() == armyId || a.isSettlerArmy())
            continue;
        int d = mine->position().distanceTo(a.position());
        if (bestDist < 0 || d < bestDist)
        {
            bestDist = d;
            bestAlly = &a;
        }
    }
    if (!bestAlly)
        return false;

    int cost = state.map().movementCostTo(mine->position(), bestAlly->position(), mine->movementPointsRemaining());
    if (cost >= 0 && cost <= mine->movementPointsRemaining())
    {
        std::string intoId = bestAlly->getId();
        std::vector<Unit> incoming = mine->units();
        Army *into = state.findArmy(intoId);
        if (!into)
            return false;
        into->addUnits(incoming);
        p.removeArmy(armyId);
        return true;
    }

    Coords targetCoords = bestAlly->position();
    constexpr int LARGE_BUDGET = 100000;

    Coords bestStep{};
    int bestPathCost = -1;
    bool found = false;

    for (const auto &dest : state.map().reachableTiles(mine->position(), mine->movementPointsRemaining()))
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

        int pathCost = state.map().movementCostTo(dest, targetCoords, LARGE_BUDGET);
        if (pathCost < 0)
            continue;

        if (!found || pathCost < bestPathCost)
        {
            bestPathCost = pathCost;
            bestStep = dest;
            found = true;
        }
    }

    if (found)
    {
        state.moveArmy(armyId, bestStep);
        return true;
    }
    return false;
}

void AIController::decideMilitaryActions(Player &p, GameState &state, GlobalPhase phase, BotMemory &memory)
{
    std::vector<std::string> armyIds;
    for (const auto &a : p.armies())
        armyIds.push_back(a.getId());

    int focusTargetId = (memory.goal == BotGoal::WarWith) ? memory.goalTargetId : -1;

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

        auto collectCandidates = [&](bool restrictToFocus)
        {
            for (const auto &enemy : state.players())
            {
                if (GameState::isAllied(state, p.id(), enemy.id()))
                    continue;
                if (restrictToFocus && enemy.id() != focusTargetId)
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
        };

        collectCandidates(focusTargetId >= 0);
        if (candidates.empty() && focusTargetId >= 0)
            collectCandidates(false);

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
            // Врагов в радиусе хода нет вообще — приоритет продолжать марш к
            // цели, а НЕ сливаться с ближайшей своей армией. Раньше порядок
            // был обратный: как только у дома появлялся свежий небольшой
            // отряд (после найма), уже марширующая армия разворачивалась к
            // нему "усилиться" — и так по кругу, партия зависала на 200+
            // ходов, потому что армия никогда не продвигалась к настоящей
            // цели. Слияние остаётся приоритетом только когда враг РЯДОМ, но
            // бой невыгоден (см. ветку ниже, после !attacked).
            bool moved = false;
            {
                Coords beforePos = mine->position();
                pursueNearestEnemy(p, state, *mine, armyId, occupied);
                Army *stillMine = state.findArmy(armyId);
                moved = stillMine && !(stillMine->position().q == beforePos.q &&
                                       stillMine->position().r == beforePos.r);
            }
            if (!moved)
                tryRallyOrMerge(p, state, armyId, occupied);
            continue;
        }

        std::sort(candidates.begin(), candidates.end(),
                 [](const Candidate &a, const Candidate &b) { return a.score > b.score; });

        bool attacked = false;
        for (size_t i = 0; i < candidates.size() && i < 3; ++i)
        {
            if (candidates[i].score <= 0.0f)
                break;

            Coords dest = candidates[i].destination;

            const Player *defenderOwner = nullptr;
            const std::vector<Unit> *defenderUnits = nullptr;
            static const std::vector<Building> emptyBuildings;
            const std::vector<Building> *defenderBuildings = &emptyBuildings;
            float terrainBonusVal = 1.0f;
            bool eliminationTarget = false;

            for (const auto &enemy : state.players())
            {
                if (GameState::isAllied(state, p.id(), enemy.id()))
                    continue;

                for (const auto &s : enemy.settlements())
                {
                    if (s.tiles().empty())
                        continue;
                    Coords c = s.tiles().front().getCoords();
                    if (c.q == dest.q && c.r == dest.r)
                    {
                        defenderOwner = &enemy;
                        defenderUnits = &s.garrison();
                        defenderBuildings = &s.builtBuildings();
                        const Tile *t = state.map().tileAt(dest.q, dest.r);
                        terrainBonusVal = t ? terrainBonusFor(t->getType()) : 1.0f;
                        eliminationTarget = (enemy.settlements().size() == 1);
                        break;
                    }
                }
                if (defenderOwner)
                    break;

                for (const auto &ea : enemy.armies())
                {
                    if (ea.position().q == dest.q && ea.position().r == dest.r)
                    {
                        defenderOwner = &enemy;
                        defenderUnits = &ea.units();
                        eliminationTarget = (enemy.settlements().size() <= 1);
                        break;
                    }
                }
                if (defenderOwner)
                    break;
            }

            if (!defenderOwner || !defenderUnits)
                continue;

            // Оборонительный удар: цель (враг) стоит рядом с одним из НАШИХ
            // городов — значит это не инициативная агрессия, а отражение
            // угрозы у границ. Ранняя осторожность здесь не действует (см.
            // worthAttacking) — иначе бот будет терпеливо ждать своего
            // расписания, пока враг уже готовится штурмовать столицу.
            constexpr int DEFENSIVE_RADIUS = 4;
            bool isDefensive = false;
            for (const auto &mySettlement : p.settlements())
            {
                if (mySettlement.tiles().empty())
                    continue;
                if (dest.distanceTo(mySettlement.tiles().front().getCoords()) <= DEFENSIVE_RADIUS)
                {
                    isDefensive = true;
                    break;
                }
            }

            if (worthAttacking(p, *mine, *defenderOwner, *defenderUnits, terrainBonusVal,
                               *defenderBuildings, eliminationTarget, state.turnNumber(), isDefensive))
            {
                state.moveArmy(armyId, dest);
                attacked = true;
                break;
            }
        }

        if (!attacked)
        {
            if (!tryRallyOrMerge(p, state, armyId, occupied))
                pursueNearestEnemy(p, state, *mine, armyId, occupied);
        }
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

    // Раньше тай-брейк шёл по сырому гекс-расстоянию до цели — жадный выбор
    // "ближайшей по прямой" реачable-клетки может застревать в ложном
    // минимуме у препятствий (вода/горы), где обход требует временно
    // отдалиться. Используем настоящую стоимость пути (та же Dijkstra, что
    // и movementCostTo для проверки досягаемости) — это медленнее, но
    // скорость не критична (см. обсуждение архитектуры ИИ), а корректность
    // важна: раньше это приводило к вечным зависшим партиям.
    constexpr int LARGE_BUDGET = 100000;

    Coords bestStep{};
    int bestPathCost = -1;
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

        int pathCost = state.map().movementCostTo(dest, targetCoords, LARGE_BUDGET);
        if (pathCost < 0)
            continue; // из этой клетки цель вообще недостижима (например, отрезана водой)

        if (!found || pathCost < bestPathCost)
        {
            bestPathCost = pathCost;
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

void AIController::decideDeployment(Player &p, GameState &state)
{
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

        size_t safeFloor = computeSafeGarrisonFloor(p, state, s);
        if (garrison.size() <= safeFloor)
            continue;

        std::vector<std::string> toDeploy;
        for (size_t i = 0; i + safeFloor < garrison.size(); ++i)
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
