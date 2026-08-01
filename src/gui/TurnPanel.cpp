#include "gui/TurnPanel.hpp"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFont>

TurnPanel::TurnPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void TurnPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);

    m_turnLabel = new QLabel(QStringLiteral("Ход: 0"), this);
    m_phaseLabel = new QLabel(QStringLiteral("Фаза: —"), this);
    layout->addWidget(m_turnLabel);
    layout->addWidget(m_phaseLabel);

    layout->addSpacing(6);

    auto* resGroup = new QWidget(this);
    auto* resGrid = new QGridLayout(resGroup);
    resGrid->setContentsMargins(0, 0, 0, 0);
    resGrid->setHorizontalSpacing(12);
    resGrid->setVerticalSpacing(2);

    m_scienceLabel = new QLabel(QStringLiteral("Наука: 0"), resGroup);
    m_industryLabel = new QLabel(QStringLiteral("Пром: 0"), resGroup);
    m_tradeLabel = new QLabel(QStringLiteral("Торговля: 0"), resGroup);
    m_diplomacyLabel = new QLabel(QStringLiteral("Дипломатия: 0"), resGroup);

    resGrid->addWidget(m_scienceLabel, 0, 0);
    resGrid->addWidget(m_industryLabel, 0, 1);
    resGrid->addWidget(m_tradeLabel, 1, 0);
    resGrid->addWidget(m_diplomacyLabel, 1, 1);

    layout->addWidget(resGroup);

    layout->addSpacing(8);

    layout->addWidget(new QLabel(QStringLiteral("Мои города:"), this));
    m_settlementList = new QListWidget(this);
    m_settlementList->setMaximumHeight(90);
    connect(m_settlementList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit settlementSelected(item->data(Qt::UserRole).toInt(), item->data(Qt::UserRole + 1).toInt());
    });
    layout->addWidget(m_settlementList);

    layout->addSpacing(8);

    layout->addWidget(new QLabel(QStringLiteral("Армии:"), this));
    m_armyList = new QListWidget(this);
    m_armyList->setMaximumHeight(120);
    connect(m_armyList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit armySelected(item->data(Qt::UserRole).toString().toStdString());
    });
    layout->addWidget(m_armyList);

    m_foundSettlementBtn = new QPushButton(QStringLiteral("Основать город"), this);
    m_foundSettlementBtn->setVisible(false);
    connect(m_foundSettlementBtn, &QPushButton::clicked, this, &TurnPanel::foundSettlementRequested);
    layout->addWidget(m_foundSettlementBtn);

    layout->addStretch();

    m_endTurnBtn = new QPushButton(QStringLiteral("Конец хода"), this);
    connect(m_endTurnBtn, &QPushButton::clicked, this, &TurnPanel::endTurnClicked);
    layout->addWidget(m_endTurnBtn);
}

void TurnPanel::updateSettlementList(const std::vector<Settlement>& settlements, const Coords& selectedAnchor) {
    m_settlementList->clear();
    int cityNumber = 0;
    for (const auto& s : settlements) {
        ++cityNumber;
        Coords anchor = s.anchorCoords();
        QString text = QStringLiteral("Город %1 (%2, %3) — нас. %4")
            .arg(cityNumber)
            .arg(anchor.q).arg(anchor.r)
            .arg(s.populationCount());
        auto* item = new QListWidgetItem(text, m_settlementList);
        item->setData(Qt::UserRole, anchor.q);
        item->setData(Qt::UserRole + 1, anchor.r);
        if (anchor.q == selectedAnchor.q && anchor.r == selectedAnchor.r) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            m_settlementList->setCurrentItem(item);
        }
    }
}

void TurnPanel::updateArmyList(const std::vector<Army>& armies) {
    m_armyList->clear();
    for (const auto& a : armies) {
        QString text = QStringLiteral("Армия (%1, %2) — %3 юн., %4/5 ОД")
            .arg(a.position().q).arg(a.position().r)
            .arg(static_cast<int>(a.units().size()))
            .arg(a.movementPointsRemaining())
            .arg(a.movementPointsMax());
        auto* item = new QListWidgetItem(text, m_armyList);
        item->setData(Qt::UserRole, QString::fromStdString(a.getId()));
    }
}

void TurnPanel::setFoundSettlementAvailable(bool available) {
    m_foundSettlementBtn->setVisible(available);
}

void TurnPanel::updateInfo(int turnNumber, const std::string& phase,
                           float science, float industry, float trade, float diplomacy) {
    m_turnLabel->setText(QStringLiteral("Ход: %1").arg(turnNumber));
    m_phaseLabel->setText(QStringLiteral("Фаза: %1").arg(QString::fromStdString(phase)));
    m_scienceLabel->setText(QStringLiteral("Наука: %1").arg(static_cast<int>(science)));
    m_industryLabel->setText(QStringLiteral("Пром: %1").arg(static_cast<int>(industry)));
    m_tradeLabel->setText(QStringLiteral("Торговля: %1").arg(static_cast<int>(trade)));
    m_diplomacyLabel->setText(QStringLiteral("Дипломатия: %1").arg(static_cast<int>(diplomacy)));
}
