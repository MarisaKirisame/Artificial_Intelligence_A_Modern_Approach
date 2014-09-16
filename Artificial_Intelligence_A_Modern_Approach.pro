TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -lboost_unit_test_framework
QMAKE_CXXFLAGS += -std=c++1y
SOURCES += main.cpp

HEADERS += \
    agent.hpp \
    search.hpp \
    test.hpp \
    CSP.hpp \
    wumpus_world.hpp \
    scope.hpp

PRECOMPILED_HEADER = $$HEADERS
