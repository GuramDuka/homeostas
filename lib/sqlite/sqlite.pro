include(../../settings.pri)

*msvc {
    QMAKE_CFLAGS += -wd4996
}

TEMPLATE = lib
CONFIG += staticlib create_prl
INCLUDEPATH += include
HEADERS = \
    include/sqlite/sqlite3.h \
    include/sqlite/sqlite3ext.h
SOURCES = src/sqlite3.c
