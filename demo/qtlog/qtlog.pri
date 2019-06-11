#增加release下debug信息导出功能,若不需要此功能,注释掉此段
#win32{
#    CONFIG(release, debug|release){
#        QMAKE_CXXFLAGS_RELEASE += /Zi
#        QMAKE_CXXFLAGS_RELEASE += /Od
#        QMAKE_LFLAGS_RELEASE += /DEBUG
#        DEFINES -= QT_NO_DEBUG_OUTPUT # enable debug output
#        DEFINES = QT_MESSAGELOGCONTEXT
#    }
#}
#else{
#    QMAKE_CXXFLAGS_RELEASE += -O0 -g
#    QMAKE_CFLAGS_RELEASE += -O0 -g
#    QMAKE_LFLAGS_RELEASE =
#}

win32:LIBS += -lDbgHelp

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/qtlog.h

SOURCES += \
    $$PWD/qtlog.cpp


