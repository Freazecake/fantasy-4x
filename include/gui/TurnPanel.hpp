#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>

#include "core/Army.hpp"
#include "core/Settlement.hpp"

class TurnPanel : public QWidget {
    Q_OBJECT
public:
    explicit TurnPanel(QWidget* parent = nullptr);

    void updateInfo(int turnNumber, const std::string& phase,
                    float science, float industry, float trade, float diplomacy);
    void updateArmyList(const std::vector<Army>& armies);
    // Список своих городов с подсветкой текущего выбранного (по anchorCoords()).
    void updateSettlementList(const std::vector<Settlement>& settlements, const Coords& selectedAnchor);
    // Показывает/скрывает кнопку "Основать город" — видна только когда
    // выбрана армия, состоящая целиком из поселенца.
    void setFoundSettlementAvailable(bool available);

signals:
    void endTurnClicked();
    void armySelected(const std::string& armyId);
    void settlementSelected(int q, int r);
    void foundSettlementRequested();

private:
    void setupUi();

    QLabel* m_turnLabel;
    QLabel* m_phaseLabel;
    QLabel* m_scienceLabel;
    QLabel* m_industryLabel;
    QLabel* m_tradeLabel;
    QLabel* m_diplomacyLabel;
    QListWidget* m_settlementList;
    QListWidget* m_armyList;
    QPushButton* m_foundSettlementBtn;
    QPushButton* m_endTurnBtn;
};
