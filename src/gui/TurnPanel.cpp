#include "gui/TurnPanel.hpp"
#include <QVBoxLayout>
#include <QGridLayout>

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

    layout->addStretch();

    m_endTurnBtn = new QPushButton(QStringLiteral("Конец хода"), this);
    connect(m_endTurnBtn, &QPushButton::clicked, this, &TurnPanel::endTurnClicked);
    layout->addWidget(m_endTurnBtn);
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
