#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>

#include "core/Unit.hpp"

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::ifstream file("../assets/units.json");
    json unitsJson = json::parse(file);
    std::vector<Unit> units = Unit::loadAllFromJson(unitsJson["units"]);

    for (const auto &unit : units)
        std::cout << unit.toString() << std::endl;

    return 0;
}