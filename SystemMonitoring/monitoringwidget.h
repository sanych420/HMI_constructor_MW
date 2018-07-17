#ifndef MONITORINGWIDGET_H
#define MONITORINGWIDGET_H

#include <QtWidgets>
#include <QtConcurrent/QtConcurrent>
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
#include "sys/times.h"
#include "sys/vtimes.h"
#include "unistd.h"
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

    QString RAMTotal, RAMUsed, RAMUsedByCurrentProcess;
    QString spaceTotal, spaceFree;
    QString CPUUsedByCurrentProcess, CPUUsedTotal;
    quint64 folderFiles = 0;
    quint64 folderSize = 0;
    QString folderSizeString;
    uint uptime_s = 0;
    QString uptime;
    QString homePath;

    QTableView* table;
    QStandardItemModel *model;
    QStandardItem *RAMUsedValue, *RAMUsedByCurrentProcessValue, *CPUUsedValue, *CPUUsedByCurrentProcessValue,
    *folderSizeValue, *folderFilesValue, *uptimeValue, *spaceFreeValue;
    QDir dir;

    void getTotalRAM();
    void getRAMUsedTotalInfo();
    void getRAMUsedByCurrentProcessInfo();
    void getUptime();
    void getDiskInfo();

    void getFolderFiles();
    void getFolderSize();

    QString getUserFriendlySize(quint64 bytes);

    bool secondPhase = false;

#ifdef _WIN32
    PDH_HQUERY cpuQueryTotal, cpuQueryCurrentProc;
    PDH_HCOUNTER cpuCounterTotal, cpuCounterCurrentProc;
    QString processName;
    void win32BeginQueryCPUTotal();
    void win32BeginQueryCPUCurrentProc();
    void win32CollectQueryCPUTotal();
    void win32CollectQueryCpuCurrentProc();
#endif


#ifdef __linux__
    unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
    int linuxParseLine(char* line);
    int linuxGetValue();
    void linuxReadProcStats1();
    void linuxReadProcStats2();
    void linuxReadProcStatsForCurrentProc1();
    void linuxReadProcStatsForCurrentProc2();
    double CPUNumbersBefore[4];
    double CPUNumbersAfter[4];
    double CPUProcessNumbersBefore[2];
    double CPUProcessNumbersAfter[2];
    double loadavg;
    double loadavgCurr;
    int totalCPUUsageBefore;
    int totalCPUUsageAfter;
    int processCPUUsageBefore;
    int processCPUUsageAfter;
    clock_t lastCPU, lastSysCPU, lastUserCPU;
    int numProcessors;
    int PID;
    const char* PIDstring;
#endif


private slots:
    void updateAll();
    void updateFolderInfo(const QString &path);

};

#endif // MONITORINGWIDGET_H
