/*-
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Guram Duka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStringList>
#ifdef Q_OS_ANDROID
#   include <QtAndroidExtras/QtAndroid>
#   include <QtAndroidExtras/QAndroidJniObject>
#endif
//------------------------------------------------------------------------------
#include "config.h"
#include "qobjects.hpp"
//------------------------------------------------------------------------------
int main(int argc, char ** argv)
{
    bool daemon = false;

    for( int i = 0; i < argc; i++ )
        if( strcmp(argv[i], "--daemon") == 0 )
            daemon = true;

    //homeostas::tests::run_tests();

    at_scope_exit(
        Homeostas::instance()->stopServer();
        Homeostas::instance()->stopTrackers();
    );

    //qSetMessagePattern("[%{type}] %{file}, %{line}: %{message}");

    auto logger = [] (const std::string & s) {
#ifdef Q_OS_ANDROID
        QAndroidJniObject::callStaticMethod<void>(
            "org/homeostas/HomeostasService",
            "logFromService",
            "(Ljava/lang/String;)V",
            QAndroidJniObject::fromString(QString::fromStdString(s)).object<jstring>());
#else
        qDebug().noquote().nospace() << QString::fromStdString(s);
#endif
    };

    if( daemon ) {
        QCoreApplication app(argc, argv);

        try {
            Homeostas::instance()->startTrackers();
            Homeostas::instance()->startServer();
            logger("warmup");
        }
        catch( const std::exception & e ) {
            logger(e.what());
        }
        catch( ... ) {
            logger("undefined c++ exception catched");
        }

        return app.exec();
    }

    // debug testing only
#if !defined(Q_OS_ANDROID) && !defined(NDEBUG)
    if( !daemon ) {
        try {
            Homeostas::instance()->startTrackers();
            Homeostas::instance()->startServer();
        }
        catch( const std::exception & e ) {
            logger(e.what());
        }
        catch( ... ) {
            logger("undefined c++ exception catched");
        }
    }
#endif

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    //qmlRegisterType<DirectoryTracker>("com.homeostas.directorytracker", 1, 0, "QDirectoryTracker");
    //qmlRegisterType<DirectoriesTrackersModel>("Backend", 1, 0, "DirectoriesTrackersModel");

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("homeostas", Homeostas::instance());
    engine.rootContext()->setContextProperty("directoriesTrackersModel", DirectoriesTrackersModel::instance());

    QFontDatabase::addApplicationFont(QLatin1String(":/digital-7-mono"));
    engine.load(QUrl("qrc:/main.qml"));

#ifdef Q_OS_ANDROID
    QAndroidJniObject::callStaticMethod<void>(
        "org/homeostas/HomeostasService",
        "startHomeostasService",
        "(Landroid/content/Context;)V",
        QtAndroid::androidActivity().object());
#endif

    return app.exec();
}
//------------------------------------------------------------------------------
