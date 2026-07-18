#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPolygonItem>
#include <QPointF>
#include <QPolygonF>
#include <vector>
#include <map>
#include <set>
#include <utility>

#include "core/Map.hpp"
#include "core/Tile.hpp"

class MapView : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapView(QWidget* parent = nullptr);

    void renderMap(const Map& map,
                   const std::vector<std::pair<int,int>>& settlementTiles = {});

signals:
    void tileClicked(int q, int r);
    void tileHovered(const QString& info);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QGraphicsScene* m_scene;
    static constexpr double kHexSize = 20.0;

    std::map<std::pair<int,int>, QGraphicsPolygonItem*> m_hexItems;
    std::map<std::pair<int,int>, TileType> m_tileTypes;
    std::set<std::pair<int,int>> m_settlementTiles;
    std::pair<int,int> m_hoveredCoords{-9999, -9999};

    QPointF axialToPixel(int q, int r) const;
    QPolygonF hexagonCorners(const QPointF& center) const;
    std::pair<int,int> pixelToAxial(const QPointF& scenePos) const;
};
