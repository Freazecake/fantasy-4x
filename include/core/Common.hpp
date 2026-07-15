#pragma once

#include <string>
#include <array>
#include <utility>
#include <algorithm>

enum class Sphere
{
    Science,
    Industry,
    Trade,
    Diplomacy,
    Military,
    Growth
};

inline std::string sphereToString(Sphere s)
{
    std::string result;
    switch (s)
    {
    case Sphere::Science:
        result = "science";
        break;
    case Sphere::Industry:
        result = "industry";
        break;
    case Sphere::Trade:
        result = "trade";
        break;
    case Sphere::Diplomacy:
        result = "diplomacy";
        break;
    case Sphere::Military:
        result = "military";
        break;
    case Sphere::Growth:
        result = "growth";
        break;
    }
    return result;
}

inline Sphere sphereFromString(std::string s)
{
    if (s == "science")
        return Sphere::Science;
    if (s == "industry")
        return Sphere::Industry;
    if (s == "trade")
        return Sphere::Trade;
    if (s == "diplomacy")
        return Sphere::Diplomacy;
    if (s == "military")
        return Sphere::Military;
    if (s == "growth")
        return Sphere::Growth;
}

struct Modifiers
{
    float science = 1.0f;
    float industry = 1.0f;
    float trade = 1.0f;
    float diplomacy = 1.0f;
    float military = 1.0f;
    float growth = 1.0f;

    Modifiers() = default;

    const std::pair<Sphere, float> findMaxModifier(const Modifiers &m)
    {
        const std::array<std::pair<Sphere, float>, 6> values = {{
            {Sphere::Science, m.science},
            {Sphere::Industry, m.industry},
            {Sphere::Trade, m.trade},
            {Sphere::Diplomacy, m.diplomacy},
            {Sphere::Military, m.military},
            {Sphere::Growth, m.growth},
        }};

        return *std::max_element(values.begin(), values.end(),
                                 [](const auto &a, const auto &b)
                                 { return a.second < b.second; });
    }

    std::string toString() const
    {
        return "Modifiers: Science: " + std::to_string(science) +
               ", Industry: " + std::to_string(industry) +
               ", Trade: " + std::to_string(trade) +
               ", Diplomacy: " + std::to_string(diplomacy) +
               ", Military: " + std::to_string(military) +
               ", Growth: " + std::to_string(growth);
    }
};

struct UnitStats
{
    float attack = 0.0f;
    float defense = 0.0f;
    float health = 0.0f;
};