#include "gui/DefeatDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QFrame>

DefeatDialog::DefeatDialog(int turnNumber, int remainingPlayers, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Поражение"));
    resize(420, 320);

    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(QStringLiteral("Поражение"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    auto *message = new QLabel(
        QStringLiteral("Ваши города захвачены. Ваша цивилизация пала."), this);
    message->setWordWrap(true);
    layout->addWidget(message);

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    layout->addWidget(line);

    auto *statsLayout = new QGridLayout();
    statsLayout->addWidget(new QLabel(QStringLiteral("Ход поражения:"), this), 0, 0);
    statsLayout->addWidget(new QLabel(QString::number(turnNumber), this), 0, 1);
    statsLayout->addWidget(new QLabel(QStringLiteral("Игроков осталось:"), this), 1, 0);
    statsLayout->addWidget(new QLabel(QString::number(remainingPlayers), this), 1, 1);

    auto *hint = new QLabel(
        QStringLiteral("Вы можете продолжить наблюдать за партией как наблюдатель."), this);
    hint->setWordWrap(true);
    QFont hintFont = hint->font();
    hintFont.setItalic(true);
    hint->setFont(hintFont);
    layout->addLayout(statsLayout);
    layout->addWidget(hint);
    layout->addStretch();

    auto *buttonsLayout = new QHBoxLayout();
    auto *spectateBtn = new QPushButton(QStringLiteral("Продолжить наблюдать"), this);
    connect(spectateBtn, &QPushButton::clicked, this, [this]()
           {
               emit spectateRequested();
               accept();
           });
    auto *newGameBtn = new QPushButton(QStringLiteral("Новая игра"), this);
    connect(newGameBtn, &QPushButton::clicked, this, [this]()
           {
               emit newGameRequested();
               accept();
           });
    auto *exitBtn = new QPushButton(QStringLiteral("Выйти"), this);
    connect(exitBtn, &QPushButton::clicked, this, [this]()
           {
               emit exitRequested();
               accept();
           });
    buttonsLayout->addWidget(spectateBtn);
    buttonsLayout->addWidget(newGameBtn);
    buttonsLayout->addWidget(exitBtn);
    layout->addLayout(buttonsLayout);
}
