TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        Source/mysort.cpp

QMAKE_CXXFLAGS += \
        -fopenmp
QMAKE_LFLAGS += \
        -fopenmp

HEADERS += \
    Source/include/cxxopts.hpp

DISTFILES += \
    Makefile \
    Readme.md
