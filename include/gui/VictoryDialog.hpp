#pragma once

#include <QDialog>

#include "core/GameState.hpp"
#include "core/Player.hpp"

class VictoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VictoryDialog(const VictoryResult &result, const Player &winner, int turnNumber,
                           QWidget *parent = nullptr);

signals:
    void newGameRequested();
    void exitRequested();
};
