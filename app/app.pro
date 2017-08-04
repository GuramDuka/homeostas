include(../settings.pri)

TEMPLATE = app
TARGET = Homeostas
CONFIG += link_prl

win32 {
    RC_FILE = homeostas.rc
    DISTFILES += homeostas.rc
}

QT += core qml quick network

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
    android/gradlew.bat \
    android/AndroidManifest.xml \
    android/res/values/libs.xml \
    android/build.gradle \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    android/src/HomeostasBroadcastReceiver.java \
    android/src/HomeostasService.java

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = $$PWD/qml

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = $$PWD/qml

INCLUDEPATH += include

HEADERS += \
    include/cdc512.hpp \
    include/client.hpp \
    include/config.h \
    include/indexer.hpp \
    include/natpmp.hpp \
    include/port.hpp \
    include/qobjects.hpp \
    include/rand.hpp \
    include/server.hpp \
    include/socket.hpp \
    include/std_ext.hpp \
    include/thread_pool.hpp \
    include/tracker.hpp \
    include/version.h \
    include/stack_trace.hpp \
    include/announcer.hpp \
    include/configuration.hpp \
    include/variant.hpp \
    include/discoverer.hpp \
    include/socket_stream.hpp \
    include/ciphers.hpp \
    include/sha512.hpp

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
    src/cdc512.cpp \
    src/indexer.cpp \
    src/main.cpp \
    src/tracker.cpp \
    src/port.cpp \
    src/qobjects.cpp \
    src/std_ext.cpp \
    src/client.cpp \
    src/server.cpp \
    src/socket.cpp \
    src/natpmp.cpp \
    src/thread_pool.cpp \
    src/announcer.cpp \
    src/configuration.cpp \
    src/discoverer.cpp \
    src/sha512.cpp \
    src/socket_stream.cpp

# link numeric
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lib/numeric/release/ -lnumeric
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../lib/numeric/debug/ -lnumeric
else:unix: LIBS += -L$$OUT_PWD/../lib/numeric/ -lnumeric

INCLUDEPATH += $$PWD/../lib/numeric/include
DEPENDPATH += $$PWD/../lib/numeric/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/numeric/release/libnumeric.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/numeric/debug/libnumeric.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/numeric/release/numeric.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/numeric/debug/numeric.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../lib/numeric/libnumeric.a

# link sqlite
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lib/sqlite/release/ -lsqlite
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../lib/sqlite/debug/ -lsqlite
else:unix: LIBS += -L$$OUT_PWD/../lib/sqlite/ -lsqlite

INCLUDEPATH += $$PWD/../lib/sqlite/include
DEPENDPATH += $$PWD/../lib/sqlite/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite/release/libsqlite.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite/debug/libsqlite.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite/release/sqlite.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite/debug/sqlite.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite/libsqlite.a

# link sqlite3pp
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lib/sqlite3pp/release/ -lsqlite3pp
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../lib/sqlite3pp/debug/ -lsqlite3pp
else:unix: LIBS += -L$$OUT_PWD/../lib/sqlite3pp/ -lsqlite3pp

INCLUDEPATH += $$PWD/../lib/sqlite3pp/include
DEPENDPATH += $$PWD/../lib/sqlite3pp/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite3pp/release/libsqlite3pp.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite3pp/debug/libsqlite3pp.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite3pp/release/sqlite3pp.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite3pp/debug/sqlite3pp.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../lib/sqlite3pp/libsqlite3pp.a

# link jsoncpp

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lib/jsoncpp/release/ -ljsoncpp
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../lib/jsoncpp/debug/ -ljsoncpp
else:unix: LIBS += -L$$OUT_PWD/../lib/jsoncpp/ -ljsoncpp

INCLUDEPATH += $$PWD/../lib/jsoncpp/include
DEPENDPATH += $$PWD/../lib/jsoncpp/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/jsoncpp/release/libjsoncpp.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/jsoncpp/debug/libjsoncpp.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/jsoncpp/release/jsoncpp.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/jsoncpp/debug/jsoncpp.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../lib/jsoncpp/libjsoncpp.a
