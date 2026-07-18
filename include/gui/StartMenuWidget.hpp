#pragma once
#include <QWidget>

class StartMenuWidget : public QWidget {
    Q_OBJECT
public:
    explicit StartMenuWidget(QWidget* parent = nullptr);

signals:
    void newGameRequested();
    void exitRequested();

private:
    void setupUi();

protected:
    void keyPressEvent(QKeyEvent* event) override;
};
