#include "core/TradeMarket.hpp"

#include <stdexcept>

TradeExchangeResult TradeMarket::exchange(float buyerTradeBonusMultiplier,
                                          const std::string &targetResource,
                                          int tradeToSpend)
{
    if (targetResource == "trade")
    {
        throw std::invalid_argument(
            "TradeMarket::exchange: нельзя покупать торговые очки");
    }
    if (targetResource != "science" && targetResource != "industry" &&
        targetResource != "diplomacy")
    {
        throw std::invalid_argument("TradeMarket::exchange: неизвестный targetResource '" +
                                    targetResource + "'");
    }
    if (tradeToSpend < 0)
    {
        throw std::invalid_argument("TradeMarket::exchange: tradeToSpend не может быть отрицательным");
    }

    TradeExchangeResult result;
    result.spentTrade = tradeToSpend;
    result.received = static_cast<int>(tradeToSpend * buyerTradeBonusMultiplier);
    result.receivedResourceType = targetResource;
    return result;
}
