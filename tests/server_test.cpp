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
#include <iostream>
//------------------------------------------------------------------------------
#include <QtDebug>
#include <QNetworkInterface>
//------------------------------------------------------------------------------
#include "server.hpp"
#include "client.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void server_test()
{
    bool fail = false;

    try {
        for( auto iface : QNetworkInterface::allInterfaces() ) {
            std::qerr
                << "if: " << iface.humanReadableName().toStdString()
                << " " << iface.name().toStdString()
                << std::endl;
            std::qerr << "     hardware: " << iface.hardwareAddress().toStdString() << std::endl;

            for( auto addr : iface.addressEntries() ) {
                std::qerr << "           ip: " << addr.ip().toString().toStdString() << std::endl;
                std::qerr << "         mask: " << addr.netmask().toString().toStdString() << std::endl;
                std::qerr << "    broadcast: " << addr.broadcast().toString().toStdString() << std::endl;
            }
        }
    }
    catch (const std::exception & e) {
        std::qerr << e << std::endl;
        fail = true;
    }
    catch (...) {
        fail = true;
    }

    std::qerr << "server test " << (fail ? "failed" : "passed") << std::endl;
}
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
