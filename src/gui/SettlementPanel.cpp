#include "gui/SettlementPanel.hpp"
#include "core/Military.hpp"
#include <QFormLayout>
#include <QHBoxLayout>

SettlementPanel::SettlementPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void SettlementPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    m_populationLabel = new QLabel(QStringLiteral("Население: 0"), this);
    mainLayout->addWidget(m_populationLabel);

    auto* allocGroup = new QGroupBox(QStringLiteral("Распределение населения"), this);
    auto* allocLayout = new QFormLayout(allocGroup);

    m_scienceSpin = new QSpinBox(allocGroup);
    m_scienceSpin->setRange(0, 9999);
    allocLayout->addRow(QStringLiteral("Наука:"), m_scienceSpin);

    m_industrySpin = new QSpinBox(allocGroup);
    m_industrySpin->setRange(0, 9999);
    allocLayout->addRow(QStringLiteral("Промышленность:"), m_industrySpin);

    m_tradeSpin = new QSpinBox(allocGroup);
    m_tradeSpin->setRange(0, 9999);
    allocLayout->addRow(QStringLiteral("Торговля:"), m_tradeSpin);

    m_diplomacySpin = new QSpinBox(allocGroup);
    m_diplomacySpin->setRange(0, 9999);
    allocLayout->addRow(QStringLiteral("Дипломатия:"), m_diplomacySpin);

    m_idleLabel = new QLabel(QStringLiteral("Свободно: 0"), allocGroup);
    allocLayout->addRow(m_idleLabel);

    m_applyBtn = new QPushButton(QStringLiteral("Применить"), allocGroup);
    connect(m_applyBtn, &QPushButton::clicked, this, &SettlementPanel::onApplyAllocation);
    allocLayout->addRow(m_applyBtn);

    mainLayout->addWidget(allocGroup);

    auto* buildGroup = new QGroupBox(QStringLiteral("Постройки"), this);
    auto* buildLayout = new QVBoxLayout(buildGroup);

    m_availableBuildingsList = new QListWidget(buildGroup);
    buildLayout->addWidget(new QLabel(QStringLiteral("Доступные:"), buildGroup));
    buildLayout->addWidget(m_availableBuildingsList);

    m_queueBuildingBtn = new QPushButton(QStringLiteral("Строить"), buildGroup);
    connect(m_queueBuildingBtn, &QPushButton::clicked, this, &SettlementPanel::onQueueBuilding);
    buildLayout->addWidget(m_queueBuildingBtn);

    m_buildingQueueList = new QListWidget(buildGroup);
    buildLayout->addWidget(new QLabel(QStringLiteral("Очередь строительства:"), buildGroup));
    buildLayout->addWidget(m_buildingQueueList);

    mainLayout->addWidget(buildGroup);

    auto* recruitGroup = new QGroupBox(QStringLiteral("Найм войск"), this);
    auto* recruitLayout = new QVBoxLayout(recruitGroup);

    m_availableUnitsList = new QListWidget(recruitGroup);
    recruitLayout->addWidget(new QLabel(QStringLiteral("Доступные юниты:"), recruitGroup));
    recruitLayout->addWidget(m_availableUnitsList);

    m_recruitBtn = new QPushButton(QStringLiteral("Нанять"), recruitGroup);
    connect(m_recruitBtn, &QPushButton::clicked, this, &SettlementPanel::onQueueRecruitment);
    recruitLayout->addWidget(m_recruitBtn);

    m_recruitQueueList = new QListWidget(recruitGroup);
    recruitLayout->addWidget(new QLabel(QStringLiteral("Очередь найма:"), recruitGroup));
    recruitLayout->addWidget(m_recruitQueueList);

    m_garrisonList = new QListWidget(recruitGroup);
    recruitLayout->addWidget(new QLabel(QStringLiteral("Гарнизон:"), recruitGroup));
    recruitLayout->addWidget(m_garrisonList);

    mainLayout->addWidget(recruitGroup);
}

void SettlementPanel::showSettlement(Settlement& settlement,
                                     const std::vector<Building>& buildingCatalog,
                                     const std::vector<Unit>& unitCatalog) {
    m_settlement = &settlement;
    m_buildingCatalog = buildingCatalog;
    m_unitCatalog = unitCatalog;

    m_populationLabel->setText(
        QStringLiteral("Население: %1").arg(settlement.populationCount()));

    const auto& alloc = settlement.allocation();
    int pop = settlement.populationCount();
    m_scienceSpin->setMaximum(pop);
    m_industrySpin->setMaximum(pop);
    m_tradeSpin->setMaximum(pop);
    m_diplomacySpin->setMaximum(pop);
    m_scienceSpin->setValue(alloc.science);
    m_industrySpin->setValue(alloc.industry);
    m_tradeSpin->setValue(alloc.trade);
    m_diplomacySpin->setValue(alloc.diplomacy);
    m_idleLabel->setText(QStringLiteral("Свободно: %1").arg(alloc.idle));

    m_availableBuildingsList->clear();
    for (const auto& b : buildingCatalog) {
        if (settlement.hasBuilt(b.getId())) continue;
        auto* item = new QListWidgetItem(
            QString::fromStdString(b.getName() + " (" + b.getId() + ")"),
            m_availableBuildingsList);
        item->setData(Qt::UserRole, QString::fromStdString(b.getId()));
    }

    m_buildingQueueList->clear();
    for (const auto& qi : settlement.buildingQueue()) {
        int remaining = qi.remainingCost();
        QString text = QString::fromStdString(qi.building.getName())
                       + QStringLiteral(" — осталось: %1").arg(remaining);
        m_buildingQueueList->addItem(text);
    }

    m_availableUnitsList->clear();
    for (const auto& u : unitCatalog) {
        bool canRecruit = Military::canRecruit(u, settlement.builtBuildings());
        QString text = QString::fromStdString(u.getName());
        if (!canRecruit)
            text += QStringLiteral(" (нужна казарма)");

        auto* item = new QListWidgetItem(text, m_availableUnitsList);
        item->setData(Qt::UserRole, QString::fromStdString(u.getId()));
        if (!canRecruit)
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }

    m_recruitQueueList->clear();
    for (const auto& ri : settlement.recruitmentQueue()) {
        int remaining = ri.remainingCost();
        QString text = QString::fromStdString(ri.unit.getName())
                       + QStringLiteral(" — осталось: %1").arg(remaining);
        m_recruitQueueList->addItem(text);
    }

    m_garrisonList->clear();
    for (const auto& u : settlement.garrison()) {
        m_garrisonList->addItem(QString::fromStdString(u.getName()));
    }

    setEnabled(true);
}

void SettlementPanel::clearSettlement() {
    m_settlement = nullptr;
    m_buildingCatalog.clear();
    m_unitCatalog.clear();
    m_populationLabel->setText(QStringLiteral("Население: —"));
    m_scienceSpin->setValue(0);
    m_industrySpin->setValue(0);
    m_tradeSpin->setValue(0);
    m_diplomacySpin->setValue(0);
    m_idleLabel->setText(QStringLiteral("Свободно: —"));
    m_availableBuildingsList->clear();
    m_buildingQueueList->clear();
    m_availableUnitsList->clear();
    m_recruitQueueList->clear();
    m_garrisonList->clear();
    setEnabled(false);
}

void SettlementPanel::onApplyAllocation() {
    if (!m_settlement) return;

    int total = m_scienceSpin->value() + m_industrySpin->value()
                + m_tradeSpin->value() + m_diplomacySpin->value();
    if (total > m_settlement->populationCount()) return;

    const auto& cur = m_settlement->allocation();

    int diffScience = m_scienceSpin->value() - cur.science;
    int diffIndustry = m_industrySpin->value() - cur.industry;
    int diffTrade = m_tradeSpin->value() - cur.trade;
    int diffDiplomacy = m_diplomacySpin->value() - cur.diplomacy;

    auto unassign = [&](Sphere s, int amount) {
        if (amount < 0) m_settlement->unassignToIdle(s, -amount);
    };
    auto assign = [&](Sphere s, int amount) {
        if (amount > 0) m_settlement->assignFromIdle(s, amount);
    };

    unassign(Sphere::Science, diffScience);
    unassign(Sphere::Industry, diffIndustry);
    unassign(Sphere::Trade, diffTrade);
    unassign(Sphere::Diplomacy, diffDiplomacy);

    assign(Sphere::Science, diffScience);
    assign(Sphere::Industry, diffIndustry);
    assign(Sphere::Trade, diffTrade);
    assign(Sphere::Diplomacy, diffDiplomacy);

    m_idleLabel->setText(
        QStringLiteral("Свободно: %1").arg(m_settlement->allocation().idle));

    emit allocationChanged();
}

void SettlementPanel::onQueueBuilding() {
    if (!m_settlement) return;

    auto selected = m_availableBuildingsList->currentItem();
    if (!selected) return;

    std::string buildingId = selected->data(Qt::UserRole).toString().toStdString();
    for (const auto& b : m_buildingCatalog) {
        if (b.getId() == buildingId) {
            m_settlement->queueBuilding(b);
            emit buildingQueued(buildingId);
            break;
        }
    }

    showSettlement(*m_settlement, m_buildingCatalog, m_unitCatalog);
}

void SettlementPanel::onQueueRecruitment() {
    if (!m_settlement) return;

    auto selected = m_availableUnitsList->currentItem();
    if (!selected) return;

    std::string unitId = selected->data(Qt::UserRole).toString().toStdString();
    for (const auto& u : m_unitCatalog) {
        if (u.getId() == unitId) {
            m_settlement->queueUnitRecruitment(u);
            emit unitRecruitmentQueued(unitId);
            break;
        }
    }

    showSettlement(*m_settlement, m_buildingCatalog, m_unitCatalog);
}
