include(../../settings.pri)

TEMPLATE = lib
CONFIG += staticlib create_prl
INCLUDEPATH += \
    $$PWD/../sqlite/include \
    include
HEADERS = \
    include/sqlite3pp/sqlite3pp.h \
    include/sqlite3pp/sqlite3ppext.h
SOURCES = src/sqlite3pp.cpp
