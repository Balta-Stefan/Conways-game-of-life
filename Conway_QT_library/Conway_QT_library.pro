
QT += core

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Conway.cpp

HEADERS += \
    Conway.h

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target

win32: LIBS += -L$$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/lib/x64/' -lOpenCL

INCLUDEPATH += $$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/include'
DEPENDPATH += $$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/include'

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/lib/x64/OpenCL.lib'
else:win32-g++: PRE_TARGETDEPS += $$PWD/'../../../../../Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.2/lib/x64/libOpenCL.a'
