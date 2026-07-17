#include "gui/TechTreePanel.hpp"

TechTreePanel::TechTreePanel(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void TechTreePanel::setupUi() {
    auto* layout = new QVBoxLayout(this);

    m_currentResearchLabel = new QLabel(QStringLiteral("Текущее исследование: —"), this);
    layout->addWidget(m_currentResearchLabel);

    m_techList = new QListWidget(this);
    layout->addWidget(m_techList);

    m_researchBtn = new QPushButton(QStringLiteral("Исследовать"), this);
    connect(m_researchBtn, &QPushButton::clicked, this, &TechTreePanel::onResearchClicked);
    layout->addWidget(m_researchBtn);
}

void TechTreePanel::showTechs(const std::vector<Technology>& catalog,
                               const std::unordered_set<std::string>& researchedIds,
                               const std::string& currentResearchId) {
    m_catalog = catalog;
    m_researchedIds = researchedIds;
    m_currentResearchId = currentResearchId;

    m_techList->clear();

    if (!currentResearchId.empty()) {
        for (auto& t : m_catalog) {
            if (t.getId() == currentResearchId) {
                m_currentResearchLabel->setText(
                    QStringLiteral("Исследуется: %1 (%2%)")
                        .arg(QString::fromStdString(t.getName()))
                        .arg(static_cast<int>(t.progress() * 100)));
                break;
            }
        }
    } else {
        m_currentResearchLabel->setText(QStringLiteral("Текущее исследование: —"));
    }

    for (auto& tech : m_catalog) {
        bool researched = researchedIds.count(tech.getId()) > 0;
        bool canResearch = !researched && tech.canResearch(researchedIds);

        QString prefix;
        if (researched)
            prefix = QStringLiteral("[+] ");
        else if (canResearch)
            prefix = QStringLiteral("[ ] ");
        else
            prefix = QStringLiteral("[X] ");

        QString text = prefix + QString::fromStdString(tech.getName())
                       + QStringLiteral(" (%1)")
                           .arg(QString::fromStdString(tech.getCategory()))
                       + QStringLiteral(" — %1 ед.").arg(tech.getCost());

        auto* item = new QListWidgetItem(text, m_techList);
        item->setData(Qt::UserRole, QString::fromStdString(tech.getId()));
        if (researched)
            item->setForeground(QColor(100, 160, 100));
        else if (!canResearch)
            item->setForeground(QColor(160, 100, 100));
    }
}

void TechTreePanel::onResearchClicked() {
    auto selected = m_techList->currentItem();
    if (!selected) return;

    std::string techId = selected->data(Qt::UserRole).toString().toStdString();

    for (auto& t : m_catalog) {
        if (t.getId() == techId) {
            if (m_researchedIds.count(techId) > 0) return;
            if (!t.canResearch(m_researchedIds)) return;
            break;
        }
    }

    emit researchRequested(techId);
}
