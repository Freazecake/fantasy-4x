#include "gui/MapView.hpp"
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPen>
#include <QPainter>
#include <QFont>
#include <QGraphicsPolygonItem>
#include <cmath>
#include <algorithm>
#include <set>

static const char *tileTypeName(TileType type)
{
    switch (type)
    {
    case TileType::Grass:
        return "Луг";
    case TileType::Water:
        return "Вода";
    case TileType::Mountain:
        return "Горы";
    case TileType::Desert:
        return "Пустыня";
    case TileType::Forest:
        return "Лес";
    }
    return "Неизвестно";
}

static const char *resourceName(ResourceType type)
{
    switch (type)
    {
    case ResourceType::Food:
        return "Еда";
    case ResourceType::Production:
        return "Ресурсы";
    case ResourceType::Gold:
        return "Золото";
    case ResourceType::Science:
        return "Наука";
    case ResourceType::None:
    default:
        return nullptr;
    }
}

static QColor resourceColor(ResourceType type)
{
    switch (type)
    {
    case ResourceType::Food:
        return QColor(120, 200, 90);
    case ResourceType::Production:
        return QColor(200, 140, 70);
    case ResourceType::Gold:
        return QColor(230, 200, 60);
    case ResourceType::Science:
        return QColor(90, 170, 230);
    case ResourceType::None:
    default:
        return QColor();
    }
}

static QString resourceGlyph(ResourceType type)
{
    switch (type)
    {
    case ResourceType::Food:
        return QStringLiteral("F");
    case ResourceType::Production:
        return QStringLiteral("P");
    case ResourceType::Gold:
        return QStringLiteral("G");
    case ResourceType::Science:
        return QStringLiteral("S");
    case ResourceType::None:
    default:
        return QString();
    }
}

static QColor tileColor(TileType type)
{
    switch (type)
    {
    case TileType::Grass:
        return QColor(74, 106, 58);
    case TileType::Water:
        return QColor(42, 74, 96);
    case TileType::Mountain:
        return QColor(90, 78, 66);
    case TileType::Desert:
        return QColor(150, 126, 74);
    case TileType::Forest:
        return QColor(46, 66, 40);
    }
    return QColor(74, 106, 58);
}

MapView::MapView(QWidget *parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
}

QPointF MapView::axialToPixel(int q, int r) const
{
    double x = kHexSize * (std::sqrt(3.0) * q + std::sqrt(3.0) / 2.0 * r);
    double y = kHexSize * (1.5 * r);
    return QPointF(x, y);
}

QPolygonF MapView::hexagonCorners(const QPointF &center) const
{
    QPolygonF hex;
    for (int i = 0; i < 6; ++i)
    {
        double angleDeg = 60.0 * i - 30.0;
        double angleRad = M_PI / 180.0 * angleDeg;
        hex << QPointF(center.x() + kHexSize * std::cos(angleRad),
                       center.y() + kHexSize * std::sin(angleRad));
    }
    return hex;
}

std::pair<int, int> MapView::pixelToAxial(const QPointF &scenePos) const
{
    double q = (std::sqrt(3.0) / 3.0 * scenePos.x() - 1.0 / 3.0 * scenePos.y()) / kHexSize;
    double r = (2.0 / 3.0 * scenePos.y()) / kHexSize;

    double cx = q, cz = r, cy = -cx - cz;
    double rx = std::round(cx), ry = std::round(cy), rz = std::round(cz);
    double xDiff = std::abs(rx - cx), yDiff = std::abs(ry - cy), zDiff = std::abs(rz - cz);
    if (xDiff > yDiff && xDiff > zDiff)
        rx = -ry - rz;
    else if (yDiff > zDiff)
        ry = -rx - rz;
    else
        rz = -rx - ry;

    return {static_cast<int>(rx), static_cast<int>(rz)};
}

void MapView::renderMap(const Map &map,
                        const std::vector<SettlementMarker> &settlementTiles,
                        const std::vector<Army> &armies,
                        int viewerPlayerId,
                        const std::map<int, PlayerRelation> &relations,
                        const std::set<std::pair<int, int>> *exploredTiles,
                        const std::set<std::pair<int, int>> *visibleTiles)
{
    clearOverlayItems();
    m_scene->clear();
    m_hexItems.clear();
    m_tileTypes.clear();
    m_tileResources.clear();
    m_settlementOwners.clear();
    for (const auto &marker : settlementTiles)
        m_settlementOwners[{marker.q, marker.r}] = marker.ownerId;
    m_viewerPlayerId = viewerPlayerId;
    m_playerRelations = relations;
    m_hoveredCoords = {-9999, -9999};

    m_fogEnabled = exploredTiles != nullptr;
    m_exploredTiles = exploredTiles ? *exploredTiles : std::set<std::pair<int, int>>();
    m_visibleTiles = visibleTiles ? *visibleTiles : std::set<std::pair<int, int>>();

    m_armies = armies;

    const auto &tiles = map.getMap();
    if (tiles.empty())
        return;

    double maxX = 0, maxY = 0;
    for (const auto &tile : tiles)
    {
        auto c = tile.getCoords();
        bool explored = !m_fogEnabled || m_exploredTiles.count({c.q, c.r}) > 0;
        bool visible = !m_fogEnabled || m_visibleTiles.count({c.q, c.r}) > 0;

        QPointF center = axialToPixel(c.q, c.r);
        auto *hex = m_scene->addPolygon(hexagonCorners(center));

        if (!explored)
        {
            // Неразведанная клетка — сплошная тьма, без деталей рельефа.
            hex->setBrush(QColor(12, 12, 14));
            hex->setPen(QPen(QColor(30, 30, 32), 1));
        }
        else
        {
            QColor color = tileColor(tile.getType());
            if (!visible)
                color = color.darker(180); // разведано, но сейчас вне поля зрения
            hex->setBrush(color);
            hex->setPen(settlementPenFor(c.q, c.r));
        }

        m_hexItems[{c.q, c.r}] = hex;
        m_tileTypes[{c.q, c.r}] = tile.getType();
        m_tileResources[{c.q, c.r}] = tile.getResource();

        if (explored && tile.getResource() != ResourceType::None)
        {
            auto *glyph = m_scene->addSimpleText(resourceGlyph(tile.getResource()));
            glyph->setBrush(resourceColor(tile.getResource()));
            QFont f = glyph->font();
            f.setBold(true);
            f.setPointSize(9);
            glyph->setFont(f);
            QRectF gb = glyph->boundingRect();
            glyph->setPos(center.x() - gb.width() / 2.0, center.y() - gb.height() / 2.0);
            glyph->setZValue(1.0);
        }

        maxX = std::max(maxX, center.x() + kHexSize);
        maxY = std::max(maxY, center.y() + kHexSize);
    }
    setSceneRect(0, 0, maxX + kHexSize, maxY + kHexSize);
    fitInView(sceneRect(), Qt::KeepAspectRatio);

    renderArmyTokens();
}

PlayerRelation MapView::relationOf(int ownerId) const
{
    if (ownerId == m_viewerPlayerId)
        return PlayerRelation::Self;
    auto it = m_playerRelations.find(ownerId);
    return it != m_playerRelations.end() ? it->second : PlayerRelation::EnemyOrNeutral;
}

QPen MapView::settlementPenFor(int q, int r) const
{
    auto it = m_settlementOwners.find({q, r});
    if (it == m_settlementOwners.end())
        return QPen(QColor(58, 50, 38), 1); // обычная клетка, ничьё поселение

    switch (relationOf(it->second))
    {
    case PlayerRelation::Self:
        return QPen(QColor(212, 175, 55), 2); // золотая — моё
    case PlayerRelation::Ally:
        return QPen(QColor(70, 170, 90), 2); // зелёная — союзник
    case PlayerRelation::Vassal:
        return QPen(QColor(150, 90, 190), 2); // фиолетовая — вассал/сюзерен
    case PlayerRelation::EnemyOrNeutral:
        return QPen(QColor(200, 60, 60), 2); // красная — враг
    }
    return QPen(QColor(200, 60, 60), 2);
}

void MapView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    auto coords = pixelToAxial(scenePos);

    if (coords != m_hoveredCoords)
    {
        auto prevIt = m_hexItems.find(m_hoveredCoords);
        if (prevIt != m_hexItems.end())
        {
            prevIt->second->setPen(settlementPenFor(m_hoveredCoords.first, m_hoveredCoords.second));
        }
        auto it = m_hexItems.find(coords);
        if (it != m_hexItems.end())
        {
            it->second->setPen(QPen(QColor(232, 220, 192), 2));

            auto typeIt = m_tileTypes.find(coords);
            if (typeIt != m_tileTypes.end())
            {
                bool explored = !m_fogEnabled || m_exploredTiles.count(coords) > 0;
                if (!explored)
                {
                    emit tileHovered(QStringLiteral("Неразведано (%1, %2)")
                                         .arg(coords.first)
                                         .arg(coords.second));
                }
                else
                {
                    QString label = QStringLiteral("%1 (%2, %3)")
                                         .arg(tileTypeName(typeIt->second))
                                         .arg(coords.first)
                                         .arg(coords.second);

                    auto resIt = m_tileResources.find(coords);
                    if (resIt != m_tileResources.end() && resIt->second != ResourceType::None)
                    {
                        const char *rn = resourceName(resIt->second);
                        if (rn)
                            label += QStringLiteral(" — ресурс: %1").arg(QString::fromUtf8(rn));
                    }

                    emit tileHovered(label);
                }
            }
        }
        m_hoveredCoords = coords;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void MapView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (!sceneRect().isEmpty())
    {
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }
}

void MapView::clearOverlayItems()
{
    m_reachableHighlights.clear();
    m_armyTokenItems.clear();
    m_armyTokenLabels.clear();
}

void MapView::setSelectedArmy(const std::string &armyId)
{
    m_selectedArmyId = armyId;
}

void MapView::clearSelection()
{
    clearReachableTiles();
    m_selectedArmyId.clear();
}

void MapView::setReachableTiles(const std::set<std::pair<int, int>> &tiles)
{
    for (auto *item : m_reachableHighlights)
        m_scene->removeItem(item);
    m_reachableHighlights.clear();
    m_reachableTiles = tiles;

    for (const auto &[q, r] : tiles)
    {
        QPointF center = axialToPixel(q, r);
        auto *overlay = m_scene->addEllipse(
            center.x() - kHexSize * 0.5, center.y() - kHexSize * 0.5,
            kHexSize, kHexSize,
            QPen(QColor(255, 255, 100, 200), 2),
            QBrush(QColor(255, 255, 100, 30)));
        m_reachableHighlights.push_back(overlay);
    }
}

void MapView::clearReachableTiles()
{
    for (auto *item : m_reachableHighlights)
        m_scene->removeItem(item);
    m_reachableHighlights.clear();
    m_reachableTiles.clear();
}

void MapView::renderArmyTokens()
{
    for (auto *item : m_armyTokenItems)
        m_scene->removeItem(item);
    m_armyTokenItems.clear();
    for (auto *label : m_armyTokenLabels)
        m_scene->removeItem(label);
    m_armyTokenLabels.clear();

    for (const auto &army : m_armies)
    {
        auto pos = army.position();
        QPointF center = axialToPixel(pos.q, pos.r);
        QColor fill;
        switch (relationOf(army.getOwnerId()))
        {
        case PlayerRelation::Self:
            fill = QColor(60, 120, 210);
            break; // синяя — моя
        case PlayerRelation::Ally:
            fill = QColor(70, 170, 90);
            break; // зелёная — союзник
        case PlayerRelation::Vassal:
            fill = QColor(150, 90, 190);
            break; // фиолетовая — вассал/сюзерен
        case PlayerRelation::EnemyOrNeutral:
            fill = QColor(200, 60, 60);
            break; // красная — враг
        }
        QPen border = (army.getId() == m_selectedArmyId)
                          ? QPen(QColor(255, 255, 0), 3)
                          : QPen(QColor(20, 20, 20), 2);

        auto *ellipse = m_scene->addEllipse(
            center.x() - 8, center.y() - 8, 16, 16,
            border, QBrush(fill));
        m_armyTokenItems.push_back(ellipse);

        auto *text = m_scene->addSimpleText(
            QString::number(static_cast<int>(army.units().size())),
            QFont("sans-serif", 8, QFont::Bold));
        text->setPos(center.x() - 4, center.y() - 6);
        text->setBrush(Qt::white);
        m_armyTokenLabels.push_back(text);
    }
}

void MapView::mousePressEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    auto [q, r] = pixelToAxial(scenePos);

    if (event->button() == Qt::RightButton)
    {
        clearSelection();
        emit selectionCleared();
        return;
    }

    for (const auto &army : m_armies)
    {
        if (army.getOwnerId() == m_viewerPlayerId &&
            army.position().q == q && army.position().r == r)
        {
            m_selectedArmyId = army.getId();
            emit armySelected(army.getId());
            renderArmyTokens();
            return;
        }
    }

    if (!m_selectedArmyId.empty() && m_reachableTiles.count({q, r}))
    {
        std::string aid = m_selectedArmyId;
        m_selectedArmyId.clear();
        m_reachableTiles.clear();
        emit armyMoveRequested(aid, q, r);
        return;
    }

    if (!m_selectedArmyId.empty())
    {
        clearSelection();
        emit selectionCleared();
    }

    emit tileClicked(q, r);
    QGraphicsView::mousePressEvent(event);
}
