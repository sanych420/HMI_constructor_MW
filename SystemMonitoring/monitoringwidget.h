#ifndef MONITORINGWIDGET_H
#define MONITORINGWIDGET_H

#include <QtWidgets>
#include "windows.h"
#include "psapi.h"
#include "TCHAR.h"
#include "pdh.h"

class MonitoringWidget : public QWidget
{
    Q_OBJECT

public:
    MonitoringWidget(QWidget *parent = 0);
    ~MonitoringWidget();

    void init();

    void getTotalRAM();
    void getRAMUsedTotalInfo();
    void getRAMUsedByCurrentProcessInfo();
    void getCPUUsedTotalInfo();
    void getCPUUsedByCurrentProcessInfo();
    void getUptime();
    void getDiskInfo();
    void getCurrentFolderSize();
    void getCurrentFolderFiles();

private:

    double RAMTotal, RAMUsed, RAMUsedByCurrentProcess;
    double spaceTotal, spaceFree;
    uint uptime_s = 0;
    uint uptime_m = 0;
    uint uptime_h = 0;
    uint uptime_d = 0;
    QString uptime;

private slots:
    void updateAll();

};

#endif // MONITORINGWIDGET_H
