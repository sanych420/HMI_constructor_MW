#include "monitoringwidget.h"
#define MB 1000000

//the following code was partly inspired by:
//https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process

MonitoringWidget::MonitoringWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MonitoringWidget::~MonitoringWidget()
{

}

void MonitoringWidget::init()
{
    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(updateAll()));
    timer->start(1000);

}

void MonitoringWidget::getTotalRAM()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    RAMTotal = totalPhysMem / MB;
}

void MonitoringWidget::getRAMUsedTotalInfo()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    RAMUsed = physMemUsed / MB;
}

void MonitoringWidget::getRAMUsedByCurrentProcessInfo()
{
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
    RAMUsedByCurrentProcess = physMemUsedByMe / MB;
}

void MonitoringWidget::getCPUUsedTotalInfo()
{

}

void MonitoringWidget::getCPUUsedByCurrentProcessInfo()
{

}

void MonitoringWidget::getDiskInfo()
{
    QStorageInfo storage = QStorageInfo(QDir::homePath());
    spaceTotal = storage.bytesTotal() / MB;
    spaceFree = storage.bytesAvailable() / MB;
}

void MonitoringWidget::getCurrentFolderSize()
{

}

void MonitoringWidget::getCurrentFolderFiles()
{

}

void MonitoringWidget::getUptime()
{
    uptime_s++;
    if (uptime_s > 59)
    {
        uptime_s = 0;
        uptime_m++;
        if (uptime_m > 59)
        {
            uptime_m = 0;
            uptime_h++;
            if (uptime_h > 23)
            {
                uptime_h = 0;
                uptime_d++;
            }
        }
    }
    uptime = tr("%1 days, %2 hours, %3 minutes, %4 seconds").arg(uptime_d).arg(uptime_h).arg(uptime_m).arg(uptime_s);
}

void MonitoringWidget::updateAll()
{
    getTotalRAM();
    getRAMUsedTotalInfo();
    getRAMUsedByCurrentProcessInfo();
    getDiskInfo();
    getUptime();
    qDebug() << RAMTotal << RAMUsed << RAMUsedByCurrentProcess << spaceTotal << spaceFree << uptime;
}
