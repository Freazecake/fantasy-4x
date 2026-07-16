#include "gui/MainWindow.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    QWidget *widget = new QWidget(this);
    resize(500, 500);
}

MainWindow::~MainWindow() {}