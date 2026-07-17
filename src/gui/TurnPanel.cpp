#include "gui/TurnPanel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>

TurnPanel::TurnPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void TurnPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);

    m_turnLabel = new QLabel(QStringLiteral("Ход: 0"), this);
    m_phaseLabel = new QLabel(QStringLiteral("Фаза: —"), this);
    layout->addWidget(m_turnLabel);
    layout->addWidget(m_phaseLabel);

    layout->addSpacing(8);

    auto* resGroup = new QWidget(this);
    auto* resLayout = new QVBoxLayout(resGroup);
    resLayout->setContentsMargins(0, 0, 0, 0);
    m_scienceLabel = new QLabel(QStringLiteral("Наука: 0"), resGroup);
    m_industryLabel = new QLabel(QStringLiteral("Пром: 0"), resGroup);
    m_tradeLabel = new QLabel(QStringLiteral("Торговля: 0"), resGroup);
    m_diplomacyLabel = new QLabel(QStringLiteral("Дипломатия: 0"), resGroup);
    resLayout->addWidget(m_scienceLabel);
    resLayout->addWidget(m_industryLabel);
    resLayout->addWidget(m_tradeLabel);
    resLayout->addWidget(m_diplomacyLabel);
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
