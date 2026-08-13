#pragma once

#include <string>
#include <vector>

#include "Tile.hpp"
#include "external/json.hpp"

using json = nlohmann::json;

struct ResourceDef
{
    ResourceType type = ResourceType::None;
    std::string sphere;
    float bonus = 0.0f;
    std::vector<TileType> allowedTerrains;

    bool allowsTerrain(TileType t) const;
};

class ResourceCatalog
{
public:
    static std::vector<ResourceDef> loadAllFromJson(const json &arr);
    static const ResourceDef *find(const std::vector<ResourceDef> &defs, ResourceType type);

    static ResourceType parseResourceType(const std::string &s);
    static std::string resourceTypeToString(ResourceType t);

private:
    static TileType parseTerrain(const std::string &s);
};
