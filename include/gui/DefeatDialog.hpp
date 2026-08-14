#pragma once

#include <QDialog>

class DefeatDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DefeatDialog(int turnNumber, int remainingPlayers, QWidget *parent = nullptr);

signals:
    void newGameRequested();
    void exitRequested();
    void spectateRequested();
};
