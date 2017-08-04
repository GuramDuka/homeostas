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
#include "config.h"
//------------------------------------------------------------------------------
#include "socket_stream.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
namespace handshake {
//------------------------------------------------------------------------------
void negotiate_channel_options(packet * server, packet * client) {
    if( client->proto_version != server->proto_version )
        server->error = handshake::ErrorInvalidProto;
    // encryption required on client side but disabled on server side
    else if( client->encryption_option == handshake::OptionDisable
         && server->encryption_option == handshake::OptionRequired )
        server->error = handshake::ErrorEncryptionDisabled;
    // encryption required on server side but disabled on client side
    else if( client->encryption_option == handshake::OptionRequired
         && server->encryption_option == handshake::OptionDisable )
        server->error = handshake::ErrorEncryptionRequired;
    // check encryption range
    else if( client->encryption >= handshake::EncryptionMaxValue )
        server->error = handshake::ErrorInvalidEncryption;
    // compression required on client side but disabled on server side
    else if( client->compression_option == handshake::OptionDisable
         && server->compression_option == handshake::OptionRequired )
        server->error = handshake::ErrorCompressionDisabled;
    // compression required on server side but disabled on client side
    else if( client->compression_option == handshake::OptionRequired
         && server->compression_option == handshake::OptionDisable )
        server->error = handshake::ErrorCompressionRequired;
    // check compression range
    else if( client->compression >= handshake::CompressionMaxValue )
        server->error = handshake::ErrorInvalidCompression;
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void negotiator::client_side_handshake(basic_socket * socket)
{
    if( proto_ == ProtocolRAW || handshaked_ )
        return;

    auto exceptions_safe = socket->exceptions();
    at_scope_exit( socket->exceptions(exceptions_safe) );
    socket->exceptions(true);

    if( proto_ == ProtocolV1 ) {
        auto req = std::make_unique<handshake::packet>();
        auto res = std::make_unique<handshake::packet>();
        auto xchg = [&] {
            req->error = ErrorNone;
            req->generate_session_key();
            req->fix();

            req->scramble();
            socket->send(req.get(), sizeof(*req));
            req->scramble();

            auto r = socket->recv(res.get(), sizeof(*res));

            if( r != sizeof(*res) )
                throw std::xruntime_error("Network error", __FILE__, __LINE__);

            res->scramble();

            throw_if_error(res->error);

            negotiate_channel_options(req.get(), res.get());

            throw_if_error(req->error);

            req->encryption  = res->encryption;
            req->compression = res->compression;

            handshake_functor_(req.get(), res.get(), nullptr, nullptr);
            throw_if_error(req->error);
        };

        socket->disable_nagle_algoritm();
        socket->send_window_size(0);

        handshake_functor_(req.get(), nullptr, nullptr, nullptr);
        throw_if_error(req->error);

        req->proto_version      = proto_;
        req->encryption_option  = encryption_option_;
        req->compression_option = compression_option_;
        req->encryption         = encryption_option_  == OptionDisable || encryption_option_  == OptionAllow ? EncryptionNone  : encryption_;
        req->compression        = compression_option_ == OptionDisable || compression_option_ == OptionAllow ? CompressionNone : compression_;

        xchg();

        if( req->encryption != EncryptionNone ) {
            std::key512 local_transport_key, remote_transport_key;
            handshake_functor_(req.get(), res.get(), &local_transport_key, &remote_transport_key);
            init_ciphers(local_transport_key, remote_transport_key);
            encryption_ = Encryption(req->encryption);
        }

        if( req->compression != CompressionNone ) {
            compression_ = Compression(req->compression);
        }

        handshaked_ = true;
    }
}
//------------------------------------------------------------------------------
void negotiator::server_side_handshake(basic_socket * socket)
{
    if( proto_ == ProtocolRAW || handshaked_ )
        return;

    auto exceptions_safe = socket->exceptions();
    at_scope_exit( socket->exceptions(exceptions_safe) );
    socket->exceptions(false);

    if( proto_ == ProtocolV1 ) {
        auto req = std::make_unique<handshake::packet>();
        auto res = std::make_unique<handshake::packet>();
        auto xchg = [&] {
            auto r = socket->recv(req.get(), sizeof(*req));

            if( r != sizeof(*req) )
                throw std::xruntime_error("Network error", __FILE__, __LINE__);

            req->scramble();

            res->error = ErrorNone;
            res->generate_session_key();
            res->fix();

            negotiate_channel_options(res.get(), req.get());

            throw_if_error(res->error);

            if( encryption_option_ == OptionAllow )
                res->encryption = req->encryption;

            if( compression_option_ == OptionAllow )
                res->compression = req->compression;

            handshake_functor_(req.get(), res.get(), nullptr, nullptr);
            throw_if_error(res->error);

            res->scramble();
            socket->send(res.get(), sizeof(*res));
            res->scramble();
        };

        socket->disable_nagle_algoritm();
        socket->send_window_size(0);

        res->proto_version      = proto_;
        res->encryption_option  = encryption_option_;
        res->compression_option = compression_option_;
        res->encryption         = encryption_;
        res->compression        = compression_;

        xchg();

        if( res->encryption != EncryptionNone ) {
            std::key512 local_transport_key, remote_transport_key;
            handshake_functor_(req.get(), res.get(), &local_transport_key, &remote_transport_key);
            init_ciphers(local_transport_key, remote_transport_key);
            encryption_ = Encryption(res->encryption);
        }

        if( res->compression != CompressionNone ) {
            compression_ = Compression(res->compression);
        }

        handshaked_ = true;
    }
}
//------------------------------------------------------------------------------
void negotiator::init_ciphers(const std::key512 & local_transport_key, const std::key512 & remote_transport_key)
{
    if( encryption_ == EncryptionLight ) {
        strong_chiper_encryptor_ = nullptr;
        light_chiper_encryptor_ = std::make_unique<light_cipher>();
        light_chiper_encryptor_->init(local_transport_key);
        strong_chiper_decryptor_ = nullptr;
        light_chiper_decryptor_ = std::make_unique<light_cipher>();
        light_chiper_decryptor_->init(remote_transport_key);
    }
    else if( encryption_ == EncryptionStrong ) {
        light_chiper_encryptor_ = nullptr;
        strong_chiper_encryptor_ = std::make_unique<strong_cipher>();
        strong_chiper_encryptor_->init(local_transport_key);
        light_chiper_decryptor_ = nullptr;
        strong_chiper_decryptor_ = std::make_unique<strong_cipher>();
        strong_chiper_decryptor_->init(remote_transport_key);
    }
}
//------------------------------------------------------------------------------
} // namespace handshake
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
