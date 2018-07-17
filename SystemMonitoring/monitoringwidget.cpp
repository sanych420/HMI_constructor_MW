#include "monitoringwidget.h"
#include "iostream"
#define KB 1024
#define MB 1048576
#define GB 1073741824

//the following code was heavily inspired by:
//https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
//and a couple of others SO pages

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
#ifdef __linux__
    PID = getpid();
#elif _WIN32
    processName = qApp->applicationName();
#endif
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(false);
    connect(timer,SIGNAL(timeout()),this,SLOT(updateAll()));
    timer->start(1000);

    setHomePath("C:/Windows/System32/drivers/etc");
    dir.setPath(homePath);

    table = new QTableView(this);
    model = new QStandardItemModel(10, 2, this);
    model->setHorizontalHeaderItem(0, new QStandardItem(tr("String")));
    model->setHorizontalHeaderItem(1, new QStandardItem(tr("Value")));
    table->setModel(model);

    getTotalRAM();
    model->setItem(0, 0, new QStandardItem(tr("Total RAM:")));
    model->setItem(0, 1, new QStandardItem(RAMTotal));

    RAMUsedValue = new QStandardItem(RAMUsed);
    model->setItem(1, 0, new QStandardItem(tr("Used RAM:")));
    model->setItem(1, 1, RAMUsedValue);

    RAMUsedByCurrentProcessValue = new QStandardItem(RAMUsedByCurrentProcess);
    model->setItem(2, 0, new QStandardItem(tr("RAM used by this process:")));
    model->setItem(2, 1, RAMUsedByCurrentProcessValue);

    folderFilesValue = new QStandardItem(tr("%1").arg(folderFiles));
    model->setItem(3, 0, new QStandardItem(tr("Files in home folder:")));
    model->setItem(3, 1, folderFilesValue);

    folderSizeValue = new QStandardItem(folderSizeString);
    model->setItem(4, 0, new QStandardItem(tr("Size of home folder:")));
    model->setItem(4, 1, folderSizeValue);

    uptimeValue = new QStandardItem(uptime);
    model->setItem(5, 0, new QStandardItem(tr("Uptime:")));
    model->setItem(5, 1, uptimeValue);

    getDiskInfo();
    model->setItem(6, 0, new QStandardItem(tr("Total disk space:")));
    model->setItem(6, 1, new QStandardItem(spaceTotal));

    spaceFreeValue = new QStandardItem(spaceFree);
    model->setItem(7, 0, new QStandardItem(tr("Free disk space:")));
    model->setItem(7, 1, spaceFreeValue);

    CPUUsedByCurrentProcessValue = new QStandardItem(CPUUsedByCurrentProcess);
    model->setItem(8, 0, new QStandardItem(tr("CPU Usage:")));
    model->setItem(8, 1, CPUUsedByCurrentProcessValue);

    CPUUsedValue = new QStandardItem(CPUUsedTotal);
    model->setItem(9, 0, new QStandardItem(tr("CPU total usage:")));
    model->setItem(9, 1, CPUUsedValue);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(table);
    QWidget* dummyWidget = new QWidget(this); // for future graphics
    layout->addWidget(dummyWidget);
    setLayout(layout);
    table->resizeColumnsToContents();

}

void MonitoringWidget::getTotalRAM()
{
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    RAMTotal = getUserFriendlySize(totalPhysMem);
#elif __linux__
    struct sysinfo memInfo;

    sysinfo (&memInfo);
    long long totalPhysMem = memInfo.totalram;
    //Multiply in next statement to avoid int overflow on right hand side...
    totalPhysMem *= memInfo.mem_unit;
    RAMTotal = getUserFriendlySize(totalPhysMem);
#endif
}

void MonitoringWidget::getRAMUsedTotalInfo()
{
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    RAMUsed = getUserFriendlySize(physMemUsed);
#elif __linux__
    struct sysinfo memInfo;

    sysinfo (&memInfo);
    long long physMemUsed = memInfo.totalram - memInfo.freeram;
    //Multiply in next statement to avoid int overflow on right hand side...
    physMemUsed *= memInfo.mem_unit;
    RAMUsed = getUserFriendlySize(physMemUsed);
#endif
}

void MonitoringWidget::getRAMUsedByCurrentProcessInfo()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
    RAMUsedByCurrentProcess = getUserFriendlySize(physMemUsedByMe);
#elif __linux__
    RAMUsedByCurrentProcess = getUserFriendlySize(linuxGetValue());
#endif
}

#ifdef __linux__
int MonitoringWidget::linuxParseLine(char *line)
{
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
    Q_UNUSED(line);
}

int MonitoringWidget::linuxGetValue() // in KB
{
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = linuxParseLine(line) * KB;
            break;
        }
    }
    fclose(file);
    return result;
}

void MonitoringWidget::linuxReadProcStats1() // reading proc/stat in two turns, then calculating the delta and getting an average CPU usage
{
    FILE *fp;
    fp = fopen("/proc/stat","r");
    fscanf(fp,"%*s %lf %lf %lf %lf", &CPUNumbersBefore[0], &CPUNumbersBefore[1],
            &CPUNumbersBefore[2], &CPUNumbersBefore[3]);
    fclose(fp);
}

void MonitoringWidget::linuxReadProcStats2()
{
    FILE *fp;
    fp = fopen("/proc/stat","r");
    fscanf(fp,"%*s %lf %lf %lf %lf", &CPUNumbersAfter[0], &CPUNumbersAfter[1],
            &CPUNumbersAfter[2], &CPUNumbersAfter[3]);
    fclose(fp);

    totalCPUUsageBefore = CPUNumbersBefore[0]+CPUNumbersBefore[1]+CPUNumbersBefore[2]+CPUNumbersBefore[3];
    totalCPUUsageAfter = CPUNumbersAfter[0]+CPUNumbersAfter[1]+CPUNumbersAfter[2]+CPUNumbersAfter[3];

    loadavg = 100 * ((CPUNumbersAfter[0]+CPUNumbersAfter[1]+CPUNumbersAfter[2]) -
            (CPUNumbersBefore[0]+CPUNumbersBefore[1]+CPUNumbersBefore[2])) /
            ((CPUNumbersAfter[0]+CPUNumbersAfter[1]+CPUNumbersAfter[2]+CPUNumbersAfter[3]) -
            (CPUNumbersBefore[0]+CPUNumbersBefore[1]+CPUNumbersBefore[2]+CPUNumbersBefore[3]));
    CPUUsedTotal = QString::number(loadavg);
}


void MonitoringWidget::linuxReadProcStatsForCurrentProc1() // same thing as with the total CPU info, but for a certain process, using current process' PID
{
    PIDstring = QString("/proc/%1/stat").arg(PID).toStdString().c_str();
    FILE *fp;
    fp = fopen(PIDstring,"r");

    fscanf(fp,
           "%*d %*s %*c %*d" //pid, command, state, ppid

           "%*d %*d %*d %*d %*u %*u %*u %*u %*u"

           "%lf %lf" //usertime,

           "%*d %*d %*d %*d %*d %*d %*u"

           "%*u", //virtual memory size in bytes
           &CPUProcessNumbersBefore[0], &CPUProcessNumbersBefore[1]);
    fclose(fp);
}

void MonitoringWidget::linuxReadProcStatsForCurrentProc2()
{
    PIDstring = QString("/proc/%1/stat").arg(PID).toStdString().c_str();
    FILE *fp;
    fp = fopen(PIDstring,"r");
    fscanf(fp,
           "%*d %*s %*c %*d" //pid, command, state, ppid

           "%*d %*d %*d %*d %*u %*u %*u %*u %*u"

           "%lf %lf" //usertime, systemtime

           "%*d %*d %*d %*d %*d %*d %*u"

           "%*u", //virtual memory size in bytes
           &CPUProcessNumbersAfter[0], &CPUProcessNumbersAfter[1]);
    fclose(fp);
    processCPUUsageBefore = CPUProcessNumbersBefore[0] + CPUProcessNumbersBefore[1];
    processCPUUsageAfter = CPUProcessNumbersAfter[0] + CPUProcessNumbersAfter[1];

    loadavgCurr = (processCPUUsageAfter - processCPUUsageBefore) * 100 / (float) (totalCPUUsageAfter - totalCPUUsageBefore);

    CPUUsedByCurrentProcess = QString::number(loadavgCurr);
}
#endif

#ifdef _WIN32
void MonitoringWidget::win32BeginQueryCPUTotal()
{
    PdhOpenQuery(NULL, NULL, &cpuQueryTotal);
    PdhAddCounter(cpuQueryTotal, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuCounterTotal);
    PdhCollectQueryData(cpuQueryTotal);
}

void MonitoringWidget::win32CollectQueryCPUTotal()
{
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQueryTotal);
    PdhGetFormattedCounterValue(cpuCounterTotal, PDH_FMT_DOUBLE, NULL, &counterVal);

    CPUUsedTotal = QString::number(counterVal.doubleValue);
}

void MonitoringWidget::win32BeginQueryCPUCurrentProc()
{
    PdhOpenQuery(NULL, NULL, &cpuQueryCurrentProc);
    PdhAddCounterA(cpuQueryCurrentProc, QString("\\Process(%1)\\% Processor Time").arg(processName).toStdString().c_str(),
                   NULL, &cpuCounterCurrentProc);
    PdhCollectQueryData(cpuQueryCurrentProc);
}

void MonitoringWidget::win32CollectQueryCpuCurrentProc()
{
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQueryCurrentProc);
    PdhGetFormattedCounterValue(cpuCounterCurrentProc, PDH_FMT_DOUBLE, NULL, &counterVal);

    CPUUsedByCurrentProcess = QString::number(counterVal.doubleValue);
}
#endif

void MonitoringWidget::getDiskInfo()
{
    QStorageInfo storage = QStorageInfo(QDir::homePath());
    spaceTotal = getUserFriendlySize(storage.bytesTotal());
    spaceFree = getUserFriendlySize(storage.bytesAvailable());
}

void MonitoringWidget::getCurrentFolderInfo()
{
    QDirIterator it(homePath, QDir::Files, QDirIterator::Subdirectories);
    folderSize = 0;
    folderFiles = 0;
    do
    {
        it.next();
        if (it.fileInfo().isFile())
        {
            folderSize += it.fileInfo().size();
            folderFiles++;
        }
        folderSizeString = getUserFriendlySize(folderSize);
    }
    while (it.hasNext());
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
    if (uptime_d > 0)
    {
        if (uptime_h > 0)
        {
            if (uptime_m > 0)
            {
                if (uptime_s > 0)
                {
                    uptime = tr("%1d, %2h, %3m, %4s").arg(uptime_d).arg(uptime_h).arg(uptime_m).arg(uptime_s);
                }
                else
                {
                    uptime = tr("%1d, %2h, %3m").arg(uptime_d).arg(uptime_h).arg(uptime_m);
                }
            }
            else
            {
                if (uptime_s > 0)
                {
                    uptime = tr("%1d, %2h, %3s").arg(uptime_d).arg(uptime_h).arg(uptime_s);
                }
                else
                {
                    uptime = tr("%1d, %2h").arg(uptime_d).arg(uptime_h);
                }
            }
        }
        else
        {
            if (uptime_m > 0)
            {
                if (uptime_s > 0)
                {
                    uptime = tr("%1d, %2m, %3s").arg(uptime_d).arg(uptime_m).arg(uptime_s);
                }
                else
                {
                    uptime = tr("%1d, %3m").arg(uptime_d).arg(uptime_m);
                }
            }
            else
            {
                if (uptime_s > 0)
                {
                    uptime = tr("%1d, %2s").arg(uptime_d).arg(uptime_s);
                }
                else
                {
                    uptime = tr("%1d").arg(uptime_d);
                }
            }
        }
    }
    else
    {
        if (uptime_h > 0)
        {
            if (uptime_m > 0)
            {
                if (uptime_s > 0)
                {
                    uptime = tr("%1h, %2m, %3s").arg(uptime_h).arg(uptime_m).arg(uptime_s);
                }
                else
                {
                    uptime = tr("%1h, %2m").arg(uptime_h).arg(uptime_m);
                }
            }
            else
            {
                if (uptime_s > 0)
                {
                    uptime = tr("%1h, %2s").arg(uptime_h).arg(uptime_s);
                }
                else
                {
                    uptime = tr("%1h").arg(uptime_h);
                }
            }
        }
        else
        {
            if (uptime_m > 0)
            {
                if (uptime_s > 0)
                {
                    uptime = tr("%1m, %2s").arg(uptime_m).arg(uptime_s);
                }
                else
                {
                    uptime = tr("%1m").arg(uptime_m);
                }
            }
            else
            {
                if (uptime_s > 0)
                {
                    uptime = tr("%1s").arg(uptime_s);
                }
                else
                {
                    uptime = tr("zero");
                }
            }
        }
    }
}

void MonitoringWidget::updateAll()
{
    getTotalRAM();
    getRAMUsedTotalInfo();
    getRAMUsedByCurrentProcessInfo();
    getDiskInfo();
    getUptime();
    getCurrentFolderInfo();
    if (!secondPhase) // while other functions are called using QTimer with 1000 ms delay,
        //these should be called every 1000 ms each, i.e. information will be updating 2x slower
    {
#ifdef __linux__
        linuxReadProcStats1();
        linuxReadProcStatsForCurrentProc1();
#elif _WIN32
        win32BeginQueryCPUTotal();
        win32BeginQueryCPUCurrentProc();
#endif
        secondPhase = true;
    }
    else
    {
#ifdef __linux__
        linuxReadProcStats2();
        linuxReadProcStatsForCurrentProc2();
#elif _WIN32
        win32CollectQueryCPUTotal();
        win32CollectQueryCpuCurrentProc();
#endif
        secondPhase = false;
    }
    RAMUsedValue->setText(RAMUsed);
    RAMUsedByCurrentProcessValue->setText(RAMUsedByCurrentProcess);
    folderFilesValue->setText(tr("%1").arg(folderFiles));
    folderSizeValue->setText(folderSizeString);
    uptimeValue->setText(uptime);
    spaceFreeValue->setText(spaceFree);
    CPUUsedByCurrentProcessValue->setText(CPUUsedByCurrentProcess + "%");
    CPUUsedValue->setText(CPUUsedTotal + "%");
}

QString MonitoringWidget::getUserFriendlySize(qint64 size)
{
    // "size" should be in bytes
    int sizeLeft = 0;
    int sizeGB = 0;
    int sizeMB = 0;
    int sizeKB = 0;
    QString newSize = QString("%1").arg(size);
    if (size > GB - 1)
    {
        sizeGB = size / GB; // GB
        sizeLeft = size % GB;
        if (sizeLeft > MB - 1)
        {
            sizeMB = sizeLeft / MB; // MB
            sizeLeft = sizeLeft % MB;
            if (sizeLeft > KB - 1)
            {
                sizeKB = sizeLeft / KB; // KB
                sizeLeft = sizeLeft % KB; //B
            }
        }
    }
    else if (size > MB - 1)
    {
        sizeMB = size / MB; // MB
        sizeLeft = size % MB;
        if (sizeLeft > KB - 1)
        {
            sizeKB = sizeLeft / KB; // KB
            sizeLeft = sizeLeft % KB; //B
        }
    }
    else if (size > KB - 1)
    {
        sizeKB = size / KB; // KB
        sizeLeft = size % KB; //B
    }
    else if (size <= KB - 1)
    {
        sizeLeft = size; //B
    }
    if (sizeGB > 0)
    {
        if (sizeMB > 0)
        {
            if (sizeKB > 0)
            {
                if (sizeLeft > 0)
                {
                    newSize = tr("%1 GB, %2 MB, %3 KB, %4 B").arg(sizeGB).arg(sizeMB).arg(sizeKB).arg(sizeLeft);
                }
                else
                {
                    newSize = tr("%1 GB, %2 MB, %3 KB").arg(sizeGB).arg(sizeMB).arg(sizeKB);
                }
            }
            else
            {
                if (sizeLeft > 0)
                {
                    newSize = tr("%1 GB, %2 MB, %3 B").arg(sizeGB).arg(sizeMB).arg(sizeLeft);
                }
                else
                {
                    newSize = tr("%1 GB, %2 MB").arg(sizeGB).arg(sizeMB);
                }
            }
        }
        else
        {
            if (sizeKB > 0)
            {
                if (sizeLeft > 0)
                {
                    newSize = tr("%1 GB, %2 KB, %3 B").arg(sizeGB).arg(sizeKB).arg(sizeLeft);
                }
                else
                {
                    newSize = tr("%1 GB, %2 KB").arg(sizeGB).arg(sizeKB);
                }
            }
            else
            {
                if (sizeLeft > 0)
                {
                    newSize = tr("%1 GB, %2 B").arg(sizeGB).arg(sizeLeft);
                }
                else
                {
                    newSize = tr("%1 GB").arg(sizeGB);
                }
            }
        }
    }
    else if (sizeMB > 0)
    {
        if (sizeKB > 0)
        {
            if (sizeLeft > 0)
            {
                newSize = tr("%1 MB, %2 KB, %3 B").arg(sizeMB).arg(sizeKB).arg(sizeLeft);
            }
            else
            {
                newSize = tr("%1 MB, %2 KB").arg(sizeMB).arg(sizeKB);
            }
        }
        else
        {
            if (sizeLeft > 0)
            {
                newSize = tr("%1 MB, %2 B").arg(sizeMB).arg(sizeLeft);
            }
            else
            {
                newSize = tr("%1 MB").arg(sizeMB);
            }
        }
    }
    else if (sizeKB > 0)
    {
        if (sizeLeft > 0)
        {
            newSize = tr("%1 KB, %2 B").arg(sizeKB).arg(sizeLeft);
        }
        else
        {
            newSize = tr("%1 KB").arg(sizeKB);
        }
    }
    else if (sizeLeft > 0)
    {
        newSize = tr("%1 B").arg(sizeLeft);
    }
    return newSize;
}
