#include <QApplication>
#include "friday.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Friday ai;
    ai.show();
    return app.exec();
}
