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
#include "server.hpp"
#include "client.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void client::worker()
{
    socket_->connectToHost(QHostAddress::LocalHost, 65535);
    socket_->waitForConnected(-1);
}
//------------------------------------------------------------------------------
void client::startup()
{
    shutdown_ = false;
    socket_ = new QTcpSocket;
    thread_.reset(new std::thread(&client::worker, this));
}
//------------------------------------------------------------------------------
void client::shutdown()
{
    if( thread_ == nullptr )
        return;

    std::unique_lock<std::mutex> lk(mtx_);
    shutdown_ = true;
    lk.unlock();
    cv_.notify_one();

    socket_->close();
    thread_->join();
    thread_ = nullptr;
    socket_ = nullptr;
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
