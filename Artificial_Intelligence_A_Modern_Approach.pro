TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -lboost_unit_test_framework
QMAKE_CXXFLAGS += -std=c++1y
SOURCES += main.cpp

HEADERS += \
    Search.hpp

