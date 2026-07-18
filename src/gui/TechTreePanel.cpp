#include "gui/TechTreePanel.hpp"
#include <QProgressBar>
#include <QHBoxLayout>
#include <map>

static QString categorySymbol(const std::string &cat)
{
    QString lower = QString::fromStdString(cat).toLower();
    if (lower == "military" || lower == "война" || lower == "военное")
        return QStringLiteral("[MI]");
    if (lower == "economy" || lower == "экономика" || lower == "экономическое")
        return QStringLiteral("[GA]");
    if (lower == "trade" || lower == "торговля" || lower == "культурное")
        return QStringLiteral("[TR]");
    if (lower == "science" || lower == "наука" || lower == "научное")
        return QStringLiteral("[SC]");
    if (lower == "industry" || lower == "промышленность")
        return QStringLiteral("[IND]");
    if (lower == "diplomacy" || lower == "дипломатия")
        return QStringLiteral("[DIP]");
    return QStringLiteral("[?]");
}

static QColor categoryColor(const std::string &cat)
{
    QString lower = QString::fromStdString(cat).toLower();
    if (lower == "military" || lower == "война" || lower == "военное")
        return QColor(180, 80, 80);
    if (lower == "economy" || lower == "экономика" || lower == "экономическое")
        return QColor(80, 160, 80);
    if (lower == "trade" || lower == "торговля" || lower == "культурное")
        return QColor(180, 120, 200);
    if (lower == "science" || lower == "наука" || lower == "научное")
        return QColor(80, 140, 200);
    if (lower == "industry" || lower == "промышленность")
        return QColor(200, 160, 60);
    if (lower == "diplomacy" || lower == "дипломатия")
        return QColor(100, 180, 180);
    return QColor(212, 175, 55);
}

TechTreePanel::TechTreePanel(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void TechTreePanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    m_currentResearchLabel = new QLabel(QStringLiteral("Текущее исследование: —"), this);
    layout->addWidget(m_currentResearchLabel);

    m_researchProgress = new QProgressBar(this);
    m_researchProgress->setObjectName("techResearchProgress");
    m_researchProgress->setRange(0, 100);
    m_researchProgress->setValue(0);
    m_researchProgress->setVisible(false);
    layout->addWidget(m_researchProgress);

    m_techList = new QListWidget(this);
    layout->addWidget(m_techList);

    m_researchBtn = new QPushButton(QStringLiteral("Исследовать"), this);
    connect(m_researchBtn, &QPushButton::clicked, this, &TechTreePanel::onResearchClicked);
    layout->addWidget(m_researchBtn);
}

void TechTreePanel::showTechs(const std::vector<Technology> &catalog,
                              const std::unordered_set<std::string> &researchedIds,
                              const std::string &currentResearchId)
{
    m_catalog = catalog;
    m_researchedIds = researchedIds;
    m_currentResearchId = currentResearchId;

    m_techList->clear();

    if (!currentResearchId.empty())
    {
        for (auto &t : m_catalog)
        {
            if (t.getId() == currentResearchId)
            {
                m_currentResearchLabel->setText(
                    QStringLiteral("Исследуется: %1")
                        .arg(QString::fromStdString(t.getName())));
                m_researchProgress->setValue(static_cast<int>(t.progress() * 100));
                m_researchProgress->setFormat(
                    QStringLiteral("%1%").arg(static_cast<int>(t.progress() * 100)));
                m_researchProgress->setVisible(true);
                break;
            }
        }
    }
    else
    {
        m_currentResearchLabel->setText(QStringLiteral("Текущее исследование: —"));
        m_researchProgress->setVisible(false);
    }

    std::map<std::string, std::vector<Technology>> byCategory;
    for (auto &t : m_catalog)
        byCategory[t.getCategory()].push_back(t);

    for (auto &[category, techs] : byCategory)
    {
        QString headerText = categorySymbol(category) + QStringLiteral(" %1 ").arg(QString::fromStdString(category)) + categorySymbol(category);

        auto *header = new QListWidgetItem(headerText, m_techList);
        header->setFlags(Qt::NoItemFlags);
        header->setForeground(categoryColor(category));
        QFont headerFont = header->font();
        headerFont.setBold(true);
        header->setFont(headerFont);

        for (auto &tech : techs)
        {
            bool researched = researchedIds.count(tech.getId()) > 0;
            bool canResearch = !researched && tech.canResearch(researchedIds);

            QString prefix;
            if (researched)
                prefix = QStringLiteral("[+] ");
            else if (canResearch)
                prefix = QStringLiteral("[ ] ");
            else
                prefix = QStringLiteral("[X] ");

            QString text = prefix + QString::fromStdString(tech.getName()) + QStringLiteral(" (%1)").arg(QString::fromStdString(tech.getCategory())) + QStringLiteral(" — %1 ед.").arg(tech.getCost());

            if (!researched && !canResearch)
            {
                QStringList missing;
                for (auto &reqId : tech.getPrerequisiteIds())
                {
                    if (m_researchedIds.count(reqId) == 0)
                    {
                        for (auto &t2 : m_catalog)
                        {
                            if (t2.getId() == reqId)
                            {
                                missing << QString::fromStdString(t2.getName());
                                break;
                            }
                        }
                    }
                }
                if (!missing.isEmpty())
                {
                    text += QStringLiteral(" (нужно: %1)").arg(missing.join(", "));
                }
            }

            auto *item = new QListWidgetItem(text, m_techList);
            item->setData(Qt::UserRole, QString::fromStdString(tech.getId()));
            if (researched)
                item->setForeground(QColor(74, 160, 74));
            else if (!canResearch)
                item->setForeground(QColor(160, 80, 80));
        }
    }
}

void TechTreePanel::onResearchClicked()
{
    auto selected = m_techList->currentItem();
    if (!selected)
        return;

    std::string techId = selected->data(Qt::UserRole).toString().toStdString();

    for (auto &t : m_catalog)
    {
        if (t.getId() == techId)
        {
            if (m_researchedIds.count(techId) > 0)
                return;
            if (!t.canResearch(m_researchedIds))
                return;
            break;
        }
    }

    emit researchRequested(techId);
}
