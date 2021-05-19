#include "qtlog.h"
#include <QLoggingCategory>
#include <QtCore/qglobal.h>
#include <qlogging.h>
#include <stdio.h>
#include <stdlib.h>
#include <QFile>

#ifdef Q_OS_WIN
#include<windows.h>
#endif

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif

bool fileLine = false;
static quint32 g_max_log_size = 0;
static qint64 logbufsecs = 5;
static bool stop_writing = false;
static bool should_flush = false;
static bool is_to_console = true;
const char*const LogSeverityNames[NUM_SEVERITIES] = {
    "DEBUG","INFO", "WARNING", "ERROR", "FATAL"
};

static QString dump_path;

/**
 * @brief DayHasChanged
 * @param day
 * @return
 * @details 此函数多线程使用不安全，调用前确保加锁互斥
 */
static bool DayHasChanged( qint32 &day)
{
    time_t raw_time;
    struct tm* tm_info;

    time(&raw_time);
    tm_info = localtime(&raw_time);

    if (tm_info->tm_mday != day)
    {
        day = tm_info->tm_mday;
        return true;
    }

    return false;
}

static quint32 MaxLogSize(){
    return (g_max_log_size > 0 ? g_max_log_size : 10);
}

static qint64 CycleClock_Now(){
    QDateTime qdtime;
    return qdtime.currentDateTime().toSecsSinceEpoch();
}

static void GetHostName(std::string* hostname) {
#if defined(Q_OS_LINUX)
    char hostname_[60] = {0};
    int res = gethostname(hostname_,sizeof(hostname_));
    if(res == 0){
        hostname->append(hostname_);
    }
    else{
        *hostname = "(unknown)";
    }
#elif defined(Q_OS_WIN)
    char buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameA(buf, &len)) {
        *hostname = buf;
    } else {
        hostname->clear();
    }
#else
# warning There is no way to retrieve the host name.
    *hostname = "(unknown)";
#endif
}

static bool systemHasStderr()
{
#if defined(Q_OS_WINRT)
    return false; // WinRT has no stderr
#endif
    return true;
}

static bool stderrHasConsoleAttached()
{
    static const bool stderrHasConsoleAttached = []() -> bool {
        if (!systemHasStderr())
            return false;

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
        return GetConsoleWindow();
#elif defined(Q_OS_UNIX)
#ifndef _PATH_TTY
#define _PATH_TTY "/dev/tty"
#endif
        // If we can open /dev/tty, we have a controlling TTY
        QFile device(_PATH_TTY);
        if(device.open(QIODevice::ReadOnly)){
            device.close();
            return true;
        }
        else if (errno == ENOENT || errno == EPERM || errno == ENXIO) {
            // Fall back to isatty for some non-critical errors
            return isatty(STDERR_FILENO);
        } else {
            return false;
        }
#else
        return false; // No way to detect if stderr has a console attached
#endif
    }();
    return stderrHasConsoleAttached;
}

bool shouldLogToStderr()
{
    return stderrHasConsoleAttached();
}

#if defined(QT_BOOTSTRAPPED)
// Boostrapped tools always print to stderr, so no need for alternate sinks
#else

#if defined(Q_OS_UNIX)
static bool unix_message_handler(QtMsgType type,
                                 const QMessageLogContext &context,
                                 const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler
    QString formattedMessage = qFormatLogMessage(type, context, message);

    if (!formattedMessage.isNull()){
        fprintf(stderr, "%s\n", formattedMessage.toLocal8Bit().constData());
        fflush(stderr);
    }
    return true; // Prevent further output to stderr
}
#endif
#ifdef Q_OS_WIN
static bool win_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (shouldLogToStderr())
        return false; // Leave logging up to stderr handler
    QString formattedMessage = qFormatLogMessage(type, context, message);
    formattedMessage.append(QLatin1Char('\n'));
    OutputDebugString(reinterpret_cast<const wchar_t *>(formattedMessage.utf16()));
    return true; // Prevent further output to stderr
}
#endif
#endif // Bootstrap check


// --------------------------------------------------------------------------
static void stderr_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString formattedMessage = qFormatLogMessage(type, context, message);
    // print nothing if message pattern didn't apply / was empty.
    // (still print empty lines, e.g. because message itself was empty)
    if (formattedMessage.isNull())
        return;
    fprintf(stderr, "%s\n", formattedMessage.toLocal8Bit().constData());
    fflush(stderr);
}

class LogFileObject{

public:
    LogFileObject(LogSeverity severity, QString &base_filename);
    LogFileObject(QByteArray category,QString &base_filename);
    ~LogFileObject();
    void setBasename(QString &basename);
    void write(bool flush, QByteArray &msg );
    void flushUnlocked();
    void flush();

private:
    bool base_filename_selected_;
    QString base_filename_;
    QMutex mutex_;
    QFile* file_;
    LogSeverity severity_;

    QByteArray category_;
    quint32 file_length_ = 0;
    quint32 bytes_since_flush_ = 0;
    qint64 next_flush_time_ = 0;

    qint32 day_;

    bool createLogfile(QString &base_filename);
};

class LogDestination{
public:
    static void setCategoryMode(bool mode);
    static void setLogDestination(LogSeverity severity,
                                  QString &pathdir);
    static void setLogDestination(QString &pathdir);

    static void LogToAllLogfiles(LogSeverity severity, QByteArray &msg, QByteArray &category);

    static bool getCategoryMode();

    static void flushAllLogs();

private:
    LogDestination(LogSeverity severity,QString &base_filename);
    LogDestination(QByteArray category,QString &base_filename);
    ~LogDestination();

    /** LogDestination的文件句柄 */
    LogFileObject fileobject_;

    /** 声明LogDestination指针数组 */
    static LogDestination* log_destinations_[NUM_SEVERITIES];

    static QMap<QByteArray,LogDestination*> log_destinations_map_;

    static bool CategoryMode_;

    static QString category_base_filename_;

    /**
     * @brief log_destinations
     * @param severity
     * @return LogDestination指针
     * @details 根据serverity获取对应的LogDestination指针
     */
    static LogDestination* log_destinations(LogSeverity severity);

    static LogDestination* log_destinations(QByteArray &category);

    QMutex logDestination_mutex;

    static void maybeLogToLogfile(LogSeverity severity, QByteArray &msg, QByteArray &category);


};


LogFileObject::LogFileObject(LogSeverity severity, QString &base_filename):
    base_filename_selected_(base_filename.size() != 0),
    base_filename_((base_filename.size() != 0) ? base_filename : QString()),
    file_(nullptr),
    severity_(severity),file_length_(0){
    category_.clear();
    day_=QDate::currentDate().day();
}

LogFileObject::LogFileObject(QByteArray category,QString &base_filename):base_filename_selected_(true),file_(nullptr)
{
    base_filename_ = base_filename;
    category_ = category;
    severity_ = -1;
    day_ = QDate::currentDate().day();
}

LogFileObject::~LogFileObject(){
    if(file_)
        delete file_;
}

void LogFileObject::setBasename(QString &basename){
    base_filename_selected_ = true;
    if (base_filename_ != basename) {
        base_filename_ = basename;
    }
}

void LogFileObject::write(bool flush, QByteArray &msg){
    QMutexLocker locker(&mutex_);

    if(base_filename_selected_&&base_filename_.isEmpty()){
        return;
    }

    if ( (file_length_ >> 20) >= MaxLogSize() || DayHasChanged(day_) ) {
        if (file_){
            file_->close();
            delete file_;
            file_ = nullptr;
        }
        file_length_ = bytes_since_flush_ = 0;
    }

    if(!file_){
        if(base_filename_selected_){
            //设置过base_filename的进行地址创建
            if(!createLogfile(base_filename_)){
                //创建失败
                printf("log file create failed!\r\n");
                printf("%s\r\n",category_.toStdString().c_str());
                return;
            }
        }
        else{
            /** We don't log if the base_name_ is "" */
            return;
        }

        QByteArray file_header_string;
        QTextStream file_header_stream(&file_header_string,QIODevice::Text | QIODevice::WriteOnly);

        std::string hostname_;
        if (hostname_.empty()) {
            GetHostName(&hostname_);
            if (hostname_.empty()) {
                hostname_ = "(unknown)";
            }
        }

        // Write a header message into the log file
        file_header_stream << "Log file created at: "
                           << QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss")<< endl
                           << "Running on machine: "
                           << hostname_.c_str() << endl
                           << "Log line format: [DIWEF]pid hh:mm:ss.zzz ";
        if(fileLine){
            file_header_stream<<"threadid](file:line _function) msg" << endl;
        }
        else{
            file_header_stream<<"threadid] msg" << endl;
        }

        file_header_stream.flush();
        const int header_len = static_cast<int>(file_header_string.size());
        file_->write(file_header_string.data(),header_len);
        file_length_ += static_cast<quint32>(header_len);
        bytes_since_flush_ += static_cast<quint32>(header_len);

    }

    /** 磁盘是否满 */
    if(!stop_writing){
        file_->write(msg);
        /** 判断磁盘是否已满，待完善，默认不会满 */
        bool diskfull = false;   /// todo
        if(diskfull){
            stop_writing = true;
            return;
        }
        else{
            quint32 length = static_cast<quint32>(msg.size());
            file_length_ += length;
            bytes_since_flush_ += length;
        }
    }
    else{
        if ( CycleClock_Now() >= next_flush_time_ )
            stop_writing = false;  /// check to see if disk has free space.
        return;
    }

    if(flush||(bytes_since_flush_ >= 1000000) ||
            ( CycleClock_Now() >= next_flush_time_ ) ){
        flushUnlocked();
    }
}

void LogFileObject::flushUnlocked()
{
    if(file_ != nullptr){
        file_->flush();
        bytes_since_flush_ = 0;
    }

    next_flush_time_ = QDateTime::currentDateTime().toSecsSinceEpoch() + logbufsecs;
}

void LogFileObject::flush()
{
    QMutexLocker locker(&mutex_);
    flushUnlocked();
}

bool LogFileObject::createLogfile(QString &base_filename){
    QString base_filename_ = base_filename;
    /** 分类模式，根据category增加目录 */
    if(LogDestination::getCategoryMode()){
        QList<QByteArray> categoryList = category_.split('.');
        for(auto value : categoryList){
            base_filename_.append(value).append("/");
        }
    }
    /** 普通模式下增加日志分级目录 */
    else{
        base_filename_.append(LogSeverityNames[severity_]).append("/");
    }
    QDir basedir(base_filename_);
    if(!basedir.exists()){
        basedir.mkpath(base_filename_);

    }
    QString base_datefilename;

    base_datefilename = base_filename_;

    // 程序PID
#if defined (Q_OS_WIN)
    QString pid = QString("%1.").arg(_getpid(),8,16,QLatin1Char('0')).toUpper();
#elif defined (Q_OS_LINUX)
    QString pid = QString("%1.").arg(getpid(),8,16,QLatin1Char('0')).toUpper();
#endif
    // 格式说明
    base_datefilename
            .append(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"))
            .append(".")
            .append(pid)
            .append("log");

    file_ = new QFile(base_datefilename);
    if(!file_->open(QIODevice::WriteOnly | QIODevice::Append)){
        delete file_;
        file_ = nullptr;
        return false;
    }
    return true;
}


LogDestination::LogDestination(LogSeverity severity,QString &base_filename):fileobject_(severity,base_filename){

}

LogDestination::LogDestination(QByteArray category,QString &base_filename):fileobject_(category,base_filename)
{

}

LogDestination::~LogDestination(){
    for(int i=0;i<NUM_SEVERITIES;i++){
        if(log_destinations_[i] != nullptr)
            delete log_destinations_[i];
        log_destinations_[i] = nullptr;
    }
    QMap<QByteArray,LogDestination*>::const_iterator i = log_destinations_map_.constBegin();
    while (i != log_destinations_map_.constEnd()) {
        delete log_destinations_map_[i.key()];
        ++i;
    }
    log_destinations_map_.clear();

}

/** 普通模式目标地址存放 */
LogDestination* LogDestination::log_destinations_[NUM_SEVERITIES];

/** 分类模式根据category划分目标地址 */
QMap<QByteArray,LogDestination*> LogDestination::log_destinations_map_;

/** 默认为普通模式 */
bool LogDestination::CategoryMode_ = false;

QString LogDestination::category_base_filename_;

inline LogDestination* LogDestination::log_destinations(LogSeverity severity){
    if(!log_destinations_[severity]){
        QString null;
        log_destinations_[severity] = new LogDestination(severity,null);
    }
    return log_destinations_[severity];
}

LogDestination *LogDestination::log_destinations(QByteArray &category)
{
    if(!log_destinations_map_.contains(category)){
        log_destinations_map_.insert(category,new LogDestination(category,category_base_filename_));
    }
    return log_destinations_map_.value(category);
}

void LogDestination::setCategoryMode(bool mode)
{
    CategoryMode_ = mode;
}

void LogDestination::setLogDestination(LogSeverity severity, QString &pathdir){
    log_destinations(severity)->fileobject_.setBasename(pathdir);
}

void LogDestination::setLogDestination(QString &pathdir)
{
    category_base_filename_ = pathdir;
}


void LogDestination::LogToAllLogfiles(LogSeverity severity, QByteArray &msg, QByteArray &category){
    //    for(int i = severity; i >= 0; --i)
    //        LogDestination::maybeLogToLogfile(i, msg, category);
    LogDestination::maybeLogToLogfile(severity, msg, category);
}

bool LogDestination::getCategoryMode()
{
    return CategoryMode_;
}

void LogDestination::flushAllLogs()
{
    if(LogDestination::CategoryMode_){
        /** 分类模式下遍历map,刷新缓存 */
        QMap<QByteArray,LogDestination*>::const_iterator i = log_destinations_map_.constBegin();
        while (i != log_destinations_map_.constEnd()) {
            i.value()->fileobject_.flush();
            ++i;
        }
    }
    else{
        /** 普通模式遍历日志等级,刷新缓存 */
        for(int i=0;i<5;i++){
            if(log_destinations_[i]){
                log_destinations_[i]->fileobject_.flush();
            }
        }
    }
}

inline void LogDestination::maybeLogToLogfile(LogSeverity severity,QByteArray &msg, QByteArray &category){
    LogDestination* destination;
    if(LogDestination::CategoryMode_)
        destination = log_destinations(category);
    else {
        destination = log_destinations(severity);
    }
    destination->fileobject_.write(should_flush,msg);
}

qtlog::qtlog()
{

}

qtlog::~qtlog(){

}

static void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray category(context.category);

    LogSeverity severity;
    switch(type)
    {
    case QtDebugMsg:
        severity = 0;
        break;

    case QtInfoMsg:
        severity = 1;
        break;

    case QtWarningMsg:
        severity = 2;
        break;

    case QtCriticalMsg:
        severity = 3;
        break;

    case QtFatalMsg:
        severity = 4;
        break;

    }

    QByteArray message;

    /** 打印到控制台 */

    if(is_to_console){
        bool handledStderr = false;
#if !defined(QT_BOOTSTRAPPED)
#if defined(Q_OS_WIN)
        handledStderr |= win_message_handler(type, context, msg);
#elif defined(Q_OS_UNIX)
        handledStderr |= unix_message_handler(type, context, msg);
# endif
#endif

        if (!handledStderr)
            stderr_message_handler(type, context, msg);
    }

    QString str;
    str = qFormatLogMessage(type,context,msg);
    str.append("\n");
    message = str.toLocal8Bit();

    LogDestination::LogToAllLogfiles(severity,message,category);

}


void qtlog::qInstallHandlers(){

    // 自定义PATTERN
    QString pattern;
    pattern.append("[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}")
            .append("%{pid} %{time h:mm:ss.zzz } %{qthreadptr}]");

    if(fileLine){
        pattern.append("%{file}:%{line} -");
    }
    if(!LogDestination::getCategoryMode()){
        pattern.append(" %{if-category}%{category}: %{endif}%{message}");
    }
    else{
        pattern.append(" %{message}");
    }

    qSetMessagePattern(pattern);

    qInstallMessageHandler(outputMessage);

#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(Application_CrashHandler)); //注冊异常捕获函数
#endif

}

void qtlog::setqtLogDestination(LogSeverity severity, QString &pathdir){
    LogDestination::setLogDestination(severity,pathdir);
}

void qtlog::setqtCategoryModeLogDestination(QString &pathdir)
{
    LogDestination::setLogDestination(pathdir);
}

#if (QT_VERSION > QT_VERSION_CHECK(5,3,0))
void qtlog::setqtLogEnv(QByteArray &file)
{
    qputenv("QT_LOGGING_CONF", file);
}
#endif

void qtlog::setqtLogMaxSize(quint32 size)
{
    g_max_log_size = size;
}

void qtlog::setqtLogbuffsecs(qint64 secs)
{
    logbufsecs = secs;
}

void qtlog::setqtLogCategoryMode(bool mode)
{
    LogDestination::setCategoryMode(mode);
}

void qtlog::setqtLogShouldflush(bool flush)
{
    should_flush = flush;
}

void qtlog::setqtLogFileLine(bool fileline)
{
    fileLine = fileline;
}

void qtlog::setdumpPath(QString path)
{
    dump_path = path;
}

void qtlog::flushqtLogNow()
{
    LogDestination::flushAllLogs();
}

void qtlog::setPrintToConsole(bool isPrint)
{
    is_to_console = isPrint;
}


#ifdef Q_OS_WIN
LONG Application_CrashHandler(EXCEPTION_POINTERS *pException){
    /*
        ***保存数据代码***
    */

    QDir basedir(dump_path);
    if(!basedir.exists()){
        basedir.mkpath(dump_path);
    }

    QString current_date_time = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz");
    QString dumppath = dump_path + current_date_time + ".dmp";
    LPCTSTR LPCTSTR_dumppath = reinterpret_cast<const wchar_t *>(dumppath.utf16());

    HANDLE hDumpFile = CreateFile(LPCTSTR_dumppath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if( hDumpFile != INVALID_HANDLE_VALUE){
        //Dump信息
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pException;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;
        //写入Dump文件内容
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, nullptr, nullptr);
    }
    //打印信息到日志
    EXCEPTION_RECORD* record = pException->ExceptionRecord;
    QString errCode(QString::number(record->ExceptionCode,16)),errAdr(QString::number(reinterpret_cast<size_t>(record->ExceptionAddress),16));
    QString printfstring = "errCode: "+errCode+"\r\n"+"errAdr: "+errAdr+"\r\n";
    printfstring = printfstring+QString("Fatal failure occurred, refer to dump/%1.dmp").arg(current_date_time);
    QLoggingCategory Category("dump");
    qCCritical(Category)<<printfstring.toLatin1().data();

    return EXCEPTION_EXECUTE_HANDLER;
}
#elif defined(Q_OS_LINUX)

#endif
