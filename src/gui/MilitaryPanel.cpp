#include "gui/MilitaryPanel.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <QStringList>
#include <cmath>

MilitaryPanel::MilitaryPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void MilitaryPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(QStringLiteral("Герои:"), this));
    m_heroList = new QListWidget(this);
    m_heroList->setMaximumHeight(120);
    layout->addWidget(m_heroList);

    layout->addSpacing(8);

    layout->addWidget(new QLabel(QStringLiteral("Армии:"), this));
    m_armyList = new QListWidget(this);
    m_armyList->setMaximumHeight(160);
    connect(m_armyList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit armySelected(item->data(Qt::UserRole).toString().toStdString());
    });
    layout->addWidget(m_armyList);

    layout->addSpacing(8);

    m_foundSettlementBtn = new QPushButton(QStringLiteral("Основать город"), this);
    m_foundSettlementBtn->setVisible(false);
    connect(m_foundSettlementBtn, &QPushButton::clicked, this, &MilitaryPanel::foundSettlementRequested);
    layout->addWidget(m_foundSettlementBtn);

    layout->addStretch();
}

void MilitaryPanel::updateArmyList(const std::vector<Army>& armies) {
    m_armyList->clear();
    for (const auto& a : armies) {
        QString text = QStringLiteral("Армия (%1, %2) — %3 юн., %4/%5 ОД")
            .arg(a.position().q).arg(a.position().r)
            .arg(static_cast<int>(a.units().size()))
            .arg(a.movementPointsRemaining())
            .arg(a.movementPointsMax());
        auto* item = new QListWidgetItem(text, m_armyList);
        item->setData(Qt::UserRole, QString::fromStdString(a.getId()));
    }
}

void MilitaryPanel::setFoundSettlementAvailable(bool available) {
    m_foundSettlementBtn->setVisible(available);
}

void MilitaryPanel::updateHeroList(const std::vector<Hero>& heroes, const QString& raceName) {
    m_heroList->clear();
    for (const auto& h : heroes) {
        Modifiers m = h.getModifiers();
        QStringList bonuses;
        auto addIfBonus = [&bonuses](const QString& label, float value) {
            if (std::abs(value - 1.0f) < 0.001f)
                return;
            int percent = static_cast<int>(std::round((value - 1.0f) * 100.0f));
            bonuses << QStringLiteral("%1 %2%3%")
                           .arg(label)
                           .arg(percent >= 0 ? QStringLiteral("+") : QString())
                           .arg(percent);
        };
        addIfBonus(QStringLiteral("Наука"), m.science);
        addIfBonus(QStringLiteral("Пром"), m.industry);
        addIfBonus(QStringLiteral("Торговля"), m.trade);
        addIfBonus(QStringLiteral("Дипломатия"), m.diplomacy);
        addIfBonus(QStringLiteral("Военное дело"), m.military);
        addIfBonus(QStringLiteral("Рост"), m.growth);

        QString text = QStringLiteral("%1 (%2) — %3")
            .arg(QString::fromStdString(h.getName()))
            .arg(raceName)
            .arg(bonuses.isEmpty() ? QStringLiteral("без бонусов")
                                   : bonuses.join(QStringLiteral(", ")));
        auto* item = new QListWidgetItem(text, m_heroList);
        item->setToolTip(text);
    }
}