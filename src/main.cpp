#include <iostream>
#include <fstream>
#include <vector>
#include <Windows.h>

#include "core/Technology.hpp"

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::ifstream file("../assets/technologies.json");
    json technologiesJson = json::parse(file);
    std::vector<Technology> technologies = Technology::loadAllFromJson(technologiesJson["technologies"]);

    for (const auto &tech : technologies)
    {
        std::cout << tech.getId() << " | " << tech.getName()
                  << " (" << tech.getCategory() << "), cost=" << tech.getCost();
        if (!tech.getPrerequisiteIds().empty())
        {
            std::cout << ", requires: ";
            for (const auto &req : tech.getPrerequisiteIds())
                std::cout << req << " ";
        }
        if (!tech.getUnloksBuildingsIds().empty())
        {
            std::cout << ", unlocks: ";
            for (const auto &req : tech.getUnloksBuildingsIds())
                std::cout << req << " ";
        }
        std::cout << std::endl;
    }

    // проверка invest()/progress() на первой технологии
    Technology &t = technologies.front();
    std::cout << "\n-- invest() на " << t.getId() << " (cost=" << t.getCost() << ") --\n";
    t.invest(t.getCost() / 2);
    std::cout << "progress=" << t.progress() << ", remaining=" << t.remainingCost()
              << ", isResearched=" << t.isResearched() << std::endl;
    t.invest(t.remainingCost());
    std::cout << "progress=" << t.progress() << ", isResearched=" << t.isResearched() << std::endl;

    // проверка canResearch на первой + второй технологии
    std::unordered_set<std::string> ResTechIds;

    if (technologies[1].canResearch(ResTechIds))
        std::cout << "True\n";
    else
        std::cout << "False\n";

    ResTechIds.emplace(t.getId());

    if (technologies[1].canResearch(ResTechIds))
        std::cout << "True\n";
    else
        std::cout << "False\n";

    return 0;
}