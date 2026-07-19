#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTabWidget>

#include "core/Settlement.hpp"
#include "core/Building.hpp"
#include "core/Unit.hpp"
#include "core/Player.hpp"
#include "core/PoliticalAction.hpp"

class SettlementPanel : public QWidget {
    Q_OBJECT
public:
    explicit SettlementPanel(QWidget* parent = nullptr);

    void showSettlement(Settlement& settlement,
                         const std::vector<Building>& buildingCatalog,
                         const std::vector<Unit>& unitCatalog);
    void updateTradeDiplomacy(const Resources& resources,
                               float tradeMultiplier,
                               const std::vector<PoliticalAction>& politicalActions);
    void setOpponents(const std::vector<int>& opponentIds);
    void clearSettlement();
    Settlement* currentSettlement() const { return m_settlement; }

signals:
    void allocationChanged();
    void buildingQueued(const std::string& buildingId);
    void unitRecruitmentQueued(const std::string& unitId);
    void actionFailed(const QString& message);
    void tradeExchangeRequested(const std::string& targetResource, int amount);
    void politicalActionRequested(const std::string& actionId, int targetId);

private slots:
    void onApplyAllocation();
    void onQueueBuilding();
    void onQueueRecruitment();
    void onExchangeTrade();
    void onExecutePoliticalAction();

private:
    void setupUi();

    Settlement* m_settlement = nullptr;

    QTabWidget* m_tabs;
    QLabel* m_populationLabel;
    QSpinBox* m_scienceSpin;
    QSpinBox* m_industrySpin;
    QSpinBox* m_tradeSpin;
    QSpinBox* m_diplomacySpin;
    QLabel* m_idleLabel;
    QPushButton* m_applyBtn;

    QListWidget* m_availableBuildingsList;
    QListWidget* m_buildingQueueList;
    QPushButton* m_queueBuildingBtn;

    QListWidget* m_availableUnitsList;
    QPushButton* m_recruitBtn;
    QListWidget* m_recruitQueueList;
    QListWidget* m_garrisonList;

    // Trade tab
    QLabel* m_tradeBalanceLabel;
    QLabel* m_tradeRateLabel;
    QSpinBox* m_tradeAmountSpin;
    QComboBox* m_tradeTargetCombo;
    QPushButton* m_exchangeBtn;
    QLabel* m_tradeResultLabel;

    // Diplomacy tab
    QLabel* m_diplomacyBalanceLabel;
    QComboBox* m_politicalTargetCombo;
    QListWidget* m_politicalActionList;
    QPushButton* m_executeActionBtn;
    std::vector<int> m_opponentIds;

    std::vector<Building> m_buildingCatalog;
    std::vector<Unit> m_unitCatalog;
    std::vector<PoliticalAction> m_politicalActionCatalog;
    float m_tradeMultiplier = 1.0f;
};
