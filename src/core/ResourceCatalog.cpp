#include "core/ResourceCatalog.hpp"

bool ResourceDef::allowsTerrain(TileType t) const
{
    for (const auto &terrain : allowedTerrains)
        if (terrain == t)
            return true;
    return false;
}

ResourceType ResourceCatalog::parseResourceType(const std::string &s)
{
    if (s == "food")
        return ResourceType::Food;
    if (s == "production")
        return ResourceType::Production;
    if (s == "gold")
        return ResourceType::Gold;
    if (s == "science")
        return ResourceType::Science;
    return ResourceType::None;
}

std::string ResourceCatalog::resourceTypeToString(ResourceType t)
{
    switch (t)
    {
    case ResourceType::Food:
        return "food";
    case ResourceType::Production:
        return "production";
    case ResourceType::Gold:
        return "gold";
    case ResourceType::Science:
        return "science";
    case ResourceType::None:
    default:
        return "none";
    }
}

TileType ResourceCatalog::parseTerrain(const std::string &s)
{
    if (s == "water")
        return TileType::Water;
    if (s == "mountain")
        return TileType::Mountain;
    if (s == "desert")
        return TileType::Desert;
    if (s == "forest")
        return TileType::Forest;
    return TileType::Grass;
}

std::vector<ResourceDef> ResourceCatalog::loadAllFromJson(const json &arr)
{
    std::vector<ResourceDef> result;
    for (const auto &obj : arr)
    {
        ResourceDef def;
        def.type = parseResourceType(obj.value("type", std::string("none")));
        def.sphere = obj.value("sphere", std::string("industry"));
        def.bonus = obj.value("bonus", 0.0f);

        for (const auto &terrain : obj["allowedTerrains"])
            def.allowedTerrains.push_back(parseTerrain(terrain.get<std::string>()));

        result.push_back(def);
    }
    return result;
}

const ResourceDef *ResourceCatalog::find(const std::vector<ResourceDef> &defs, ResourceType type)
{
    for (const auto &d : defs)
        if (d.type == type)
            return &d;
    return nullptr;
}
