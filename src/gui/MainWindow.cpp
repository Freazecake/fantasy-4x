#include "gui/MainWindow.hpp"
#include "gui/MapView.hpp"
#include "gui/SettlementPanel.hpp"
#include "gui/TechTreePanel.hpp"
#include "gui/TurnPanel.hpp"
#include "gui/StartMenuWidget.hpp"

#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QApplication>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1024, 700);
    setWindowTitle(QStringLiteral("4X Стратегия"));

    auto *menuBar = this->menuBar();
    auto *gameMenu = menuBar->addMenu(QStringLiteral("Игра"));
    auto *endTurnAction = gameMenu->addAction(QStringLiteral("Завершить ход"));
    connect(endTurnAction, &QAction::triggered, this, &MainWindow::onEndTurn);

    m_startMenu = new StartMenuWidget(this);
    connect(m_startMenu, &StartMenuWidget::newGameRequested, this, &MainWindow::onNewGame);
    connect(m_startMenu, &StartMenuWidget::exitRequested, qApp, &QApplication::quit);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_startMenu);
    m_stack->addWidget(buildGameView());
    setCentralWidget(m_stack);
    m_stack->setCurrentIndex(0);

    statusBar()->showMessage(QStringLiteral("4X Стратегия v0.1.0"));
}

QWidget *MainWindow::buildGameView()
{
    m_mapView = new MapView(this);
    m_settlementPanel = new SettlementPanel(this);
    m_techTreePanel = new TechTreePanel(this);
    m_turnPanel = new TurnPanel(this);

    auto *centralSplitter = new QSplitter(Qt::Horizontal);
    centralSplitter->addWidget(m_mapView);

    auto *rightPanel = new QWidget(centralSplitter);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    auto *toggleRow = new QWidget(rightPanel);
    auto *toggleLayout = new QHBoxLayout(toggleRow);
    toggleLayout->setContentsMargins(0, 0, 0, 0);
    auto *settlementToggle = new QPushButton(QStringLiteral("Поселение"), toggleRow);
    auto *techToggle = new QPushButton(QStringLiteral("Технологии"), toggleRow);
    settlementToggle->setCheckable(true);
    techToggle->setCheckable(true);
    settlementToggle->setChecked(true);
    toggleLayout->addWidget(settlementToggle);
    toggleLayout->addWidget(techToggle);
    rightLayout->addWidget(toggleRow);

    auto *legendRow = new QWidget(rightPanel);
    auto *legendLayout = new QHBoxLayout(legendRow);
    legendLayout->setContentsMargins(0, 0, 0, 0);
    auto addSwatch = [&](const QString &colorHex, const QString &text)
    {
        auto *swatch = new QLabel(legendRow);
        swatch->setFixedSize(10, 10);
        swatch->setStyleSheet(QStringLiteral("background-color:%1; border:1px solid #222;").arg(colorHex));
        legendLayout->addWidget(swatch);
        legendLayout->addWidget(new QLabel(text, legendRow));
    };
    addSwatch(QStringLiteral("#3C78D2"), QStringLiteral("Вы"));
    addSwatch(QStringLiteral("#46AA5A"), QStringLiteral("Союзник"));
    addSwatch(QStringLiteral("#965ABE"), QStringLiteral("Вассал"));
    addSwatch(QStringLiteral("#C83C3C"), QStringLiteral("Враг"));
    legendLayout->addStretch();
    rightLayout->addWidget(legendRow);

    auto *panelStack = new QStackedWidget(rightPanel);

    auto *settlementScroll = new QScrollArea(rightPanel);
    settlementScroll->setWidgetResizable(true);
    settlementScroll->setWidget(m_settlementPanel);
    panelStack->addWidget(settlementScroll);

    auto *techScroll = new QScrollArea(rightPanel);
    techScroll->setWidgetResizable(true);
    techScroll->setWidget(m_techTreePanel);
    panelStack->addWidget(techScroll);

    rightLayout->addWidget(panelStack, 1);
    rightLayout->addWidget(m_turnPanel);

    connect(settlementToggle, &QPushButton::clicked, this, [=]()
            {
        panelStack->setCurrentIndex(0);
        settlementToggle->setChecked(true);
        techToggle->setChecked(false); });
    connect(techToggle, &QPushButton::clicked, this, [=]()
            {
        panelStack->setCurrentIndex(1);
        techToggle->setChecked(true);
        settlementToggle->setChecked(false); });

    centralSplitter->addWidget(rightPanel);
    centralSplitter->setStretchFactor(0, 1);
    centralSplitter->setStretchFactor(1, 0);
    centralSplitter->setSizes({700, 300});

    connect(m_turnPanel, &TurnPanel::endTurnClicked, this, &MainWindow::onEndTurn);
    connect(m_turnPanel, &TurnPanel::armySelected, this, &MainWindow::onArmySelected);
    connect(m_turnPanel, &TurnPanel::settlementSelected, this, &MainWindow::onSettlementListSelected);
    connect(m_turnPanel, &TurnPanel::foundSettlementRequested, this, &MainWindow::onFoundSettlementRequested);
    connect(m_mapView, &MapView::tileClicked, this, &MainWindow::onTileClicked);
    connect(m_mapView, &MapView::tileHovered, this, &MainWindow::onTileHovered);
    connect(m_mapView, &MapView::armySelected, this, &MainWindow::onArmySelected);
    connect(m_mapView, &MapView::armyMoveRequested, this, &MainWindow::onArmyMoveRequested);
    connect(m_mapView, &MapView::selectionCleared, this, &MainWindow::onSelectionCleared);

    connect(m_techTreePanel, &TechTreePanel::researchRequested, this,
            [this](const std::string &techId)
            {
                m_engine.state().players()[0].setCurrentResearch(techId);
                refreshAllPanels();
                statusBar()->showMessage(
                    QStringLiteral("Начато исследование: %1").arg(QString::fromStdString(techId)));
            });

    connect(m_settlementPanel, &SettlementPanel::allocationChanged, this, [this]()
            { refreshAllPanels(); });
    connect(m_settlementPanel, &SettlementPanel::buildingQueued, this, [this](const std::string &)
            {
        Player& human = m_engine.state().players()[0];
        Settlement* settlement = m_settlementPanel->currentSettlement();
        if (settlement)
            human.spendIndustryOnQueues(*settlement);
        refreshAllPanels(); });
    connect(m_settlementPanel, &SettlementPanel::unitRecruitmentQueued, this, [this](const std::string &)
            {
        Player& human = m_engine.state().players()[0];
        Settlement* settlement = m_settlementPanel->currentSettlement();
        if (settlement)
            human.spendIndustryOnQueues(*settlement);
        refreshAllPanels(); });
    connect(m_settlementPanel, &SettlementPanel::actionFailed, this, [this](const QString &msg)
            { statusBar()->showMessage(msg, 5000); });
    connect(m_settlementPanel, &SettlementPanel::marchRequested, this, &MainWindow::onMarchRequested);

    connect(m_settlementPanel, &SettlementPanel::tradeExchangeRequested, this,
            [this](const std::string &targetResource, int amount)
            {
                Player &human = m_engine.state().players()[0];
                if (human.exchangeTrade(targetResource, amount))
                {
                    statusBar()->showMessage(
                        QStringLiteral("Обмен совершён: %1 ед. торговли → %2").arg(amount).arg(QString::fromStdString(targetResource)), 3000);
                }
                else
                {
                    statusBar()->showMessage(QStringLiteral("Недостаточно торговли"), 3000);
                }
                refreshAllPanels();
            });

    connect(m_settlementPanel, &SettlementPanel::politicalActionRequested, this,
            [this](const std::string &actionId, int targetId)
            {
                std::string msg = m_engine.applyPoliticalAction(0, actionId, targetId);
                statusBar()->showMessage(QString::fromStdString(msg), 5000);

                auto v = m_engine.checkVictoryCondition();
                if (v.hasWinner())
                    showVictoryDialog(v);

                refreshAllPanels();
            });

    return centralSplitter;
}

Settlement *MainWindow::selectedHumanSettlement(Player &human)
{
    if (human.settlements().empty())
    {
        m_hasSelectedSettlement = false;
        return nullptr;
    }

    if (m_hasSelectedSettlement)
    {
        for (auto &s : human.settlements())
        {
            Coords a = s.anchorCoords();
            if (a.q == m_selectedSettlementAnchor.q && a.r == m_selectedSettlementAnchor.r)
                return &s;
        }
    }

    Settlement &fallback = human.settlements().front();
    m_selectedSettlementAnchor = fallback.anchorCoords();
    m_hasSelectedSettlement = true;
    return &fallback;
}

void MainWindow::selectSettlement(const Settlement &settlement)
{
    m_selectedSettlementAnchor = settlement.anchorCoords();
    m_hasSelectedSettlement = true;
}

void MainWindow::onNewGame()
{
    m_engine.loadFromAssets(ASSETS_DIR);
    m_hasSelectedSettlement = false;
    refreshAllPanels();
    m_stack->setCurrentIndex(1);
    statusBar()->showMessage(QStringLiteral("Партия начата"), 3000);
}

MainWindow::~MainWindow() {}

void MainWindow::refreshAllPanels()
{
    Player &human = m_engine.state().players()[0];

    std::vector<SettlementMarker> settlementTiles;
    std::vector<Army> allPlayersArmies;
    std::map<int, PlayerRelation> relations;
    const auto &vassalizations = m_engine.state().vassalizations();
    for (const auto &p : m_engine.state().players())
    {
        for (const auto &s : p.settlements())
        {
            for (const auto &t : s.tiles())
            {
                auto c = t.getCoords();
                settlementTiles.push_back({c.q, c.r, p.id()});
            }
        }
        for (const auto &a : p.armies())
            allPlayersArmies.push_back(a);

        if (p.id() == human.id())
        {
            relations[p.id()] = PlayerRelation::Self;
            continue;
        }
        bool allied = GameState::isAllied(m_engine.state(), human.id(), p.id());
        auto vAsVassal = vassalizations.find(p.id());
        auto vHumanVassal = vassalizations.find(human.id());
        bool vassalRelation =
            (vAsVassal != vassalizations.end() && vAsVassal->second == human.id()) ||
            (vHumanVassal != vassalizations.end() && vHumanVassal->second == p.id());

        if (allied)
            relations[p.id()] = PlayerRelation::Ally;
        else if (vassalRelation)
            relations[p.id()] = PlayerRelation::Vassal;
        else
            relations[p.id()] = PlayerRelation::EnemyOrNeutral;
    }

    m_mapView->clearReachableTiles();
    m_mapView->renderMap(m_engine.state().map(), settlementTiles, allPlayersArmies, human.id(), relations);
    m_turnPanel->updateArmyList(human.armies());

    Settlement *selected = selectedHumanSettlement(human);
    if (selected)
    {
        m_settlementPanel->showSettlement(
            *selected, m_engine.buildingCatalog(), m_engine.unitCatalog());
    }
    m_turnPanel->updateSettlementList(human.settlements(), m_selectedSettlementAnchor);

    m_techTreePanel->showTechs(
        human.technologies(),
        human.researchedTechIds(),
        human.currentResearchId());

    const auto &res = human.resources();
    std::string phaseName;
    switch (m_engine.state().CurPhase())
    {
    case GlobalPhase::NotStarted:
        phaseName = "Начало";
        break;
    case GlobalPhase::Phase2_OpenMap:
        phaseName = "Открытая карта";
        break;
    case GlobalPhase::Phase3_Final:
        phaseName = "Финал";
        break;
    }

    m_turnPanel->updateInfo(
        m_engine.state().turnNumber(), phaseName,
        res.science, res.industry, res.trade, res.diplomacy);

    m_settlementPanel->updateTradeDiplomacy(
        human.resources(),
        human.effectiveTradeMultiplier(),
        m_engine.politicalActionCatalog());

    std::vector<int> opponentIds;
    for (const auto &p : m_engine.state().players())
        if (p.id() != 0)
            opponentIds.push_back(p.id());
    m_settlementPanel->setOpponents(opponentIds);
}

void MainWindow::onTileClicked(int q, int r)
{
    if (m_stack->currentIndex() != 1)
        return;

    statusBar()->showMessage(
        QStringLiteral("Клетка: %1, %2").arg(q).arg(r));

    Player &human = m_engine.state().players()[0];
    for (auto &settlement : human.settlements())
    {
        for (const auto &tile : settlement.tiles())
        {
            auto c = tile.getCoords();
            if (c.q == q && c.r == r)
            {
                selectSettlement(settlement);
                m_settlementPanel->showSettlement(settlement, m_engine.buildingCatalog(), m_engine.unitCatalog());
                m_turnPanel->updateSettlementList(human.settlements(), m_selectedSettlementAnchor);
                statusBar()->showMessage(QStringLiteral("Поселение"), 5000);
                return;
            }
        }
    }
}

void MainWindow::onTileHovered(const QString &info)
{
    if (m_stack->currentIndex() != 1)
        return;
    statusBar()->showMessage(info);
}

void MainWindow::onArmySelected(const std::string &armyId)
{
    Army *army = m_engine.state().findArmy(armyId);
    if (!army)
        return;

    m_selectedArmyId = armyId;
    m_mapView->setSelectedArmy(armyId);
    m_turnPanel->setFoundSettlementAvailable(army->isSettlerArmy());

    auto reachable = m_engine.state().map().reachableTiles(
        army->position(), army->movementPointsRemaining());
    std::set<std::pair<int, int>> tileSet;
    for (const auto &c : reachable)
        tileSet.insert({c.q, c.r});
    m_mapView->setReachableTiles(tileSet);

    statusBar()->showMessage(
        QStringLiteral("Армия выбрана: %1 юн., %2/%3 ОД")
            .arg(static_cast<int>(army->units().size()))
            .arg(army->movementPointsRemaining())
            .arg(army->movementPointsMax()),
        5000);
}

void MainWindow::onFoundSettlementRequested()
{
    if (m_selectedArmyId.empty())
        return;

    auto result = m_engine.state().foundSettlementFromArmy(m_selectedArmyId);
    statusBar()->showMessage(QString::fromStdString(
                                 result.success ? std::string("Город основан!") : result.errorMessage),
                             5000);

    if (result.success)
    {
        m_selectedArmyId.clear();
        m_hasSelectedSettlement = false; // выбираем новый город в refreshAllPanels
        m_mapView->clearSelection();
        m_turnPanel->setFoundSettlementAvailable(false);
    }

    refreshAllPanels();
}

void MainWindow::onArmyMoveRequested(const std::string &armyId, int q, int r)
{
    auto result = m_engine.state().moveArmy(armyId, {q, r});
    if (!result.success)
    {
        statusBar()->showMessage(QString::fromStdString(result.errorMessage), 5000);
        refreshAllPanels();
        return;
    }

    if (result.combatOccurred)
    {
        QString combatMsg;
        if (result.settlementCaptured)
            combatMsg = QStringLiteral("Поселение захвачено! ");
        combatMsg += result.combat.attackerWon
                         ? QStringLiteral("Атака победила")
                         : QStringLiteral("Защита победила");
        statusBar()->showMessage(combatMsg, 8000);
    }

    refreshAllPanels();
    m_mapView->clearSelection();
    m_selectedArmyId.clear();
    m_turnPanel->setFoundSettlementAvailable(false);
}

void MainWindow::onSettlementListSelected(int q, int r)
{
    Player &human = m_engine.state().players()[0];
    for (auto &settlement : human.settlements())
    {
        if (settlement.anchorCoords().q == q && settlement.anchorCoords().r == r)
        {
            selectSettlement(settlement);
            m_settlementPanel->showSettlement(settlement, m_engine.buildingCatalog(), m_engine.unitCatalog());
            m_turnPanel->updateSettlementList(human.settlements(), m_selectedSettlementAnchor);
            return;
        }
    }
}

void MainWindow::onSelectionCleared()
{
    m_mapView->clearSelection();
    m_selectedArmyId.clear();
    m_turnPanel->setFoundSettlementAvailable(false);
    statusBar()->clearMessage();
}

void MainWindow::onMarchRequested()
{
    Settlement *settlement = m_settlementPanel->currentSettlement();
    if (!settlement)
        return;

    Player &human = m_engine.state().players()[0];

    std::vector<std::string> allUnitIds;
    for (const auto &u : settlement->garrison())
        allUnitIds.push_back(u.getId());

    if (allUnitIds.empty())
    {
        statusBar()->showMessage(QStringLiteral("Нет войск в гарнизоне"), 5000);
        return;
    }

    human.detachArmy(*settlement, allUnitIds);
    refreshAllPanels();
    statusBar()->showMessage(QStringLiteral("Армия выступила в поход"), 5000);
}

void MainWindow::onEndTurn()
{
    if (m_stack->currentIndex() != 1)
        return;

    m_engine.endTurn();

    auto v = m_engine.checkVictoryCondition();
    if (v.hasWinner())
    {
        showVictoryDialog(v);
        return;
    }

    refreshAllPanels();
    statusBar()->showMessage(
        QStringLiteral("Ход %1 завершён").arg(m_engine.state().turnNumber()), 3000);
}

void MainWindow::showVictoryDialog(const VictoryResult &v)
{
    QString typeStr = v.type == "political"
                          ? QStringLiteral("Политическая победа")
                          : QStringLiteral("Военная победа");
    QMessageBox::information(this, QStringLiteral("Победа!"),
                             QStringLiteral("Игрок %1 победил! (%2)").arg(v.winnerId).arg(typeStr));
    m_stack->setCurrentIndex(0);
}
