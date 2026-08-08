#include "gui/VictoryDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QFrame>

VictoryDialog::VictoryDialog(const VictoryResult &result, const Player &winner, int turnNumber,
                             QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Победа!"));
    resize(420, 380);

    auto *layout = new QVBoxLayout(this);

    QString typeStr = result.type == "political"
                          ? QStringLiteral("Политическая победа")
                          : QStringLiteral("Военная победа");

    auto *title = new QLabel(
        QStringLiteral("Игрок %1 (%2) побеждает!")
            .arg(winner.id())
            .arg(QString::fromStdString(winner.race().getName())),
        this);
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setWordWrap(true);
    layout->addWidget(title);

    auto *typeLabel = new QLabel(typeStr, this);
    QFont typeFont = typeLabel->font();
    typeFont.setItalic(true);
    typeLabel->setFont(typeFont);
    layout->addWidget(typeLabel);

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    layout->addWidget(line);

    auto *statsLayout = new QGridLayout();
    int row = 0;
    auto addStat = [&](const QString &label, const QString &value)
    {
        statsLayout->addWidget(new QLabel(label, this), row, 0);
        auto *valueLabel = new QLabel(value, this);
        QFont f = valueLabel->font();
        f.setBold(true);
        valueLabel->setFont(f);
        statsLayout->addWidget(valueLabel, row, 1);
        ++row;
    };

    addStat(QStringLiteral("Ход победы:"), QString::number(turnNumber));
    addStat(QStringLiteral("Городов:"), QString::number(winner.settlements().size()));
    addStat(QStringLiteral("Армий:"), QString::number(winner.armies().size()));
    addStat(QStringLiteral("Героев:"), QString::number(winner.heroes().size()));

    const Resources &res = winner.resources();
    addStat(QStringLiteral("Наука:"), QString::number(static_cast<int>(res.science)));
    addStat(QStringLiteral("Промышленность:"), QString::number(static_cast<int>(res.industry)));
    addStat(QStringLiteral("Торговля:"), QString::number(static_cast<int>(res.trade)));
    addStat(QStringLiteral("Дипломатия:"), QString::number(static_cast<int>(res.diplomacy)));

    layout->addLayout(statsLayout);
    layout->addStretch();

    auto *buttonsLayout = new QHBoxLayout();
    auto *newGameBtn = new QPushButton(QStringLiteral("Новая игра"), this);
    connect(newGameBtn, &QPushButton::clicked, this, [this]()
           {
               emit newGameRequested();
               accept();
           });
    auto *exitBtn = new QPushButton(QStringLiteral("Выход"), this);
    connect(exitBtn, &QPushButton::clicked, this, [this]()
           {
               emit exitRequested();
               accept();
           });
    buttonsLayout->addWidget(newGameBtn);
    buttonsLayout->addWidget(exitBtn);
    layout->addLayout(buttonsLayout);
}
