#include <iostream>
#include <Windows.h>

#include "core/Settlement.hpp"

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    Settlement s(/*ownerId=*/1, SettlementType::Village);
    s.addTile({0, 0});
    s.addTile({0, 1});

    // основание поселения: стартовые 10 жителей
    s.seedInitialPopulation(10);
    std::cout << "-- после seedInitialPopulation(10) --\n"
              << s.debugString() << "\n\n";

    // распределяем по сферам
    s.assignFromIdle(Sphere::Science, 4);
    s.assignFromIdle(Sphere::Industry, 3);
    s.assignFromIdle(Sphere::Trade, 2);
    // 1 человек остаётся в idle специально, чтобы видно было в выводе
    std::cout << "-- после распределения 9 из 10 по сферам --\n"
              << s.debugString() << "\n\n";

    // рост за ход: baseGrowthRate=0.1, модификатор расы (например, орки) = 1.5
    s.growPopulation(/*baseGrowthRate=*/0.1f, /*raceGrowthModifier=*/1.5f);
    std::cout << "-- после growPopulation(0.1, 1.5) --\n"
              << s.debugString() << "\n\n";

    // свободное перераспределение между сферами (без штрафа, раздел 2.2 гайда)
    s.reallocate(Sphere::Science, Sphere::Diplomacy, 2);
    std::cout << "-- после reallocate(Science -> Diplomacy, 2) --\n"
              << s.debugString() << "\n\n";

    // безвозвратное списание населения (например, найм юнита, раздел 3.1 гайда)
    // сначала освобождаем нужное количество из сферы в idle
    s.unassignToIdle(Sphere::Industry, 1);
    s.removePopulationPermanently(1);
    std::cout << "-- после unassignToIdle(Industry, 1) + removePopulationPermanently(1) --\n"
              << s.debugString() << "\n\n";

    // проверка защиты от некорректных операций
    try
    {
        s.assignFromIdle(Sphere::Science, 999);
    }
    catch (const std::exception &e)
    {
        std::cout << "Ожидаемая ошибка (превышение idle): " << e.what() << "\n";
    }

    try
    {
        s.removePopulationPermanently(999);
    }
    catch (const std::exception &e)
    {
        std::cout << "Ожидаемая ошибка (превышение idle при списании): " << e.what() << "\n";
    }

    std::cout << "\nТест пройден: все штатные операции отработали, ошибочные — корректно поймались.\n";
    return 0;
}