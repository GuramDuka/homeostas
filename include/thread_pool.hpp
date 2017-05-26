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
template <typename T>
class thread_pool {
    public:
        // the destructor joins all threads
        ~thread_pool() {
            join();
        }

        void join() {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
            lock.unlock();
            condition_.notify_all();
            lock.lock();
            condition_.wait(lock, [&] { return workers_.empty(); });

            for( auto p : dead_ ) {
                p->join();
                delete p;
            }
        }

        // the constructor just launches some amount of workers
        thread_pool(size_t min_threads = 0, size_t max_threads = 0) :
            min_threads_(min_threads),
            max_threads_(max_threads),
            idle_threads_(0),
            stop_(false)
        {
            if( min_threads_ == 0 )
                min_threads_ = std::thread::hardware_concurrency();
            if( max_threads_ == 0 )
                max_threads_ = min_threads_ * 2;
        }

        // add new work item to the pool
        template <class F, class... Args>
        auto enqueue(F&& f, Args&&... args)
            -> std::future<typename std::result_of<F(Args...)>::type>
        {
            using return_type = typename std::result_of<F(Args...)>::type;
            auto task = std::make_shared<std::packaged_task<return_type()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            std::future<return_type> res = task->get_future();
            std::unique_lock<std::mutex> lock(queue_mutex_);

            // don't allow enqueueing after stopping the pool
            if( stop_ )
                throw std::runtime_error("enqueue on stopped in thread pool");

            tasks_.emplace([task] { (*task)(); });

            auto need_new_thread =
                idle_threads_ < tasks_.size()
                    && workers_.size() < max_threads_;

            if( need_new_thread ) {
                std::unique_ptr<max_align_t> worker_ptr(
                    new max_align_t [sizeof(T) / sizeof(max_align_t)
                        + (sizeof(T) % sizeof(max_align_t) ? 1 : 0)]);
                T * p_worker = reinterpret_cast<T *>(worker_ptr.get());

                new (p_worker) T([&] (T * a_worker) {
                    static thread_local T * p_this_thread = nullptr;
                    p_this_thread = a_worker;

                    for(;;) {
                        std::unique_lock<std::mutex> lock(queue_mutex_);

                        auto now = std::chrono::system_clock::now();
                        using namespace std::chrono_literals;
                        auto deadline = now + 30s;
                        idle_threads_++;

                        auto wait_result = condition_.wait_until(lock, deadline, [&] {
                            return stop_ || !tasks_.empty();
                        });

                        if( !tasks_.empty() ) {
                            std::function<void()> task = std::move(tasks_.front());
                            tasks_.pop();

                            idle_threads_--;
                            lock.unlock();

                            task();
                        }
                        else if( !wait_result || stop_ ) { // timeout
                            idle_threads_--;

                            if( stop_ || workers_.size() > min_threads_ ) {
                                auto it = workers_.find(p_this_thread);

                                if( it == workers_.end() )
                                    throw std::runtime_error("undefined behavior in thread pool");

                                dead_.emplace_back(it->second);
                                workers_.erase(it);

                                if( stop_ ) {
                                    lock.unlock();
                                    condition_.notify_one();
                                }
                                break;
                            }
                        }
                    }
                }, p_worker);

                std::unique_ptr<T> worker(p_worker);
                workers_.emplace(std::make_pair(p_worker, p_worker));
                worker.release();
            }

            while( !dead_.empty() ) {
                std::unique_ptr<T> p(dead_.back());
                dead_.pop_back();
                p->join();
            }

            lock.unlock();
            condition_.notify_one();

            return res;
        }
    private:
        size_t min_threads_;
        size_t max_threads_;
        size_t idle_threads_;
        // need to keep track of threads so we can join them
        std::unordered_map<T *, T *> workers_;
        // dead threads by idle timeout
        std::vector<T *> dead_;
        // the task queue
        std::queue<std::function<void()>> tasks_;

        // synchronization
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        bool stop_;
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
