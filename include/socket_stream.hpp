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
#ifndef SOCKET_STREAM_HPP_INCLUDED
#define SOCKET_STREAM_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "ciphers.hpp"
#include "socket.hpp"
//------------------------------------------------------------------------------
namespace homeostas { namespace handshake {
//------------------------------------------------------------------------------
enum Proto {
    ProtocolRAW = 0,
    ProtocolV1  = 1
};
//------------------------------------------------------------------------------
enum Encryption {
    EncryptionNone   = 0,
    EncryptionLight  = 1,
    EncryptionStrong = 2
};
//------------------------------------------------------------------------------
enum Compression {
    CompressionNone = 0,
    CompressionLZ4  = 1,
    CompressionZSTD = 2
};
//------------------------------------------------------------------------------
enum Option {
    OptionDisable  = 0,  // only try a non encrypted connection
    OptionAllow    = 1,  // first try a non encrypted connection; if that fails, try an encrypted connection
    OptionPrefer   = 2,  // first try an encrypted connection; if that fails, try a non encrypted connection
    OptionRequired = 3   // only try an encrypted connection
};
//------------------------------------------------------------------------------
enum Error {
    ErrorNone                = 0,
    ErrorRestartHandshake    = 1, // not error
    ErrorInvalidProto        = 2,
    ErrorInvalidEncryption   = 3,
    ErrorEncryptionRequired  = 4,
    ErrorInvalidCompression  = 5,
    ErrorCompressionRequired = 6,
    ErrorInvalidFingerprint  = 7
};
//------------------------------------------------------------------------------
inline void throw_if_error(Error e) {
    switch( e ) {
        case ErrorNone                :
            break;
        case ErrorInvalidProto        :
            throw std::xruntime_error("Invalid protocol", __FILE__, __LINE__);
        case ErrorInvalidEncryption   :
            throw std::xruntime_error("Invalid encryption type", __FILE__, __LINE__);
        case ErrorEncryptionRequired  :
            throw std::xruntime_error("Encryption required", __FILE__, __LINE__);
        case ErrorInvalidCompression  :
            throw std::xruntime_error("Invalid compression type", __FILE__, __LINE__);
        case ErrorCompressionRequired :
            throw std::xruntime_error("Compression required", __FILE__, __LINE__);
        case ErrorInvalidFingerprint  :
            throw std::xruntime_error("Invalid fingerprint", __FILE__, __LINE__);
        default                       :
            throw std::xruntime_error("Unknown error", __FILE__, __LINE__);
    }
}
//------------------------------------------------------------------------------
inline void throw_if_error(uint8_t e) {
    throw_if_error(Error(e));
}
//------------------------------------------------------------------------------
#if _WIN32
#   pragma pack(1)
#endif
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct PACKED packet {
    void generate_session_key() {
        cdc512 ctx(leave_uninitialized);
        ctx.generate_entropy_fast();
        std::copy(std::begin(session_key), std::end(session_key), std::begin(ctx), std::end(ctx));
    }

    void scramble() {
        auto chiper = std::make_unique<light_cipher>();
        chiper->init(session_key);
        auto arena = std::make_range(public_key, sizeof(*this) - offsetof(packet, public_key));
        chiper->encode(arena, arena);
    }

    std::key512::value_type session_key[std::key512::ssize()];
    std::key512::value_type public_key [std::key512::ssize()];
    std::key512::value_type fingerprint[std::key512::ssize()];
    uint8_t                 error;
    uint8_t                 proto_version;
    uint8_t                 encryption         :6;
    uint8_t                 encryption_option  :2;
    uint8_t                 compression        :6;
    uint8_t                 compression_option :2;
};
//------------------------------------------------------------------------------
#if _WIN32
#   pragma pack()
#endif
//------------------------------------------------------------------------------
} // namespace handshake
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_streambuf : public std::basic_streambuf<_Elem, _Traits> {
public:
#if __GNUG__
    using typename std::basic_streambuf<_Elem, _Traits>::__streambuf_type;
    using typename __streambuf_type::traits_type;
    using typename __streambuf_type::char_type;
    using typename __streambuf_type::int_type;

    using __streambuf_type::gptr;
    using __streambuf_type::egptr;
    using __streambuf_type::gbump;
    using __streambuf_type::setg;
    using __streambuf_type::eback;
    using __streambuf_type::pbase;
    using __streambuf_type::pptr;
    using __streambuf_type::epptr;
    using __streambuf_type::pbump;
    using __streambuf_type::setp;
#endif

    virtual ~basic_socket_streambuf() {
        overflow(traits_type::eof()); // flush write buffer
    }

    basic_socket_streambuf(basic_socket & socket) : basic_socket_streambuf(&socket) {}

    basic_socket_streambuf(basic_socket * socket) {
        reset(socket);
    }

    void reset(basic_socket * socket) {
        socket_ = socket;
        gbuf_ = std::make_unique<gbuf_type>();
        setg(gbuf_->data(), gbuf_->data() + gbuf_->size(), gbuf_->data() + gbuf_->size());
        pbuf_ = std::make_unique<pbuf_type>();
        setp(pbuf_->data(), pbuf_->data() + pbuf_->size());
    }

    void reset(basic_socket & socket) {
        reset(&socket);
    }

    void reset() {
        reset(socket_);
    }

protected:
    virtual std::streamsize showmanyc() {
        // return the number of chars in the input sequence
        if( gptr() != nullptr && gptr() < egptr() )
            return egptr() - gptr();

        return 0;
    }

    virtual std::streamsize xsgetn(char_type * s, std::streamsize n) {
        int rval = showmanyc();

        if( rval >= n ) {
            memcpy(s, gptr(), n * sizeof(char_type));
            gbump(n);
            return n;
        }

        memcpy(s, gptr(), rval * sizeof(char_type));
        gbump(rval);

        if( underflow() != traits_type::eof() )
            return rval + xsgetn(s + rval, n - rval);

        return rval;
    }

    virtual int_type underflow() {
        assert( gptr() != nullptr );
        // input stream has been disabled
        // return traits_type::eof();

        if( gptr() >= egptr() ) {
            server_side_handshake();

            auto exceptions_safe = socket_->exceptions();
            at_scope_exit( socket_->exceptions(exceptions_safe) );
            socket_->exceptions(false);

            auto r = socket_->recv(eback(), 0, gbuf_->size() * sizeof(char_type));

            if( encryption_ == handshake::EncryptionLight ) {
                light_chiper_decryptor_->encode(std::make_range((const uint8_t *) eback(), r), std::make_range((uint8_t *) eback(), r));
            }
            else if( encryption_ == handshake::EncryptionStrong ) {
                strong_chiper_decryptor_->encode(std::make_range((const uint8_t *) eback(), r), std::make_range((uint8_t *) eback(), r));
            }

            if( r <= 0 )
                return traits_type::eof();

            // reset input buffer pointers
            setg(gbuf_->data(), gbuf_->data(), gbuf_->data() + r);
        }

        return *gptr();
    }

    virtual int_type uflow() {
        auto ret = underflow();

        if( ret != traits_type::eof() )
            gbump(1);

        return ret;
    }

    virtual int_type pbackfail(int_type c = traits_type::eof()) {
        c = c;
        return traits_type::eof();
    }

    virtual std::streamsize xsputn(const char_type * s, std::streamsize n) {
        std::streamsize x = 0;

        while( n != 0 ) {
            auto wval = epptr() - pptr();
            auto mval = std::min(wval, decltype(wval) (n));

            memcpy(pptr(), s, mval * sizeof(char_type));
            pbump(mval);

            n -= mval;
            s += mval;
            x += mval;

            if( n == 0 || overflow() == traits_type::eof() )
                break;
        }

        return x;
    }

    virtual int_type overflow(int_type c = traits_type::eof()) {
        // if pbase () == 0, no write is allowed and thus return eof.
        // if c == eof, we sync the output and return 0.
        // if pptr () == epptr (), buffer is full and thus sync the output,
        //                         insert c into buffer, and return c.
        // In all cases, if error happens, throw exception.

        if( pbase() == nullptr )
            return traits_type::eof();

        if( c == traits_type::eof() )
            return sync();

        if( pptr() == epptr() )
            sync();

        *pptr() = char_type(c);
        pbump(1);

        return c;
    }

    virtual int sync() {
        // returns ​0​ on success, -1 otherwise.
        auto wval = pptr() - pbase();

        // we have some data to flush
        if( wval != 0 ) {
            client_side_handshake();

            auto exceptions_safe = socket_->exceptions();
            at_scope_exit( socket_->exceptions(exceptions_safe) );
            socket_->exceptions(false);

            size_t bytes = wval * sizeof(char_type), w;

            if( compression_ == handshake::CompressionLZ4 ) {

            }

            if( encryption_ == handshake::EncryptionLight ) {
                light_chiper_encryptor_->encode(std::make_range((const uint8_t *) pbase(), bytes), std::make_range((uint8_t *) pbase(), bytes));
            }
            else if( encryption_ == handshake::EncryptionStrong ) {
                strong_chiper_encryptor_->encode(std::make_range((const uint8_t *) pbase(), bytes), std::make_range((uint8_t *) pbase(), bytes));
            }

            w = socket_->send((const void *) pbase(), bytes);

            if( w != wval * sizeof(char_type) )
                return -1;

            // reset output buffer pointers
            setp(pbuf_->data(), pbuf_->data() + pbuf_->size());
        }

        return 0;
    }

    basic_socket * socket_;

    typedef std::array<char_type, RX_MAX_BYTES / sizeof(char_type)> gbuf_type;
    typedef std::array<char_type, TX_MAX_BYTES / sizeof(char_type)> pbuf_type;

    std::unique_ptr<gbuf_type> gbuf_;
    std::unique_ptr<pbuf_type> pbuf_;

    //--------------------------------------------
    // handshaking section
    //--------------------------------------------
public:
    typedef std::function<void (handshake::packet * req, handshake::packet * res, std::key512 * p_local_transport_key, std::key512 * p_remote_transport_key)> handshake_functor_t;

    auto & handshake_functor(const handshake_functor_t & f) {
        handshake_functor_ = f;
        return *this;
    }
protected:
    handshake_functor_t handshake_functor_;

    void client_side_handshake() {
        if( proto_ == handshake::ProtocolRAW || handshaked_ )
            return;

        auto exceptions_safe = socket_->exceptions();
        at_scope_exit( socket_->exceptions(exceptions_safe) );
        socket_->exceptions(true);

        if( proto_ == handshake::ProtocolV1 ) {
            auto req = std::make_unique<handshake::packet>();

            req->proto_version      = proto_;
            req->encryption         = encryption_;
            req->encryption_option  = encryption_option_;
            req->compression        = compression_;
            req->compression_option = compression_option_;

            auto res = std::make_unique<handshake::packet>();

            int ntry;

            for( ntry = 5; ntry > 0; ntry-- ) {
                req->generate_session_key();

                handshake_functor_(req.get(), nullptr, nullptr, nullptr);

                req->scramble();
                socket_->send(req.get(), sizeof(*req));
                req->scramble();

                auto r = socket_->recv(res.get(), sizeof(*res));

                if( r != sizeof(*res) )
                    throw std::xruntime_error("Network error", __FILE__, __LINE__);

                res->scramble();

                // throw server error
                if( res->error != handshake::ErrorNone )
                    handshake::throw_if_error(res->error);

                handshake_functor_(req.get(), res.get(), nullptr, nullptr);

                // throw client error
                if( req->error != handshake::ErrorNone )
                    handshake::throw_if_error(req->error);

                std::key512 local_transport_key, remote_transport_key;
                handshake_functor_(req.get(), res.get(), &local_transport_key, &remote_transport_key);

                init_ciphers(local_transport_key, remote_transport_key);
            }
        }

        handshaked_ = true;
    }

    void server_side_handshake() {
        if( proto_ == handshake::ProtocolRAW || handshaked_ )
            return;

        auto exceptions_safe = socket_->exceptions();
        at_scope_exit( socket_->exceptions(exceptions_safe) );
        socket_->exceptions(false);

        if( proto_ == handshake::ProtocolV1 ) {
            auto req = std::make_unique<handshake::packet>();
            auto res = std::make_unique<handshake::packet>();
            int ntry;

            for( ntry = 5; ntry > 0; ntry-- ) {
                auto r = socket_->recv(req.get(), sizeof(*req));

                if( r != sizeof(*req) )
                    throw std::xruntime_error("Network error", __FILE__, __LINE__);

                req->scramble();

                handshake_functor_(req.get(), nullptr, nullptr, nullptr);

                res->proto_version      = proto_;
                res->encryption         = encryption_;
                res->encryption_option  = encryption_option_;
                res->compression        = compression_;
                res->compression_option = compression_option_;
                res->generate_session_key();

                if( req->proto_version != res->proto_version )
                    res->error = handshake::ErrorInvalidProto;
                else
                    res->error = handshake::ErrorNone;

                handshake_functor_(req.get(), res.get(), nullptr, nullptr);

                res->scramble();
                socket_->send(res.get(), sizeof(*res));
                res->scramble();

                r = socket_->recv(&req->error, sizeof(req->error));

                if( r != sizeof(req->error) )
                    throw std::xruntime_error("Network error", __FILE__, __LINE__);

                if( req->error == handshake::ErrorRestartHandshake )
                    continue;

                // throw my error
                if( res->error != handshake::ErrorNone )
                    handshake::throw_if_error(res->error);

                // throw client error
                if( req->error != handshake::ErrorNone )
                    handshake::throw_if_error(req->error);

                std::key512 local_transport_key, remote_transport_key;
                handshake_functor_(req.get(), res.get(), &local_transport_key, &remote_transport_key);

                init_ciphers(local_transport_key, remote_transport_key);
                break;
            }

            if( ntry == 0 )
                throw std::xruntime_error("The maximum number of handshakes has been exceeded", __FILE__, __LINE__);
        }

        handshaked_ = true;
    }

    void init_ciphers(const std::key512 & local_transport_key, const std::key512 & remote_transport_key) {
        if( encryption_ == handshake::EncryptionLight ) {
            if( light_chiper_encryptor_ == nullptr )
                light_chiper_encryptor_ = std::make_unique<light_cipher>();

            light_chiper_encryptor_->init(local_transport_key);

            if( light_chiper_decryptor_ == nullptr )
                light_chiper_decryptor_ = std::make_unique<light_cipher>();

            light_chiper_decryptor_->init(remote_transport_key);
        }
        else if( encryption_ == handshake::EncryptionStrong ) {
            if( strong_chiper_encryptor_ == nullptr )
                strong_chiper_encryptor_ = std::make_unique<strong_cipher>();

            strong_chiper_encryptor_->init(local_transport_key);

            if( strong_chiper_decryptor_ == nullptr )
                strong_chiper_decryptor_ = std::make_unique<strong_cipher>();

            strong_chiper_decryptor_->init(remote_transport_key);
        }
    }

    std::unique_ptr<light_cipher>  light_chiper_encryptor_;
    std::unique_ptr<light_cipher>  light_chiper_decryptor_;
    std::unique_ptr<strong_cipher> strong_chiper_encryptor_;
    std::unique_ptr<strong_cipher> strong_chiper_decryptor_;

    handshake::Proto               proto_              = handshake::ProtocolRAW;
    handshake::Encryption          encryption_         = handshake::EncryptionNone;
    handshake::Option              encryption_option_  = handshake::OptionDisable;
    handshake::Compression         compression_        = handshake::CompressionNone;
    handshake::Option              compression_option_ = handshake::OptionDisable;
    bool                           handshaked_         = false;
};
//------------------------------------------------------------------------------
typedef basic_socket_streambuf<char, std::char_traits<char>> socket_streambuf;
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_istream : public std::basic_istream<_Elem, _Traits> {
public:
#if __GNUG__
    using typename std::basic_istream<_Elem, _Traits>::__istream_type;
    using typename __istream_type::__streambuf_type;
    using typename __istream_type::traits_type;
    using typename __istream_type::int_type;

    using __istream_type::exceptions;
    using __istream_type::unsetf;
    using __istream_type::rdbuf;
    using __istream_type::setstate;
#endif

    basic_socket_istream(basic_socket_streambuf<_Elem, _Traits> * sbuf) :
        std::basic_istream<_Elem, _Traits>(sbuf
#if _MSC_VER
            , std::ios::binary
#endif
        )
    {
        // http://en.cppreference.com/w/cpp/io/basic_ios/exceptions
        exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        unsetf(std::ios::skipws);
        unsetf(std::ios::unitbuf);
    }

    auto & delimiter(const int_type & t) {
        delimiter_.clear();
        delimiter_.push_back(t);
        return *this;
    }

    template <class _Alloc>
    auto & delimiter(const std::basic_string<_Elem, _Traits, _Alloc> & t) {
        delimiter_.clear();
        for( const auto & c : t )
            delimiter_.push_back(traits_type::to_int_type(t));
        return *this;
    }

    auto & delimiter(const _Elem * t) {
        delimiter_.clear();
        while( *t != _Elem() ) {
            delimiter_.push_back(traits_type::to_int_type(*t));
            t++;
        }
        return *this;
    }

    const auto & delimiter() const {
        return delimiter_;
    }
protected:
    std::vector<int_type> delimiter_;
};
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
homeostas::basic_socket_istream<_Elem, _Traits> & getline(
    homeostas::basic_socket_istream<_Elem, _Traits> & ss,
    basic_string<_Elem, _Traits, _Alloc> & s,
    const _Elem d)
{
    getline(*static_cast<basic_istream<_Elem, _Traits> *>(&ss), s, d);
    return ss;
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
homeostas::basic_socket_istream<_Elem, _Traits> & operator >> (
    homeostas::basic_socket_istream<_Elem, _Traits> & ss,
    basic_string<_Elem, _Traits, _Alloc> & s)
{
    const auto & delimiter = ss.delimiter();
    const auto b = delimiter.begin(), e = delimiter.end();

    getline(ss, s, _Elem(*b));

    auto i = b + 1;

    while( i != e && ss ) {
        auto c = ss.get();

        if( c == _Traits::eof() )
            break;

        if( _Elem(c) != *i ) {
            s.push_back(_Elem(c));
            basic_string<_Elem, _Traits, _Alloc> t;
            getline(ss, t, _Elem(*(i = b)));
            s += t;
        }

        i++;
    }

    return ss;
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
homeostas::basic_socket_istream<_Elem, _Traits> & operator >> (
    homeostas::basic_socket_istream<_Elem, _Traits> & ss,
    basic_ostringstream<_Elem, _Traits, _Alloc> & s)
{
    basic_string<_Elem, _Traits, _Alloc> t;
    ss >> t;
    s << t;
    return ss;
}
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_ostream : public std::basic_ostream<_Elem, _Traits> {
public:
#if __GNUG__
    using typename std::basic_ostream<_Elem, _Traits>::__ostream_type;
    using typename __ostream_type::__streambuf_type;
    using typename __ostream_type::traits_type;
    using typename __ostream_type::int_type;

    using __ostream_type::exceptions;
    using __ostream_type::unsetf;
    using __ostream_type::rdbuf;
    using __ostream_type::setstate;
#endif

    basic_socket_ostream(basic_socket_streambuf<_Elem, _Traits> * sbuf) :
        std::basic_ostream<_Elem, _Traits>(sbuf
#if _MSC_VER
            , std::ios::binary
#endif
        )
    {
        // http://en.cppreference.com/w/cpp/io/basic_ios/exceptions
        exceptions(std::ios::failbit | std::ios::badbit);
        unsetf(std::ios::skipws);
        unsetf(std::ios::unitbuf);
    }

    //template <typename T,
    //    typename std::enable_if<
    //        std::is_base_of<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>::value &&
    //        std::is_same<typename T::value_type, _Elem>::value &&
    //        std::is_same<typename T::traits_type, _Traits>::value
    //    >::type * = nullptr
    //>
    //auto & operator << (const T & s) {
    //    // send a null-terminated string
    //    *static_cast<std::basic_ostream<_Elem, _Traits> *>(this) << (s) << std::ends;
    //    return *this;
    //}

    //typedef std::basic_ostream<_Elem, _Traits> & (* __func_manipul_type)(std::basic_ostream<_Elem, _Traits> &);

    //basic_socket_ostream<_Elem, _Traits> & operator << (__func_manipul_type f) {

    //template <typename T,
    //    typename std::enable_if<
    //        std::is_base_of<T, std::basic_ostream<_Elem, _Traits>>::value
    //    >::type * = nullptr
    //>
    //basic_socket_ostream<_Elem, _Traits> & operator << (T & (* f)(T &)) {
    //    (*f) (*this);
    //    return *this;
    //}

    basic_socket_ostream<_Elem, _Traits> & write(const _Elem * s) {
        static_cast<std::basic_ostream<_Elem, _Traits> *>(this)->write(s, slen(s));
        return *this;
    }

    template <class _Alloc> inline
    basic_socket_ostream<_Elem, _Traits> & write(const std::basic_string<_Elem, _Traits, _Alloc> & s) {
        static_cast<std::basic_ostream<_Elem, _Traits> *>(this)->write(s.c_str(), s.size());
        return *this;
    }

    auto & noends() {
        ends_.clear();
        return *this;
    }

    auto & ends(const int_type & t) {
        ends_.clear();
        ends_.push_back(traits_type::to_int_type(t));
        return *this;
    }

    template <class _Alloc>
    auto & ends(const std::basic_string<_Elem, _Traits, _Alloc> & t) {
        ends_.clear();
        for( const auto & c : t )
            ends_.push_back(traits_type::to_int_type(_Elem()));
        return *this;
    }

    const auto & ends() const {
        return ends_;
    }

protected:
    std::vector<int_type> ends_;

    //enum StringTxType {
    //    StringTxNull     = 1,
    //    StringTxCRLF     = 2,
    //    StringTxLF       = 3,
    //    StringTxCR       = 4,
    //    StringTxCustom   = 5,
    //};
};
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
namespace std {
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
homeostas::basic_socket_ostream<_Elem, _Traits> & operator << (
    homeostas::basic_socket_ostream<_Elem, _Traits> & ss,
    const basic_string<_Elem, _Traits, _Alloc> & s)
{
    auto & bos = *static_cast<basic_ostream<_Elem, _Traits> *>(&ss);

    bos << s;

    for( const auto & c : ss.ends() )
        bos << _Traits::to_char_type(c);

    return ss;
}
//------------------------------------------------------------------------------
template <class _Elem, class _Traits, class _Alloc> inline
homeostas::basic_socket_ostream<_Elem, _Traits> & operator << (
    homeostas::basic_socket_ostream<_Elem, _Traits> & ss,
    const basic_ostringstream<_Elem, _Traits, _Alloc> & s)
{
    auto & bos = *static_cast<basic_ostream<_Elem, _Traits> *>(&ss);

    bos << s.str();

    for( const auto & c : ss.ends() )
        bos << _Traits::to_char_type(c);

    return ss;
}
//------------------------------------------------------------------------------
} // namespace std
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_stream :
    public basic_socket_streambuf<_Elem, _Traits>,
    public basic_socket_istream<_Elem, _Traits>,
    public basic_socket_ostream<_Elem, _Traits>
{
public:
    typedef _Traits traits_type;
#if __GNUG__
    typedef std::ios_base::iostate iostate;
#endif

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::unique_ptr<typename T::element_type, typename T::deleter_type>>::value
        >::type * = nullptr
    >
    basic_socket_stream(T & socket) : basic_socket_stream(*socket) {}

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::shared_ptr<typename T::element_type>>::value
        >::type * = nullptr
    >
    basic_socket_stream(T socket) : basic_socket_stream(*socket) {}

    basic_socket_stream(basic_socket & socket) : basic_socket_stream(&socket) {}

    basic_socket_stream(basic_socket * socket = nullptr) :
        basic_socket_streambuf<_Elem, _Traits>(socket),
        basic_socket_istream<_Elem, _Traits>(this),
        basic_socket_ostream<_Elem, _Traits>(this) {
    }

    iostate exceptions() const {
        return basic_socket_istream<_Elem, _Traits>::exceptions();
    }

    void exceptions(iostate except) {
        basic_socket_istream<_Elem, _Traits>::exceptions(except);
        basic_socket_ostream<_Elem, _Traits>::exceptions(except);
    }

    void reset(basic_socket * socket) {
        basic_socket_streambuf<_Elem, _Traits>::reset(socket);
        basic_socket_istream<_Elem, _Traits>::clear();
        basic_socket_ostream<_Elem, _Traits>::clear();
    }

    void reset(basic_socket & socket) {
        reset(&socket);
    }

    void reset() {
        basic_socket_streambuf<_Elem, _Traits>::reset();
        basic_socket_istream<_Elem, _Traits>::clear();
        basic_socket_ostream<_Elem, _Traits>::clear();
    }

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::unique_ptr<typename T::element_type, typename T::deleter_type>>::value
        >::type * = nullptr
    >
    void reset(T & socket) {
        reset(*socket);
    }

    template <typename T,
        typename std::enable_if<
            std::is_base_of<T, std::shared_ptr<typename T::element_type>>::value
        >::type * = nullptr
    >
    void reset(T socket) {
        reset(*socket);
    }
};
//------------------------------------------------------------------------------
typedef basic_socket_stream<char, std::char_traits<char>> socket_stream;
//------------------------------------------------------------------------------
namespace tests {
//------------------------------------------------------------------------------
void socket_test();
//------------------------------------------------------------------------------
} // namespace tests
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
#endif // SOCKET_STREAM_HPP_INCLUDED
//------------------------------------------------------------------------------
