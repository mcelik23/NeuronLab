QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG -= warn_on

# Include path
INCLUDEPATH += include .

# Source files
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/neuralnetwork.cpp \
    src/renderarea.cpp \
    src/errorgraph.cpp

# Header files
HEADERS += \
    include/errorgraph.h \
    include/mainwindow.h \
    include/neuralnetwork.h \
    include/renderarea.h

# Form files
FORMS += \
    forms/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    .gitignore \
    README.md
