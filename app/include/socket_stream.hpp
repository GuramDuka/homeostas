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
    EncryptionStrong = 2,
    EncryptionMaxValue
};
//------------------------------------------------------------------------------
enum Compression {
    CompressionNone = 0,
    CompressionLZ4  = 1,
    CompressionZSTD = 2,
    CompressionMaxValue
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
    ErrorNone                 =  0,
    ErrorRestartHandshake     =  1,
    ErrorInvalidProto         =  2,
    ErrorInvalidEncryption    =  3,
    ErrorEncryptionDisabled   =  4,
    ErrorEncryptionRequired   =  5,
    ErrorInvalidCompression   =  6,
    ErrorCompressionDisabled  =  7,
    ErrorCompressionRequired  =  8,
    ErrorInvalidFingerprint   =  9,
    ErrorInvalidHostPublicKey = 10
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
        case ErrorEncryptionDisabled  :
            throw std::xruntime_error("Encryption disabled", __FILE__, __LINE__);
        case ErrorEncryptionRequired  :
            throw std::xruntime_error("Encryption required", __FILE__, __LINE__);
        case ErrorInvalidCompression  :
            throw std::xruntime_error("Invalid compression type", __FILE__, __LINE__);
        case ErrorCompressionDisabled :
            throw std::xruntime_error("Compression disabled", __FILE__, __LINE__);
        case ErrorCompressionRequired :
            throw std::xruntime_error("Compression required", __FILE__, __LINE__);
        case ErrorInvalidFingerprint  :
            throw std::xruntime_error("Invalid fingerprint", __FILE__, __LINE__);
        case ErrorInvalidHostPublicKey:
            throw std::xruntime_error("Invalid host public key", __FILE__, __LINE__);
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
        cdc512 ctx;
        ctx.generate_entropy_fast();
        std::copy(std::begin(ctx), std::end(ctx),
            std::begin(session_key), std::end(session_key));
    }

    void scramble() {
        auto chiper = std::make_unique<light_cipher>();
        chiper->init(session_key);
        auto arena = std::make_range(public_key, sizeof(*this) - offsetof(packet, public_key));
        chiper->encode(arena, arena);
    }

    void fix() {
        if( encryption_option != handshake::OptionDisable
                && (encryption == handshake::EncryptionNone
                    || encryption >= handshake::EncryptionMaxValue) )
            encryption = handshake::EncryptionStrong;
        else if( encryption_option == handshake::OptionDisable
                && encryption != handshake::EncryptionNone )
            encryption = handshake::EncryptionNone;

        if( compression_option != handshake::OptionDisable
                && (compression == handshake::CompressionNone
                    || compression >= handshake::CompressionMaxValue) )
            compression = handshake::CompressionLZ4;
        else if( compression_option == handshake::OptionDisable
                && compression != handshake::CompressionNone )
            encryption = handshake::CompressionNone;
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
void negotiate_channel_options(packet * server, packet * client);
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class negotiator {
public:
    typedef std::function<void (
            handshake::packet * req,
            handshake::packet * res,
            std::key512 * p_local_transport_key,
            std::key512 * p_remote_transport_key)> handshake_functor_t;

    void client_side_handshake(basic_socket * socket);
    void server_side_handshake(basic_socket * socket);
    void init_ciphers(const std::key512 & local_transport_key, const std::key512 & remote_transport_key);

    template <typename InpRange, typename OutRange, typename std::enable_if<
            std::is_same<typename std::remove_const<typename InpRange::value_type>::type, typename OutRange::value_type>::value
            //&& std::is_same<typename std::iterator_traits<InpRange>::iterator_category, std::random_access_iterator_tag>::value
            //&& std::is_same<typename std::iterator_traits<OutRange>::iterator_category, std::random_access_iterator_tag>::value
        >::type * = nullptr
    >
    void encrypt(InpRange inp_range, OutRange out_range) {
        assert( std::distance(inp_range.begin(), inp_range.end()) == std::distance(out_range.begin(), out_range.end()) );

        if( encryption_ == EncryptionLight )
            light_chiper_encryptor_->encode(inp_range, out_range);
        else if( encryption_ == EncryptionStrong )
            strong_chiper_encryptor_->encode(inp_range, out_range);
    }

    template <typename InpRange, typename OutRange, typename std::enable_if<
            std::is_same<typename std::remove_const<typename InpRange::value_type>::type, typename OutRange::value_type>::value
            //&& std::is_same<typename std::iterator_traits<InpRange>::iterator_category, std::random_access_iterator_tag>::value
            //&& std::is_same<typename std::iterator_traits<OutRange>::iterator_category, std::random_access_iterator_tag>::value
        >::type * = nullptr
    >
    void decrypt(InpRange inp_range, OutRange out_range) {
        assert( std::distance(inp_range.begin(), inp_range.end()) == std::distance(out_range.begin(), out_range.end()) );

        if( encryption_ == EncryptionLight )
            light_chiper_decryptor_->encode(inp_range, out_range);
        else if( encryption_ == EncryptionStrong )
            strong_chiper_decryptor_->encode(inp_range, out_range);
    }

    handshake_functor_t handshake_functor_;
    std::unique_ptr<light_cipher>  light_chiper_encryptor_;
    std::unique_ptr<light_cipher>  light_chiper_decryptor_;
    std::unique_ptr<strong_cipher> strong_chiper_encryptor_;
    std::unique_ptr<strong_cipher> strong_chiper_decryptor_;

    Proto               proto_              = ProtocolRAW;
    Encryption          encryption_         = EncryptionNone;
    Option              encryption_option_  = OptionDisable;
    Compression         compression_        = CompressionNone;
    Option              compression_option_ = OptionDisable;
    bool                handshaked_         = false;
};
//------------------------------------------------------------------------------
} // namespace handshake
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
template <class _Elem, class _Traits>
class basic_socket_streambuf :
        public std::basic_streambuf<_Elem, _Traits>,
        protected handshake::negotiator
{
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
        handshaked_ = false;
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
            server_side_handshake(socket_);

            auto exceptions_safe = socket_->exceptions();
            at_scope_exit( socket_->exceptions(exceptions_safe) );
            socket_->exceptions(false);

            auto r = socket_->recv(eback(), 0, gbuf_->size() * sizeof(char_type));

            if( r <= 0 )
                return traits_type::eof();

            decrypt(std::make_range((const uint8_t *) eback(), r),
                std::make_range((uint8_t *) eback(), r));

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
            client_side_handshake(socket_);

            auto exceptions_safe = socket_->exceptions();
            at_scope_exit( socket_->exceptions(exceptions_safe) );
            socket_->exceptions(false);

            size_t bytes = wval * sizeof(char_type), w;

            encrypt(std::make_range((const uint8_t *) pbase(), bytes),
                std::make_range((uint8_t *) pbase(), bytes));

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
    auto & handshake_functor(const handshake_functor_t & f) {
        handshake_functor_ = f;
        return *this;
    }

    auto & proto(const handshake::Proto & proto) {
        proto_ = proto;
        return *this;
    }

    const auto & proto() const {
        return proto_;
    }

    auto & encryption(const handshake::Encryption & e) {
        encryption_ = e;
        return *this;
    }

    const auto & encryption() const {
        return encryption_;
    }

    auto & encryption_option(const handshake::Option & o) {
        encryption_option_ = o;
        return *this;
    }

    const auto & encryption_option() const {
        return encryption_option_;
    }

    auto & compression(const handshake::Compression & e) {
        compression_ = e;
        return *this;
    }

    const auto & compression() const {
        return compression_;
    }

    auto & compression_option(const handshake::Option & o) {
        compression_option_ = o;
        return *this;
    }

    const auto & compression_option() const {
        return compression_option_;
    }
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
        if( t != _Traits::eof() )
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
        while( *t != _Elem('\0') ) {
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
    auto b = delimiter.cbegin(), e = delimiter.cend();
    auto d = distance(b, e);
    auto m = _Traits::to_char_type(d == 0 ? '\0' : *b);

    getline(ss, s, m);

    if( d != 0 ) {
        auto i = b + 1;

        while( i != e && ss ) {
            auto c = ss.get();

            if( c == _Traits::eof() )
                break;

            if( c != *i ) {
                s.push_back(_Traits::to_char_type(c));
                basic_string<_Elem, _Traits, _Alloc> t;
                getline(ss, t, _Traits::to_char_type(*(i = b)));
                s += t;
            }

            i++;
        }
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
        ),
        ends_({ 0 })
    {
        // http://en.cppreference.com/w/cpp/io/basic_ios/exceptions
        exceptions(std::ios::failbit | std::ios::badbit);
        unsetf(std::ios::skipws);
        unsetf(std::ios::unitbuf);
    }

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
template <class _Traits> inline
homeostas::basic_socket_ostream<char, _Traits> & operator << (
    homeostas::basic_socket_ostream<char, _Traits> & ss,
    const char * s)
{
    auto & bos = *static_cast<basic_ostream<char, _Traits> *>(&ss);

    bos << s;

    for( const auto & c : ss.ends() )
        bos << _Traits::to_char_type(c);

    return ss;
}
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

    template <typename T, typename std::enable_if<std::container_traits<T>::is_unique_or_shared_ptr>::type * = nullptr>
    basic_socket_stream(T & socket) : basic_socket_stream(*socket) {}

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

    template <typename T, typename std::enable_if<std::container_traits<T>::is_unique_or_shared_ptr>::type * = nullptr>
    void reset(T & socket) {
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
