TEMPLATE = app
TARGET = Homeostas

QT += core qml quick network

CONFIG += resources_big c++14

DEFINES += \
    QT_DEPRECATED_WARNINGS \
    SQLITE_THREADSAFE=1

win32 {
    DEFINES += _WIN32_WINNT=0x0601
    LIBS += -lws2_32 -liphlpapi -ladvapi32
    RC_FILE = homeostas.rc
    DISTFILES += homeostas.rc
    CONFIG(debug, debug|release) {
        CONFIG += console
    }
}

unix {
    LIBS += -ldl
}

*msvc {
    DEFINES += _CRT_SECURE_NO_WARNINGS FKG_FORCED_USAGE
    QMAKE_CFLAGS += -wd4996
    QMAKE_CXXFLAGS += -std:c++latest

    isEmpty(QMAKE_CFLAGS_OPTIMIZE_FULL): QMAKE_CFLAGS_OPTIMIZE_FULL = -Ox
    isEmpty(QMAKE_CXXFLAGS_OPTIMIZE_FULL): QMAKE_CXXFLAGS_OPTIMIZE_FULL = -Ox

    QMAKE_CFLAGS_RELEASE += $$QMAKE_CFLAGS_OPTIMIZE_FULL
    QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CXXFLAGS_OPTIMIZE_FULL
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO += $$QMAKE_CFLAGS_OPTIMIZE_FULL
    QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO += $$QMAKE_CXXFLAGS_OPTIMIZE_FULL
}

android {
    isEmpty(QMAKE_CFLAGS_OPTIMIZE_FULL): QMAKE_CFLAGS_OPTIMIZE_FULL = -O3
    isEmpty(QMAKE_CXXFLAGS_OPTIMIZE_FULL): QMAKE_CXXFLAGS_OPTIMIZE_FULL = -O3

    CONFIG(release, debug|release) {
        DEFINES += NDEBUG
    }
}

CONFIG(release, debug|release) {
    CONFIG += optimize_full
}

RESOURCES += \
    qml/qml.qrc \

DISTFILES += \
    qml/main.qml \
    android/AndroidManifest.xml \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = $$PWD/qml

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = $$PWD/qml

INCLUDEPATH += . include

HEADERS += \
    include/cdc512.hpp \
    include/client.hpp \
    include/config.h \
    include/indexer.hpp \
    include/locale_traits.hpp \
    include/natpmp.hpp \
    include/port.hpp \
    include/qobjects.hpp \
    include/rand.hpp \
    include/scope_exit.hpp \
    include/server.hpp \
    include/socket.hpp \
    include/std_ext.hpp \
    include/thread_pool.hpp \
    include/tracker.hpp \
    include/version.h \
    include/sqlite3pp/sqlite3pp.h \
    include/sqlite3pp/sqlite3ppext.h \
    include/sqlite/sqlite3.h \
    include/sqlite/sqlite3ext.h \
    include/numeric/id.hpp \
    include/numeric/ii.hpp \
    include/numeric/nn.hpp \
    include/numeric/tlsf.hpp \
    include/numeric/wk.hpp \
    include/stack_trace.hpp \
    include/discovery.hpp \
    include/announcer.hpp \
    include/configuration.hpp \
    include/variant.hpp

SOURCES += \
    tests/all_tests.cpp \
    tests/client_test.cpp \
    tests/server_test.cpp \
    tests/thread_pool_test.cpp \
    tests/cdc512_test.cpp \
    tests/indexer_test.cpp \
    tests/locale_traits_test.cpp \
    tests/tracker_test.cpp \
    tests/rand_test.cpp \
    tests/socket_test.cpp \
    include/numeric/id.cpp \
    src/cdc512.cpp \
    src/indexer.cpp \
    src/main.cpp \
    src/sqlite3.c \
    src/locale_traits.cpp \
    src/tracker.cpp \
    src/port.cpp \
    src/qobjects.cpp \
    src/std_ext.cpp \
    src/client.cpp \
    src/server.cpp \
    src/socket.cpp \
    src/natpmp.cpp \
    src/thread_pool.cpp \
    src/discovery.cpp \
    src/announcer.cpp \
    src/configuration.cpp
