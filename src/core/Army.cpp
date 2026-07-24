#include "core/Army.hpp"

Army::Army() {}

Army::Army(const std::string &id, int ownerId, Coords position, std::vector<Unit> units)
{
    m_id = id;
    m_ownerId = ownerId;
    m_position = position;
    m_units = units;
    m_movementPointsRemaining = movementPointsMax();
}

int Army::movementPointsMax() const
{
    if (m_units.empty())
        return 0;
    int minPoints = m_units[0].getMovementPoints();
    for (const auto &unit : m_units)
    {
        if (unit.getMovementPoints() < minPoints)
            minPoints = unit.getMovementPoints();
    }
    return minPoints;
}

void Army::resetMovement()
{
    m_movementPointsRemaining = movementPointsMax();
}

void Army::moveTo(const Coords &dest, int cost)
{
    m_position = dest;
    m_movementPointsRemaining -= cost;
}