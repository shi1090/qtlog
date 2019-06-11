win32:LIBS += -lDbgHelp

# 支持release模式下，行号等信息导出
CONFIG(release, debug|release){
    DEFINES += QT_MESSAGELOGCONTEXT
}


INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/qtlog.h

SOURCES += \
    $$PWD/qtlog.cpp


