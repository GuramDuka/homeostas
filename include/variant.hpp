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
                    new (placeholder_) bool(*reinterpret_cast<const bool *>(o.placeholder_));
                    break;
                case Integer :
                    new (placeholder_) int64_t(*reinterpret_cast<const int64_t *>(o.placeholder_));
                    break;
                case Real    :
                    new (placeholder_) double(*reinterpret_cast<const double *>(o.placeholder_));
                    break;
                case Text    :
                    new (placeholder_) string(*reinterpret_cast<const string *>(o.placeholder_));
                    break;
                case Blob    :
                    new (placeholder_) blob(*reinterpret_cast<const blob *>(o.placeholder_));
                    break;
                case Key512  :
                    new (placeholder_) key512(*reinterpret_cast<const key512 *>(o.placeholder_));
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
                    new (placeholder_) bool(move(*reinterpret_cast<bool *>(o.placeholder_)));
                    break;
                case Integer :
                    new (placeholder_) int64_t(move(*reinterpret_cast<int64_t *>(o.placeholder_)));
                    break;
                case Real    :
                    new (placeholder_) double(move(*reinterpret_cast<double *>(o.placeholder_)));
                    break;
                case Text    :
                    new (placeholder_) string(move(*reinterpret_cast<string *>(o.placeholder_)));
                    break;
                case Blob    :
                    new (placeholder_) blob(move(*reinterpret_cast<blob *>(o.placeholder_)));
                    break;
                case Key512  :
                    new (placeholder_) key512(move(*reinterpret_cast<key512 *>(o.placeholder_)));
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
                return *reinterpret_cast<const bool *>(placeholder_);
            case Integer :
                return QVariant::fromValue(*reinterpret_cast<const int64_t *>(placeholder_));
            case Real    :
                return *reinterpret_cast<const double *>(placeholder_);
            case Text    :
                return QString::fromStdString(*reinterpret_cast<const string *>(placeholder_));
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
                return *reinterpret_cast<const bool *>(placeholder_) ? "true" : "false";
            case Integer :
                return QString::fromStdString(std::to_string(*reinterpret_cast<const int64_t *>(placeholder_)));
            case Real    :
                return QString::fromStdString(std::to_string(*reinterpret_cast<const double *>(placeholder_)));
            case Text    :
                return QString::fromStdString(*reinterpret_cast<const string *>(placeholder_));
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
                reinterpret_cast<string *>(placeholder_)->~basic_string();
                break;
            case Blob    :
                reinterpret_cast<blob *>(placeholder_)->~blob();
                break;
            case Key512  :
                reinterpret_cast<key512 *>(placeholder_)->~key512();
                break;
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        type_ = Null;

        return *this;
    }

    template <typename T,
        typename enable_if<
            is_same<T, bool>::value ||
            is_same<T, int64_t>::value ||
            is_same<T, double>::value ||
            is_same<T, string>::value ||
            is_same<T, blob>::value ||
            is_same<T, key512>::value
        >::type * = nullptr
    >
    const T & reference() const {
        return *reinterpret_cast<const T *>(placeholder_);
    }

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
        *reinterpret_cast<int64_t *>(placeholder_) = int64_t(v);
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
        *reinterpret_cast<double *>(placeholder_) = double(v);
        type_ = Real;
        return *this;
    }

    variant & operator = (const char * v) {
        clear();
        new (placeholder_) string(v);
        type_ = Text;
        return *this;
    }

    variant & operator = (const string & v) {
        clear();
        new (placeholder_) string(v);
        type_ = Text;
        return *this;
    }

    variant & operator = (string && v) {
        clear();
        new (placeholder_) string(move(v));
        type_ = Text;
        return *this;
    }

    variant & operator = (const blob & v) {
        clear();
        new (placeholder_) blob(v);
        type_ = Blob;
        return *this;
    }

    variant & operator = (blob && v) {
        clear();
        new (placeholder_) blob(move(v));
        type_ = Blob;
        return *this;
    }

    variant & operator = (const key512 & v) {
        clear();
        new (placeholder_) key512(v);
        type_ = Key512;
        return *this;
    }

    variant & operator = (key512 && v) {
        clear();
        new (placeholder_) key512(move(v));
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
        *reinterpret_cast<bool *>(placeholder_) = v;
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
                return *reinterpret_cast<const bool *>(placeholder_);
            case Integer :
                return *reinterpret_cast<const int64_t *>(placeholder_) != 0;
            case Real    :
                return *reinterpret_cast<const double *>(placeholder_) != 0;
            case Text    :
                return [this] {
                    const auto & s = *reinterpret_cast<const string *>(placeholder_);

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
                return *reinterpret_cast<const bool *>(placeholder_) ? 1 : 0;
            case Integer :
                return T(*reinterpret_cast<const int64_t *>(placeholder_));
            case Real    :
                return T(*reinterpret_cast<const double *>(placeholder_));
            case Text    :
                return stoint<T>(begin(*reinterpret_cast<const string *>(placeholder_)),
                    end(*reinterpret_cast<const string *>(placeholder_)));
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
                return *reinterpret_cast<const bool *>(placeholder_) ? 1 : 0;
            case Integer :
                return T(*reinterpret_cast<const int64_t *>(placeholder_));
            case Real    :
                return T(*reinterpret_cast<const double *>(placeholder_));
            case Text    :
                return T(stod(*reinterpret_cast<const string *>(placeholder_)));
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T(0);
    }

    template <typename T,
        typename enable_if<
            is_base_of<T, string>::value && !is_const<T>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Boolean :
                return *reinterpret_cast<const bool *>(placeholder_) ? "true" : "false";
            case Integer :
                return std::to_string(*reinterpret_cast<const int64_t *>(placeholder_));
            case Real    :
                return std::to_string(*reinterpret_cast<const double *>(placeholder_));
            case Text    :
                return *reinterpret_cast<const string *>(placeholder_);
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T();
    }

    template <typename T,
        typename enable_if<
            is_base_of<T, blob>::value && !is_const<T>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Blob    :
                return *reinterpret_cast<const blob *>(placeholder_);
            default:
                throw xruntime_error("Invalid variant type", __FILE__, __LINE__);
        }

        return T();
    }

    template <typename T,
        typename enable_if<
            is_base_of<T, key512>::value && !is_const<T>::value
        >::type * = nullptr
    >
    T get() const {
        switch( type_ ) {
            case Null    :
                break;
            case Key512    :
                return *reinterpret_cast<const key512 *>(placeholder_);
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
        if( type_ != type ) {
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
        }

        return variant();
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

    std::string to_string() const {
        if( type_ == Text )
            return get<std::string>();
        return cast(Text).get<std::string>();
    }

    blob to_blob() const {
        if( type_ == Blob )
            return get<blob>();
        return cast(Blob).get<blob>();
    }

    key512 to_digest() const {
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
    };
#ifndef NDEBUG
    const bool & bool_          = *reinterpret_cast<const bool *>(placeholder_);
    const int64_t & int_        = *reinterpret_cast<const int64_t *>(placeholder_);
    const double & real_        = *reinterpret_cast<const double *>(placeholder_);
    const std::string & string_ = *reinterpret_cast<const std::string *>(placeholder_);
    const blob & blob_          = *reinterpret_cast<const blob *>(placeholder_);
    const key512 & digest_      = *reinterpret_cast<const key512 *>(placeholder_);
#endif
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
