#pragma once
#ifndef _GALIK_SOCKETSTREAM_H
#define _GALIK_SOCKETSTREAM_H

/*-----------------------------------------------------------------.
| Copyright (C) 2011 Galik grafterman@googlemail.com               |
'------------------------------------------------------------------'

This code is was created from code (C) Copyright Nicolai M. Josuttis 2001
with the following Copyright Notice:
*/

/* The following code declares classes to read from and write to
 * file descriptore or file handles.
 *
 * See
 *      http://www.josuttis.com/cppcode
 * for details and the latest version.
 *
 * - open:
 *      - integrating BUFSIZ on some systems?
 *      - optimized reading of multiple characters
 *      - stream for reading AND writing
 *      - i18n
 *
 * (C) Copyright Nicolai M. Josuttis 2001.
 * Permission to copy, use, modify, sell and distribute this software
 * is granted provided this copyright notice appears in all copies.
 * This software is provided "as is" without express or implied
 * warranty, and with no claim as to its suitability for any purpose.
 *
 * Version: Jul 28, 2002
 * History:
 *  Jul 28, 2002: bugfix memcpy() => memmove()
 *                fdinbuf::underflow(): cast for return statements
 *  Aug 05, 2001: first public version
 */

/*

Permission to copy, use, modify, sell and distribute this software
is granted under the same conditions.

'----------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#   include <config.h>
#   ifdef HAVE_SYS_SOCKET_H
#       include <sys/socket.h>
#   endif
#   ifdef HAVE_NETINET_IN_H
#       include <netinet/in.h>
#   endif
#   ifdef HAVE_NETDB_H
#       include <netdb.h>
#   endif
#   ifdef HAVE_UNISTD_H
#       include <unistd.h>
#   endif
#else
#   ifdef _MSC_VER
#       include <winsock2.h>
#   endif
#endif

#include <streambuf>
#include <istream>
#include <ostream>

namespace galik {
    namespace net {

        template <typename CharT, typename Traits = std::char_traits<CharT> >
        class basic_socketbuf : public std::basic_streambuf<CharT, Traits> {
        public:
            typedef std::basic_streambuf<CharT, Traits> super;
            typedef basic_socketbuf<CharT, Traits> self;

            typedef typename super::char_type char_type;
            typedef typename super::traits_type traits_type;
            typedef typename super::int_type int_type;
            typedef typename super::pos_type pos_type;
            typedef typename super::off_type off_type;

        protected:
            static const int CHAR_SIZE = sizeof(char_type);
#ifdef BUFSIZ
            static const int SIZE = BUFSIZ;
#else
            static const int SIZE = 1024;
#endif
            char_type _M_obuf[SIZE];
            char_type _M_ibuf[SIZE];

            int sock;

        public:
            basic_socketbuf() : sock(0) {
                super::setp(_M_obuf, _M_obuf + (SIZE - 1));
                super::setg(_M_ibuf, _M_ibuf, _M_ibuf);
            }

            virtual ~basic_socketbuf() { sync(); }

            void set_socket(int sock) { this->sock = sock; }
            int get_socket() { return this->sock; }

        protected:
            int_type output_buffer() {
                int_type num = super::pptr() - super::pbase();
                if (send(sock, reinterpret_cast<char *>(_M_obuf), num * CHAR_SIZE,
                         0) != num)
                    return traits_type::eof();
                super::pbump(-num);
                return num;
            }

            virtual int_type overflow(int_type c) {
                if (c != traits_type::eof()) {
                    *super::pptr() = c;
                    super::pbump(1);
                }

                if (output_buffer() == traits_type::eof())
                    return traits_type::eof();
                return c;
            }

            virtual int sync() {
                if (output_buffer() == traits_type::eof())
                    return traits_type::eof();
                return 0;
            }

            virtual int_type underflow() {
                if (super::gptr() < super::egptr())
                    return *super::gptr();

                int num;
                if ((num = ::recv(sock, reinterpret_cast<char *>(_M_ibuf),
                                SIZE * CHAR_SIZE, 0)) <= 0)
                    return traits_type::eof();

                super::setg(_M_ibuf, _M_ibuf, _M_ibuf + num);
                return *super::gptr();
            }
        };

        typedef basic_socketbuf<char> socketbuf;
        typedef basic_socketbuf<wchar_t> wsocketbuf;

        template <typename CharT, typename Traits = std::char_traits<CharT> >
        class basic_socketstream : public std::basic_iostream<CharT, Traits> {
        public:
            typedef std::basic_iostream<CharT, Traits> super;
            typedef basic_socketbuf<CharT, Traits> streambuffer;
            typedef typename super::char_type char_type;
            typedef typename super::traits_type traits_type;
            typedef typename super::int_type    int_type;
            typedef typename super::pos_type    pos_type;
            typedef typename super::off_type    off_type;

        protected:
            streambuffer _M_buffer;

        public:
            basic_socketstream() : super(&_M_buffer) { }

            basic_socketstream(int s) : super(&_M_buffer) {
                _M_buffer.set_socket(s);
            }

            void close() {
                if (_M_buffer.get_socket() != 0) {
                    ::close(_M_buffer.get_socket());
                    _M_buffer.set_socket(0);
                }
                super::clear();
            }

            bool open(const std::string &host, uint16_t port) {
                this->close();
                int sd = ::socket(AF_INET, SOCK_STREAM, 0);
                struct ::sockaddr_in sin;
                struct ::hostent *he = ::gethostbyname(host.c_str());

                std::copy(reinterpret_cast<char*>(he->h_addr),
                          reinterpret_cast<char*>(he->h_addr) + he->h_length,
                          reinterpret_cast<char*>(&sin.sin_addr.s_addr));
                sin.sin_family = AF_INET;
                sin.sin_port = htons(port);

                if (::connect(sd, reinterpret_cast<struct ::sockaddr*>(&sin),
                            sizeof(sin)) < 0)
                    super::setstate(std::ios::failbit);
                else
                    _M_buffer.set_socket(sd);
                return *this;
            }
        };

        typedef basic_socketstream<char> socketstream;
        typedef basic_socketstream<wchar_t> wsocketstream;
    }
} // galik::net

#endif // _GALIK_SOCKETSTREAM_H
