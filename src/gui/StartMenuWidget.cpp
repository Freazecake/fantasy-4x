#include "gui/StartMenuWidget.hpp"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QKeyEvent>
#include <QVariant>
#include <QPoint>

StartMenuWidget::StartMenuWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void StartMenuWidget::setupUi() {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(40, 40, 40, 40);

    auto* frame = new QFrame(this);
    frame->setObjectName("startMenuFrame");
    auto* layout = new QVBoxLayout(frame);
    layout->addStretch(2);

    auto* title = new QLabel(QStringLiteral("4X СТРАТЕГИЯ"), frame);
    title->setObjectName("startMenuTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("фэнтезийная пошаговая стратегия"), frame);
    subtitle->setObjectName("startMenuSubtitle");
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    layout->addSpacing(20);

    auto* separator = new QFrame(frame);
    separator->setObjectName("startMenuSeparator");
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(3);
    layout->addWidget(separator);

    layout->addSpacing(40);

    // Блок 3.11: настройки новой партии.
    auto* settingsForm = new QFormLayout();

    m_raceCombo = new QComboBox(frame);
    m_raceCombo->addItem(QStringLiteral("Люди"), QStringLiteral("humans"));
    m_raceCombo->addItem(QStringLiteral("Дворфы"), QStringLiteral("dwarves"));
    m_raceCombo->addItem(QStringLiteral("Гномы"), QStringLiteral("gnomes"));
    m_raceCombo->addItem(QStringLiteral("Эльфы"), QStringLiteral("elves"));
    m_raceCombo->addItem(QStringLiteral("Орки"), QStringLiteral("orcs"));
    settingsForm->addRow(QStringLiteral("Ваша раса:"), m_raceCombo);

    m_botsSpin = new QSpinBox(frame);
    m_botsSpin->setRange(1, 4);
    m_botsSpin->setValue(4);
    settingsForm->addRow(QStringLiteral("Число ботов:"), m_botsSpin);

    m_mapSizeCombo = new QComboBox(frame);
    m_mapSizeCombo->addItem(QStringLiteral("Малая (14x12)"), QVariant::fromValue(QPoint(14, 12)));
    m_mapSizeCombo->addItem(QStringLiteral("Средняя (20x18)"), QVariant::fromValue(QPoint(20, 18)));
    m_mapSizeCombo->addItem(QStringLiteral("Большая (28x24)"), QVariant::fromValue(QPoint(28, 24)));
    m_mapSizeCombo->setCurrentIndex(1);
    settingsForm->addRow(QStringLiteral("Размер карты:"), m_mapSizeCombo);

    auto* settingsRow = new QHBoxLayout();
    settingsRow->addStretch();
    settingsRow->addLayout(settingsForm);
    settingsRow->addStretch();
    layout->addLayout(settingsRow);

    layout->addSpacing(24);

    auto* newGameBtn = new QPushButton(QStringLiteral("Новая игра"), frame);
    newGameBtn->setObjectName("startMenuPrimaryBtn");
    connect(newGameBtn, &QPushButton::clicked, this, &StartMenuWidget::newGameRequested);

    auto* exitBtn = new QPushButton(QStringLiteral("Выход"), frame);
    exitBtn->setObjectName("startMenuPrimaryBtn");
    connect(exitBtn, &QPushButton::clicked, this, &StartMenuWidget::exitRequested);

    auto* btnCol = new QVBoxLayout();
    btnCol->addWidget(newGameBtn);
    btnCol->addSpacing(10);
    btnCol->addWidget(exitBtn);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addLayout(btnCol);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    layout->addStretch(2);

    auto* versionLabel = new QLabel(QStringLiteral("v0.1.0"), frame);
    versionLabel->setObjectName("startMenuVersion");
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    layout->addStretch(1);

    outer->addWidget(frame);
}

GameStartOptions StartMenuWidget::selectedOptions() const {
    GameStartOptions options;
    options.humanRaceId = m_raceCombo->currentData().toString().toStdString();
    options.numBots = m_botsSpin->value();
    QPoint mapSize = m_mapSizeCombo->currentData().value<QPoint>();
    options.mapWidth = mapSize.x();
    options.mapHeight = mapSize.y();
    return options;
}

void StartMenuWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit newGameRequested();
    } else if (event->key() == Qt::Key_Escape) {
        emit exitRequested();
    }
    QWidget::keyPressEvent(event);
}
