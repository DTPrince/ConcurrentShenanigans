TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++0x -pthread
LIBS += -pthread

SOURCES += \
#    Source/counter.cpp \
    Source/barriers.cpp \
    Source/counter.cpp \
    Source/locks.cpp

HEADERS += \
    Source/include/barrier.hpp \
    Source/include/barriers.hpp \
    Source/include/cxxopts.hpp \
    Source/include/locks.hpp
