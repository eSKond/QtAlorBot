#include <QCoreApplication>
#include "applicationcontroller.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ApplicationController *appController = new ApplicationController();
    int res = a.exec();
    delete appController;
    return res;
}
