TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++0x -pthread
LIBS += -pthread

SOURCES += \
    Source/barriers.cpp \
    Source/locks.cpp \
    Source/mysort.cpp

HEADERS += \
    Source/include/barrier.hpp \
    Source/include/barriers.hpp \
    Source/include/cpu_relax.hpp \
    Source/include/cxxopts.hpp \
    Source/include/locks.hpp

DISTFILES += \
    Makefile \
    Readme.md
