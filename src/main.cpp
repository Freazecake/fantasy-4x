#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include "gui/MainWindow.hpp"

static void loadPixelFonts() {
    QFontDatabase::addApplicationFont(QString(ASSETS_DIR) + "/fonts/PressStart2P-Regular.ttf");
    QFontDatabase::addApplicationFont(QString(ASSETS_DIR) + "/fonts/VT323-Regular.ttf");
}

static QString loadStyleSheet() {
    QFile f(QString(ASSETS_DIR) + "/style/theme.qss");
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    loadPixelFonts();
    app.setStyleSheet(loadStyleSheet());

    MainWindow window;
    window.show();
    return app.exec();
}
