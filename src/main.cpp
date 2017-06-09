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
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>
//------------------------------------------------------------------------------
#include "config.h"
#include "qobjects.hpp"
//------------------------------------------------------------------------------
int main(int argc,char ** argv)
{
    //qSetMessagePattern("[%{type}] %{file}, %{line}: %{message}");
    //homeostas::tests::run_tests();

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    //qmlRegisterType<DirectoryTracker>("com.homeostas.directorytracker", 1, 0, "QDirectoryTracker");
    //qmlRegisterType<DirectoriesTrackersModel>("Backend", 1, 0, "DirectoriesTrackersModel");

    QQmlApplicationEngine engine;

    Homeostas homeostas;
    engine.rootContext()->setContextProperty("homeostas", &homeostas);
    HomeostasConfiguration homeostasConfiguration;
    engine.rootContext()->setContextProperty("homeostasConfiguration", &homeostasConfiguration);
    DirectoriesTrackersModel directoriesTrackersModel;
    engine.rootContext()->setContextProperty("directoriesTrackersModel", &directoriesTrackersModel);

    homeostas.startTrackers();
    homeostas.startServer();

    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    QFontDatabase::addApplicationFont(QLatin1String("qrc:/digital-7.ttf"));

    std::qerr << homeostas.newUniqueId().toStdString() << std::endl;

    return app.exec();
}
//------------------------------------------------------------------------------
