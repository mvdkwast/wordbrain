TEMPLATE = app
TARGET = wordbrain
INCLUDEPATH += .
CONFIG += c++11 
# CONFIG += debug
QT -= gui

# Input
SOURCES += main.cpp

QMAKE_CXXFLAGS_DEBUG += -Wall -pedantic
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3
