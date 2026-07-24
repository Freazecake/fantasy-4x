#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>

#include "core/Army.hpp"

class TurnPanel : public QWidget {
    Q_OBJECT
public:
    explicit TurnPanel(QWidget* parent = nullptr);

    void updateInfo(int turnNumber, const std::string& phase,
                    float science, float industry, float trade, float diplomacy);
    void updateArmyList(const std::vector<Army>& armies);

signals:
    void endTurnClicked();
    void armySelected(const std::string& armyId);

private:
    void setupUi();

    QLabel* m_turnLabel;
    QLabel* m_phaseLabel;
    QLabel* m_scienceLabel;
    QLabel* m_industryLabel;
    QLabel* m_tradeLabel;
    QLabel* m_diplomacyLabel;
    QListWidget* m_armyList;
    QPushButton* m_endTurnBtn;
};
