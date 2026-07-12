#pragma once

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

struct Modifiers
{
    float science = 1.0f;
    float industry = 1.0f;
    float trade = 1.0f;
    float diplomacy = 1.0f;
    float military = 1.0f;
    float growth = 1.0f;

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
};

struct UnitStats
{
    float attack = 0.0f;
    float defense = 0.0f;
    float health = 0.0f;
};