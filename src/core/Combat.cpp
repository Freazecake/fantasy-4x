#include "core/Combat.hpp"
#include <algorithm>
#include <cmath>

static constexpr int MAX_ROUNDS = 100;

struct CombatUnit
{
    std::string id;
    std::string name;
    float attack;
    float defense;
    float currentHealth;
    float maxHealth;
    bool alive = true;
};

static std::vector<CombatUnit> prepareUnits(const std::vector<Unit> &units,
                                             const std::vector<Hero> &heroes,
                                             float raceModifier,
                                             const std::vector<Building> &buildings)
{
    std::vector<CombatUnit> result;
    for (const auto &u : units)
    {
        UnitStats stats = Military::effectiveStats(u, buildings, raceModifier, heroes);
        float baseHealth = u.getStats().health;
        float hpScale = baseHealth > 0.0f ? stats.health / baseHealth : 1.0f;

        CombatUnit cu;
        cu.id = u.getId();
        cu.name = u.getName();
        cu.attack = stats.attack;
        cu.defense = stats.defense;
        cu.maxHealth = stats.health;
        cu.currentHealth = u.getCurrentHealth() * hpScale;
        if (cu.currentHealth <= 0.0f)
            cu.alive = false;
        result.push_back(cu);
    }
    return result;
}

static int findWeakest(const std::vector<CombatUnit> &units)
{
    int idx = -1;
    float minHp = 1e9f;
    for (int i = 0; i < static_cast<int>(units.size()); ++i)
    {
        if (units[i].alive && units[i].currentHealth < minHp)
        {
            minHp = units[i].currentHealth;
            idx = i;
        }
    }
    return idx;
}

CombatResult Combat::resolve(std::vector<Unit> attackers,
                             std::vector<Unit> defenders,
                             std::vector<Hero> attackerHeroes,
                             std::vector<Hero> defenderHeroes,
                             float defenderTerrainBonus,
                             const std::vector<Building> &builtBuildings,
                             float attackerRaceModifier,
                             float defenderRaceModifier)
{
    CombatResult result;

    auto atkUnits = prepareUnits(attackers, attackerHeroes, attackerRaceModifier, {});
    auto defUnits = prepareUnits(defenders, defenderHeroes, defenderRaceModifier, builtBuildings);

    bool timedOut = false;

    for (int round = 1; round <= MAX_ROUNDS; ++round)
    {
        // Цель каждой стороны на этот раунд — самый слабый живой юнит противника,
        // определённый ДО того, как кто-либо нанёс урон в этом раунде (снимок на
        // начало раунда). Это гарантирует, что все живые юниты обеих сторон бьют
        // "одновременно" в рамках раунда, а не по очереди с преимуществом первого.
        int defenderTargetIdx = findWeakest(defUnits); // кого бьют атакующие
        int attackerTargetIdx = findWeakest(atkUnits);  // кого бьют защитники

        if (defenderTargetIdx < 0 || attackerTargetIdx < 0)
            break;

        // Урон копится в буферах по ИНДЕКСУ цели (не по id — id это тип юнита,
        // а не уникальный экземпляр, в армии может быть несколько юнитов одного
        // типа) и применяется после того, как все удары раунда посчитаны, иначе
        // юнит, погибший в середине раунда, не смог бы нанести свой удар, хотя
        // формально был жив в начале раунда.
        std::vector<float> dmgToDefenders(defUnits.size(), 0.0f);
        std::vector<float> dmgToAttackers(atkUnits.size(), 0.0f);

        for (size_t i = 0; i < atkUnits.size(); ++i)
        {
            if (!atkUnits[i].alive)
                continue;
            const auto &target = defUnits[defenderTargetIdx];
            float dmg = std::max(1.0f, atkUnits[i].attack - target.defense * defenderTerrainBonus);
            dmgToDefenders[defenderTargetIdx] += dmg;

            CombatRoundLog logEntry;
            logEntry.roundNumber = round;
            logEntry.attackerUnitId = atkUnits[i].id;
            logEntry.defenderUnitId = target.id;
            logEntry.damageDealt = dmg;
            logEntry.defenderUnitDied = false; // выставляется ниже, после применения урона
            result.log.push_back(logEntry);
        }

        for (size_t i = 0; i < defUnits.size(); ++i)
        {
            if (!defUnits[i].alive)
                continue;
            const auto &target = atkUnits[attackerTargetIdx];
            float dmg = std::max(1.0f, defUnits[i].attack - target.defense);
            dmgToAttackers[attackerTargetIdx] += dmg;

            CombatRoundLog logEntry;
            logEntry.roundNumber = round;
            logEntry.attackerUnitId = defUnits[i].id;
            logEntry.defenderUnitId = target.id;
            logEntry.damageDealt = dmg;
            logEntry.defenderUnitDied = false;
            result.log.push_back(logEntry);
        }

        // Применяем урон и сразу же, по индексу (без поиска по id — он может
        // быть неоднозначным при дубликатах типа юнита), правим последние
        // добавленные в этом раунде записи лога, которые били именно в эту цель.
        if (dmgToDefenders[defenderTargetIdx] > 0.0f)
        {
            defUnits[defenderTargetIdx].currentHealth -= dmgToDefenders[defenderTargetIdx];
            if (defUnits[defenderTargetIdx].currentHealth <= 0.0f)
            {
                defUnits[defenderTargetIdx].currentHealth = 0.0f;
                defUnits[defenderTargetIdx].alive = false;
            }
        }
        if (dmgToAttackers[attackerTargetIdx] > 0.0f)
        {
            atkUnits[attackerTargetIdx].currentHealth -= dmgToAttackers[attackerTargetIdx];
            if (atkUnits[attackerTargetIdx].currentHealth <= 0.0f)
            {
                atkUnits[attackerTargetIdx].currentHealth = 0.0f;
                atkUnits[attackerTargetIdx].alive = false;
            }
        }

        for (auto &entry : result.log)
        {
            if (entry.roundNumber != round)
                continue;
            if (entry.defenderUnitId == defUnits[defenderTargetIdx].id &&
                !defUnits[defenderTargetIdx].alive)
                entry.defenderUnitDied = true;
            if (entry.defenderUnitId == atkUnits[attackerTargetIdx].id &&
                !atkUnits[attackerTargetIdx].alive)
                entry.defenderUnitDied = true;
        }

        int atkAlive = 0, defAlive = 0;
        for (const auto &u : atkUnits)
            if (u.alive) ++atkAlive;
        for (const auto &u : defUnits)
            if (u.alive) ++defAlive;

        if (atkAlive == 0 || defAlive == 0)
            break;

        if (round == MAX_ROUNDS)
            timedOut = true;
    }

    // Build survivor lists
    for (size_t i = 0; i < atkUnits.size(); ++i)
    {
        if (atkUnits[i].alive)
        {
            Unit survivor = attackers[i];
            float hpScale = survivor.getStats().health > 0.0f
                                ? atkUnits[i].currentHealth / atkUnits[i].maxHealth
                                : 0.0f;
            survivor.setCurrentHealth(survivor.getStats().health * hpScale);
            result.attackerSurvivors.push_back(survivor);
        }
    }
    for (size_t i = 0; i < defUnits.size(); ++i)
    {
        if (defUnits[i].alive)
        {
            Unit survivor = defenders[i];
            float hpScale = survivor.getStats().health > 0.0f
                                ? defUnits[i].currentHealth / defUnits[i].maxHealth
                                : 0.0f;
            survivor.setCurrentHealth(survivor.getStats().health * hpScale);
            result.defenderSurvivors.push_back(survivor);
        }
    }

    bool defendersWiped = result.defenderSurvivors.empty();
    bool attackersWiped = result.attackerSurvivors.empty();

    result.attackerWon = defendersWiped && !attackersWiped;
    // Ничья возможна только если обрыв случился по MAX_ROUNDS, а не по гибели
    // одной из сторон — с симультанным уроном за раунд это должно быть большой
    // редкостью, но обрабатывается явно, а не молча засчитывается как победа.
    result.isDraw = timedOut && !defendersWiped && !attackersWiped;

    return result;
}
