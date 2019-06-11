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
 * 根据日志级别导出日志，同一级别下日志存储到一个日志文件中\r\n
 * 2.分类模式\r\n
 * 配合qt日志库QLoggingCategory类使用,支持日志按需分类,根据Category进行路径区分，支持多级路径。
 * 分类模式下同一Category分类下日志级别不区分文件，统一存放到一个分类日志下。
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
     * @details 普通模式下日志目标地址设置,不同等级日志可导出到不同日志下
     */
    static void setqtLogDestination(LogSeverity severity,QString &pathdir);

    /**
     * @brief setqtCategoryModeLogDestination
     * @param pathdir
     * @details 分类模式下日志根目录，日志的具体路径在此根目录下根据Category信息创建
     */
    static void setqtCategoryModeLogDestination(QString &pathdir);

    /**
     * @brief setqtLogEnv
     * @param file
     * @details 配置文件地址环境变量配置，日志导出控制可在对用配置文件中进行配置，具体配置方法见Qt帮助文档
     */
    static void setqtLogEnv(QByteArray& file);

    /**
     * @brief setqtLogMaxSize
     * @param size
     * @details 日志文件大小设置，超过设置值后，文件自动创建新文件
     *
     */
    static void setqtLogMaxSize(quint32 size);

    /**
     * @brief setqtLogbuffsecs
     * @param secs
     * @details 文件flush间隔时间设置
     * @note 功能全局设置，设置后所有写操作都按此设置进行操作
     */
    static void setqtLogbuffsecs(qint64 secs);

    /**
     * @brief setqtLogCategoryMode
     * @param mode
     * @details 开启分类模式接口
     * @note 功能全局设置，设置后所有写操作都按此设置进行操作
     */
    static void setqtLogCategoryMode(bool mode);

    /**
     * @brief setqtLogShouldflush
     * @param flush
     * @details 缓存立即刷新设置接口
     * @note 功能全局设置，设置后所有写操作都按此设置进行操作
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
