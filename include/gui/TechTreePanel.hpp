#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

#include "core/Technology.hpp"
#include "core/Player.hpp"

class TechTreePanel : public QWidget {
    Q_OBJECT
public:
    explicit TechTreePanel(QWidget* parent = nullptr);

    void showTechs(const std::vector<Technology>& catalog,
                   const std::unordered_set<std::string>& researchedIds,
                   const std::string& currentResearchId);

signals:
    void researchRequested(const std::string& techId);

private slots:
    void onResearchClicked();

private:
    void setupUi();

    QListWidget* m_techList;
    QPushButton* m_researchBtn;
    QLabel* m_currentResearchLabel;

    std::vector<Technology> m_catalog;
    std::unordered_set<std::string> m_researchedIds;
    std::string m_currentResearchId;
};
