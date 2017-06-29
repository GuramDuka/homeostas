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
#ifndef NATPMP_HPP_INCLUDED
#define NATPMP_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <functional>
#include <condition_variable>
//------------------------------------------------------------------------------
#include "socket.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
constexpr uint16_t NATPMP_PORT = 5351;
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class natpmp {
public:
    ~natpmp() {
        shutdown();
    }

    bool started() const {
        return thread_ != nullptr;
    }

    void startup();
    void shutdown();

    template <typename T>
    auto & private_port(const T & port) {
        private_port_ = decltype(private_port_) (port);
        return *this;
    }

    const auto & private_port() const {
        return private_port_;
    }

    template <typename T>
    auto & public_port(const T & port) {
        public_port_ = decltype(public_port_) (port);
        return *this;
    }

    auto public_port() const {
        return public_port_;
    }

    const auto & public_addr() const {
        return public_addr_;
    }

    template <typename T>
    auto & port_mapping_lifetime(const T & lifetime) {
        port_mapping_lifetime_ = decltype(port_mapping_lifetime_) (lifetime);
        return *this;
    }

    const auto & port_mapping_lifetime() const {
        return port_mapping_lifetime_;
    }

    auto & mapped_callback(const std::function<void()> & f) {
        mapped_callback_ = f;
        return *this;
    }

    const auto & gateway() const {
        return gateway_;
    }

    const std::vector<socket_addr> & interfaces() const {
        return interfaces_;
    }

    auto & interfaces(const std::vector<socket_addr> & ifs) {
        interfaces_ = ifs;
        return *this;
    }
protected:
#if _WIN32
#   pragma pack(1)
#endif
    struct PACKED public_address_request {
		uint8_t  version = 0;
		uint8_t  op_code = 0;
	};

    struct PACKED public_address_response {
		uint8_t  version;
		uint8_t  op_code;
        uint16_t result_code = ResultCodeInvalid;
		uint32_t seconds;				// Seconds since port mapping table was initialized
        uint32_t addr;                  // Public IP Address
	};

    struct PACKED new_port_mapping_request {
        uint8_t  version               = 0;
        uint8_t  op_code               = 0; 	// OP Code = 1 for UDP or 2 for TCP
        uint16_t reserved              = 0;		// must be 0
        uint16_t private_port          = 0;
        uint16_t public_port           = 0;     // requested public port or zero for dynamic allocation
        uint32_t port_mapping_lifetime = 0;		// Requested port mapping lifetime in seconds
	};

    struct PACKED new_port_mapping_response {
		uint8_t  version;
		uint8_t  op_code;				// OP Code = 129 for UDP or 130 for TCP
		uint16_t result_code;
		uint32_t seconds;				// Seconds since port mapping table was initialized
		uint16_t private_port;
		uint16_t mapped_public_port;	// Mapped public port
        uint32_t port_mapping_lifetime; // Port mapping lifetime in seconds
	};
#if _WIN32
#   pragma pack()
#endif

	typedef enum {
		ResultCodeSuccess             = 0,
		ResultCodeUnsupportedVersion  = 1,
		ResultCodeNotAuthorized       = 2,
		ResultCodeNetworkFailure      = 3,
		ResultCodeOutOfResources      = 4,
        ResultCodeUnsupportedOpcode   = 5,
        ResultCodeInvalid             = 0xFFFF
	} ResultCode;

	static const char * str_result_code(ResultCode code) {
		switch( code ) {
			case ResultCodeSuccess            :
				return "Success";
			case ResultCodeUnsupportedVersion :
				return "Unsupported version";
			case ResultCodeNotAuthorized      :
				return "Not Authorized/Refused (e.g. box supports mapping, but user has turned feature off)";
			case ResultCodeNetworkFailure     :
				return "Network Failure (e.g. NAT box itself has not obtained a DHCP lease)";
			case ResultCodeOutOfResources     :
				return "Out of resources (NAT box cannot create any more mappings at this time)";
			case ResultCodeUnsupportedOpcode  :
			default                           :
				return "Unsupported opcode";
		};
	}

    void worker();

    std::function<void()> mapped_callback_;

    std::vector<socket_addr> interfaces_;
    socket_addr gateway_;
    socket_addr public_addr_;

	std::unique_ptr<active_socket> socket_;
    std::unique_ptr<std::thread> thread_;
    std::mutex mtx_;
    std::condition_variable cv_;

    uint32_t port_mapping_lifetime_ = 0;
    uint16_t public_port_           = 0;
    uint16_t private_port_          = 0;

    bool shutdown_;
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void natpmp_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // NATPMP_HPP_INCLUDED
//------------------------------------------------------------------------------
