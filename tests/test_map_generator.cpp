#include "test_framework.hpp"

#include "core/MapGenerator.hpp"
#include "core/StartLayout.hpp"

namespace
{

bool isFullyConnectedForTest(const Map &map)
{
    // Тот же метод, что использует сам генератор для собственной проверки —
    // если он сломан, ломаются оба места разом, что и обнаружит тест.
    const auto &tiles = map.getMap();

    Coords start{};
    bool foundStart = false;
    int landCount = 0;
    for (const auto &t : tiles)
    {
        if (t.getType() == TileType::Water)
            continue;
        ++landCount;
        if (!foundStart)
        {
            start = t.getCoords();
            foundStart = true;
        }
    }
    if (!foundStart)
        return true;

    int budget = static_cast<int>(tiles.size()) * 3 + 10;
    auto reachable = map.reachableTiles(start, budget);
    return static_cast<int>(reachable.size()) + 1 >= landCount;
}

} // namespace

TEST_CASE("MapGenerator::generate: карта полностью связна по суше")
{
    MapGenerationParams params;
    params.seed = 42;
    params.guaranteedLandCenters = {{7, 5}};

    Map map = MapGenerator::generate(params);
    CHECK(isFullyConnectedForTest(map));
}

TEST_CASE("MapGenerator::generate: та же карта связна на нескольких разных seed подряд")
{
    // Один зелёный прогон на связность мог бы оказаться удачным совпадением
    // — прогоняем несколько разных seed, чтобы не полагаться на один удачный.
    for (unsigned int seed = 1; seed <= 10; ++seed)
    {
        MapGenerationParams params;
        params.seed = seed;
        params.guaranteedLandCenters = {{7, 5}};
        Map map = MapGenerator::generate(params);
        CHECK(isFullyConnectedForTest(map));
    }
}

TEST_CASE("MapGenerator::generate: стартовая зона (радиус вокруг центра) не бывает водой")
{
    MapGenerationParams params;
    params.seed = 7;
    params.guaranteedLandCenters = {{7, 5}};
    params.guaranteedLandRadius = 2;

    Map map = MapGenerator::generate(params);

    bool anyWaterInZone = false;
    for (int q = 5; q <= 9; ++q)
    {
        for (int r = 3; r <= 7; ++r)
        {
            if (Coords(7, 5).distanceTo({q, r}) > 2)
                continue;
            const Tile *t = map.tileAt(q, r);
            CHECK(t != nullptr);
            if (t != nullptr && t->getType() == TileType::Water)
                anyWaterInZone = true;
        }
    }
    CHECK(!anyWaterInZone);
}

TEST_CASE("MapGenerator::generate: несколько стартовых центров — все защищены одновременно")
{
    MapGenerationParams params;
    params.seed = 99;
    params.width = 20;
    params.height = 14;
    params.guaranteedLandCenters = {{2, 2}, {17, 11}};
    params.guaranteedLandRadius = 2;

    Map map = MapGenerator::generate(params);

    for (const auto &center : params.guaranteedLandCenters)
    {
        const Tile *t = map.tileAt(center.q, center.r);
        CHECK(t != nullptr);
        if (t != nullptr)
            CHECK(t->getType() != TileType::Water);
    }
    CHECK(isFullyConnectedForTest(map));
}

TEST_CASE("MapGenerator::generate: доля воды остаётся в разумных пределах")
{
    MapGenerationParams params;
    params.seed = 1;
    params.waterFraction = 0.12f;
    params.guaranteedLandCenters = {{7, 5}};

    Map map = MapGenerator::generate(params);

    int waterCount = 0;
    for (const auto &t : map.getMap())
        if (t.getType() == TileType::Water)
            ++waterCount;

    float actualFraction = static_cast<float>(waterCount) / static_cast<float>(map.getMap().size());
    CHECK(actualFraction > 0.02f); // не "почти совсем без воды"
    CHECK(actualFraction < 0.30f); // не "почти вся карта — вода"
}

TEST_CASE("MapGenerator::generate: карта содержит несколько разных типов местности")
{
    // Главная цель всей задачи — карта больше не однородная равнина.
    MapGenerationParams params;
    params.seed = 5;
    params.guaranteedLandCenters = {{7, 5}};

    Map map = MapGenerator::generate(params);

    bool hasWater = false, hasMountain = false, hasForest = false;
    for (const auto &t : map.getMap())
    {
        if (t.getType() == TileType::Water) hasWater = true;
        if (t.getType() == TileType::Mountain) hasMountain = true;
        if (t.getType() == TileType::Forest) hasForest = true;
    }
    CHECK(hasWater);
    CHECK(hasMountain);
    CHECK(hasForest);
}

TEST_CASE("MapGenerator::generate: одинаковый seed даёт одинаковую карту (детерминизм)")
{
    MapGenerationParams params;
    params.seed = 123;
    params.guaranteedLandCenters = {{7, 5}};

    Map a = MapGenerator::generate(params);
    Map b = MapGenerator::generate(params);

    CHECK(a.getMap().size() == b.getMap().size());
    bool allMatch = true;
    for (size_t i = 0; i < a.getMap().size() && i < b.getMap().size(); ++i)
    {
        if (a.getMap()[i].getType() != b.getMap()[i].getType())
            allMatch = false;
    }
    CHECK(allMatch);
}

TEST_CASE("MapGenerator::generate: seed=0 не падает и не зацикливается (случайный режим)")
{
    MapGenerationParams params;
    params.seed = 0;
    params.guaranteedLandCenters = {{7, 5}};

    CHECK_NOTHROW(MapGenerator::generate(params));
}

TEST_CASE("MapGenerator::generate: нулевые доли местности — карта остаётся полностью Grass")
{
    MapGenerationParams params;
    params.seed = 3;
    params.waterFraction = 0.0f;
    params.mountainFraction = 0.0f;
    params.forestFraction = 0.0f;
    params.desertFraction = 0.0f;
    params.guaranteedLandCenters = {{7, 5}};

    Map map = MapGenerator::generate(params);
    for (const auto &t : map.getMap())
        CHECK(t.getType() == TileType::Grass);
}

TEST_CASE("MapGenerator::generate: маленькая карта (меньше стартового радиуса) не падает")
{
    MapGenerationParams params;
    params.seed = 11;
    params.width = 3;
    params.height = 3;
    params.guaranteedLandCenters = {{1, 1}};
    params.guaranteedLandRadius = 2;

    CHECK_NOTHROW(MapGenerator::generate(params));
}

// ─── Раскладка 5 столиц (StartLayout) — то, что реально использует GameEngine ───

TEST_CASE("StartLayout::pentagonCapitals: все 5 точек умещаются в границах карты")
{
    for (const auto &c : StartLayout::pentagonCapitals())
    {
        CHECK(c.q >= 0);
        CHECK(c.q < StartLayout::MAP_WIDTH);
        CHECK(c.r >= 0);
        CHECK(c.r < StartLayout::MAP_HEIGHT);
    }
}

TEST_CASE("StartLayout::pentagonCapitals: соседние столицы на разумной дистанции (7..12 клеток)")
{
    const auto &pts = StartLayout::pentagonCapitals();
    for (size_t i = 0; i < pts.size(); ++i)
    {
        const auto &a = pts[i];
        const auto &b = pts[(i + 1) % pts.size()];
        int dist = a.distanceTo(b);
        CHECK(dist >= 7);
        CHECK(dist <= 12);
    }
}

TEST_CASE("MapGenerator::generate: карта под все 5 столиц пятиугольника связна, "
          "и ни одна стартовая зона не на воде")
{
    MapGenerationParams params;
    params.seed = 42;
    params.width = StartLayout::MAP_WIDTH;
    params.height = StartLayout::MAP_HEIGHT;
    params.guaranteedLandCenters = StartLayout::pentagonCapitals();
    params.guaranteedLandRadius = StartLayout::CAPITAL_LAND_RADIUS;

    Map map = MapGenerator::generate(params);
    CHECK(isFullyConnectedForTest(map));

    for (const auto &center : StartLayout::pentagonCapitals())
    {
        for (int q = 0; q < params.width; ++q)
        {
            for (int r = 0; r < params.height; ++r)
            {
                if (center.distanceTo({q, r}) > params.guaranteedLandRadius)
                    continue;
                const Tile *t = map.tileAt(q, r);
                CHECK(t != nullptr);
                if (t != nullptr)
                    CHECK(t->getType() != TileType::Water);
            }
        }
    }
}

TEST_CASE("MapGenerator::generate: раскладка 5 столиц связна на нескольких seed подряд")
{
    for (unsigned int seed = 1; seed <= 8; ++seed)
    {
        MapGenerationParams params;
        params.seed = seed;
        params.width = StartLayout::MAP_WIDTH;
        params.height = StartLayout::MAP_HEIGHT;
        params.guaranteedLandCenters = StartLayout::pentagonCapitals();
        params.guaranteedLandRadius = StartLayout::CAPITAL_LAND_RADIUS;

        Map map = MapGenerator::generate(params);
        CHECK(isFullyConnectedForTest(map));
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Блок 3.4: ресурсные клетки
// ═══════════════════════════════════════════════════════════════════════

namespace
{
    std::vector<ResourceDef> testResourceCatalog()
    {
        ResourceDef food;
        food.type = ResourceType::Food;
        food.sphere = "growth";
        food.bonus = 6;
        food.allowedTerrains = {TileType::Grass, TileType::Forest};

        ResourceDef production;
        production.type = ResourceType::Production;
        production.sphere = "industry";
        production.bonus = 8;
        production.allowedTerrains = {TileType::Mountain, TileType::Forest};

        return {food, production};
    }
}

TEST_CASE("MapGenerator::generate: без каталога ресурсов клетки остаются без ресурса")
{
    MapGenerationParams params;
    params.width = 20;
    params.height = 18;
    params.seed = 777;
    params.resourceDensity = 0.5f; // высокая плотность, но каталог пуст

    Map map = MapGenerator::generate(params);

    for (const auto &tile : map.getMap())
        CHECK(tile.getResource() == ResourceType::None);
}

TEST_CASE("MapGenerator::generate: с каталогом и высокой плотностью появляются ресурсы")
{
    MapGenerationParams params;
    params.width = 20;
    params.height = 18;
    params.seed = 777;
    params.resourceCatalog = testResourceCatalog();
    params.resourceDensity = 0.9f; // почти на каждой подходящей клетке

    Map map = MapGenerator::generate(params);

    int withResource = 0;
    for (const auto &tile : map.getMap())
        if (tile.getResource() != ResourceType::None)
            ++withResource;

    CHECK(withResource > 0);
}

TEST_CASE("MapGenerator::generate: ресурс появляется только на разрешённых типах местности")
{
    MapGenerationParams params;
    params.width = 20;
    params.height = 18;
    params.seed = 42;
    params.resourceCatalog = testResourceCatalog(); // food: grass/forest, production: mountain/forest
    params.resourceDensity = 0.9f;

    Map map = MapGenerator::generate(params);

    for (const auto &tile : map.getMap())
    {
        if (tile.getResource() == ResourceType::None)
            continue;

        if (tile.getResource() == ResourceType::Food)
        {
            CHECK(tile.getType() == TileType::Grass || tile.getType() == TileType::Forest);
        }
        else if (tile.getResource() == ResourceType::Production)
        {
            CHECK(tile.getType() == TileType::Mountain || tile.getType() == TileType::Forest);
        }
        CHECK(tile.getType() != TileType::Water); // на воде ресурсов не бывает
    }
}

TEST_CASE("MapGenerator::generate: стартовая зона капиталов не получает ресурсов")
{
    MapGenerationParams params;
    params.width = 20;
    params.height = 18;
    params.seed = 123;
    params.guaranteedLandCenters = {Coords(10, 9)};
    params.guaranteedLandRadius = 3;
    params.resourceCatalog = testResourceCatalog();
    params.resourceDensity = 1.0f; // максимум — если бы старт не защищался, он бы точно получил ресурс

    Map map = MapGenerator::generate(params);

    for (int dq = -3; dq <= 3; ++dq)
    {
        for (int dr = -3; dr <= 3; ++dr)
        {
            Coords c(10 + dq, 9 + dr);
            if (c.distanceTo(Coords(10, 9)) > 3)
                continue;
            const Tile *t = map.tileAt(c.q, c.r);
            if (!t)
                continue;
            CHECK(t->getResource() == ResourceType::None);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Блок 3.11: параметризуемое число игроков
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("StartLayout::capitalsForPlayerCount: 5 игроков — тот же пятиугольник, что и раньше")
{
    auto capitals = StartLayout::capitalsForPlayerCount(5);
    const auto &pentagon = StartLayout::pentagonCapitals();

    CHECK(capitals.size() == pentagon.size());
    for (size_t i = 0; i < capitals.size(); ++i)
    {
        CHECK(capitals[i].q == pentagon[i].q);
        CHECK(capitals[i].r == pentagon[i].r);
    }
}

TEST_CASE("StartLayout::capitalsForPlayerCount: возвращает ровно N уникальных точек для 2..4")
{
    for (int n = 2; n <= 4; ++n)
    {
        auto capitals = StartLayout::capitalsForPlayerCount(n, StartLayout::MAP_WIDTH,
                                                             StartLayout::MAP_HEIGHT);
        CHECK(static_cast<int>(capitals.size()) == n);

        for (size_t i = 0; i < capitals.size(); ++i)
            for (size_t j = i + 1; j < capitals.size(); ++j)
                CHECK(!(capitals[i].q == capitals[j].q && capitals[i].r == capitals[j].r));
    }
}

TEST_CASE("StartLayout::capitalsForPlayerCount: точки не выходят за границы карты "
          "с учётом отступа под стартовую зону")
{
    for (int n = 2; n <= 5; ++n)
    {
        auto capitals = StartLayout::capitalsForPlayerCount(n, StartLayout::MAP_WIDTH,
                                                             StartLayout::MAP_HEIGHT);
        for (const auto &c : capitals)
        {
            CHECK(c.q >= 0);
            CHECK(c.r >= 0);
            CHECK(c.q < StartLayout::MAP_WIDTH);
            CHECK(c.r < StartLayout::MAP_HEIGHT);
        }
    }
}

TEST_CASE("MapGenerator::generate: карта под 2/3/4 игроков связна, старты не на воде")
{
    for (int n = 2; n <= 4; ++n)
    {
        auto capitals = StartLayout::capitalsForPlayerCount(n, StartLayout::MAP_WIDTH,
                                                             StartLayout::MAP_HEIGHT);

        MapGenerationParams params;
        params.width = StartLayout::MAP_WIDTH;
        params.height = StartLayout::MAP_HEIGHT;
        params.seed = 4242 + n;
        params.guaranteedLandCenters = capitals;
        params.guaranteedLandRadius = StartLayout::CAPITAL_LAND_RADIUS;

        Map map = MapGenerator::generate(params);
        CHECK(isFullyConnectedForTest(map));

        for (const auto &c : capitals)
        {
            const Tile *t = map.tileAt(c.q, c.r);
            CHECK(t != nullptr);
            if (t)
                CHECK(t->getType() != TileType::Water);
        }
    }
}
