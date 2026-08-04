#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPolygonItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QPointF>
#include <QPolygonF>
#include <QPen>
#include <vector>
#include <map>
#include <set>
#include <utility>

#include "core/Map.hpp"
#include "core/Tile.hpp"
#include "core/Army.hpp"

struct SettlementMarker
{
    int q;
    int r;
    int ownerId;
};

enum class PlayerRelation
{
    Self,
    Ally,
    Vassal,
    EnemyOrNeutral
};

class MapView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MapView(QWidget *parent = nullptr);

    void renderMap(const Map &map,
                   const std::vector<SettlementMarker> &settlementTiles = {},
                   const std::vector<Army> &armies = {},
                   int viewerPlayerId = 0,
                   const std::map<int, PlayerRelation> &relations = {},
                   const std::set<std::pair<int, int>> *exploredTiles = nullptr,
                   const std::set<std::pair<int, int>> *visibleTiles = nullptr);

    void setReachableTiles(const std::set<std::pair<int, int>> &tiles);
    void clearReachableTiles();
    void setSelectedArmy(const std::string &armyId);
    void clearSelection();

signals:
    void tileClicked(int q, int r);
    void tileHovered(const QString &info);
    void armySelected(const std::string &armyId);
    void armyMoveRequested(const std::string &armyId, int q, int r);
    void selectionCleared();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QGraphicsScene *m_scene;
    static constexpr double kHexSize = 20.0;

    std::map<std::pair<int, int>, QGraphicsPolygonItem *> m_hexItems;
    std::map<std::pair<int, int>, TileType> m_tileTypes;
    std::map<std::pair<int, int>, ResourceType> m_tileResources;
    bool m_fogEnabled = false;
    std::set<std::pair<int, int>> m_exploredTiles;
    std::set<std::pair<int, int>> m_visibleTiles;
    std::map<std::pair<int, int>, int> m_settlementOwners;
    int m_viewerPlayerId = 0;
    std::map<int, PlayerRelation> m_playerRelations;
    std::pair<int, int> m_hoveredCoords{-9999, -9999};

    std::vector<Army> m_armies;
    std::set<std::pair<int, int>> m_reachableTiles;
    std::string m_selectedArmyId;
    std::vector<QGraphicsEllipseItem *> m_armyTokenItems;
    std::vector<QGraphicsSimpleTextItem *> m_armyTokenLabels;
    std::vector<QGraphicsEllipseItem *> m_reachableHighlights;

    void renderArmyTokens();
    void clearOverlayItems();
    QPen settlementPenFor(int q, int r) const;
    PlayerRelation relationOf(int ownerId) const;

    QPointF axialToPixel(int q, int r) const;
    QPolygonF hexagonCorners(const QPointF &center) const;
    std::pair<int, int> pixelToAxial(const QPointF &scenePos) const;
};
