#include "test_framework.hpp"

#include "core/TradeMarket.hpp"

TEST_CASE("TradeMarket::exchange: базовый курс 1:1 без бонуса") {
    TradeExchangeResult result = TradeMarket::exchange(1.0f, "science", 10);

    CHECK(result.spentTrade == 10);
    CHECK(result.received == 10);
    CHECK(result.receivedResourceType == "science");
}

TEST_CASE("TradeMarket::exchange: бонус покупателя увеличивает курс (+40% -> 1.4:1)") {
    TradeExchangeResult result = TradeMarket::exchange(1.4f, "industry", 10);

    CHECK(result.received == 14);
}

TEST_CASE("TradeMarket::exchange: покупка торговли за торговлю запрещена") {
    CHECK_THROWS(TradeMarket::exchange(1.0f, "trade", 10));
}

TEST_CASE("TradeMarket::exchange: неизвестный ресурс бросает исключение") {
    CHECK_THROWS(TradeMarket::exchange(1.0f, "magic", 10));
}

TEST_CASE("TradeMarket::exchange: отрицательный tradeToSpend бросает исключение") {
    CHECK_THROWS(TradeMarket::exchange(1.0f, "science", -5));
}

TEST_CASE("TradeMarket::exchange: работает для всех трёх допустимых целевых ресурсов") {
    for (const std::string &target : {std::string("science"), std::string("industry"),
                                       std::string("diplomacy")}) {
        TradeExchangeResult result = TradeMarket::exchange(1.2f, target, 5);
        CHECK(result.receivedResourceType == target);
        CHECK(result.received == 6);
    }
}
