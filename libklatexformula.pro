QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    src/klfbackend.cpp \
    src/klfblockprocess.cpp \
    src/klfdatautil.cpp \
    src/klfdebug.cpp \
    src/klfdefs.cpp \
    src/klffactory.cpp \
    src/klffilterprocess.cpp \
    src/klflatexpreviewthread.cpp \
    src/klfpobj.cpp \
    src/klfsysinfo.cpp \
    src/klfuserscript.cpp \
    src/klfutil.cpp

HEADERS += \
    src/klfbackend.h \
    src/klfbackend_p.h \
    src/klfblockprocess.h \
    src/klfdatautil.h \
    src/klfdebug.h \
    src/klfdefs.h \
    src/klffactory.h \
    src/klffilterprocess.h \
    src/klffilterprocess_p.h \
    src/klflatexpreviewthread.h \
    src/klflatexpreviewthread_p.h \
    src/klfpobj.h \
    src/klfsysinfo.h \
    src/klfuserscript.h \
    src/klfutil.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
