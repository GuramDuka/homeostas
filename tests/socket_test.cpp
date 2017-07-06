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
#include <iomanip>
//------------------------------------------------------------------------------
#include "cdc512.hpp"
#include "thread_pool.hpp"
#include "socket_stream.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void socket_test()
{
    bool fail = false;

    try {
        auto myip = [] (auto compile_time) {
            if( compile_time )
                return;

            active_socket client;
            client.connect("myip.ru", "http");
            std::cerr << client.local_addr() << std::endl;
            std::cerr << client.remote_addr() << std::endl;
            client.send(
                "GET / HTTP/1.1\r\n"
                "Host: myip.ru\r\n"
                "User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:53.0) Gecko/20100101 Firefox/53.0\r\n"
                "Accept: text/html,application/xhtml+xml,application/xml;q=0.9," "*" "/" "*;q=0.8\r\n"
                "Accept-Language: ru,en-US;q=0.7,en;q=0.3\r\n"
                "Accept-Encoding: identity\r\n"
                "Connection: close\r\n"
                "\r\n");
            std::cerr << client.recv<std::string>() << std::endl;
        };

        myip(true);

        uint16_t lport = 0;
        std::vector<passive_socket> lsocks;
        auto wildcards = passive_socket::interfaces();//passive_socket::wildcards();

        std::mutex mtx;
        std::condition_variable cond;
        std::atomic_size_t counter(0);

        thread_pool_t pool; // pool destructor call must be first

        lsocks.resize(wildcards.size());

        for( size_t i = 0; i < wildcards.size(); i++ ) {
            auto & server = lsocks[i];
            wildcards[i].port(std::rhash(lport));
            server.listen(wildcards[i]);

            pool.enqueue([&] {
                if( ++counter == wildcards.size() )
                    cond.notify_one();

                for(;;) {
                    auto socket = server.accept_shared();

                    if( !socket || socket->invalid() || socket->fail() || server.interrupt() )
                        break;

                    pool.enqueue([&] (auto client_socket) {
                        socket_stream ss(*client_socket);

                        try {
                            std::string s;
                            ss >> s;
                            cdc512 ctx(s.begin(), s.end());
                            ss << std::to_string(ctx) << std::flush;

                            //std::unique_lock<std::mutex> lock(mtx);
                            //std::qerr
                            //    << "accepted: "
                            //    << std::to_string(client_socket->local_addr())
                            //    << " -> "
                            //    << std::to_string(client_socket->remote_addr())
                            //    << std::endl;
                        }
                        catch( const std::ios_base::failure & e ) {
                            std::unique_lock<std::mutex> lock(mtx);
                            std::cerr << e << std::endl;
                            throw;
                        }
                    }, socket);
                }
            });
        }

        // wait for listening threads initialized
        {
            std::unique_lock<std::mutex> lock(mtx);
            cond.wait(lock);
        }

        std::vector<std::shared_ptr<active_socket>> rsocks;

        size_t CONNECTORS_THREADS = std::thread::hardware_concurrency() * 2;
        size_t CONNECTIONS_PER_THREAD = std::max(size_t(3), wildcards.size());
        //size_t CONNECTIONS = CONNECTORS_THREADS * CONNECTIONS_PER_THREAD;

        counter = CONNECTORS_THREADS;

        for( size_t t = 0; t < CONNECTORS_THREADS; t++ ) {
            rsocks.push_back(active_socket::shared());

            pool.enqueue([&] (auto client_socket) {
                at_scope_exit(
                    if( --counter == 0 )
                        cond.notify_one();
                );

                //client_socket->exceptions(false);

                for( size_t i = 0; i < CONNECTIONS_PER_THREAD; i++ ) {
                    //std::this_thread::sleep_for(std::chrono::seconds(2));

                    cdc512 ctx1;
                    ctx1.generate_entropy_fast();
                    const auto s1 = std::to_string(ctx1);
                    cdc512 ctx2(s1.begin(), s1.end());
                    const auto s2 = std::to_string(ctx2);

                    try {
                        client_socket->connect(wildcards[i % wildcards.size()]);
                    }
                    catch( const std::xruntime_error & e ) {
                        std::unique_lock<std::mutex> lock(mtx);
                        std::cerr << e << std::endl;
                        continue;
                    }

                    socket_stream ss(*client_socket);

                    try {
                        ss << s1 << std::flush;
                        std::string s;
                        ss >> s;

                        //std::unique_lock<std::mutex> lock(mtx);
                        //std::qerr
                        //    << "connected: "
                        //    << std::to_string(client_socket->local_addr())
                        //    << " -> "
                        //    << std::to_string(client_socket->remote_addr())
                        //    << ", " << (s == s2)
                        //    << std::endl;
                        if( s != s2 )
                            throw std::xruntime_error("invalid implementation", __FILE__, __LINE__);
                    }
                    catch( const std::ios_base::failure & e ) {
                        std::unique_lock<std::mutex> lock(mtx);
                        std::cerr << e << std::endl;
                        throw;
                    }
                }
            }, rsocks.back());
        }

        // wait for connections done
        {
            std::unique_lock<std::mutex> lock(mtx);
            cond.wait(lock);
        }

        // interrupt connectors threads
        for( auto & sock : rsocks )
            sock->interrupt(true).close();

        // interrupt socket listening threads
        for( auto & sock : lsocks )
            sock.interrupt(true).close();

        // pool destructor called automaticaly
        pool.join();

        auto pool_stat = pool.stat();

        if( !pool_stat.empty() )
            std::cerr
                << "socket threads pool statistics:" << std::endl
                << pool_stat;
    }
    catch (const std::exception & e) {
        std::cerr << e << std::endl;
        fail = true;
    }
    catch (...) {
        fail = true;
    }

    std::cerr << "socket test " << (fail ? "failed" : "passed") << std::endl;
}
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
