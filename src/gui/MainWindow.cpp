#include "gui/MainWindow.hpp"
#include "gui/MapView.hpp"
#include "gui/SettlementPanel.hpp"
#include "gui/TechTreePanel.hpp"
#include "gui/TurnPanel.hpp"

#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    resize(1024, 700);
    setWindowTitle(QStringLiteral("4X Стратегия"));

    auto* menuBar = this->menuBar();
    auto* gameMenu = menuBar->addMenu(QStringLiteral("Игра"));
    gameMenu->addAction(QStringLiteral("Завершить ход"));

    m_mapView = new MapView(this);
    m_settlementPanel = new SettlementPanel(this);
    m_techTreePanel = new TechTreePanel(this);
    m_turnPanel = new TurnPanel(this);

    auto* centralSplitter = new QSplitter(Qt::Horizontal, this);
    centralSplitter->addWidget(m_mapView);

    auto* rightPanel = new QWidget(centralSplitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    auto* topRightSplitter = new QSplitter(Qt::Vertical, rightPanel);
    topRightSplitter->addWidget(m_settlementPanel);
    topRightSplitter->addWidget(m_techTreePanel);
    rightLayout->addWidget(topRightSplitter);
    rightLayout->addWidget(m_turnPanel);

    centralSplitter->addWidget(rightPanel);
    centralSplitter->setStretchFactor(0, 3);
    centralSplitter->setStretchFactor(1, 2);
    setCentralWidget(centralSplitter);

    statusBar()->showMessage(QStringLiteral("Загрузка..."));

    connect(m_turnPanel, &TurnPanel::endTurnClicked, this, &MainWindow::onEndTurn);
    connect(m_mapView, &MapView::tileClicked, this, &MainWindow::onTileClicked);

    connect(m_techTreePanel, &TechTreePanel::researchRequested, this,
        [this](const std::string& techId) {
            m_engine.state().players()[0].setCurrentResearch(techId);
            refreshAllPanels();
            statusBar()->showMessage(
                QStringLiteral("Начато исследование: %1").arg(QString::fromStdString(techId)));
        });

    connect(m_settlementPanel, &SettlementPanel::allocationChanged, this, [this]() {
        refreshAllPanels();
    });
    connect(m_settlementPanel, &SettlementPanel::buildingQueued, this, [this](const std::string&) {
        Player& human = m_engine.state().players()[0];
        Settlement* settlement = m_settlementPanel->currentSettlement();
        if (settlement)
            human.spendIndustryOnQueues(*settlement);
        refreshAllPanels();
    });
    connect(m_settlementPanel, &SettlementPanel::unitRecruitmentQueued, this, [this](const std::string&) {
        Player& human = m_engine.state().players()[0];
        Settlement* settlement = m_settlementPanel->currentSettlement();
        if (settlement)
            human.spendIndustryOnQueues(*settlement);
        refreshAllPanels();
    });
    connect(m_settlementPanel, &SettlementPanel::actionFailed, this, [this](const QString& msg) {
        statusBar()->showMessage(msg, 5000);
    });

    m_engine.loadFromAssets(ASSETS_DIR);
    refreshAllPanels();
    statusBar()->showMessage(QStringLiteral("Готово"));
}

MainWindow::~MainWindow() {}

void MainWindow::refreshAllPanels() {
    Player& human = m_engine.state().players()[0];

    std::vector<std::pair<int,int>> settlementTiles;
    for (const auto& s : human.settlements()) {
        for (const auto& t : s.tiles()) {
            auto c = t.getCoords();
            settlementTiles.emplace_back(c.q, c.r);
        }
    }
    m_mapView->renderMap(m_engine.state().map(), settlementTiles);

    if (!human.settlements().empty()) {
        m_settlementPanel->showSettlement(
            human.settlements()[0], m_engine.buildingCatalog(), m_engine.unitCatalog());
    }

    m_techTreePanel->showTechs(
        m_engine.techCatalog(),
        human.researchedTechIds(),
        human.currentResearchId());

    const auto& res = human.resources();
    std::string phaseName;
    switch (m_engine.state().CurPhase()) {
        case GlobalPhase::NotStarted:    phaseName = "Начало"; break;
        case GlobalPhase::Phase2_OpenMap: phaseName = "Открытая карта"; break;
        case GlobalPhase::Phase3_Final:  phaseName = "Финал"; break;
    }

    m_turnPanel->updateInfo(
        m_engine.state().turnNumber(), phaseName,
        res.science, res.industry, res.trade, res.diplomacy);
}

void MainWindow::onTileClicked(int q, int r) {
    statusBar()->showMessage(
        QStringLiteral("Клетка: %1, %2").arg(q).arg(r));

    Player& human = m_engine.state().players()[0];
    for (auto& settlement : human.settlements()) {
        for (const auto& tile : settlement.tiles()) {
            auto c = tile.getCoords();
            if (c.q == q && c.r == r) {
                m_settlementPanel->showSettlement(settlement, m_engine.buildingCatalog(), m_engine.unitCatalog());
                statusBar()->showMessage(
                    QStringLiteral("Поселение"),
                    5000);
                return;
            }
        }
    }
}

void MainWindow::onEndTurn() {
    m_engine.endTurn();
    refreshAllPanels();
    statusBar()->showMessage(
        QStringLiteral("Ход %1 завершён").arg(m_engine.state().turnNumber()), 3000);
}
