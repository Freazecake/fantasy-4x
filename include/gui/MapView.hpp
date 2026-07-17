#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPointF>
#include <QPolygonF>
#include <vector>
#include <utility>

#include "core/Map.hpp"

class MapView : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapView(QWidget* parent = nullptr);

    void renderMap(const Map& map,
                   const std::vector<std::pair<int,int>>& settlementTiles = {});

signals:
    void tileClicked(int q, int r);

private:
    QGraphicsScene* m_scene;
    static constexpr double kHexSize = 20.0;

    QPointF axialToPixel(int q, int r) const;
    QPolygonF hexagonCorners(const QPointF& center) const;
    std::pair<int,int> pixelToAxial(const QPointF& scenePos) const;

    void mousePressEvent(QMouseEvent* event) override;
};
