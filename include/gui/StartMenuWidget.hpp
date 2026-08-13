#pragma once
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>

#include "core/GameEngine.hpp"

class StartMenuWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StartMenuWidget(QWidget *parent = nullptr);

    GameStartOptions selectedOptions() const;

signals:
    void newGameRequested();
    void exitRequested();

private:
    void setupUi();

    QComboBox *m_raceCombo;
    QSpinBox *m_botsSpin;
    QComboBox *m_mapSizeCombo;

protected:
    void keyPressEvent(QKeyEvent *event) override;
};
