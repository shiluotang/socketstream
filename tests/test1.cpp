#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <sstream>
#include <limits>
#include <vector>
#include <algorithm>

#include "../src/socketstream.hpp"

namespace org {
    namespace sqg {

        template <typename U, typename V> U sstream_cast(V const&);

        template <typename U, typename V>
        U sstream_cast(V const &v) {
            U u;
            std::stringstream ss;
            ss << v;
            ss >> u;
            return u;
        }

        class url {
            public:
                url(std::string const &scheme,
                        std::string const &user, std::string const &password,
                        std::string const &host, std::size_t port,
                        std::string const &path,
                        std::string const &query,
                        std::string const &hash)
                    :_M_scheme(scheme)
                     , _M_user(user), _M_password(password)
                     , _M_host(host), _M_port(port)
                     , _M_path(path)
                     , _M_query(query)
                     , _M_hash(hash)
                {
                }
                virtual ~url() {}

            private:
                static void parse_authority(std::string const &authority,
                        std::string &user, std::string &password) {
                    using namespace std;
                    string::size_type pos(0);
                    string::size_type loc(string::npos);

                    if ((loc = authority.find(":", pos)) == string::npos) {
                        user = authority;
                        password = "";
                    } else {
                        user = authority.substr(pos, loc - pos);
                        password = authority.substr(loc + 1);
                    }
                }

                static void parse_service(std::string const &service,
                        std::string &host, std::size_t &port) {
                    using namespace std;
                    string::size_type pos(0);
                    string::size_type loc(string::npos);

                    if ((loc = service.find(":", pos)) == string::npos) {
                        host = service;
                        port = 0;
                    } else {
                        host = service.substr(pos, loc - pos);
                        port = sstream_cast<std::size_t>(service.substr(loc + 1));
                    }
                }

                static void parse_remote(std::string const &remote,
                        std::string &user, std::string &password,
                        std::string &host, std::size_t &port) {
                    using namespace std;
                    string::size_type pos(0);
                    string::size_type loc(string::npos);

                    string service;
                    string authority;

                    if ((loc = remote.find("@", pos)) == string::npos) {
                        service = remote.substr(loc + 1);
                        user = "";
                        password = "";
                        parse_service(service, host, port);
                    } else {
                        authority = remote.substr(pos, loc - pos);
                        service = remote.substr(loc + 1);
                        parse_authority(authority, user, password);
                        parse_service(service, host, port);
                    }
                }
            public:
                std::string scheme() const { return _M_scheme; }
                std::string user() const { return _M_user; }
                std::string password() const { return _M_password; }
                std::string host() const { return _M_host; }
                std::size_t port() const { return _M_port; }
                std::string path() const { return _M_path; }
                std::string query() const { return _M_query; }
                std::string hash() const { return _M_hash; }

                static url parse(std::string const &s) {
                    using namespace std;
                    string::size_type pos(0);
                    string::size_type loc(string::npos);

                    string scheme;
                    string remote, user, password, host;
                    std::size_t port;
                    string path, query, hash;

                    // URL
                    //   [scheme] :// [user]:[password]@[host]:[port] [path] ? [query] # [hash]
                    //   scheme:[//[user[:password]@]host[:port]][/path][?query][#fragment]
                    if ((loc = s.find("://", pos)) == std::string::npos) {
                        if ((loc = s.find(":/")) == std::string::npos)
                            throw std::runtime_error("no scheme found!");
                        scheme = s.substr(pos, loc - pos);
                        pos = loc + 2;
                        user = "";
                        password = "";
                        host = "";
                        port = 0;
                        if ((loc = s.find("?", pos) == string::npos)) {
                            if ((loc = s.find("#", pos)) == string::npos) {
                                path = s.substr(pos);
                                query = "";
                                hash = "";
                            } else {
                                path = s.substr(pos, loc - pos);
                                query = "";
                                hash = s.substr(loc + 1);
                            }
                        } else {
                            path = s.substr(pos, loc - pos);
                            pos = loc + 1;
                            if ((loc = s.find("#", pos)) == string::npos) {
                                query = s.substr(pos);
                                hash = "";
                            } else {
                                query = s.substr(pos, loc - pos);
                                hash = s.substr(loc + 1);
                            }
                        }
                    } else {
                        scheme = s.substr(pos, loc - pos);
                        pos = loc + 3;
                        if ((loc = s.find("/", pos)) == std::string::npos) {
                            remote = s.substr(pos);
                            parse_remote(remote, user, password, host, port);
                        } else {
                            remote = s.substr(pos, loc - pos);
                            parse_remote(remote, user, password, host, port);
                            pos = loc + 1;
                            if ((loc = s.find("?", pos)) == string::npos) {
                                if ((loc = s.find("#", pos)) == string::npos) {
                                    path = s.substr(pos);
                                    query = "";
                                    hash = "";
                                } else {
                                    path = s.substr(pos, loc - pos);
                                    query = "";
                                    hash = s.substr(loc + 1);
                                }
                            } else {
                                path = s.substr(pos, loc - pos);
                                pos = loc + 1;
                                if ((loc = s.find("#", pos)) == string::npos) {
                                    query = s.substr(pos);
                                    hash = "";
                                } else {
                                    query = s.substr(pos, loc - pos);
                                    hash = s.substr(loc + 1);
                                }
                            }
                        }
                    }
                    return url(scheme, user, password, host, port, path, query, hash);
                }
            private:
                std::string _M_scheme;
                std::string _M_user;
                std::string _M_password;
                std::string _M_host;
                std::size_t _M_port;
                std::string _M_path;
                std::string _M_query;
                std::string _M_hash;
        };

        std::ostream& operator << (std::ostream &os, url const &url) {
            os << url.scheme() << ":";
            if (url.host().length() > 0) {
                os << "//";
                if (url.user().length() > 0) {
                    os << url.user();
                    if (url.password().length() > 0)
                        os << ":" << url.password();
                    os << "@";
                }
                os << url.host();
                if (url.port() != 0)
                    os << ":" << url.port();
            }
            if (url.path().length() > 0 || url.query().length() > 0 || url.hash().length() > 0)
                os << "/" << url.path();
            if (url.query().length() > 0)
                os << "?" << url.query();
            if (url.hash().length() > 0)
                os << "#" << url.hash();
            return os;
        }

        template <typename CharT, typename Traits, typename Allocator>
        std::basic_istream<CharT, Traits>&
        ignore(std::basic_istream<CharT, Traits> &is, std::streamsize n,
                std::basic_string<CharT, Traits, Allocator> const& delim) {
            typedef std::basic_istream<CharT, Traits> istream;
            typedef std::basic_string<CharT, Traits, Allocator> string;
            typedef typename istream::char_type     char_type;
            typedef typename istream::traits_type   traits_type;

            if (delim.length() < 1)
                return is.ignore(n);

            char_type ch;
            std::vector<char_type> chars;
            do {
                is.ignore(n, delim[0]);
                chars.push_back(delim[0]);
                for (typename string::size_type i = 1, n = delim.size(); i < n && is.get(ch); ++i)
                    chars.push_back(ch);
                if (chars.size() != delim.length())
                    return is;
                if (traits_type::compare(&chars[0], &delim[0], delim.length()) == 0)
                    return is;
                if (!is)
                    return is;
                for (typename string::size_type i = 1, n = chars.size(); i < n && is.unget(); ++i)
                chars.clear();
            } while (true);
            return is;
        }

        std::string http_get(std::string const &host, std::size_t port, std::string const &path) {
            using namespace std;
            using namespace ::galik::net;

            string const CONTENT_LENGTH_HEADER = "\r\nContent-Length: ";
            string const HTTP_HEADER_END = "\r\n\r\n";

            socketstream ss;
            ss.open(host, port);
            if (!ss)
                throw std::runtime_error("Faild to connect to ");
            if (ss) {
                ss << "GET " << path << " HTTP/1.1\r\n"
                    << "Host: " << host << "\r\n"
                    << "\r\n"
                    << "\r\n"
                    << std::flush;
            }

            long length = 0;
            std::string response;
            if (!ignore(ss, std::numeric_limits<streamsize>::max(), CONTENT_LENGTH_HEADER)) {
                clog << "Can't find content-length header option!" << endl;
                return "";
            }
            ss >> length;
            clog << "http body length is " << length << endl;
            shared_ptr<char> body(new char[length], std::default_delete<char[]>());
            streamsize nbytes;
            if (!ignore(ss, std::numeric_limits<streamsize>::max(), HTTP_HEADER_END)) {
                clog << "Can't find end-of-http-header!" << endl;
                return "";
            }
            ss.read(body.get(), length);
            nbytes = ss.gcount();
            if (nbytes != length)
                clog << "something wrong!!!" << endl;
            ss.close();
            return string(body.get(), length);
        }

        std::string http_get(url const &url) {
            std::string scheme = url.scheme();
            std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
            if (scheme != "http")
                throw std::runtime_error("Not a HTTP url!");
            std::string path = "/";
            path += url.path();
            if (url.query().length() > 0) {
                path += "?";
                path += url.query();
            }
            if (url.hash().length() > 0) {
                path += "#";
                path += url.hash();
            }
            return http_get(url.host(), url.port() == 0 ? 80 : url.port(), path);
        }

        std::string http_get(std::string const &url) {
            return http_get(url::parse(url));
        }

    }
}

int main(int argc, char* argv[]) try {
    using namespace std;
    using namespace ::galik::net;
    using namespace ::org::sqg;

    string const URL = "http://www.csdn.net/search?q=字母&go=提交&qs=n&form=QBLH&sp=-1&pq=字母&sc=0-2&sk=&cvid=B07E7C70829C4931897DBA93035EB4FD&intlF=0/";

    url const &url = url::parse(URL);
    cout << url << endl;
    cout << http_get(url) << endl;
    return EXIT_SUCCESS;
} catch (std::exception const &e) {
    std::cerr << "[C++ Exception]: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
