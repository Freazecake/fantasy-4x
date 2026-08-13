#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>

#include "core/Army.hpp"
#include "core/Hero.hpp"

class MilitaryPanel : public QWidget
{
    Q_OBJECT
public:
    explicit MilitaryPanel(QWidget *parent = nullptr);

    void updateArmyList(const std::vector<Army> &armies);
    void updateHeroList(const std::vector<Hero> &heroes, const QString &raceName);
    void setFoundSettlementAvailable(bool available);

signals:
    void armySelected(const std::string &armyId);
    void foundSettlementRequested();

private:
    void setupUi();

    QListWidget *m_heroList;
    QListWidget *m_armyList;
    QPushButton *m_foundSettlementBtn;
};
