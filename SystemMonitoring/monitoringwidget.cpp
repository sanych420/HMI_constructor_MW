#include "monitoringwidget.h"
#define KB 1000
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


    setHomePath(QDir::homePath());

    table = new QTableView(this);
    model = new QStandardItemModel(7,2,this); //7 Rows and 2 Columns
    model->setHorizontalHeaderItem(0, new QStandardItem(tr("String")));
    model->setHorizontalHeaderItem(1, new QStandardItem(tr("Value")));
    table->setModel(model);

    //    RAMstring = new QStandardItem(tr("Total RAM:"));
    //    RAMvalue->setText(tr("%1").arg(RAMTotal));
    //    model->setItem(0,0,RAMstring);
    //    model->setItem(0,1,RAMvalue);

    QHBoxLayout *layout = new QHBoxLayout(this);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(table);
    layout->addStretch(1);
    setLayout(layout);

}

void MonitoringWidget::getTotalRAM()
{
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    RAMTotal = totalPhysMem / MB;
#elif __linux__
    struct sysinfo memInfo;

    sysinfo (&memInfo);
    long long totalPhysMem = memInfo.totalram;
    //Multiply in next statement to avoid int overflow on right hand side...
    totalPhysMem *= memInfo.mem_unit;
    RAMTotal = totalPhysMem / MB;
#endif
}

void MonitoringWidget::getRAMUsedTotalInfo()
{
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    RAMUsed = physMemUsed / MB;
#elif __linux__
    struct sysinfo memInfo;

    sysinfo (&memInfo);
    long long physMemUsed = memInfo.totalram - memInfo.freeram;
    //Multiply in next statement to avoid int overflow on right hand side...
    physMemUsed *= memInfo.mem_unit;
    RAMUsed = physMemUsed / MB;
#endif
}

int MonitoringWidget::linuxParseLine(char *line)
{
#ifdef __linux__
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
#endif
    Q_UNUSED(line);
    return 0;
}

int MonitoringWidget::linuxGetValue() // in KB
{
#ifdef __linux__
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = linuxParseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
#endif
    return 0;
}

void MonitoringWidget::getRAMUsedByCurrentProcessInfo()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
    RAMUsedByCurrentProcess = physMemUsedByMe / MB;
#elif __linux__
    RAMUsedByCurrentProcess = linuxGetValue() / KB;
#endif
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
    QDir directory(homePath);
    QStringList files = directory.entryList(QStringList(),QDir::Files);
    foreach(QString filename, files)
    {
        // todo
        QString path = QFileInfo(filename).canonicalFilePath();
        folderSize += QFileInfo(path).size();
        qDebug() <<  "path" << path;
    }
}

void MonitoringWidget::getCurrentFolderFiles()
{
    folderFiles = dir.count();
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
    getCurrentFolderFiles();
    getCurrentFolderSize();
    qDebug() << "ramtotal" << RAMTotal << "ramused" << RAMUsed  << "ramusedbycurrentprocess" << RAMUsedByCurrentProcess
             << "spacetotal" << spaceTotal << "spacefree" << spaceFree << uptime << "filesinfolder" << folderFiles
             << "foldersize" << folderSize;

}
