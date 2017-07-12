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
#ifndef VARIANT_HPP_INCLUDED
#define VARIANT_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#if QT_CORE_LIB
#   include <QString>
#   include <QDateTime>
#   include <QVariant>
#endif
#include <cassert>
#include <type_traits>
#include <string>
#include <ostream>
#include <iomanip>
#include <array>
#include <vector>
//------------------------------------------------------------------------------
#include "std_ext.hpp"
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class variant {
public:
    enum Type {
        Null    = 0,
        Boolean = 1,
        Integer = 2,
        Real    = 3,
        Text    = 4,
        Blob    = 5,
        Key512  = 6
    };

    ~variant() {}
    variant() : type_(Null) {}

    variant(const variant & o) : type_(Null) {
        *this = o;
    }

    variant & operator = (const variant & o) {
        if( this != &o ) {
            clear();

            switch( o.type_ ) {
                case Null    :
                    break;
                case Boolean :
#if __GNUG__
                    b_ = o.b_;
#else
                    new (placeholder_) bool(*reinterpret_cast<const bool *>(o.placeholder_));
#endif
                    break;
                case Integer :
#if __GNUG__
                    i_ = o.i_;
#else
                    new (placeholder_) int64_t(*reinterpret_cast<const int64_t *>(o.placeholder_));
#endif
                    break;
                case Real    :
#if __GNUG__
                    r_ = o.r_;
#else
                    new (placeholder_) double(*reinterpret_cast<const double *>(o.placeholder_));
#endif
                    break;
                case Text    :
#if __GNUG__
                    new (&s_) string(o.s_);
#else
                    new (placeholder_) string(*reinterpret_cast<const string *>(o.placeholder_));
#endif
                    break;
                case Blob    :
#if __GNUG__
                    new (&l_) blob(o.l_);
#else
                    new (placeholder_) blob(*reinterpret_cast<const blob *>(o.placeholder_));
#endif
                    break;
                case Key512  :
#if __GNUG__
                    new (&k_) key512(o.k_);
#else
                    new (placeholder_) key512(*reinterpret_cast<const key512 *>(o.placeholder_));
#endif
                    break;
                default:
                    throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
            }

            type_ = o.type_;
        }

        return *this;
    }

    variant(variant && o) : type_(Null) {
        *this = move(o);
    }

    variant & operator = (variant && o) {
        if( this != &o ) {
            clear();

            switch( o.type_ ) {
                case Null    :
                    break;
                case Boolean :
#if __GNUG__
                    b_ = o.b_;
#else
                    new (placeholder_) bool(move(*reinterpret_cast<bool *>(o.placeholder_)));
#endif
                    break;
                case Integer :
#if __GNUG__
                    i_ = o.i_;
#else
                    new (placeholder_) int64_t(move(*reinterpret_cast<int64_t *>(o.placeholder_)));
#endif
                    break;
                case Real    :
#if __GNUG__
                    r_ = o.r_;
#else
                    new (placeholder_) double(move(*reinterpret_cast<double *>(o.placeholder_)));
#endif
                    break;
                case Text    :
#if __GNUG__
                    new (&s_) string(move(o.s_));
#else
                    new (placeholder_) string(move(*reinterpret_cast<string *>(o.placeholder_)));
#endif
                    break;
                case Blob    :
#if __GNUG__
                    new (&l_) blob(move(o.l_));
#else
                    new (placeholder_) blob(move(*reinterpret_cast<blob *>(o.placeholder_)));
#endif
                    break;
                case Key512  :
#if __GNUG__
                    new (&k_) key512(move(o.k_));
#else
                    new (placeholder_) key512(move(*reinterpret_cast<key512 *>(o.placeholder_)));
#endif
                    break;
                default:
                    throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
            }

            type_ = o.type_;
        }

        return *this;
    }

#if QT_CORE_LIB
    variant(const QVariant & o) : type_(Null) {
        *this = o;
    }

    variant & operator = (const QVariant & o) {
        clear();

        switch( o.type() ) {
            case QVariant::Invalid   :
                break;
            case QVariant::Bool      :
                new (placeholder_) bool(o.toBool());
                type_ = Boolean;
                break;
            case QVariant::Int       :
                new (placeholder_) int64_t(o.toInt());
                type_ = Integer;
                break;
            case QVariant::UInt      :
                new (placeholder_) int64_t(o.toUInt());
                type_ = Integer;
                break;
            case QVariant::LongLong  :
                new (placeholder_) int64_t(o.toLongLong());
                type_ = Integer;
                break;
            case QVariant::ULongLong :
                new (placeholder_) int64_t(o.toULongLong());
                type_ = Integer;
                break;
            case QVariant::Double    :
                new (placeholder_) double(o.toDouble());
                type_ = Real;
                break;
            case QVariant::String    :
                new (placeholder_) string(o.toString().toStdString());
                type_ = Text;
                break;
            default                  :
                throw xruntime_error("Unsupported QVariant type", __FILE__, __LINE__);
        }

        return *this;
    }

    template <typename T,
        typename enable_if<
            is_same<T, QVariant>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Boolean :
#if __GNUG__
                return b_;
#else
                return *reinterpret_cast<const bool *>(placeholder_);
#endif
            case Integer :
#if __GNUG__
                return QVariant::fromValue(i_);
#else
                return QVariant::fromValue(*reinterpret_cast<const int64_t *>(placeholder_));
#endif
            case Real    :
#if __GNUG__
                return r_;
#else
                return *reinterpret_cast<const double *>(placeholder_);
#endif
            case Text    :
#if __GNUG__
                return QString::fromStdString(s_);
#else
                return QString::fromStdString(*reinterpret_cast<const string *>(placeholder_));
#endif
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T();
    }

    QVariant toQVariant() const {
        return get<QVariant>();
    }

    template <typename T,
        typename enable_if<
            is_same<T, QString>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Boolean :
#if __GNUG__
                return b_ ? "true" : "false";
#else
                return *reinterpret_cast<const bool *>(placeholder_) ? "true" : "false";
#endif
            case Integer :
#if __GNUG__
                return QString::fromStdString(std::to_string(i_));
#else
                return QString::fromStdString(std::to_string(*reinterpret_cast<const int64_t *>(placeholder_)));
#endif
            case Real    :
#if __GNUG__
                return QString::fromStdString(std::to_string(r_));
#else
                return QString::fromStdString(std::to_string(*reinterpret_cast<const double *>(placeholder_)));
#endif
            case Text    :
#if __GNUG__
                return QString::fromStdString(s_);
#else
                return QString::fromStdString(*reinterpret_cast<const string *>(placeholder_));
#endif
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T();
    }

    QString toQString() const {
        return get<QString>();
    }
#endif

    variant & clear() {
        switch( type_ ) {
            case Null    :
                break;
            case Boolean :
                //reinterpret_cast<bool *>(placeholder_)->~bool();
                break;
            case Integer :
                //reinterpret_cast<int64_t *>(placeholder_)->~int64_t();
                break;
            case Real    :
                //reinterpret_cast<double *>(placeholder_)->~double();
                break;
            case Text    :
#if __GNUG__
                s_.~basic_string();
#else
                reinterpret_cast<string *>(placeholder_)->~basic_string();
#endif
                break;
            case Blob    :
#if __GNUG__
                l_.~vector();
#else
                reinterpret_cast<blob *>(placeholder_)->~blob();
#endif
                break;
            case Key512  :
#if __GNUG__
                k_.~key512();
#else
                reinterpret_cast<key512 *>(placeholder_)->~key512();
#endif
                break;
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        type_ = Null;

        return *this;
    }

    template <typename T, typename enable_if<is_same<T, bool>::value>::type * = nullptr>
    const T & ref() const {
#ifndef NDEBUG
        assert( type_ == Boolean );
#endif
#if __GNUG__
        return b_;
#else
        return *reinterpret_cast<const T *>(placeholder_);
#endif
    }

    template <typename T, typename enable_if<is_same<T, int64_t>::value>::type * = nullptr>
    const T & ref() const {
#ifndef NDEBUG
        assert( type_ == Integer );
#endif
#if __GNUG__
        return i_;
#else
        return *reinterpret_cast<const T *>(placeholder_);
#endif
    }

    template <typename T, typename enable_if<is_same<T, double>::value>::type * = nullptr>
    const T & ref() const {
#ifndef NDEBUG
        assert( type_ == Real );
#endif
#if __GNUG__
        return r_;
#else
        return *reinterpret_cast<const T *>(placeholder_);
#endif
    }

    template <typename T, typename enable_if<is_same<T, string>::value>::type * = nullptr>
    const T & ref() const {
#ifndef NDEBUG
        assert( type_ == Text );
#endif
#if __GNUG__
        return s_;
#else
        return *reinterpret_cast<const T *>(placeholder_);
#endif
    }

    template <typename T, typename enable_if<is_same<T, blob>::value>::type * = nullptr>
    const T & ref() const {
#ifndef NDEBUG
        assert( type_ == Blob );
#endif
#if __GNUG__
        return l_;
#else
        return *reinterpret_cast<const T *>(placeholder_);
#endif
    }

    template <typename T, typename enable_if<is_same<T, key512>::value>::type * = nullptr>
    const T & ref() const {
#ifndef NDEBUG
        assert( type_ == Key512 );
#endif
#if __GNUG__
        return k_;
#else
        return *reinterpret_cast<const T *>(placeholder_);
#endif
    }

// !!!WARNING!!! used only for debug
#ifndef NDEBUG
    template <typename T>
    const T & ref_debug() const {
        const void * vp = placeholder_;
        uintptr_t p = (uintptr_t) vp;
        const void * pv = (const void *) p;
        return *reinterpret_cast<const T *>(pv);
    }
#endif

    template <typename T,
        typename enable_if<
            is_same<T, bool>::value && !is_integral<T>::value
        >::type * = nullptr
    >
    variant & operator = (const T & v) {
        clear();
        *reinterpret_cast<bool *>(placeholder_) = bool(v);
        type_ = Boolean;
        return *this;
    }

    template <typename T,
        typename enable_if<
            !is_same<T, bool>::value && is_integral<T>::value
        >::type * = nullptr
    >
    variant & operator = (const T & v) {
        clear();
#if __GNUG__
        i_ = int64_t(v);
#else
        *reinterpret_cast<int64_t *>(placeholder_) = int64_t(v);
#endif
        type_ = Integer;
        return *this;
    }

    template <typename T,
        typename enable_if<
            is_floating_point<T>::value
        >::type * = nullptr
    >
    variant & operator = (const T & v) {
        clear();
#if __GNUG__
        r_ = double(v);
#else
        *reinterpret_cast<double *>(placeholder_) = double(v);
#endif
        type_ = Real;
        return *this;
    }

    variant & operator = (const char * v) {
        clear();
#if __GNUG__
        new (&s_) string(v);
#else
        new (placeholder_) string(v);
#endif
        type_ = Text;
        return *this;
    }

    variant & operator = (const string & v) {
        clear();
#if __GNUG__
        new (&s_) string(v);
#else
        new (placeholder_) string(v);
#endif
        type_ = Text;
        return *this;
    }

    variant & operator = (string && v) {
        clear();
#if __GNUG__
        new (&s_) string(move(v));
#else
        new (placeholder_) string(v);
#endif
        type_ = Text;
        return *this;
    }

    variant & operator = (const blob & v) {
        clear();
#if __GNUG__
        new (&l_) blob(v);
#else
        new (placeholder_) blob(v);
#endif
        type_ = Blob;
        return *this;
    }

    variant & operator = (blob && v) {
        clear();
#if __GNUG__
        new (&l_) blob(move(v));
#else
        new (placeholder_) blob(v);
#endif
        type_ = Blob;
        return *this;
    }

    variant & operator = (const key512 & v) {
        clear();
#if __GNUG__
        new (&k_) key512(v);
#else
        new (placeholder_) key512(v);
#endif
        type_ = Key512;
        return *this;
    }

    variant & operator = (key512 && v) {
        clear();
#if __GNUG__
        new (&k_) key512(move(v));
#else
        new (placeholder_) key512(v);
#endif
        type_ = Key512;
        return *this;
    }

    variant & operator = (const nullptr_t &) {
        clear();
        return *this;
    }

    template <typename T,
        typename enable_if<
            is_same<T, bool>::value
        >::type * = nullptr
    >
    variant(const T & v) : type_(Boolean) {
#if __GNUG__
        b_ = v;
#else
        *reinterpret_cast<bool *>(placeholder_) = v;
#endif
    }

    template <typename T,
        typename enable_if<
            !is_same<T, bool>::value && is_integral<T>::value
        >::type * = nullptr
    >
    variant(const T & v) : type_(Null) {
        *this = v;
    }

    template <typename T,
        typename enable_if<
            is_floating_point<T>::value
        >::type * = nullptr
    >
    variant(const T & v) : type_(Null) {
        *this = v;
    }

    variant(const char * v) : type_(Null) {
        *this = v;
    }

    variant(const string & v) : type_(Null) {
        *this = v;
    }

    variant(string && v) : type_(Null) {
        *this = move(v);
    }

    variant(const blob & v) : type_(Null) {
        *this = v;
    }

    variant(blob && v) : type_(Null) {
        *this = move(v);
    }

    variant(const key512 & v) : type_(Null) {
        *this = v;
    }

    variant(key512 && v) : type_(Null) {
        *this = move(v);
    }

    variant(const nullptr_t &) : type_(Null) {}

    template <typename T,
        typename enable_if<
            is_same<T, bool>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Boolean :
#if __GNUG__
                return b_;
#else
                return *reinterpret_cast<const bool *>(placeholder_);
#endif
            case Integer :
#if __GNUG__
                return i_;
#else
                return *reinterpret_cast<const int64_t *>(placeholder_) != 0;
#endif
            case Real    :
#if __GNUG__
                return r_;
#else
                return *reinterpret_cast<const double *>(placeholder_) != 0;
#endif
            case Text    :
                return [this] {
#if __GNUG__
                    const auto & s = s_;
#else
                    const auto & s = *reinterpret_cast<const string *>(placeholder_);
#endif

                    if( s.empty() || equal_case(s, "false") || equal_case(s, "no") || equal_case(s, "off") )
                        return false;
                    if( equal_case(s, "true") || equal_case(s, "yes") || equal_case(s, "on") )
                        return true;

                    throw xruntime_error("Invalid variant value", __FILE__, __LINE__);
                }();
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return false;
    }

    template <typename T,
        typename enable_if<
            !is_same<T, bool>::value && is_integral<T>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Boolean :
#if __GNUG__
                return b_ ? T(1) : T(0);
#else
                return *reinterpret_cast<const bool *>(placeholder_) ? T(1) : T(0);
#endif
            case Integer :
#if __GNUG__
                return T(i_);
#else
                return T(*reinterpret_cast<const int64_t *>(placeholder_));
#endif
            case Real    :
#if __GNUG__
                return T(r_);
#else
                return T(*reinterpret_cast<const double *>(placeholder_));
#endif
            case Text    :
#if __GNUG__
                return stoint<T>(begin(s_), end(s_));
#else
                return stoint<T>(begin(*reinterpret_cast<const string *>(placeholder_)),
                    end(*reinterpret_cast<const string *>(placeholder_)));
#endif
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T(0);
    }

    template <typename T,
        typename enable_if<
            is_floating_point<T>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Boolean :
#if __GNUG__
                return b_ ? T(1) : T(0);
#else
                return *reinterpret_cast<const bool *>(placeholder_) ? 1 : 0;
#endif
            case Integer :
#if __GNUG__
                return T(i_);
#else
                return T(*reinterpret_cast<const int64_t *>(placeholder_));
#endif
            case Real    :
#if __GNUG__
                return T(r_);
#else
                return T(*reinterpret_cast<const double *>(placeholder_));
#endif
            case Text    :
#if __GNUG__
                return T(stod(s_));
#else
                return T(stod(*reinterpret_cast<const string *>(placeholder_)));
#endif
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T(0);
    }

    template <typename T,
        typename enable_if<
            is_base_of<string, T>::value && !is_const<T>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Boolean :
#if __GNUG__
                return b_ ? "true" : "false";
#else
                return *reinterpret_cast<const bool *>(placeholder_) ? "true" : "false";
#endif
            case Integer :
#if __GNUG__
                return to_string(i_);
#else
                return to_string(*reinterpret_cast<const int64_t *>(placeholder_));
#endif
            case Real    :
#if __GNUG__
                return to_string(r_);
#else
                return to_string(*reinterpret_cast<const double *>(placeholder_));
#endif
            case Text    :
#if __GNUG__
                return s_;
#else
                return *reinterpret_cast<const string *>(placeholder_);
#endif
            case Blob    :
#if __GNUG__
                return to_string(l_);
#else
                return to_string(*reinterpret_cast<const blob *>(placeholder_));
#endif
            case Key512  :
#if __GNUG__
            return to_string(k_);
#else
            return to_string(*reinterpret_cast<const key512 *>(placeholder_));
#endif
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T();
    }

    template <typename T,
        typename enable_if<
            is_base_of<blob, T>::value && !is_const<T>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Blob    :
#if __GNUG__
                return l_;
#else
                return *reinterpret_cast<const blob *>(placeholder_);
#endif
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T();
    }

    template <typename T,
        typename enable_if<
            is_base_of<key512, T>::value && !is_const<T>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Key512    :
#if __GNUG__
                return k_;
#else
                return *reinterpret_cast<const key512 *>(placeholder_);
#endif
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T();
    }

    const auto & type() const {
        return type_;
    }

    auto & type(Type type) {
        *this = cast(type);
        return *this;
    }

    variant cast(Type type) const {
        if( type_ == type )
            return *this;

        switch( type ) {
            case Null    :
                break;
            case Boolean :
                return get<bool>();
            case Integer :
                return get<int64_t>();
            case Real    :
                return get<double>();
            case Text    :
                return get<string>();
            case Blob    :
                return get<blob>();
            case Key512  :
                return get<key512>();
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return variant();
    }

    template <typename T,
        typename enable_if<
            is_same<T, bool>::value
        >::type * = nullptr
    >
    operator T () const {
        return type_ == Boolean ? ref<T>() : cast(Boolean).get<T>();
    }

    template <typename T,
        typename enable_if<
            !is_same<T, bool>::value && is_integral<T>::value
        >::type * = nullptr
    >
    operator T () const {
        return type_ == Integer ? T(ref<int64_t>()) : cast(Integer).get<T>();
    }

    template <typename T,
        typename enable_if<
            is_floating_point<T>::value
        >::type * = nullptr
    >
    operator T () const {
        return type_ == Integer ? T(ref<double>()) : cast(Real).get<T>();
    }

    template <typename T,
        typename enable_if<
            is_base_of<string, T>::value && !is_const<T>::value
        >::type * = nullptr
    >
    operator T () const {
        return type_ == Text ? T(ref<T>()) : cast(Text).get<T>();
    }

    template <typename T,
        typename enable_if<
            is_base_of<blob, T>::value && !is_const<T>::value
        >::type * = nullptr
    >
    operator T () const {
        return type_ == Blob ? T(ref<T>()) : cast(Blob).get<T>();
    }

    template <typename T,
        typename enable_if<
            is_base_of<key512, T>::value && !is_const<T>::value
        >::type * = nullptr
    >
    operator T () const {
        return type_ == Key512 ? T(ref<T>()) : cast(Key512).get<T>();
    }

    bool operator == (const nullptr_t) const {
        return type_ == Null;
    }

    bool operator != (const nullptr_t) const {
        return type_ != Null;
    }

    bool to_bool() const {
        if( type_ == Boolean )
            return get<bool>();
        return cast(Boolean).get<bool>();
    }

    int64_t to_int() const {
        if( type_ == Integer )
            return get<int64_t>();
        return cast(Integer).get<int64_t>();
    }

    double to_real() const {
        if( type_ == Real )
            return get<double>();
        return cast(Real).get<double>();
    }

    std::string to_text() const {
        if( type_ == Text )
            return get<std::string>();
        return cast(Text).get<std::string>();
    }

    blob to_blob() const {
        if( type_ == Blob )
            return get<blob>();
        return cast(Blob).get<blob>();
    }

    key512 to_key512() const {
        if( type_ == Key512 )
            return get<key512>();
        return cast(Key512).get<key512>();
    }
protected:
    union {
        uint8_t placeholder_[
            xmax(sizeof(key512),
                xmax(sizeof(blob),
                     xmax(sizeof(double),
                        xmax(sizeof(int64_t),
                            xmax(sizeof(bool),
                                sizeof(string))))))];
#if __GNUG__
        bool b_;
        int64_t i_;
        double r_;
        string s_;
        blob l_;
        key512 k_;
#endif
    };
    Type type_;
};
//------------------------------------------------------------------------------
inline auto to_string(const variant & v) {
    return v.get<string>();
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits> inline
basic_ostream<_Elem, _Traits> & operator << (
    basic_ostream<_Elem, _Traits> & ss,
    const variant & v)
{
    return ss << v.get<string>();
}
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
#endif // VARIANT_HPP_INCLUDED
//------------------------------------------------------------------------------
