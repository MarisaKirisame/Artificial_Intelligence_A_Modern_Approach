TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -lboost_unit_test_framework
QMAKE_CXXFLAGS += -std=c++1z -stdlib=libc++
QMAKE_LFLAGS += -stdlib=libc++
SOURCES += main.cpp
HEADERS += \
    agent.hpp \
    search.hpp \
    test.hpp \
    wumpus_world.hpp
INCLUDEPATH += \
    ../first_order_logic_prover \
    ../hana/include/
PRECOMPILED_HEADER = $$HEADERS
