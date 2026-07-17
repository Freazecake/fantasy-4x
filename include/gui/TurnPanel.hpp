#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class TurnPanel : public QWidget {
    Q_OBJECT
public:
    explicit TurnPanel(QWidget* parent = nullptr);

    void updateInfo(int turnNumber, const std::string& phase,
                    float science, float industry, float trade, float diplomacy);

signals:
    void endTurnClicked();

private:
    void setupUi();

    QLabel* m_turnLabel;
    QLabel* m_phaseLabel;
    QLabel* m_scienceLabel;
    QLabel* m_industryLabel;
    QLabel* m_tradeLabel;
    QLabel* m_diplomacyLabel;
    QPushButton* m_endTurnBtn;
};
