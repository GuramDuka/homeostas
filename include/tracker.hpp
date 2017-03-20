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
#ifndef TRACKER_HPP_INCLUDED
#define TRACKER_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <condition_variable>
//------------------------------------------------------------------------------
#include "indexer.hpp"
//------------------------------------------------------------------------------
namespace spacenet {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class directory_tracker {
    private:
        string dir_user_defined_name_;
        string dir_path_name_;
        string db_name_;
        string db_path_;
        string db_path_name_;

        string error_;
        std::unique_ptr<std::thread> thread_;
        std::mutex mtx_;
        std::condition_variable cv_;
        bool shutdown_;

        void worker();

    protected:
    public:
        ~directory_tracker() {
            shutdown();
        }

        const string & dir_user_defined_name() const {
            return dir_user_defined_name_;
        }

        directory_tracker & dir_user_defined_name(const string & dir_user_defined_name) {
            dir_user_defined_name_ = dir_user_defined_name;
            return *this;
        }

        const string & dir_path_name() const {
            return dir_path_name_;
        }

        directory_tracker & dir_path_name(const string & dir_path_name) {
            dir_path_name_ = dir_path_name;
            return *this;
        }

        void run();
        void shutdown();
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void tracker_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace spacenet
//------------------------------------------------------------------------------
#endif // TRACKER_HPP_INCLUDED
//------------------------------------------------------------------------------
