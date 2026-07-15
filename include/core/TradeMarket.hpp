#pragma once

#include <string>

struct TradeExchangeResult
{
    int spentTrade = 0;
    int received = 0;
    std::string receivedResourceType;
};

class TradeMarket
{
public:
    static TradeExchangeResult exchange(float buyerTradeBonusMultiplier,
                                        const std::string &targetResource,
                                        int tradeToSpend);
};
