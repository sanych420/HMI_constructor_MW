#include "monitoringwidget.h"
#define KB 1000
#define MB 1000000
#define GB 1000000000

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

    setHomePath("C:/testFolder/");
    dir.setPath(homePath);

    table = new QTableView(this);
    model = new QStandardItemModel(7, 2, this); //7 Rows and 2 Columns
    model->setHorizontalHeaderItem(0, new QStandardItem(tr("String")));
    model->setHorizontalHeaderItem(1, new QStandardItem(tr("Value")));
    table->setModel(model);

    RAMTotalValue = new QStandardItem(tr("%1").arg(RAMTotal));
    model->setItem(0, 0, new QStandardItem(tr("Total RAM:")));
    model->setItem(0, 1, RAMTotalValue);
    RAMUsedValue = new QStandardItem(tr("%1").arg(RAMUsed));
    model->setItem(1, 0, new QStandardItem(tr("Used RAM:")));
    model->setItem(1, 1, RAMUsedValue);
    RAMUsedByCurrentProcessValue = new QStandardItem(tr("%1").arg(RAMUsedByCurrentProcess));
    model->setItem(2, 0, new QStandardItem(tr("RAM used by this process:")));
    model->setItem(2, 1, RAMUsedByCurrentProcessValue);
    folderFilesValue = new QStandardItem(tr("%1").arg(folderFiles));
    model->setItem(3, 0, new QStandardItem(tr("Files in home folder:")));
    model->setItem(3, 1, folderFilesValue);
    folderSizeValue = new QStandardItem(tr("%1").arg(folderSize));
    model->setItem(4, 0, new QStandardItem(tr("Size of home folder:")));
    model->setItem(4, 1, folderSizeValue);
    uptimeValue = new QStandardItem(uptime);
    model->setItem(5, 0, new QStandardItem(tr("Uptime:")));
    model->setItem(5, 1, uptimeValue);


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

void MonitoringWidget::getCurrentFolderInfo()
{
    QDirIterator it(homePath, QDir::Files, QDirIterator::Subdirectories);
    folderSize = 0;
    folderFiles = 0;
    folderFolders = 0;
    do
    {
        it.next();
        if (it.fileInfo().isFile())
        {
            folderSize += it.fileInfo().size();
            folderFiles++;
        }
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
    uptime = tr("%1d, %2h, %3m, %4s").arg(uptime_d).arg(uptime_h).arg(uptime_m).arg(uptime_s);
}

void MonitoringWidget::updateAll()
{
    getTotalRAM();
    getRAMUsedTotalInfo();
    getRAMUsedByCurrentProcessInfo();
    getDiskInfo();
    getUptime();
    getCurrentFolderInfo();
    getUserFriendlySize(1234);
    qDebug() << "ramtotal" << RAMTotal << "ramused" << RAMUsed  << "ramusedbycurrentprocess" << RAMUsedByCurrentProcess
             << "spacetotal" << spaceTotal << "spacefree" << spaceFree << uptime << "filesinfolder" << folderFiles
             << "foldersize" << folderSize;
    RAMTotalValue->setText(tr("%1").arg(RAMTotal));
    RAMUsedValue->setText(tr("%1").arg(RAMUsed));
    RAMUsedByCurrentProcessValue->setText(tr("%1").arg(RAMUsedByCurrentProcess));
    folderFilesValue->setText(tr("%1").arg(folderFiles));
    folderSizeValue->setText(tr("%1").arg(folderSize));
    uptimeValue->setText(uptime);
}

QString MonitoringWidget::getUserFriendlySize(int size)
{
    // basic size is in bytes
    int sizeLeft, sizeDiv = 0;
    QString newSize = QString("%1").arg(size);
    if (size > GB - 1)
    {
        sizeDiv = size / GB; // GB
        sizeLeft = size % GB;
        qDebug() << sizeDiv << "GB";
        if (sizeLeft > MB - 1)
        {
            sizeDiv = sizeLeft / MB; // MB
            sizeLeft = sizeLeft % MB;
            qDebug() << sizeDiv << "MB";
            if (sizeLeft > KB - 1)
            {
                sizeDiv = sizeLeft / KB; // KB
                sizeLeft = sizeLeft % KB;
                qDebug() << sizeDiv << "KB";
                qDebug() << sizeLeft << "B";
            }
        }
    }
    else if (size > MB - 1)
    {
        sizeDiv = size / MB; // MB
        sizeLeft = size % MB;
        qDebug() << sizeDiv << "MB";
        if (sizeLeft > KB - 1)
        {
            sizeDiv = sizeLeft / KB; // KB
            sizeLeft = sizeLeft % KB;
            qDebug() << sizeDiv << "KB";
            qDebug() << sizeLeft << "B";
        }
    }
    else if (size > KB - 1)
    {
        sizeDiv = size / KB; // KB
        sizeLeft = size % KB;
        qDebug() << sizeDiv << "KB";
        qDebug() << sizeLeft << "B";
    }
    return newSize;
}
