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
#ifndef THREAD_POOL_HPP_INCLUDED
#define THREAD_POOL_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <iostream>
#ifndef NDEBUG
#   include <sstream>
#   include <iomanip>
#endif
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <queue>
#include <future>
#include <functional>
#include <stdexcept>
#include <condition_variable>
#include <type_traits>
//------------------------------------------------------------------------------
#include "std_ext.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <typename T
    //, typename std::enable_if<std::is_base_of<T, std::thread>::value>::type * Base = nullptr
>
class thread_pool {
public:
    // the destructor joins all threads
    ~thread_pool() noexcept {
        join();
    }

    void join() noexcept {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        auto min_threads_safe = min_threads_;
        min_threads_ = 0;

        lock.unlock();
        queue_cond_.notify_all();

        lock.lock();
        join_cond_.wait(lock, [&] { return workers_.empty(); });
        clear_dead();

        min_threads_ = min_threads_safe;
    }

    // the constructor just launches some amount of workers
    thread_pool(size_t min_threads = 0, size_t max_threads = 0, uint64_t idle_timeout = 30 * 1000000000ull)
        noexcept :
        min_threads_(min_threads),
        max_threads_(max_threads),
        idle_threads_(0),
        idle_timeout_(idle_timeout)
    {
        static_assert(std::is_base_of<T, std::thread>::value, "Must be derived from std::thread");

        if( min_threads_ == 0 )
            min_threads_ = std::thread::hardware_concurrency();

        if( max_threads_ != 0 && max_threads_ < min_threads_ )
            max_threads_ = min_threads_;
    }

    void enqueuef(const std::function<void()> & task) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        tasks_.emplace(task);

        auto need_extra_thread = idle_threads_ == 0
            && (max_threads_ == 0 || workers_.size() < max_threads_);

        if( need_extra_thread ) {
            std::unique_ptr<max_align_t> worker_ptr(
                new max_align_t [sizeof(T) / sizeof(max_align_t)
                    + (sizeof(T) % sizeof(max_align_t) ? 1 : 0)]);
            T * p_worker = reinterpret_cast<T *>(worker_ptr.get());

            new (p_worker) T(std::bind(&thread_pool<T>::worker_function, this, p_worker));

            std::unique_ptr<T> worker(p_worker);
            worker_ptr.release();
            workers_.emplace(std::make_pair(p_worker, p_worker));
            worker.release();
#ifndef NDEBUG
            if( workers_.size() > max_threads_stat_ )
                max_threads_stat_ = workers_.size();
            new_thread_exec_stat_++;
#endif
        }
#ifndef NDEBUG
        else {
            idle_thread_exec_stat_++;
        }
#endif

        clear_dead();
        lock.unlock();
        queue_cond_.notify_one();
    }

    // add new work item to the pool
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();

        enqueuef([task] { (*task)(); });

        return res;
    }

    std::string stat() const {
#if NDEBUG
        return std::string();
#else
        std::ostringstream ss;
        ss
            << "  max threads                    : " << std::setw(12) << std::right << max_threads_stat_ << std::endl
            << "  max idle threads               : " << std::setw(12) << std::right << max_idle_threads_stat_ << std::endl
            << "  dead threads by timeout        : " << std::setw(12) << std::right << dead_threads_stat_ << std::endl
            << "  task new thread executions     : " << std::setw(12) << std::right << new_thread_exec_stat_ << std::endl
            << "  task idle thread executions    : " << std::setw(12) << std::right << idle_thread_exec_stat_ << std::endl
            << "  task new/idle thread exec ratio: " << std::setw(12) << std::right << std::fixed << std::setprecision(5)
                << (idle_thread_exec_stat_ ? double(new_thread_exec_stat_) / idle_thread_exec_stat_ : 0) << std::endl
        ;
        return ss.str();
#endif
    }

    static auto instance() {
        static thread_pool<T> singleton;
        return &singleton;
    }
protected:
    void worker_function(T * a_worker) {
        auto ql = [&] {
            return !tasks_.empty() || min_threads_ == 0;
        };

        p_this_thread() = a_worker;

        for(;;) {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            idle_threads_++;

#ifndef NDEBUG
            if( idle_threads_ > max_idle_threads_stat_ )
                max_idle_threads_stat_ = idle_threads_;
#endif

#ifndef NDEBUG
            auto ws =
#endif
            queue_cond_.wait_for(lock, std::chrono::nanoseconds(idle_timeout_), ql);

            if( tasks_.empty() ) {
#ifndef NDEBUG
                if( !ws )
                    dead_threads_stat_++;
#endif
                // timeout or join all threads
                // thread are unnecessary and must die
                if( idle_threads_ > min_threads_ ) {
                    idle_threads_--;
                    terminate_herself();
                    break;
                }

                idle_threads_--;
            }
            else {
                idle_threads_--;

                // execute task
                std::function<void()> task = std::move(tasks_.front());
                tasks_.pop();

                lock.unlock();

                try {
                    task();
                }
                catch( const std::exception & e ) {
                    std::cerr << e << std::endl;
                }
                catch( ... ) {
                    std::cerr << "undefined c++ exception catched, thread terminated with unhandled exception" << std::endl;
                }
            }
        }
    }

    void clear_dead() {
        for( auto & p : dead_ ) {
            p->join();
            delete p;
        }

        dead_.clear();
    }

    void terminate_herself() {
        auto it = workers_.find(p_this_thread());

        if( it == workers_.end() )
            throw std::xruntime_error("undefined behavior in thread pool", __FILE__, __LINE__);

        dead_.emplace_back(it->second);
        workers_.erase(it);

        if( workers_.empty() ) // join pool ?
            join_cond_.notify_one();
    }

    size_t min_threads_;
    size_t max_threads_;
    size_t idle_threads_;
    uint64_t idle_timeout_;
    // need to keep track of threads so we can join them
    std::unordered_map<T *, T *> workers_;
    // dead threads by idle timeout
    std::vector<T *> dead_;
    // the task queue
    std::queue<std::function<void()>> tasks_;

    // synchronization
    std::mutex queue_mutex_;
    std::condition_variable queue_cond_;
    std::condition_variable join_cond_;
#ifndef NDEBUG
    uint64_t max_threads_stat_ = 0;
    uint64_t max_idle_threads_stat_ = 0;
    uint64_t new_thread_exec_stat_ = 0;
    uint64_t idle_thread_exec_stat_ = 0;
    uint64_t dead_threads_stat_ = 0;
#endif

    auto & p_this_thread() {
        static thread_local T * p_this_thread = nullptr;
        return p_this_thread;
    }
};
//------------------------------------------------------------------------------
typedef thread_pool<std::thread> thread_pool_t;
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void thread_pool_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // THREAD_POOL_HPP_INCLUDED
//------------------------------------------------------------------------------
