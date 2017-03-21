QT += qml quick

CONFIG += c++14

*-g++ {
    equals(ANDROID_TARGET_ARCH, armeabi-v7a) | equals(ANDROID_TARGET_ARCH, armeabi) {
    }
    else {
        isEmpty(QMAKE_CXXFLAGS_OPTIMIZE_FULL): QMAKE_CXXFLAGS_OPTIMIZE_FULL = -O3
    }

    CXX_VERSION = $$system("$${QMAKE_CXX} -dumpversion")

    contains(CXX_VERSION, "6.[0-9]\\.?[0-9,x]?") {
        #message("g++ version 6.x found")
        QMAKE_GNUC = 6
    }
    else {
        contains(CXX_VERSION, "5.[0-9]\\.?[0-9,x]?") {
            #message("g++ version 5.x found")
            QMAKE_GNUC = 5
        }
        else {
            contains(CXX_VERSION, "4.[0-9]\\.?[0-9,x]?") {
                #message("g++ version 4.x found")
                QMAKE_GNUC = 4
            }
            else {
                #message("g++ version $${CXX_VERSION} found")
                QMAKE_GNUC = 3
            }
        }
    }
}

#contains(QMAKE_COMPILER, "msvc") {
#}
*msvc {
    isEmpty(QMAKE_CXXFLAGS_OPTIMIZE_FULL): QMAKE_CXXFLAGS_OPTIMIZE_FULL = -Ox
}

#CONFIG -= c++11 c++14 c++1z

#greaterThan(QMAKE_GNUC, 5) {
#    CONFIG += c++1z
#    #QMAKE_CXXFLAGS += $$QMAKE_CXXFLAGS_CXX1Z
#}
#else {
#    CONFIG += c++14
#    #QMAKE_CXXFLAGS += $$QMAKE_CXXFLAGS_CXX14
#}

CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CXXFLAGS_OPTIMIZE_FULL
    QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO += $$QMAKE_CXXFLAGS_OPTIMIZE_FULL
    CONFIG += optimize_full
}

#message($$QMAKE_COMPILER)
#message($$QMAKE_CXXFLAGS)
#message($$QMAKE_CXXFLAGS_RELEASE)

DEFINES += SQLITE_THREADSAFE=1
#BUILD_DATE = $$system("date")
#DEFINES += BUILD_DATE=$$BUILD_DATE
#GIT_VERSION = $$system("git describe --always")
#DEFINES += GIT_VERSION=$$GIT_VERSION

SOURCES += \
    ../src/cdc512.cpp \
    ../src/indexer.cpp \
    ../src/main.cpp \
    ../src/sqlite3.c \
    ../src/locale_traits.cpp \
    ../src/tracker.cpp \
    ../tests/cdc512_test.cpp \
    ../tests/indexer_test.cpp \
    ../tests/locale_traits_test.cpp \
    ../tests/tracker_test.cpp \
    ../tests/rand_test.cpp \
    ../tests/all_tests.cpp \
    ../src/port.cpp \
    ../src/qobjects.cpp \
    ../src/std_ext.cpp

HEADERS += \
    ../include/cdc512.hpp \
    ../include/config.h \
    ../include/indexer.hpp \
    ../include/locale_traits.hpp \
    ../include/scope_exit.hpp \
    ../include/std_ext.hpp \
    ../include/version.h \
    ../include/sqlite3pp/sqlite3pp.h \
    ../include/sqlite3pp/sqlite3ppext.h \
    ../include/sqlite/sqlite3.h \
    ../include/sqlite/sqlite3ext.h \
    ../include/tracker.hpp \
    ../include/rand.hpp \
    ../include/port.hpp \
    ../include/qobjects.hpp

INCLUDEPATH += .
INCLUDEPATH += ../include

RESOURCES += ../src/qml/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = ../src

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = ../src

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

