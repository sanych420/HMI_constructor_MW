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
    setHomePath("/usr/bin");
#elif _WIN32
    processName = qApp->applicationName();
    PdhOpenQuery(NULL, 0, &cpuQueryTotal);
    PdhOpenQuery(NULL, 0, &cpuQueryCurrentProc);
    setHomePath("C:/Windows");
#endif
    getTotalRAM();
    getRAMUsedTotalInfo();
    getRAMUsedByCurrentProcessInfo();
    getDiskInfo();
    getUptime();

    QTimer *timer = new QTimer(this);
    timer->setSingleShot(false);
    connect(timer,SIGNAL(timeout()),this,SLOT(updateAll()));
    timer->start(1000);


    dir.setPath(homePath);
    updateFolderInfo(homePath);

    QFileSystemWatcher* watcher = new QFileSystemWatcher;
    watcher->addPath(homePath);
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(updateFolderInfo(QString)));

    table = new QTableView(this);
    model = new QStandardItemModel(10, 2, this);
    model->setHorizontalHeaderItem(0, new QStandardItem(tr("Parameter")));
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
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->verticalHeader()->setVisible(false);
    table->resizeColumnToContents(0);
    table->horizontalHeader()->setStretchLastSection(true);

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
    CPUUsedTotal = QString::number(loadavg, 'f', 1);
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

    CPUUsedByCurrentProcess = QString::number(loadavgCurr, 'f', 1);
}
#endif

#ifdef _WIN32
void MonitoringWidget::win32BeginQueryCPUTotal()
{

    PdhAddCounter(cpuQueryTotal, L"\\Processor(_Total)\\% Processor Time", 0, &cpuCounterTotal);
    PdhCollectQueryData(cpuQueryTotal);
}

void MonitoringWidget::win32CollectQueryCPUTotal()
{
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQueryTotal);
    PdhGetFormattedCounterValue(cpuCounterTotal, PDH_FMT_DOUBLE, NULL, &counterVal);

    CPUUsedTotal = QString::number(counterVal.doubleValue, 'f', 1);
}

void MonitoringWidget::win32BeginQueryCPUCurrentProc()
{

    PdhAddCounterA(cpuQueryCurrentProc, QString("\\Process(%1)\\% Processor Time").arg(processName).toStdString().c_str(),
                   0, &cpuCounterCurrentProc);
    PdhCollectQueryData(cpuQueryCurrentProc);
}

void MonitoringWidget::win32CollectQueryCpuCurrentProc()
{
    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQueryCurrentProc);
    PdhGetFormattedCounterValue(cpuCounterCurrentProc, PDH_FMT_DOUBLE, NULL, &counterVal);

    CPUUsedByCurrentProcess = QString::number(counterVal.doubleValue, 'f', 1);
}
#endif

void MonitoringWidget::getDiskInfo()
{
    QStorageInfo storage = QStorageInfo(homePath);
    spaceTotal = getUserFriendlySize(storage.bytesTotal());
    spaceFree = getUserFriendlySize(storage.bytesAvailable());
}

void MonitoringWidget::getFolderFiles()
{
    QDirIterator it(homePath, QDir::Files, QDirIterator::Subdirectories);
    folderFiles = 0;
    while (it.hasNext())
    {
        it.next();
        if (it.fileInfo().isFile())
        {
            folderFiles++;
        }
    }
}

void MonitoringWidget::getFolderSize()
{
    QDirIterator it(homePath, QDir::Files, QDirIterator::Subdirectories);
    folderSize = 0;
    while (it.hasNext())
    {
        it.next();
        if (it.fileInfo().isFile())
        {
            folderSize += it.fileInfo().size();
        }
        folderSizeString = getUserFriendlySize(folderSize);
    }
}

void MonitoringWidget::getUptime()
{
    uptime_s++;
    uptime = QDateTime::fromTime_t(uptime_s).toUTC().toString("hh:mm:ss");
}

void MonitoringWidget::updateAll()
{
    getTotalRAM();
    getRAMUsedTotalInfo();
    getRAMUsedByCurrentProcessInfo();
    getDiskInfo();
    getUptime();
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

QString MonitoringWidget::getUserFriendlySize(quint64 bytes)
{
    QString number;

    if(bytes < 0x400) //If less than 1 KB, report in B
    {
        number = QLocale::system().toString(bytes);
        number.append(" B");
        return number;
    }
    else
    {
        if(bytes >= 0x400 && bytes < 0x100000) //If less than 1 MB, report in KB, unless rounded result is 1024 KB, then report in MB
        {
            qlonglong result = (bytes + (0x400 / 2)) / 0x400;

            if(result < 0x400)
            {
                number = QLocale::system().toString(result);
                number.append(" KB");
                return number;
            }
            else
            {
                qlonglong result = (bytes + (0x100000 / 2)) / 0x100000;
                number = QLocale::system().toString(result);
                number.append(" MB");
                return number;
            }
        }
        else
        {
            if(bytes >= 0x100000 && bytes < 0x40000000) //If less than 1 GB, report in MB, unless rounded result is 1024 MB, then report in GB
            {
                qlonglong result = (bytes + (0x100000 / 2)) / 0x100000;

                if(result < 0x100000)
                {
                    number = QLocale::system().toString(result);
                    number.append(" MB");
                    return number;
                }
                else
                {
                    qlonglong result = (bytes + (0x40000000 / 2)) / 0x40000000;
                    number = QLocale::system().toString(result);
                    number.append(" GB");
                    return number;
                }
            }
            else
            {
                if(bytes >= 0x40000000 && bytes < 0x10000000000) //If less than 1 TB, report in GB, unless rounded result is 1024 GB, then report in TB
                {
                    qlonglong result = (bytes + (0x40000000 / 2)) / 0x40000000;

                    if(result < 0x40000000)
                    {
                        number = QLocale::system().toString(result);
                        number.append(" GB");
                        return number;
                    }
                    else
                    {
                        qlonglong result = (bytes + (0x10000000000 / 2)) / 0x10000000000;
                        number = QLocale::system().toString(result);
                        number.append(" TB");
                        return number;
                    }
                }
                else
                {
                    qlonglong result = (bytes + (0x10000000000 / 2)) / 0x10000000000; //If more than 1 TB, report in TB
                    number = QLocale::system().toString(result);
                    number.append(" TB");
                    return number;
                }
            }
        }
    }
    return number;
}

void MonitoringWidget::updateFolderInfo(const QString &path)
{
    QtConcurrent::run(this, &MonitoringWidget::getFolderFiles);
    QtConcurrent::run(this, &MonitoringWidget::getFolderSize);
    Q_UNUSED(path);
}
