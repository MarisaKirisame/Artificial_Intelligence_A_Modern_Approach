TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -lboost_unit_test_framework
QMAKE_CXXFLAGS += -std=c++1z
SOURCES += main.cpp
HEADERS += \
    agent.hpp \
    search.hpp \
    test.hpp \
    wumpus_world.hpp

PRECOMPILED_HEADER = $$HEADERS
