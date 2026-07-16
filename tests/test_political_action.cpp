#include "test_framework.hpp"

#include "core/PoliticalAction.hpp"

namespace {

json makeActionJson(const std::string &id, const std::string &type, int cost) {
    return json{{"id", id}, {"name", id + "_name"}, {"type", type}, {"diplomacyCost", cost}};
}

}  // namespace

TEST_CASE("PoliticalAction::loadFromJson читает id, стоимость и тип") {
    PoliticalAction action = PoliticalAction::loadFromJson(makeActionJson("pa_alliance", "alliance", 50));

    CHECK(action.getId() == "pa_alliance");
    CHECK(action.getCost() == 50);
    CHECK(action.getType() == PoliticalActionType::Alliance);
}

TEST_CASE("PoliticalAction: только politicalConquest считается победным действием") {
    PoliticalAction alliance = PoliticalAction::loadFromJson(makeActionJson("pa_alliance", "alliance", 50));
    PoliticalAction conquest =
        PoliticalAction::loadFromJson(makeActionJson("pa_conquest", "politicalConquest", 300));

    CHECK(!alliance.isVictoryAction());
    CHECK(conquest.isVictoryAction());
}

TEST_CASE("PoliticalAction::loadAllFromJson загружает весь массив действий") {
    json arr = json::array({
        makeActionJson("pa_alliance", "alliance", 50),
        makeActionJson("pa_vassalize", "vassalize", 150),
    });

    std::vector<PoliticalAction> actions = PoliticalAction::loadAllFromJson(arr);

    CHECK(actions.size() == 2u);
    CHECK(actions[0].getType() == PoliticalActionType::Alliance);
    CHECK(actions[1].getType() == PoliticalActionType::Vassalize);
}

TEST_CASE("politicalActionTypeFromString/ToString: все 4 типа туда-обратно") {
    std::vector<std::string> names = {"alliance", "vassalize", "demandTribute", "politicalConquest"};
    for (const auto &name : names) {
        PoliticalActionType t = politicalActionTypeFromString(name);
        CHECK(politicalActionTypeToString(t) == name);
    }
}

TEST_CASE("politicalActionTypeFromString: неизвестная строка бросает исключение") {
    CHECK_THROWS(politicalActionTypeFromString("betrayal"));
}
