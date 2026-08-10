#pragma once

#include <QMainWindow>
#include "core/GameEngine.hpp"

class QStackedWidget;
class MapView;
class SettlementPanel;
class TechTreePanel;
class TurnPanel;
class MilitaryPanel;
class StartMenuWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTileClicked(int q, int r);
    void onTileHovered(const QString &info);
    void onEndTurn();
    void onNewGame();
    void onSaveGame();
    void onLoadGame();
    void onShowRules();
    void onArmySelected(const std::string &armyId);
    void onArmyMoveRequested(const std::string &armyId, int q, int r);
    void onSelectionCleared();
    void onMarchRequested(const std::vector<std::string>& unitIds);
    void onSettlementListSelected(int q, int r);
    void onFoundSettlementRequested();

private:
    QWidget *buildGameView();
    void refreshAllPanels();
    void showVictoryDialog(const VictoryResult &v);
    Settlement *selectedHumanSettlement(Player &human);
    void selectSettlement(const Settlement &settlement);

    GameEngine m_engine;

    QStackedWidget *m_stack;
    StartMenuWidget *m_startMenu;

    MapView *m_mapView;
    SettlementPanel *m_settlementPanel;
    TechTreePanel *m_techTreePanel;
    TurnPanel *m_turnPanel;
    MilitaryPanel *m_militaryPanel;

    Coords m_selectedSettlementAnchor{-1, -1};
    bool m_hasSelectedSettlement = false;
    std::string m_selectedArmyId;
};
