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
#include <string>
//------------------------------------------------------------------------------
#include "announcer.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void announcer::worker()
{
    for(;;) {
        try {
            at_scope_exit( socket_->close() );

            std::string host = "enosys.rf.gd", service = "http";
            host    = "127.0.0.1";
            service = "65480";

            socket_->connect(host, service);
            //std::qerr << socket_->local_addr() << std::endl;
            //std::qerr << socket_->remote_addr() << std::endl;

            socket_stream ss(socket_);

            ss <<
            "GET /announcer.php HTTP/1.1\r\n"
            "Host: " << host << ":" << socket_->remote_addr().port() << "\r\n"
            "User-Agent: homeostas\r\n"
            "Accept: text/html\r\n"
            "Accept-Encoding: identity\r\n"
            "Connection: close\r\n"
            "Cache-Control: max-age=0, no-cache, no-store, must-revalidate\r\n";

            for( const auto & a : pubs_ )
                ss << "Announce-Address: " << a << "\r\n";

            ss << "\r\n" << std::flush;

            ss.delimiter("\r\n");
            ss.exceptions(ss.exceptions() & ~std::ios::eofbit);
            size_t content_length = 0;

            while( !ss.eof() ) {
                std::string resp;
                ss >> resp;

                if( std::starts_with_case(resp, "Content-Length:") )
                    content_length = std::stoint<size_t>(resp.begin() + 15, resp.end());

                if( resp.empty() )
                    break;

                std::cerr << resp << std::endl;
            }

            ss.delimiter(socket_stream::traits_type::eof());

            std::string resp;
            ss >> resp;

            if( content_length != resp.size() )
                std::cerr << "invalid content length" << std::endl;

            std::cerr << resp << std::endl;

            break;
        }
        catch( const std::exception & e ) {
            std::cerr << e << std::endl;
        }
        catch( ... ) {
            std::cerr << "undefined c++ exception catched" << std::endl;
        }

        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, std::chrono::seconds(60), [&] { return shutdown_; });

        if( shutdown_ )
            break;
    }
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
