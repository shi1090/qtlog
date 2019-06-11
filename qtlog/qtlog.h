#ifndef QTLOG_H
#define QTLOG_H

#if _MSC_VER >= 1600
#pragma execution_character_set("UTF-8")
#endif

#include <QObject>
#include <QCoreApplication>
#include <QMutex>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QThread>
#include <QTextStream>
#include <QDate>
#include <time.h>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

typedef int LogSeverity;

#define QDEBUG   0
#define QINFO    1
#define QWARING  2
#define QERROR   3   //Critical
#define QFATAL   4

#define NUM_SEVERITIES  5

#ifdef Q_OS_WIN
#include "Windows.h"
#include <DbgHelp.h>
LONG Application_CrashHandler(EXCEPTION_POINTERS *pException);
#elif Q_OS_LINUX

#endif

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

/**
 * @brief The qtlog class
 * @details qt日志配置纯静态类，配合qt日志引擎，支持两种日志导出方式\n
 * 1.普通模式\r\n
 * 根据日志级别导出日志，创建分级目录，同一级别下日志存储到一个日志文件中,支持日志级别为 debug info warning error fatal\r\n
 * 2.分类模式\r\n
 * 配合qt日志库QLoggingCategory类使用,支持日志按需分类,根据Category进行路径区分，支持多级路径。\r\n
 * 分类模式下同一Category分类下日志级别不区分文件，统一存放到一个分类日志下。\r\n
 * @note 主要移植修改glog日志库的文件导出类，并结合Qt日志库特点进行功能扩展。
 */
class qtlog : public QObject
{
    Q_OBJECT
public:
    /** 注册输出接口函数 */
    static void qInstallHandlers();

    /**
     * @brief setqtLogDestination
     * @param pathdir
     * @details 普通模式下日志目标地址设置,不同等级日志可导出到不同日志下,不设置将自动丢弃日志
     * @note setqtLogDestination 和 setqtCategoryModeLogDestination不能同时使用
     */
    static void setqtLogDestination(LogSeverity severity,QString &pathdir);

    /**
     * @brief setqtCategoryModeLogDestination
     * @param pathdir
     * @details 分类模式下日志根目录，日志的具体路径在此根目录下根据Category信息创建
     * @note setqtLogDestination 和 setqtCategoryModeLogDestination不能同时使用
     */
    static void setqtCategoryModeLogDestination(QString &pathdir);

#if (QT_VERSION > QT_VERSION_CHECK(5,3,0))

    /**
     * @brief setqtLogEnv
     * @param file
     * @details QT_LOGGING_CONF配置文件地址设置,qt5.3.0以上版本支持
     */
    static void setqtLogEnv(QByteArray& file);

#endif

    /**
     * @brief setqtLogMaxSize
     * @param size
     * @details 日志文件大小设置，超过设置值后，文件自动创建新文件，单位为M，默认为10M
     */
    static void setqtLogMaxSize(quint32 size);

    /**
     * @brief setqtLogbuffsecs
     * @param secs
     * @details 文件flush到本地间隔时间设置
     * @note 功能全局设置，设置后所有写操作都按此设置进行操作。注意此处时间间隔判断方法采用上一次写入时间点
     * 和最新一次日志信息之间时间差做判断，可能存在有数据一直不flush到本地的情况。可调用 @see flushqtLogNow()
     * 立即flush数据到本地
     */
    static void setqtLogbuffsecs(qint64 secs);

    /**
     * @brief setqtLogCategoryMode
     * @param mode
     * @details 开启分类模式，分类模式和普通模式不可以同时使用
     */
    static void setqtLogCategoryMode(bool mode);

    /**
     * @brief setqtLogShouldflush
     * @param flush
     * @details 缓存立即刷新设置接口
     * @note 功能全局设置，设置后所有写操作都按此设置进行操作 @see setqtLogbuffsecs 设置将无效
     */
    static void setqtLogShouldflush(bool flush);

    /**
     * @brief setdumpPath
     * @param path
     * @details dump地址根路径设置
     */
    static void setdumpPath(QString path);

    /**
     * @brief flushqtLogNow
     * @details 立即刷新日志缓存
     */
    static void flushqtLogNow();

    /** 是否详细打印，包含行号等，只在调试-d模式下支持 */
    static bool richText;


private:
    explicit qtlog(QObject *parent = nullptr);
    ~qtlog();





signals:

public slots:
};

#endif // QT5LOG_H
