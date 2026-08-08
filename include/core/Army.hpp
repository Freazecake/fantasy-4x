#pragma once

#include <string>
#include <vector>

#include "Unit.hpp"
#include "Tile.hpp"

class Army
{
private:
    std::string m_id;
    int m_ownerId;
    Coords m_position;
    std::vector<Unit> m_units;
    int m_movementPointsRemaining = 0;

public:
    Army();
    Army(const std::string &id, int ownerId, Coords position, std::vector<Unit> units);
    ~Army() = default;

    const std::string &getId() const { return m_id; }
    int getOwnerId() const { return m_ownerId; }
    const Coords &position() const { return m_position; }
    const std::vector<Unit> &units() const { return m_units; }
    bool isDestroyed() const { return m_units.empty(); }
    bool isSettlerArmy() const
    {
        if (m_units.empty())
            return false;
        for (const auto &u : m_units)
            if (!u.isSettler())
                return false;
        return true;
    }

    int movementPointsMax() const;
    int movementPointsRemaining() const { return m_movementPointsRemaining; }
    void resetMovement();
    void moveTo(const Coords &dest, int cost);
    void setMovementPointsRemaining(int mp) { m_movementPointsRemaining = mp; }

    void applyCasualties(const std::vector<Unit> &survivors) { m_units = survivors; }
    void addUnits(const std::vector<Unit> &units)
    {
        m_units.insert(m_units.end(), units.begin(), units.end());
    }
};