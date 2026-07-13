#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>

#include "core/Building.hpp"

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::ifstream file("../assets/buildings.json");
    json buildingsJson = json::parse(file);
    std::vector<Building> buildings;
    for (const auto &buildingJson : buildingsJson["buildings"])
    {
        Building building = Building::loadFromJson(buildingJson);
        buildings.push_back(building);
        std::cout << building.toString() << std::endl;
    }
    return 0;
}