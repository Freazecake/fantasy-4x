#include "gui/MapView.hpp"
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPen>
#include <QPainter>
#include <QGraphicsPolygonItem>
#include <cmath>
#include <algorithm>
#include <set>

static const char* tileTypeName(TileType type) {
    switch (type) {
        case TileType::Grass:    return "Луг";
        case TileType::Water:    return "Вода";
        case TileType::Mountain: return "Горы";
        case TileType::Desert:   return "Пустыня";
        case TileType::Forest:   return "Лес";
    }
    return "Неизвестно";
}

static QColor tileColor(TileType type) {
    switch (type) {
        case TileType::Grass:    return QColor(74, 106, 58);
        case TileType::Water:    return QColor(42, 74, 96);
        case TileType::Mountain: return QColor(90, 78, 66);
        case TileType::Desert:   return QColor(150, 126, 74);
        case TileType::Forest:   return QColor(46, 66, 40);
    }
    return QColor(74, 106, 58);
}

MapView::MapView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
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
    m_hexItems.clear();
    m_tileTypes.clear();
    m_settlementTiles = std::set<std::pair<int,int>>(settlementTiles.begin(), settlementTiles.end());
    m_hoveredCoords = {-9999, -9999};

    const auto& tiles = map.getMap();
    if (tiles.empty()) return;

    double maxX = 0, maxY = 0;
    for (const auto& tile : tiles) {
        auto c = tile.getCoords();
        QPointF center = axialToPixel(c.q, c.r);
        auto* hex = m_scene->addPolygon(hexagonCorners(center));
        hex->setBrush(tileColor(tile.getType()));

        bool isSettlement = m_settlementTiles.count({c.q, c.r}) > 0;
        hex->setPen(isSettlement ? QPen(QColor(212, 175, 55), 2)
                                  : QPen(QColor(58, 50, 38), 1));

        m_hexItems[{c.q, c.r}] = hex;
        m_tileTypes[{c.q, c.r}] = tile.getType();
        maxX = std::max(maxX, center.x() + kHexSize);
        maxY = std::max(maxY, center.y() + kHexSize);
    }
    setSceneRect(0, 0, maxX + kHexSize, maxY + kHexSize);
    fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void MapView::mouseMoveEvent(QMouseEvent* event) {
    QPointF scenePos = mapToScene(event->pos());
    auto coords = pixelToAxial(scenePos);

    if (coords != m_hoveredCoords) {
        auto prevIt = m_hexItems.find(m_hoveredCoords);
        if (prevIt != m_hexItems.end()) {
            bool wasSettlement = m_settlementTiles.count(m_hoveredCoords) > 0;
            prevIt->second->setPen(wasSettlement ? QPen(QColor(212, 175, 55), 2)
                                                  : QPen(QColor(58, 50, 38), 1));
        }
        auto it = m_hexItems.find(coords);
        if (it != m_hexItems.end()) {
            it->second->setPen(QPen(QColor(232, 220, 192), 2));

            auto typeIt = m_tileTypes.find(coords);
            if (typeIt != m_tileTypes.end()) {
                emit tileHovered(QStringLiteral("%1 (%2, %3)")
                    .arg(tileTypeName(typeIt->second))
                    .arg(coords.first)
                    .arg(coords.second));
            }
        }
        m_hoveredCoords = coords;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void MapView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (!sceneRect().isEmpty()) {
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }
}

void MapView::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = mapToScene(event->pos());
    auto [q, r] = pixelToAxial(scenePos);
    emit tileClicked(q, r);
    QGraphicsView::mousePressEvent(event);
}
