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
#ifndef CONFIGURATION_HPP_INCLUDED
#define CONFIGURATION_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <string>
#include <cassert>
#include <unordered_map>
#include <iterator>
//------------------------------------------------------------------------------
#include "variant.hpp"
#include "std_ext.hpp"
#include "sqlite3pp/sqlite3pp.h"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
class configuration;
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class variable {
friend class configuration;
public:
    typedef typename std::unordered_map<std::string, variable> childs_type;

    ~variable() {
        childs()->~childs_type();
    }

    variable(const variable & o) {
        new (childs_) childs_type;
        *this = o;
    }

    variable & operator = (const variable & o) {
        if( this != &o ) {
            pid_      = o.pid_;
            id_       = o.id_;
            name_     = o.name_;
            value_    = o.value_;
            *childs() = *o.childs();
        }
        return *this;
    }

    variable(variable && o) {
        new (childs_) childs_type;
        *this = std::move(o);
    }

    variable & operator = (variable && o) {
        if( this != &o ) {
            pid_      = std::move(o.pid_);
            id_       = std::move(o.id_);
            name_     = std::move(o.name_);
            value_    = std::move(o.value_);
            *childs() = std::move(*o.childs());
        }
        return *this;
    }

    template <typename std::size_t _Sz>
    auto & operator [] (const char (&name) [_Sz]) {
        return childs()->at(name);
    }

    template <typename std::size_t _Sz>
    const auto & operator [] (const char (&name) [_Sz]) const {
        return childs()->at(name);
    }

    auto & operator [] (const std::string & name) {
        return childs()->at(name);
    }

    const auto & operator [] (const std::string & name) const {
        return childs()->at(name);
    }

    template <typename std::size_t _Sz>
    auto find(const char (&name) [_Sz]) {
        return childs()->find(name);
    }

    template <typename std::size_t _Sz>
    auto find(const char (&name) [_Sz]) const {
        return childs()->find(name);
    }

    auto find(const std::string & name) {
        return childs()->find(name);
    }

    auto find(const std::string & name) const {
        return childs()->find(name);
    }

    template <typename std::size_t _Sz>
    auto exists(const char (&name) [_Sz]) {
        return childs()->find(name) != childs()->end();
    }

    template <typename std::size_t _Sz>
    auto exists(const char (&name) [_Sz]) const {
        return childs()->find(name) != childs()->end();
    }

    auto exists(const std::string & name) {
        return childs()->find(name) != childs()->end();
    }

    auto exists(const std::string & name) const {
        return childs()->find(name) != childs()->end();
    }

#if QT_CORE_LIB
    auto & operator [] (const QString & name) {
        return childs()->at(name.toStdString());
    }

    const auto & operator [] (const QString & name) const {
        return childs()->at(name.toStdString());
    }

    auto find(const QString & name) {
        return childs()->find(name.toStdString());
    }

    auto find(const QString & name) const {
        return childs()->find(name.toStdString());
    }

    auto exists(const QString & name) {
        return childs()->find(name.toStdString()) != childs()->end();
    }

    auto exists(const QString & name) const {
        return childs()->find(name.toStdString()) != childs()->end();
    }
#endif

    auto empty() const {
        return childs()->empty();
    }

    auto begin() {
        return childs()->begin();
    }

    auto begin() const {
        return childs()->begin();
    }

    auto cbegin() const {
        return childs()->cbegin();
    }

    auto end() {
        return childs()->end();
    }

    auto end() const {
        return childs()->end();
    }

    auto cend() const {
        return childs()->cend();
    }

    const auto & name() const {
        return name_;
    }

    const auto & value() const {
        return value_;
    }
protected:
    variable() {
        static_assert( sizeof(childs_) >= sizeof(childs_type), "sizeof(childs_) >= sizeof(childs_type)" );
        new (childs_) childs_type;
    }

    variable(uint64_t pid, uint64_t id, const char * name, std::variant && value) :
        pid_(pid), id_(id), name_(name), value_(std::move(value))
    {
        static_assert( sizeof(childs_) >= sizeof(childs_type), "sizeof(childs_) >= sizeof(childs_type)" );
        new (childs_) childs_type;
    }

    childs_type * childs() {
        return reinterpret_cast<childs_type *>(childs_);
    }

    const childs_type * childs() const {
        return reinterpret_cast<const childs_type *>(childs_);
    }

    uint64_t pid_;
    uint64_t id_;
    std::string name_;
    std::variant value_;
#ifndef NDEBUG
    const bool & bool_               = value_.ref_debug<bool>();
    const int64_t & int_             = value_.ref_debug<int64_t>();
    const double & real_             = value_.ref_debug<double>();
    const std::string & string_      = value_.ref_debug<std::string>();
    const std::blob & blob_          = value_.ref_debug<std::blob>();
    const std::key512 & key_         = value_.ref_debug<std::key512>();
#endif

    //mutable childs_type childs_;
    mutable uint8_t childs_[sizeof(std::unordered_map<std::string, std::string>)];
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class configuration {
public:
    ~configuration();
    configuration();

    static auto instance() {
        static configuration singleton;
        return &singleton;
    }

    configuration & set(const char * var_name, const std::variant & val);

    template <typename std::size_t _Sz>
    configuration & set(const char * var_name, const char (&val) [_Sz]) {
        return set(var_name, val, 0);
    }

    template <typename std::size_t _Sz>
    auto & set(const std::string & var_name, const char (&val) [_Sz]) {
        return set(var_name.c_str(), val, 0);
    }

    auto & set(const std::string & var_name, const std::variant & val) {
        return set(var_name.c_str(), val);
    }

    std::variant get(const char * var_name, const std::variant * def_val = nullptr);

    auto get(const char * var_name, const std::variant & def_val) {
        return get(var_name, &def_val);
    }

    auto get(const std::string & var_name, const std::variant & def_val) {
        return get(var_name.c_str(), &def_val);
    }

    configuration & set(const variable & var) {
        connect_db();

        sqlite3pp::transaction tx(db_.get());

        for( const auto & v : var )
            set(v.second);

        set(var.pid_, var.id_, var.name_.c_str(), var.value_);

        return *this;
    }

    variable get_tree(const char * var_name);

    auto get_tree(const std::string & var_name) {
        return get_tree(var_name.c_str());
    }

    bool exists(const char * var_name);

    bool exists(const std::string & var_name) {
        return exists(var_name.c_str());
    }

#if QT_CORE_LIB
    struct string_hash {
        size_t operator () (const QString & val) const {
            return std::itifhash<size_t>(val.begin(), val.end(),
                [] (const auto & c) { return c.unicode(); });
        }
    };

    struct string_equal {
       bool operator () (const QString & val1, const QString & val2) const {
          return val1.compare(val2, Qt::CaseSensitive) == 0;
       }
    };

    QVariant get(const QString & varName, const QVariant * p_defVal) {
        return get(varName.toStdString().c_str(), p_defVal != nullptr ? std::variant(*p_defVal) : std::variant()).toQVariant();
    }

    QVariant get(const QString & varName, const QVariant & defVal) {
        return get(varName.toStdString().c_str(), std::variant(defVal)).toQVariant();
    }

    auto & set(const QString & varName, const QVariant & val) {
        return set(varName.toStdString().c_str(), std::variant(val));
    }

    bool exists(const QString & var_name) {
        return exists(var_name.toStdString().c_str());
    }
#endif

    auto & detach_db() {
        st_sel_ = nullptr;
        st_sel_by_pid_ = nullptr;
        st_ins_ = nullptr;
        st_upd_ = nullptr;
        st_row_next_id_ = nullptr;
        db_ = nullptr;
        return *this;
    }
protected:
    configuration & set(const char * var_name, const char * val, int);
private:
    configuration(const configuration &) = delete;
    void operator = (const configuration &) = delete;

    void connect_db();

    std::variant get(uint64_t pid, const char * var_name, const std::variant * p_def_val = nullptr, uint64_t * p_id = nullptr);
    uint64_t get_pid(const char * var_name, const char ** p_name, bool * p_pid_valid);

    std::variant get(sqlite3pp::query::iterator & i) {
        int vt = i->column_isnull("value_type") ? std::variant::Null : i->get<int>("value_type");

        switch( vt ) {
            case std::variant::Null    :
                break;
            case std::variant::Boolean :
                return i->get<int>("value") ? true : false;
            case std::variant::Integer :
                return i->get<int64_t>("value");
            case std::variant::Real    :
                return i->get<double>("value");
            case std::variant::Text    :
                return i->get<const char *>("value");
            case std::variant::Blob    :
                return i->get<std::blob>("value");
            case std::variant::Key512    :
                return i->get<std::key512>("value");
            default                    :
                throw std::xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return nullptr;
    }

    template <typename F>
    auto & set(uint64_t pid, uint64_t id, const char * var_name, F val_setter) {
        connect_db();

        auto bindey = [&] (auto & st) {
            st->bind("parent_id", pid);
            st->bind("name"     , var_name, sqlite3pp::nocopy);
        };

        auto bindex = [&] (auto & st) {
            bindey(st);
            st->bind("value_b"  , nullptr);
            st->bind("value_i"  , nullptr);
            st->bind("value_n"  , nullptr);
            st->bind("value_s"  , nullptr);
            st->bind("value_l"  , nullptr);
        };

        if( id == 0 )
            id = row_next_id_();

        //bindey(st_sel_);
        //auto i = st_sel_->begin();
        db_->exceptions(false);
        st_ins_->bind("id", id);
        bindex(st_ins_);
        val_setter(st_ins_);
        db_->exceptions(true);

        if( st_ins_->rc() == SQLITE_CONSTRAINT_UNIQUE ) {
            bindex(st_upd_);
            val_setter(st_upd_);
        }

        return *this;
    }

    configuration & set(uint64_t pid, uint64_t id, const char * var_name, const std::variant & val) {
        return set(pid, id, var_name, [&] (auto & st) {
            switch( val.type() ) {
                case std::variant::Null            :
                    break;
                case std::variant::Boolean         :
                    st->bind("value_type", int8_t(std::variant::Boolean));
                    st->bind("value_b", val.get<bool>());
                    break;
                case std::variant::Integer         :
                    st->bind("value_type", int8_t(std::variant::Integer));
                    st->bind("value_i", val.get<int64_t>());
                    break;
                case std::variant::Real            :
                    st->bind("value_type", int8_t(std::variant::Real));
                    st->bind("value_n", val.get<double>());
                    break;
                case std::variant::Text            :
                    st->bind("value_type", int8_t(std::variant::Text));
                    st->bind("value_s", val.ref<std::string>(), sqlite3pp::nocopy);
                    break;
                case std::variant::Blob            :
                    st->bind("value_type", int8_t(std::variant::Blob));
                    st->bind("value_l", val.ref<std::blob>(), sqlite3pp::nocopy);
                    break;
                case std::variant::Key512          :
                    st->bind("value_type", int8_t(std::variant::Key512));
                    st->bind("value_l", val.ref<std::key512>(), sqlite3pp::nocopy);
                    break;
                default                         :
                    throw std::xruntime_error("Invalid variant type", __FILE__, __LINE__);
            }

            st->execute();
        });
    }

    uint64_t row_id_;
    std::function<uint64_t()> row_next_id_;
    std::unique_ptr<sqlite3pp::database> db_;
    std::unique_ptr<sqlite3pp::query> st_row_next_id_;
    std::unique_ptr<sqlite3pp::query> st_sel_;
    std::unique_ptr<sqlite3pp::query> st_sel_by_pid_;
    std::unique_ptr<sqlite3pp::command> st_ins_;
    std::unique_ptr<sqlite3pp::command> st_upd_;
};
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // CONFIGURATION_HPP_INCLUDED
//------------------------------------------------------------------------------
