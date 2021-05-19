#include <QCoreApplication>
#include "qtlog.h"
#include <QtDebug>
#include <QSettings>
#include <QLoggingCategory>


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    /** 配置qtlog */
    QString logpath;
    QString dumppath;
    bool fileline;
    bool showConsole;
    quint32 maxSize;
    qint64 secs;
    bool category;
    bool ImmediatelyFlush;

    QString settingsPath = QCoreApplication::applicationDirPath()+"/settings.ini";
    QSettings settings_(settingsPath,QSettings::IniFormat);

    settings_.beginGroup("Rules");
    settings_.setValue("*.debug",true);
    settings_.endGroup();

    /** 日志分类筛选功能,采用QT_LOGGING_CONF环境变量方式,支持配置文件方式
            * 此方法Qt官方在Qt5.3以上版本已支持,通过配置文件进行输出控制
            * [Rules]
            *  *.debug=false
            *  socket.Msg.debug = true
            */
    QByteArray rulesAddr = settingsPath.toLatin1();
    qtlog::setqtLogEnv(rulesAddr);



    /** 日志系统基础信息配置 */
    settings_.beginGroup("LOG");
    /**  日志文件导出根目录 */
    if(!settings_.contains("PATH")){
        settings_.setValue("PATH",QCoreApplication::applicationDirPath()+"/logs/");
        logpath = QCoreApplication::applicationDirPath()+"/logs/";
    }
    else{
        logpath = settings_.value("PATH").toString();
    }
    /** FileLine 支持行号信息导出,默认不开启 */
    if(!settings_.contains("FileLine")){
        settings_.setValue("FileLine",false);
        fileline = false;
    }
    else{
        fileline = settings_.value("FileLine").toBool();
    }
    /** 是否打印到控制台 */
    if(!settings_.contains("Console")){
        settings_.setValue("Console",true);
        showConsole = true;
    }
    else{
        showConsole = settings_.value("Console").toBool();
    }

    /** MaxSize 日志文件大小,超过设定值后创建新日志,单位M */
    if(!settings_.contains("MaxSize")){
        settings_.setValue("MaxSize",100);
        maxSize = 100;
    }
    else{
        maxSize = static_cast<quint32>(settings_.value("MaxSize").toInt());
    }
    /** buff flush sec 缓存刷新到本地时间,时间为s */
    if(!settings_.contains("BuffSecs")){
        settings_.setValue("BuffSecs",5);
        secs = 5;
    }
    else{
        secs = static_cast<qint64>(settings_.value("BuffSecs").toInt());
    }

    /** Category模式,Category模式下根据Category在日志根目录下创建分类目录. 此模式下不同等级日志存放在一个文件下 */
    if(!settings_.contains("Category")){
        settings_.setValue("Category",true);
        category = true;
    }
    else{
        category = settings_.value("Category").toBool();
    }
    /** 缓存是否立即刷新到本地,开启此功能缓存时间间隔将无效,日志信息将从内存实时刷新到本地 */
    if(!settings_.contains("ImmediatelyFlush")){
        settings_.setValue("ImmediatelyFlush",true);
        ImmediatelyFlush = true;
    }
    else{
        ImmediatelyFlush = settings_.value("ImmediatelyFlush").toBool();
    }
    settings_.endGroup();

    /** dump导出地址设置 */
    settings_.beginGroup("Dump");
    if(!settings_.contains("PATH")){
        // 添加默认配置信息
        settings_.setValue("PATH",QCoreApplication::applicationDirPath()+"/Dumps/");
        dumppath = QCoreApplication::applicationDirPath()+"/Dumps/";
    }
    else{
        dumppath = settings_.value("PATH").toString();
    }
    settings_.endGroup();

    /** set log ini */
    qtlog::setqtLogFileLine(fileline);
    qtlog::setPrintToConsole(showConsole);
    qtlog::setqtLogMaxSize(maxSize);
    qtlog::setqtLogbuffsecs(secs);
    qtlog::setqtLogShouldflush(ImmediatelyFlush);
    qtlog::setqtLogCategoryMode(category);
    if(category)
        qtlog::setqtCategoryModeLogDestination(logpath);
    else{
        // 不同等级日志存放到一起
        qtlog::setqtLogDestination(QDEBUG,logpath);
        qtlog::setqtLogDestination(QINFO,logpath);
        qtlog::setqtLogDestination(QWARING,logpath);
        qtlog::setqtLogDestination(QERROR,logpath);
        qtlog::setqtLogDestination(QFATAL,logpath);
    }

    /** dump */
    qtlog::setdumpPath(dumppath);

    qtlog::qInstallHandlers();
    qDebug()<<"测试 ";

    /** 不配置时默认Category为defult */
    qDebug()<<"log debug";
    qInfo()<<"log info";
    qWarning()<<"log warning";
    qCritical()<<"log error";

    QLoggingCategory Category("socket.Msg");
    qCDebug(Category)<<"Category->socket.Msg log debug";
    qCInfo(Category)<<"Category->socket.Msg log info";
    qCWarning(Category)<<"Category->socket.Msg log warning";
    qCCritical(Category)<<"Category->socket.Msg log error";

    return a.exec();
}
