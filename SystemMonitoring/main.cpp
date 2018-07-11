#include "monitoringwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MonitoringWidget w;
    w.show();

    return a.exec();
}
