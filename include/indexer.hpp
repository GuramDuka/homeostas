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
#ifndef INDEXER_HPP_INCLUDED
#define INDEXER_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#if __GNUC__ >= 5
#include <io.h>
#endif
#include <functional>
#include <string>
#include <forward_list>
//------------------------------------------------------------------------------
#include "sqlite3pp/sqlite3pp.h"
#include "locale_traits.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct directory_reader {
    std::function<void()> manipulator_;

    std::string path_;
    std::string path_name_;
    std::string name_;
    std::string mask_;
    std::string exclude_;
    uintptr_t level_ = 0;
    uintptr_t max_level_ = 0;

    bool list_dot_ = false;
    bool list_dotdot_ = false;
    bool list_directories_ = false;
    bool recursive_ = false;

    uint64_t atime_ = 0;
    uint64_t ctime_ = 0;
    uint64_t mtime_ = 0;
    uint32_t atime_ns_ = 0;
    uint32_t ctime_ns_ = 0;
    uint32_t mtime_ns_ = 0;
    uint64_t fsize_ = 0;
    bool is_dir_ = false;
    bool is_reg_ = false;
    bool is_lnk_ = false;

    bool abort_ = false;
    bool skip_ = false;

    template <typename Manipul>
    void read(const std::string & root_path, const Manipul & ml) {
        this->manipulator_ = [&] {
            ml();
        };

        read(root_path);
    }

    void read(const std::string & root_path);
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class directory_indexer {
    private:
        bool modified_only_ = true;
    protected:
    public:
        const auto & modified_only() const {
            return modified_only_;
        }

        directory_indexer & modified_only(decltype(modified_only_) modified_only) {
            modified_only_ = modified_only;
            return *this;
        }

        void reindex(
            sqlite3pp::database & db,
            const std::string & dir_path_name,
            bool * p_shutdown = nullptr);
};
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void indexer_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // INDEXER_HPP_INCLUDED
//------------------------------------------------------------------------------
