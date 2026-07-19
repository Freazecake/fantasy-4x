#include "test_framework.hpp"

#include "core/GameState.hpp"
#include "core/Player.hpp"
#include "core/PoliticalAction.hpp"

namespace
{

    json makeActionJson(const std::string &id, const std::string &type, int cost)
    {
        return json{{"id", id}, {"name", id + "_name"}, {"type", type}, {"diplomacyCost", cost}};
    }

    Race makeRace(const std::string &id)
    {
        json obj = {
            {"id", id},
            {"name", id},
            {"startingSettlementType", "village"},
            {"modifiers",
             {{"science", 1}, {"industry", 1}, {"trade", 1}, {"diplomacy", 1}, {"military", 1}, {"growth", 1}}},
        };
        return Race::loadFromJson(obj);
    }

    // Вспомогательный набор: 2 игрока с 30 science, 20 industry, 10 trade, 5 diplomacy
    struct TwoPlayerFixture
    {
        GameState state;
        int srcId = 0, tgtId = 1;

        TwoPlayerFixture()
        {
            for (int id : {srcId, tgtId})
            {
                Player p;
                p.setId(id);
                p.setRace(makeRace("humans"));
                Settlement s(id, SettlementType::Village);
                s.seedInitialPopulation(80);
                s.assignFromIdle(Sphere::Science, 30);
                s.assignFromIdle(Sphere::Industry, 20);
                s.assignFromIdle(Sphere::Trade, 10);
                s.assignFromIdle(Sphere::Diplomacy, 5);
                p.addSettlement(s);
                p.advanceTurn();
                state.addPlayer(p);
            }
        }
    };

} // namespace

// --- takeFractionFromEconomy / addResources ---

TEST_CASE("Player::takeFractionFromEconomy: берёт долю и вычитает из экономики")
{
    Player p;
    p.setRace(makeRace("humans"));
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(30);
    s.assignFromIdle(Sphere::Science, 20);
    s.assignFromIdle(Sphere::Industry, 10);
    p.addSettlement(s);
    p.advanceTurn();

    Resources taken = p.takeFractionFromEconomy(0.5f);

    CHECK_NEAR(taken.science, 10.0f, 0.01f);
    CHECK_NEAR(taken.industry, 5.0f, 0.01f);
    CHECK_NEAR(p.resources().science, 10.0f, 0.01f);
    CHECK_NEAR(p.resources().industry, 5.0f, 0.01f);
}

TEST_CASE("Player::takeFractionFromEconomy: нулевая доля — ничего не меняет")
{
    Player p;
    p.setRace(makeRace("humans"));
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(30);
    s.assignFromIdle(Sphere::Science, 20);
    p.addSettlement(s);
    p.advanceTurn();

    Resources taken = p.takeFractionFromEconomy(0.0f);

    CHECK_NEAR(taken.science, 0.0f, 0.01f);
    CHECK_NEAR(p.resources().science, 20.0f, 0.01f);
}

TEST_CASE("Player::addResources: добавляет ресурсы к накопленным")
{
    Player p;
    p.setRace(makeRace("humans"));
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(10);
    s.assignFromIdle(Sphere::Industry, 10);
    p.addSettlement(s);
    p.advanceTurn();

    Resources extra;
    extra.science = 15.0f;
    extra.industry = 5.0f;
    p.addResources(extra);

    CHECK_NEAR(p.resources().science, 15.0f, 0.01f);
    CHECK_NEAR(p.resources().industry, 15.0f, 0.01f); // 10 + 5
}

// --- Alliance ---

TEST_CASE("GameState::newAlliance: создаёт альянс и ставит бонус обоим")
{
    GameState state;
    Player p1, p2;
    p1.setId(0); p2.setId(1);
    state.addPlayer(p1);
    state.addPlayer(p2);

    state.newAlliance(state.players()[0], state.players()[1], 10);

    CHECK(state.alliances().size() == 1u);
    CHECK(state.alliances()[0].playerA == 0);
    CHECK(state.alliances()[0].playerB == 1);
    CHECK(state.alliances()[0].expiryTurn == 10);
    CHECK(state.players()[0].allianceBonus());
    CHECK(state.players()[1].allianceBonus());
}

TEST_CASE("GameState::checkAlliances: не удаляет активный альянс")
{
    GameState state;
    Player p1, p2;
    p1.setId(0); p2.setId(1);
    state.addPlayer(p1);
    state.addPlayer(p2);

    state.newAlliance(state.players()[0], state.players()[1], 5);
    // turnNumber = 0, expiryTurn = 5 → 5 > 0 = true → keep
    state.checkAlliances();

    CHECK(state.alliances().size() == 1u);
    CHECK(state.players()[0].allianceBonus());
}

TEST_CASE("GameState::checkAlliances: удаляет истёкший альянс и снимает бонус")
{
    GameState state;
    Player p1, p2;
    p1.setId(0); p2.setId(1);
    state.addPlayer(p1);
    state.addPlayer(p2);

    // turnNumber = 0, expiryTurn = 0 → expires immediately
    state.newAlliance(state.players()[0], state.players()[1], 0);
    state.checkAlliances();

    CHECK(state.alliances().empty());
    CHECK(!state.players()[0].allianceBonus());
    CHECK(!state.players()[1].allianceBonus());
}

TEST_CASE("GameState::checkAlliances: удаляет только истёкшие, активные остаются")
{
    GameState state;
    Player p1, p2, p3;
    p1.setId(0); p2.setId(1); p3.setId(2);
    state.addPlayer(p1);
    state.addPlayer(p2);
    state.addPlayer(p3);

    // p1-p2 expires at 0, p1-p3 expires at 5
    state.newAlliance(state.players()[0], state.players()[1], 0);
    state.newAlliance(state.players()[0], state.players()[2], 5);
    // turnNumber = 0, first alliance expires, second stays
    state.checkAlliances();

    CHECK(state.alliances().size() == 1u);
    CHECK(state.alliances()[0].playerA == 0);
    CHECK(state.alliances()[0].playerB == 2);
    CHECK(state.players()[0].allianceBonus());   // p1 still in alliance with p3
    CHECK(!state.players()[1].allianceBonus()); // expired
    CHECK(state.players()[2].allianceBonus());   // still in active
}

// --- DemandTribute ---

TEST_CASE("GameState::applyDemandTribute: передаёт 30% ресурсов от цели источнику")
{
    TwoPlayerFixture f;
    // srcId=0: science=30, industry=20, trade=10, diplomacy=5
    // tgtId=1: same

    f.state.applyDemandTribute(f.srcId, f.tgtId);

    float expectedTaken = 30.0f * 0.3f; // 9.0
    float expectedRemaining = 30.0f - expectedTaken; // 21.0

    // Источник получил 30% от цели
    CHECK_NEAR(f.state.players()[0].resources().science, 30.0f + expectedTaken, 0.1f);
    CHECK_NEAR(f.state.players()[0].resources().industry, 20.0f + 20.0f * 0.3f, 0.1f);

    // Цель потеряла 30%
    CHECK_NEAR(f.state.players()[1].resources().science, expectedRemaining, 0.1f);
    CHECK_NEAR(f.state.players()[1].resources().industry, 20.0f * 0.7f, 0.1f);
}

TEST_CASE("GameState::applyDemandTribute: не падает если цель не найдена")
{
    GameState state;
    Player p;
    p.setId(0);
    state.addPlayer(p);

    CHECK_NOTHROW(state.applyDemandTribute(0, 999));
}

// --- Vassalization ---

TEST_CASE("GameState::applyVassalize: записывает вассалитет")
{
    GameState state;
    Player p1, p2;
    p1.setId(0); p2.setId(1);
    state.addPlayer(p1);
    state.addPlayer(p2);

    state.applyVassalize(0, 1);

    CHECK(state.vassalizations().size() == 1u);
    CHECK(state.vassalizations().at(1) == 0);
}

TEST_CASE("GameState::processVassalage: передаёт 30% накопленных ресурсов вассала хозяину")
{
    GameState state;
    for (int id : {0, 1})
    {
        Player p;
        p.setId(id);
        p.setRace(makeRace("humans"));
        Settlement s(id, SettlementType::Village);
        s.seedInitialPopulation(80);
        s.assignFromIdle(Sphere::Science, 20);
        s.assignFromIdle(Sphere::Industry, 10);
        p.addSettlement(s);
        p.advanceTurn();
        state.addPlayer(p);
    }

    // advanceTurn дал игроку 1 (вассалу): science=20, industry=10
    state.applyVassalize(0, 1);
    state.processVassalage();

    // Вассал потерял 30%
    CHECK_NEAR(state.players()[1].resources().science, 14.0f, 0.1f);  // 20 - 6
    CHECK_NEAR(state.players()[1].resources().industry, 7.0f, 0.1f);  // 10 - 3

    // Хозяин получил 30% от вассала
    CHECK_NEAR(state.players()[0].resources().science, 26.0f, 0.1f);  // 20 + 6
    CHECK_NEAR(state.players()[0].resources().industry, 13.0f, 0.1f); // 10 + 3
}

// --- Political Conquest ---

TEST_CASE("GameState::applyPoliticalConquest: поселения цели переходят завоевателю")
{
    GameState state;
    for (int id : {0, 1})
    {
        Player p;
        p.setId(id);
        Settlement s(id, SettlementType::Village);
        s.seedInitialPopulation(10);
        p.addSettlement(s);
        // второе поселение
        Settlement s2(id, SettlementType::Town);
        s2.seedInitialPopulation(10);
        p.addSettlement(s2);
        state.addPlayer(p);
    }

    state.applyPoliticalConquest(0, 1);

    CHECK(state.players()[1].settlements().empty());
    CHECK(state.players()[0].settlements().size() == 4u); // 2 своих + 2 цели
    for (const auto &s : state.players()[0].settlements())
        CHECK(s.ownerId() == 0);
}

TEST_CASE("GameState::applyPoliticalConquest: ресурсы цели переходят завоевателю")
{
    GameState state;
    for (int id : {0, 1})
    {
        Player p;
        p.setId(id);
        p.setRace(makeRace("humans"));
        Settlement s(id, SettlementType::Village);
        s.seedInitialPopulation(80);
        s.assignFromIdle(Sphere::Science, 20);
        p.addSettlement(s);
        p.advanceTurn();
        state.addPlayer(p);
    }

    state.applyPoliticalConquest(0, 1);

    // У цели 0 ресурсов
    CHECK_NEAR(state.players()[1].resources().science, 0.0f, 0.1f);
    // У завоевателя были свои 20 + забрал 20 у цели
    CHECK_NEAR(state.players()[0].resources().science, 40.0f, 0.1f);
}

TEST_CASE("GameState::applyPoliticalConquest: если остался 1 игрок → политическая победа")
{
    GameState state;
    for (int id : {0, 1})
    {
        Player p;
        p.setId(id);
        Settlement s(id, SettlementType::Village);
        s.seedInitialPopulation(10);
        p.addSettlement(s);
        state.addPlayer(p);
    }

    state.applyPoliticalConquest(0, 1);

    VictoryResult v = state.checkVictory();
    CHECK(v.hasWinner());
    CHECK(v.winnerId == 0);
    CHECK(v.type == "political");
}

TEST_CASE("GameState::applyPoliticalConquest: при 3+ игроках conquest не даёт победу")
{
    GameState state;
    for (int id : {0, 1, 2})
    {
        Player p;
        p.setId(id);
        Settlement s(id, SettlementType::Village);
        s.seedInitialPopulation(10);
        p.addSettlement(s);
        state.addPlayer(p);
    }

    state.applyPoliticalConquest(0, 1);

    VictoryResult v = state.checkVictory();
    CHECK(!v.hasWinner());
}

// --- checkVictory (military) ---

TEST_CASE("GameState::checkVictory: military — один игрок без поселений → победы нет")
{
    GameState state;
    Player p1, p2;
    p1.setId(0);
    p2.setId(1);
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(10);
    p1.addSettlement(s);
    // p2 without settlements
    state.addPlayer(p1);
    state.addPlayer(p2);

    VictoryResult v = state.checkVictory();
    CHECK(v.hasWinner());
    CHECK(v.winnerId == 0);
    CHECK(v.type == "military");
}

TEST_CASE("GameState::checkVictory: military — все игроки с поселениями → нет победы")
{
    GameState state;
    for (int id : {0, 1})
    {
        Player p;
        p.setId(id);
        Settlement s(id, SettlementType::Village);
        s.seedInitialPopulation(10);
        p.addSettlement(s);
        state.addPlayer(p);
    }

    CHECK(!state.checkVictory().hasWinner());
}

TEST_CASE("GameState::checkVictory: military — 1 игрок всего → нет победы (никого не победил)")
{
    GameState state;
    Player p;
    p.setId(0);
    Settlement s(0, SettlementType::Village);
    s.seedInitialPopulation(10);
    p.addSettlement(s);
    state.addPlayer(p);

    CHECK(!state.checkVictory().hasWinner());
}

TEST_CASE("GameState::checkVictory: military — 1 живой из 2 → победа")
{
    GameState state;
    for (int id : {0, 1})
    {
        Player p;
        p.setId(id);
        if (id == 0)
        {
            Settlement s(0, SettlementType::Village);
            s.seedInitialPopulation(10);
            p.addSettlement(s);
        }
        state.addPlayer(p);
    }

    VictoryResult v = state.checkVictory();
    CHECK(v.hasWinner());
    CHECK(v.winnerId == 0);
    CHECK(v.type == "military");
}
