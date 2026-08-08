#include "gui/TurnReportDialog.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QDialogButtonBox>

namespace
{

QString eventTypeLabel(TurnEventType type)
{
    switch (type)
    {
    case TurnEventType::Combat:
        return QStringLiteral("⚔ Бой");
    case TurnEventType::CityCaptured:
        return QStringLiteral("🏰 Захват города");
    case TurnEventType::CityFounded:
        return QStringLiteral("🏗 Основание города");
    case TurnEventType::AllianceFormed:
        return QStringLiteral("🤝 Союз");
    case TurnEventType::Vassalized:
        return QStringLiteral("👑 Вассалитет");
    case TurnEventType::TributeDemanded:
        return QStringLiteral("💰 Дань");
    case TurnEventType::Absorbed:
        return QStringLiteral("⭐ Поглощение");
    case TurnEventType::TradeExchange:
        return QStringLiteral("📦 Торговля");
    case TurnEventType::Victory:
        return QStringLiteral("🏆 Победа");
    }
    return QStringLiteral("Событие");
}

} // namespace

TurnReportDialog::TurnReportDialog(const std::vector<TurnEvent> &events, int turnNumber,
                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Отчёт о ходе %1").arg(turnNumber));
    resize(520, 400);

    auto *layout = new QVBoxLayout(this);

    if (events.empty())
    {
        layout->addWidget(new QLabel(
            QStringLiteral("За этот ход ничего примечательного не произошло."), this));
    }
    else
    {
        layout->addWidget(new QLabel(
            QStringLiteral("Пока вы распределяли ресурсы, произошло:"), this));

        auto *list = new QListWidget(this);
        for (const auto &ev : events)
        {
            QString text = QStringLiteral("%1 — %2")
                                .arg(eventTypeLabel(ev.type))
                                .arg(QString::fromStdString(ev.description));
            auto *item = new QListWidgetItem(text, list);
            item->setToolTip(text);
        }
        layout->addWidget(list);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}
