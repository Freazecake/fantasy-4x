#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>

#include "core/Settlement.hpp"

class TurnPanel : public QWidget {
    Q_OBJECT
public:
    explicit TurnPanel(QWidget* parent = nullptr);

    void updateInfo(int turnNumber, const std::string& phase,
                    float science, float industry, float trade, float diplomacy);
    // Список своих городов с подсветкой текущего выбранного (по anchorCoords()).
    void updateSettlementList(const std::vector<Settlement>& settlements, const Coords& selectedAnchor);

signals:
    void endTurnClicked();
    void settlementSelected(int q, int r);

private:
    void setupUi();

    QLabel* m_turnLabel;
    QLabel* m_phaseLabel;
    QLabel* m_scienceLabel;
    QLabel* m_industryLabel;
    QLabel* m_tradeLabel;
    QLabel* m_diplomacyLabel;
    QListWidget* m_settlementList;
    QPushButton* m_endTurnBtn;
};
