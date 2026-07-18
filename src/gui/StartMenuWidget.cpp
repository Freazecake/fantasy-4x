#include "gui/StartMenuWidget.hpp"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QKeyEvent>

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

void StartMenuWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit newGameRequested();
    } else if (event->key() == Qt::Key_Escape) {
        emit exitRequested();
    }
    QWidget::keyPressEvent(event);
}
