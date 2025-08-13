#include <cstring>
#include <iomanip>
#include <streambuf>
#include <fstream>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

namespace {

/**
 * socketbuf, something like @ref std::basic_streambuf<char>.
 *
 * FIXME std::basic_streambuf should handle mulitibyte encoding conversion,
 * which is currently not handled. so it is currently only for raw byte stream.
 */
class socketbuf
    : public std::streambuf {
public:
    typedef std::streambuf super;
    typedef typename super::int_type int_type;
    typedef typename super::char_type char_type;
    typedef typename super::traits_type traits_type;

    socketbuf()
        : _M_ibuf(0)
        , _M_obuf(0)
        , _M_ibuflen(BUFSIZ)
        , _M_obuflen(BUFSIZ)
    {
        allocate_buffer();
        char_type *p = 0;
        p = reinterpret_cast<char_type*>(_M_ibuf);
        this->setg(p, p, p);
        p = reinterpret_cast<char_type*>(_M_obuf);
        this->setp(p, p + _M_obuflen);
    }

    virtual ~socketbuf() {
        deallocate_buffer();
    }

    void allocate_buffer() {
        _M_ibuf = _M_allocator.allocate(_M_ibuflen);
        _M_obuf = _M_allocator.allocate(_M_obuflen);
    }

    void deallocate_buffer() {
        if (_M_ibuf)
            _M_allocator.deallocate(_M_ibuf, _M_ibuflen);
        _M_ibuf = 0;
        if (_M_obuf)
            _M_allocator.deallocate(_M_obuf, _M_obuflen);
        _M_obuf = 0;
    }

    int get_socket() const {
        return _M_socket;
    }

    void set_socket(int value) {
        _M_socket = value;
    }

    virtual int_type underflow() override {
        if (!sync_input())
            return traits_type::eof();
        return this->sgetc();
    }

    virtual int_type overflow(int_type ch) override {
        if (!sync_output())
            return traits_type::eof();
        return this->sputc(traits_type::to_char_type(ch));
    }

    // trigger via flush
    virtual int sync() override {
        return sync_output() ? 0 : -1;
    }
protected:
    virtual bool sync_output() {
        std::ptrdiff_t nbytes = 0;
        nbytes = this->pptr() - this->pbase();
        if (nbytes <= 0)
            return true;
        ssize_t n = ::send(_M_socket, this->pbase(), nbytes, 0);
        if (n <= 0)
            return false;
        std::char_traits<char>::move(
                this->pbase(),
                this->pbase() + n,
                nbytes - n);
        this->pbump(-n);
        return true;
    }

    virtual bool sync_input() {
        std::ptrdiff_t nbytes = this->egptr() - this->gptr();
        if (nbytes > 0)
            return true;
        ssize_t n = ::recv(_M_socket, _M_ibuf, _M_ibuflen, 0);
        if (n <= 0)
            return false;
        char_type *p = reinterpret_cast<char_type*>(_M_ibuf);
        this->setg(p, p, p + n);
        return true;
    }
private:
    int _M_socket;
    unsigned char *_M_ibuf;
    unsigned char *_M_obuf;
    int_type _M_ibuflen;
    int_type _M_obuflen;
    std::allocator<unsigned char> _M_allocator;
};

} // namespace anonymous

int main(int argc, char* argv[]) try {
    int s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
        throw std::runtime_error("socket");
    struct sockaddr_in saddr;
    std::memset(&saddr, 0, sizeof(saddr));
    inet_pton(AF_INET, "localhost", &saddr.sin_addr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8000);
    int rc = ::connect(s, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr));
    if (rc == -1)
        throw std::runtime_error("connect");
    socketbuf buffer;
    buffer.set_socket(s);
    std::iostream stream(&buffer);
    stream
        << "GET / HTTP/1.1\r\n"
        << "Host: localhost\r\n"
        << "\r\n";
    stream.flush();
    std::string line;
    while (std::getline(stream, line)) {
        std::cout << line << std::endl;
    }
    ::shutdown(s, SHUT_RDWR);
    ::close(s);
    std::filebuf bf;
    return EXIT_SUCCESS;
} catch (std::exception const &e) {
    std::cerr << "[c++ exception] " << e.what() << std::endl;
    return EXIT_FAILURE;
}
