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
#include <random>
#include <QtDebug>
//------------------------------------------------------------------------------
#include "cdc512.hpp"
#include "rand.hpp"
#include "port.hpp"
#include "qobjects.hpp"
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
QVariant QHomeostas::getVar(const QString & varName)
{
    return QVariant();
}
//------------------------------------------------------------------------------
void QHomeostas::setVar(const QString & varName, const QVariant & val)
{
    sqlite3pp::database db;

    sqlite3_enable_shared_cache(1);

    db.connect(
        homeostas::home_path(false)
            + ".homeostas" + homeostas::path_delimiter + "homeostas.sqlite",
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE
    );

    auto rc = sqlite3pp::command(db, R"EOS(
        PRAGMA page_size = 4096;
        PRAGMA journal_mode = WAL;
        PRAGMA count_changes = OFF;
        PRAGMA auto_vacuum = NONE;
        PRAGMA cache_size = -2048;
        PRAGMA synchronous = NORMAL;
        /*PRAGMA temp_store = MEMORY;*/
    )EOS").execute_all();
};
//------------------------------------------------------------------------------
QString QHomeostas::newUniqueId()
{
    std::vector<uint8_t> entropy;

    for( auto iface : QNetworkInterface::allInterfaces() ) {
        uint8_t c, q = 0, i = 0;

        for( auto qc : iface.hardwareAddress() ) {
            if( !qc.isLetterOrNumber() )
                continue;

            if( i == 2 ) {
                entropy.push_back(q);
                q = i = 0;
            }

            c = qc.toLatin1();
            q <<= 4;

            if( qc.isDigit() )
                q |= c - '0';
            else if( qc.isLower() )
                q |= c - 'a';
            else
                q |= c - 'A';

        }

        if( i )
            entropy.push_back(q);
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    auto clock_ns = 1000000000ull * ts.tv_sec + ts.tv_nsec;

    while( clock_ns ) {
        uint8_t q = uint8_t(clock_ns & 0xff);
        if( q != 0 )
            entropy.push_back(clock_ns & 0xff);
        clock_ns >>= 8;
    }

    typedef homeostas::rand<> rand;
    rand r;
    rand::value_type v = 0;

    std::random_device rd;
    std::uniform_int_distribution<int> dist(1, 15);

    while( entropy.size() < rand::srand_size )
        entropy.push_back(dist(rd));

    r.srand(&entropy[0], entropy.size());
    r.warming();

    QString id;
    homeostas::cdc512 ctx;

    for( auto & q : ctx.digest )
        for(;;) {
            if( v == 0 )
                v = r.get();

            if( (v & 0xf) != 0 && ((v >> 4) & 0xf) != 0 ) {
                q = v & 0xff;
                v >>= 8;
                break;
            }

            v >>= 8;
        }

    return QString::fromStdString(
        ctx.to_short_string("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", '-', 6));
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
//void QDirectoryTracker::doSend(int count)
//{
//    qDebug() << "Received in C++ from QML:" << count;
//}
//------------------------------------------------------------------------------
void QDirectoryTracker::startTracker()
{
    dt_.startup();
}
//------------------------------------------------------------------------------
void QDirectoryTracker::stopTracker()
{
    dt_.shutdown();
}
//------------------------------------------------------------------------------
