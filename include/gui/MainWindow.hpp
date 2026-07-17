#pragma once

#include <QMainWindow>
#include "core/GameEngine.hpp"

class MapView;
class SettlementPanel;
class TechTreePanel;
class TurnPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onTileClicked(int q, int r);
    void onEndTurn();

private:
    void refreshAllPanels();

    GameEngine m_engine;

    MapView* m_mapView;
    SettlementPanel* m_settlementPanel;
    TechTreePanel* m_techTreePanel;
    TurnPanel* m_turnPanel;
};
