#include "gui/MainWindow.hpp"
#include "gui/MapView.hpp"
#include "gui/SettlementPanel.hpp"
#include "gui/TechTreePanel.hpp"
#include "gui/TurnPanel.hpp"
#include "gui/MilitaryPanel.hpp"
#include "gui/StartMenuWidget.hpp"
#include "gui/TurnReportDialog.hpp"
#include "gui/VictoryDialog.hpp"
#include "gui/DefeatDialog.hpp"
#include "gui/RulesDialog.hpp"

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
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1024, 700);
    setWindowTitle(QStringLiteral("4X Стратегия"));

    auto *menuBar = this->menuBar();
    auto *gameMenu = menuBar->addMenu(QStringLiteral("Игра"));
    auto *endTurnAction = gameMenu->addAction(QStringLiteral("Завершить ход"));
    connect(endTurnAction, &QAction::triggered, this, &MainWindow::onEndTurn);

    gameMenu->addSeparator();
    auto *saveAction = gameMenu->addAction(QStringLiteral("Сохранить..."));
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveGame);
    auto *loadAction = gameMenu->addAction(QStringLiteral("Загрузить..."));
    connect(loadAction, &QAction::triggered, this, &MainWindow::onLoadGame);

    gameMenu->addSeparator();
    auto *rulesAction = gameMenu->addAction(QStringLiteral("Правила"));
    connect(rulesAction, &QAction::triggered, this, &MainWindow::onShowRules);

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
    m_militaryPanel = new MilitaryPanel(this);

    auto *centralSplitter = new QSplitter(Qt::Horizontal);
    centralSplitter->addWidget(m_mapView);

    auto *rightPanel = new QWidget(centralSplitter);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    // Кнопки-переключатели вместо одновременного показа обеих панелей —
    // раньше SettlementPanel и TechTreePanel были видны сразу вдвоём (через
    // QSplitter), из-за чего вместе с длинными списками построек/техов
    // правая часть требовала намного больше места, чем ей отводил
    // stretch-фактор, и карта слева сжималась до минимума.
    auto *toggleRow = new QWidget(rightPanel);
    auto *toggleLayout = new QHBoxLayout(toggleRow);
    toggleLayout->setContentsMargins(0, 0, 0, 0);
    auto *settlementToggle = new QPushButton(QStringLiteral("Поселение"), toggleRow);
    auto *techToggle = new QPushButton(QStringLiteral("Технологии"), toggleRow);
    auto *militaryToggle = new QPushButton(QStringLiteral("Войска/Герои"), toggleRow);
    settlementToggle->setCheckable(true);
    techToggle->setCheckable(true);
    militaryToggle->setCheckable(true);
    settlementToggle->setChecked(true);
    toggleLayout->addWidget(settlementToggle);
    toggleLayout->addWidget(techToggle);
    toggleLayout->addWidget(militaryToggle);
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

    // Каждая панель — в своём QScrollArea: если список построек/техов не
    // помещается по высоте, скроллится содержимое ВНУТРИ панели, а не
    // раздувает окно целиком (это и было причиной, что кнопка конца хода
    // улетала за нижний край экрана).
    auto *panelStack = new QStackedWidget(rightPanel);

    auto *settlementScroll = new QScrollArea(rightPanel);
    settlementScroll->setWidgetResizable(true);
    settlementScroll->setWidget(m_settlementPanel);
    panelStack->addWidget(settlementScroll);

    auto *techScroll = new QScrollArea(rightPanel);
    techScroll->setWidgetResizable(true);
    techScroll->setWidget(m_techTreePanel);
    panelStack->addWidget(techScroll);

    auto *militaryScroll = new QScrollArea(rightPanel);
    militaryScroll->setWidgetResizable(true);
    militaryScroll->setWidget(m_militaryPanel);
    panelStack->addWidget(militaryScroll);

    rightLayout->addWidget(panelStack, /*stretch=*/1);
    // TurnPanel — намеренно ВНЕ QStackedWidget/QScrollArea выше: сводка
    // ресурсов, список армий и кнопка "Конец хода" должны быть видны всегда,
    // независимо от того, что происходит в переключаемых панелях.
    rightLayout->addWidget(m_turnPanel);

    connect(settlementToggle, &QPushButton::clicked, this, [=]()
            {
        panelStack->setCurrentIndex(0);
        settlementToggle->setChecked(true);
        techToggle->setChecked(false);
        militaryToggle->setChecked(false); });
    connect(techToggle, &QPushButton::clicked, this, [=]()
            {
        panelStack->setCurrentIndex(1);
        techToggle->setChecked(true);
        settlementToggle->setChecked(false);
        militaryToggle->setChecked(false); });
    connect(militaryToggle, &QPushButton::clicked, this, [=]()
            {
        panelStack->setCurrentIndex(2);
        militaryToggle->setChecked(true);
        settlementToggle->setChecked(false);
        techToggle->setChecked(false); });

    centralSplitter->addWidget(rightPanel);
    centralSplitter->setStretchFactor(0, 1);
    centralSplitter->setStretchFactor(1, 0);
    // Явные стартовые размеры — без этого QSplitter отталкивается от
    // минимальных размеров содержимого правой панели, а не от stretch-
    // факторов, и карта при первом запуске получала едва ли четверть окна.
    centralSplitter->setSizes({700, 300});

    connect(m_turnPanel, &TurnPanel::endTurnClicked, this, &MainWindow::onEndTurn);
    connect(m_turnPanel, &TurnPanel::settlementSelected, this, &MainWindow::onSettlementListSelected);
    connect(m_militaryPanel, &MilitaryPanel::armySelected, this, &MainWindow::onArmySelected);
    connect(m_militaryPanel, &MilitaryPanel::foundSettlementRequested, this, &MainWindow::onFoundSettlementRequested);
    connect(m_mapView, &MapView::tileClicked, this, &MainWindow::onTileClicked);
    connect(m_mapView, &MapView::tileHovered, this, &MainWindow::onTileHovered);
    connect(m_mapView, &MapView::armySelected, this, &MainWindow::onArmySelected);
    connect(m_mapView, &MapView::armyMoveRequested, this, &MainWindow::onArmyMoveRequested);
    connect(m_mapView, &MapView::selectionCleared, this, &MainWindow::onSelectionCleared);

    connect(m_techTreePanel, &TechTreePanel::researchRequested, this,
            [this](const std::string &techId)
            {
                Player *human = humanPlayer();
                if (!human || m_spectatorMode)
                    return;
                human->setCurrentResearch(techId);
                refreshAllPanels();
                statusBar()->showMessage(
                    QStringLiteral("Начато исследование: %1").arg(QString::fromStdString(techId)));
            });

    connect(m_settlementPanel, &SettlementPanel::allocationChanged, this, [this]()
            { refreshAllPanels(); });
    connect(m_settlementPanel, &SettlementPanel::buildingQueued, this, [this](const std::string &)
            {
        if (m_spectatorMode)
            return;
        Player *human = humanPlayer();
        if (!human)
            return;
        Settlement* settlement = m_settlementPanel->currentSettlement();
        if (settlement)
            human->spendIndustryOnQueues(*settlement);
        refreshAllPanels(); });
    connect(m_settlementPanel, &SettlementPanel::unitRecruitmentQueued, this, [this](const std::string &)
            {
        if (m_spectatorMode)
            return;
        Player *human = humanPlayer();
        if (!human)
            return;
        Settlement* settlement = m_settlementPanel->currentSettlement();
        if (settlement)
            human->spendIndustryOnQueues(*settlement);
        refreshAllPanels(); });
    connect(m_settlementPanel, &SettlementPanel::actionFailed, this, [this](const QString &msg)
            { statusBar()->showMessage(msg, 5000); });
    connect(m_settlementPanel, &SettlementPanel::marchRequested, this, &MainWindow::onMarchRequested);

    connect(m_settlementPanel, &SettlementPanel::tradeExchangeRequested, this,
            [this](const std::string &targetResource, int amount)
            {
                Player *human = humanPlayer();
                if (!human || m_spectatorMode)
                    return;
                if (human->exchangeTrade(targetResource, amount))
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
                Player *human = humanPlayer();
                if (!human || m_spectatorMode)
                    return;
                std::string msg = m_engine.applyPoliticalAction(human->id(), actionId, targetId);
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

    // Выбранное поселение не найдено (ещё не выбирали, или было потеряно) —
    // переключаемся на первое оставшееся своё поселение.
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

Player *MainWindow::humanPlayer()
{
    for (auto &p : m_engine.state().players())
        if (p.isHuman())
            return &p;
    return nullptr;
}

int MainWindow::humanId()
{
    Player *human = humanPlayer();
    return human ? human->id() : -1;
}

bool MainWindow::humanIsDefeated()
{
    Player *human = humanPlayer();
    return human && human->settlements().empty();
}

void MainWindow::onNewGame()
{
    m_engine.loadFromAssets(ASSETS_DIR, m_startMenu->selectedOptions());
    m_hasSelectedSettlement = false;
    m_spectatorMode = false;
    refreshAllPanels();
    m_stack->setCurrentIndex(1);
    statusBar()->showMessage(QStringLiteral("Партия начата"), 3000);
}

void MainWindow::onSaveGame()
{
    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Сохранить партию"), QString(),
        QStringLiteral("Файлы сохранений (*.json)"));
    if (path.isEmpty())
        return;

    if (!path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
        path += QStringLiteral(".json");

    if (m_engine.saveGame(path.toStdString()))
        statusBar()->showMessage(QStringLiteral("Партия сохранена: %1").arg(path), 5000);
    else
        QMessageBox::warning(this, QStringLiteral("Ошибка"),
                             QStringLiteral("Не удалось сохранить партию в %1").arg(path));
}

void MainWindow::onLoadGame()
{
    QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Загрузить партию"), QString(),
        QStringLiteral("Файлы сохранений (*.json)"));
    if (path.isEmpty())
        return;

    if (!m_engine.loadGame(path.toStdString(), ASSETS_DIR))
    {
        QMessageBox::warning(this, QStringLiteral("Ошибка"),
                             QStringLiteral("Не удалось загрузить партию из %1").arg(path));
        return;
    }

    m_hasSelectedSettlement = false;
    m_selectedArmyId.clear();
    m_mapView->clearSelection();
    m_spectatorMode = false;
    refreshAllPanels();
    m_stack->setCurrentIndex(1);
    statusBar()->showMessage(QStringLiteral("Партия загружена: %1").arg(path), 5000);

    if (!humanPlayer())
    {
        m_spectatorMode = true;
        refreshAllPanels();
    }
    else if (humanIsDefeated())
    {
        showDefeatDialog();
    }
}

void MainWindow::onShowRules()
{
    RulesDialog dialog(this);
    dialog.exec();
}

MainWindow::~MainWindow() {}

void MainWindow::refreshAllPanels()
{
    Player *human = humanPlayer();
    bool spectator = m_spectatorMode || (human == nullptr);
    int viewerId = human ? human->id() : -1;

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

    // На карте нужно видеть ВСЕХ игроков — иначе чужие поселения/армии
    // просто не существуют для человека, и атаковать/оценивать обстановку
    // невозможно. В панели управления (updateArmyList) по-прежнему только
    // свои армии — чужими командовать нельзя, только видеть их на карте.
    std::vector<SettlementMarker> settlementTiles;
    std::vector<Army> allPlayersArmies;
    std::map<int, PlayerRelation> relations;
    const auto &vassalizations = m_engine.state().vassalizations();
    for (const auto &p : m_engine.state().players())
    {
        bool isSelf = (p.id() == viewerId);

        for (const auto &s : p.settlements())
        {
            for (const auto &t : s.tiles())
            {
                auto c = t.getCoords();
                // Чужое поселение показываем только в зоне видимости
                // человека (блок 3.6, туман войны); своё — всегда. В режиме
                // наблюдателя видна вся карта без тумана.
                if (spectator || isSelf || (human && human->isVisible(c)))
                    settlementTiles.push_back({c.q, c.r, p.id()});
            }
        }
        for (const auto &a : p.armies())
        {
            if (spectator || isSelf || (human && human->isVisible(a.position())))
                allPlayersArmies.push_back(a);
        }

        if (human && p.id() == human->id())
        {
            relations[p.id()] = PlayerRelation::Self;
            continue;
        }
        bool allied = human && GameState::isAllied(m_engine.state(), human->id(), p.id());
        auto vAsVassal = vassalizations.find(p.id());         // p — чей-то вассал?
        auto vHumanVassal = human ? vassalizations.find(human->id()) : vassalizations.end(); // человек — чей-то вассал?
        bool vassalRelation =
            (vAsVassal != vassalizations.end() && vAsVassal->second == viewerId) || // p — вассал человека
            (vHumanVassal != vassalizations.end() && vHumanVassal->second == p.id()); // человек — вассал p

        if (allied)
            relations[p.id()] = PlayerRelation::Ally;
        else if (vassalRelation)
            relations[p.id()] = PlayerRelation::Vassal;
        else
            relations[p.id()] = PlayerRelation::EnemyOrNeutral;
    }

    m_mapView->clearReachableTiles();
    if (spectator)
    {
        // В режиме наблюдателя туман войны полностью снят (nullptr == нет тумана).
        m_mapView->renderMap(m_engine.state().map(), settlementTiles, allPlayersArmies, viewerId, relations);
    }
    else
    {
        m_mapView->renderMap(m_engine.state().map(), settlementTiles, allPlayersArmies, human->id(), relations,
                             &human->exploredTiles(), &human->visibleTiles());
    }

    if (!human)
    {
        // Сохранение без человеческого игрока: только наблюдение.
        m_militaryPanel->updateArmyList({});
        m_militaryPanel->updateHeroList({}, QString());
        m_settlementPanel->clearSettlement();
        m_turnPanel->updateSettlementList({}, {-1, -1});
        m_techTreePanel->showTechs({}, {}, {});
        m_turnPanel->updateInfo(m_engine.state().turnNumber(), phaseName, 0.f, 0.f, 0.f, 0.f);
        m_settlementPanel->updateTradeDiplomacy(Resources{}, 1.0f, m_engine.politicalActionCatalog());
        m_settlementPanel->setOpponents({});
        return;
    }

    m_militaryPanel->updateArmyList(human->armies());
    m_militaryPanel->updateHeroList(human->heroes(), QString::fromStdString(human->race().getName()));

    Settlement *selected = selectedHumanSettlement(*human);
    if (selected)
    {
        m_settlementPanel->showSettlement(
            *selected, m_engine.buildingCatalog(), m_engine.unitCatalog());
    }
    else
    {
        // Выбранное поселение исчезло (захвачено врагом) или своих городов
        // больше нет — иначе SettlementPanel держит протухший указатель на
        // элемент vector<Settlement> после реаллокации/стирания, и следующий
        // клик по кнопкам панели приводит к падению.
        m_settlementPanel->clearSettlement();
    }
    m_turnPanel->updateSettlementList(human->settlements(), m_selectedSettlementAnchor);

    m_techTreePanel->showTechs(
        human->technologies(),
        human->researchedTechIds(),
        human->currentResearchId());

    const auto &res = human->resources();

    m_turnPanel->updateInfo(
        m_engine.state().turnNumber(), phaseName,
        res.science, res.industry, res.trade, res.diplomacy);

    m_settlementPanel->updateTradeDiplomacy(
        human->resources(),
        human->effectiveTradeMultiplier(),
        m_engine.politicalActionCatalog());

    std::vector<int> opponentIds;
    for (const auto &p : m_engine.state().players())
        if (p.id() != human->id())
            opponentIds.push_back(p.id());
    m_settlementPanel->setOpponents(opponentIds);
}

void MainWindow::onTileClicked(int q, int r)
{
    if (m_stack->currentIndex() != 1)
        return;

    statusBar()->showMessage(
        QStringLiteral("Клетка: %1, %2").arg(q).arg(r));

    Player *human = humanPlayer();
    if (!human || m_spectatorMode)
        return;
    for (auto &settlement : human->settlements())
    {
        for (const auto &tile : settlement.tiles())
        {
            auto c = tile.getCoords();
            if (c.q == q && c.r == r)
            {
                selectSettlement(settlement);
                m_settlementPanel->showSettlement(settlement, m_engine.buildingCatalog(), m_engine.unitCatalog());
                m_turnPanel->updateSettlementList(human->settlements(), m_selectedSettlementAnchor);
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
    if (m_spectatorMode)
        return;

    Army *army = m_engine.state().findArmy(armyId);
    if (!army)
        return;

    m_selectedArmyId = armyId;
    m_mapView->setSelectedArmy(armyId);
    m_militaryPanel->setFoundSettlementAvailable(army->isSettlerArmy());

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
    if (m_spectatorMode || m_selectedArmyId.empty())
        return;

    auto result = m_engine.state().foundSettlementFromArmy(m_selectedArmyId);
    statusBar()->showMessage(QString::fromStdString(
        result.success ? std::string("Город основан!") : result.errorMessage), 5000);

    if (result.success)
    {
        m_selectedArmyId.clear();
        m_hasSelectedSettlement = false; // выбираем новый город в refreshAllPanels
        m_mapView->clearSelection();
        m_militaryPanel->setFoundSettlementAvailable(false);
    }

    refreshAllPanels();
}

void MainWindow::onArmyMoveRequested(const std::string &armyId, int q, int r)
{
    if (m_spectatorMode)
        return;

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
    m_militaryPanel->setFoundSettlementAvailable(false);

    // Захват города армией может сразу решить партию (взят последний вражеский
    // город) — раньше это обнаруживалось только на onEndTurn, и до конца хода
    // игра "молчала" даже после решающего удара. Проверяем сразу же, как
    // проверка уже делается после политических действий (см. выше).
    auto v = m_engine.checkVictoryCondition();
    if (v.hasWinner())
        showVictoryDialog(v);
}

void MainWindow::onSettlementListSelected(int q, int r)
{
    Player *human = humanPlayer();
    if (!human || m_spectatorMode)
        return;
    for (auto &settlement : human->settlements())
    {
        if (settlement.anchorCoords().q == q && settlement.anchorCoords().r == r)
        {
            selectSettlement(settlement);
            m_settlementPanel->showSettlement(settlement, m_engine.buildingCatalog(), m_engine.unitCatalog());
            m_turnPanel->updateSettlementList(human->settlements(), m_selectedSettlementAnchor);
            return;
        }
    }
}

void MainWindow::onSelectionCleared()
{
    m_mapView->clearSelection();
    m_selectedArmyId.clear();
    m_militaryPanel->setFoundSettlementAvailable(false);
    statusBar()->clearMessage();
}

void MainWindow::onMarchRequested(const std::vector<std::string> &unitIds)
{
    if (m_spectatorMode)
        return;

    Settlement *settlement = m_settlementPanel->currentSettlement();
    if (!settlement)
        return;

    Player *human = humanPlayer();
    if (!human)
        return;

    if (unitIds.empty())
    {
        statusBar()->showMessage(QStringLiteral("Нет войск в гарнизоне"), 5000);
        return;
    }

    human->detachArmy(*settlement, unitIds);
    refreshAllPanels();
    statusBar()->showMessage(
        QStringLiteral("Армия выступила в поход (%1 юн.)").arg(static_cast<int>(unitIds.size())),
        5000);
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

    if (humanIsDefeated() && !m_spectatorMode)
    {
        showDefeatDialog();
        return;
    }

    refreshAllPanels();

    const auto &events = m_engine.state().turnEvents();
    statusBar()->showMessage(
        QStringLiteral("Ход %1 завершён, → %2 событ%3")
            .arg(m_engine.state().turnNumber())
            .arg(events.size())
            .arg(events.size() == 1 ? QStringLiteral("ие") : QStringLiteral("ий")),
        3000);

    if (!events.empty())
    {
        TurnReportDialog dialog(events, m_engine.state().turnNumber(), this);
        dialog.exec();
    }
}

void MainWindow::showVictoryDialog(const VictoryResult &v)
{
    const Player *winner = nullptr;
    for (const auto &p : m_engine.state().players())
    {
        if (p.id() == v.winnerId)
        {
            winner = &p;
            break;
        }
    }

    if (!winner)
    {
        // На всякий случай — если победителя почему-то не нашли, не падаем.
        QMessageBox::information(this, QStringLiteral("Победа!"),
                                 QStringLiteral("Игрок %1 победил!").arg(v.winnerId));
        m_stack->setCurrentIndex(0);
        return;
    }

    VictoryDialog dialog(v, *winner, m_engine.state().turnNumber(), this);
    bool startedNewGame = false;
    connect(&dialog, &VictoryDialog::newGameRequested, this, [this, &startedNewGame]()
           {
               startedNewGame = true;
               onNewGame();
           });
    connect(&dialog, &VictoryDialog::exitRequested, qApp, &QApplication::quit);
    dialog.exec();

    if (!startedNewGame)
        m_stack->setCurrentIndex(0);
}

void MainWindow::showDefeatDialog()
{
    int remaining = 0;
    for (const auto &p : m_engine.state().players())
        if (!p.settlements().empty())
            ++remaining;

    DefeatDialog dialog(m_engine.state().turnNumber(), remaining, this);
    bool startedNewGame = false;
    connect(&dialog, &DefeatDialog::newGameRequested, this, [this, &startedNewGame]()
           {
               startedNewGame = true;
               onNewGame();
           });
    connect(&dialog, &DefeatDialog::exitRequested, qApp, &QApplication::quit);
    connect(&dialog, &DefeatDialog::spectateRequested, this, [this]()
           {
               m_spectatorMode = true;
               refreshAllPanels();
               statusBar()->showMessage(
                   QStringLiteral("Режим наблюдателя: вся карта видна"), 5000);
           });
    dialog.exec();

    if (!startedNewGame && !m_spectatorMode)
        m_stack->setCurrentIndex(0);
}
