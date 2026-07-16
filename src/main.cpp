#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>

#include "core/Hero.hpp"

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::ifstream file("../assets/heroes.json");
    json heroesJson = json::parse(file);
    std::vector<Hero> heroes = Hero::loadAllFromJson(heroesJson["heroes"]);

    for (const auto &hero : heroes)
        std::cout << hero.toString() << std::endl;

    return 0;
}