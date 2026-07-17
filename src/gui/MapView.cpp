#include "gui/MapView.hpp"
#include <QMouseEvent>
#include <QPen>
#include <QGraphicsPolygonItem>
#include <cmath>
#include <algorithm>
#include <set>

MapView::MapView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
}

static QColor tileColor(TileType type) {
    switch (type) {
        case TileType::Grass:    return QColor(100, 160, 80);
        case TileType::Water:    return QColor(60, 100, 180);
        case TileType::Mountain: return QColor(140, 120, 100);
        case TileType::Desert:   return QColor(200, 180, 100);
        case TileType::Forest:   return QColor(40, 110, 40);
    }
    return QColor(100, 150, 100);
}

QPointF MapView::axialToPixel(int q, int r) const {
    double x = kHexSize * (std::sqrt(3.0) * q + std::sqrt(3.0) / 2.0 * r);
    double y = kHexSize * (1.5 * r);
    return QPointF(x, y);
}

QPolygonF MapView::hexagonCorners(const QPointF& center) const {
    QPolygonF hex;
    for (int i = 0; i < 6; ++i) {
        double angleDeg = 60.0 * i - 30.0;
        double angleRad = M_PI / 180.0 * angleDeg;
        hex << QPointF(center.x() + kHexSize * std::cos(angleRad),
                       center.y() + kHexSize * std::sin(angleRad));
    }
    return hex;
}

std::pair<int,int> MapView::pixelToAxial(const QPointF& scenePos) const {
    double q = (std::sqrt(3.0) / 3.0 * scenePos.x() - 1.0 / 3.0 * scenePos.y()) / kHexSize;
    double r = (2.0 / 3.0 * scenePos.y()) / kHexSize;

    double cx = q, cz = r, cy = -cx - cz;
    double rx = std::round(cx), ry = std::round(cy), rz = std::round(cz);
    double xDiff = std::abs(rx - cx), yDiff = std::abs(ry - cy), zDiff = std::abs(rz - cz);
    if (xDiff > yDiff && xDiff > zDiff) rx = -ry - rz;
    else if (yDiff > zDiff)             ry = -rx - rz;
    else                                 rz = -rx - ry;

    return { static_cast<int>(rx), static_cast<int>(rz) };
}

void MapView::renderMap(const Map& map,
                        const std::vector<std::pair<int,int>>& settlementTiles) {
    m_scene->clear();
    const auto& tiles = map.getMap();
    if (tiles.empty()) return;

    std::set<std::pair<int,int>> settlementSet(settlementTiles.begin(),
                                                settlementTiles.end());

    double maxX = 0, maxY = 0;

    for (const auto& tile : tiles) {
        auto c = tile.getCoords();
        QPointF center = axialToPixel(c.q, c.r);
        auto* hex = m_scene->addPolygon(hexagonCorners(center));
        hex->setBrush(tileColor(tile.getType()));

        bool isSettlement = settlementSet.count({c.q, c.r}) > 0;
        hex->setPen(isSettlement ? QPen(QColor(220, 180, 50), 2)
                                 : QPen(QColor(30, 30, 30), 1));

        maxX = std::max(maxX, center.x() + kHexSize);
        maxY = std::max(maxY, center.y() + kHexSize);
    }
    setSceneRect(0, 0, maxX + kHexSize, maxY + kHexSize);
}

void MapView::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = mapToScene(event->pos());
    auto [q, r] = pixelToAxial(scenePos);
    emit tileClicked(q, r);
    QGraphicsView::mousePressEvent(event);
}
