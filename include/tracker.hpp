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
#include <memory>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
//------------------------------------------------------------------------------
#include "indexer.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class directory_tracker_interface {
public:
    directory_tracker_interface() {}

    directory_tracker_interface(const directory_tracker_interface & o) {
        *this = o;
    }

    directory_tracker_interface(directory_tracker_interface && o) {
        *this = std::move(o);
    }

    directory_tracker_interface & operator = (const directory_tracker_interface & o) {
        dir_user_defined_name_ = o.dir_user_defined_name_;
        dir_path_name_ = o.dir_path_name_;
        return *this;
    }

    directory_tracker_interface & operator = (directory_tracker_interface && o) {
        if( this != &o ) {
            dir_user_defined_name_ = std::move(o.dir_user_defined_name_);
            dir_path_name_ = std::move(o.dir_path_name_);
        }
        return *this;
    }

    directory_tracker_interface(
        const std::string & dir_user_defined_name,
        const std::string & dir_path_name) :
        dir_user_defined_name_(dir_user_defined_name),
        dir_path_name_(dir_path_name) {}

    const auto & dir_user_defined_name() const {
        return dir_user_defined_name_;
    }

    auto & dir_user_defined_name(const std::string & dir_user_defined_name) {
        dir_user_defined_name_ = dir_user_defined_name;
        return *this;
    }

    const auto & dir_path_name() const {
        return dir_path_name_;
    }

    auto & dir_path_name(const std::string & dir_path_name) {
        dir_path_name_ = dir_path_name;
        return *this;
    }
protected:
    std::string dir_user_defined_name_;
    std::string dir_path_name_;
private:
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class directory_tracker : public directory_tracker_interface {
public:
    ~directory_tracker() {
        shutdown();
    }

    directory_tracker() {}

    directory_tracker(
        const std::string & dir_user_defined_name,
        const std::string & dir_path_name) :
            directory_tracker_interface(dir_user_defined_name, dir_path_name) {}

    const auto & oneshot() const {
        return oneshot_;
    }

    auto & oneshot(bool oneshot) {
        oneshot_ = oneshot;
        return *this;
    }

    void startup();
    void shutdown();
protected:
    void worker();

    std::string db_name_;
    std::string db_path_;
    std::string db_path_name_;

    std::string error_;
    std::shared_future<void> worker_result_;
    std::mutex mtx_;
    std::condition_variable cv_;

    bool started_  = false;
    bool shutdown_ = false;
    bool oneshot_  = false;
private:
    directory_tracker(const directory_tracker &) = delete;
    void operator = (const directory_tracker &) = delete;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class directory_tracker_proxy : protected directory_tracker_interface {
public:
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class directory_trackers_proxy : protected std::vector<directory_tracker_proxy> {
public:
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void tracker_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // TRACKER_HPP_INCLUDED
//------------------------------------------------------------------------------
