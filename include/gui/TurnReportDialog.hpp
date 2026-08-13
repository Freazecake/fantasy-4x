#pragma once

#include <QDialog>
#include <vector>
#include <string>

#include "core/GameState.hpp"

class TurnReportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TurnReportDialog(const std::vector<TurnEvent> &events, int turnNumber,
                              QWidget *parent = nullptr);
};
