#include <QApplication>
#include "FarmController.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    FarmController controller;
    MainWindow window(&controller);
    window.show();

    return app.exec();
}