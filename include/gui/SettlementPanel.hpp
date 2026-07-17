#pragma once

#include <QWidget>
#include <QSpinBox>
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

class SettlementPanel : public QWidget {
    Q_OBJECT
public:
    explicit SettlementPanel(QWidget* parent = nullptr);

    void showSettlement(Settlement& settlement,
                         const std::vector<Building>& buildingCatalog,
                         const std::vector<Unit>& unitCatalog);
    void clearSettlement();
    Settlement* currentSettlement() const { return m_settlement; }

signals:
    void allocationChanged();
    void buildingQueued(const std::string& buildingId);
    void unitRecruitmentQueued(const std::string& unitId);
    void actionFailed(const QString& message);

private slots:
    void onApplyAllocation();
    void onQueueBuilding();
    void onQueueRecruitment();

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

    std::vector<Building> m_buildingCatalog;
    std::vector<Unit> m_unitCatalog;
};
