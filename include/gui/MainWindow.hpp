#pragma once

#include <QMainWindow>
#include "core/GameEngine.hpp"

class QStackedWidget;
class MapView;
class SettlementPanel;
class TechTreePanel;
class TurnPanel;
class StartMenuWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onTileClicked(int q, int r);
    void onTileHovered(const QString& info);
    void onEndTurn();
    void onNewGame();

private:
    QWidget* buildGameView();
    void refreshAllPanels();

    GameEngine m_engine;

    QStackedWidget* m_stack;
    StartMenuWidget* m_startMenu;

    MapView* m_mapView;
    SettlementPanel* m_settlementPanel;
    TechTreePanel* m_techTreePanel;
    TurnPanel* m_turnPanel;
};
