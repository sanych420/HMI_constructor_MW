#ifndef MONITORINGWIDGET_H
#define MONITORINGWIDGET_H

#include <QtWidgets>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#ifdef _WIN32
#include "windows.h"
#include "psapi.h"
#include "TCHAR.h"
#include "pdh.h"
#elif __linux__
#include "sys/types.h"
#include "sys/sysinfo.h"
#endif

class MonitoringWidget : public QWidget
{
    Q_OBJECT

public:
    MonitoringWidget(QWidget *parent = 0);
    ~MonitoringWidget();

    void init();
    void setHomePath(const QString &path) { homePath = path;}

private:

    double RAMTotal, RAMUsed, RAMUsedByCurrentProcess;
    double spaceTotal, spaceFree;
    int folderFiles = 0;
    int folderFolders = 0;
    qreal folderSize = 0;
    uint uptime_s = 0;
    uint uptime_m = 0;
    uint uptime_h = 0;
    uint uptime_d = 0;
    QString uptime;
    QString homePath;

    QTableView* table;
    QStandardItemModel *model;
    QStandardItem *RAMTotalValue, *RAMUsedValue, *RAMUsedByCurrentProcessValue, *CPUUsedValue,
                  *CPUUsedByCurrentProcessValue, *folderSizeValue, *folderFilesValue, *uptimeValue,
                  *spaceTotalValue, *spaceFreeValue;
    QDir dir;

    void getTotalRAM();
    void getRAMUsedTotalInfo();
    void getRAMUsedByCurrentProcessInfo();
    void getCPUUsedTotalInfo();
    void getCPUUsedByCurrentProcessInfo();
    void getUptime();
    void getDiskInfo();
    void getCurrentFolderInfo();

    QString getUserFriendlySize(int size);

    int linuxParseLine(char* line);
    int linuxGetValue();

private slots:
    void updateAll();

};

#endif // MONITORINGWIDGET_H
