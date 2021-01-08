QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Canvas.cpp \
    LifeCalculator.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Canvas.h \
    LifeCalculator.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target




win32: LIBS += -L$$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/lib/x64/' -lOpenCL

INCLUDEPATH += $$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/include'
DEPENDPATH += $$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/include'

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/lib/x64/OpenCL.lib'
else:win32-g++: PRE_TARGETDEPS += $$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/lib/x64/libOpenCL.a'


win32: LIBS += -L$$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Release/release/ -lConway_QT_library

INCLUDEPATH += $$PWD/../Conway_QT_library
DEPENDPATH += $$PWD/../Conway_QT_library

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Release/release/Conway_QT_library.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Release/release/libConway_QT_library.a
