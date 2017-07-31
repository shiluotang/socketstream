#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <sstream>

#include "../src/socketstream.hpp"


template <typename U, typename V>
U sstream_cast(V const &v) {
    U u;
    std::stringstream ss;
    ss << v;
    ss >> u;
    return u;
}


int main(int argc, char* argv[]) try {
    using namespace std;
    using namespace ::galik::net;

    string const HOST = "www.csdn.net";
    string const PATH = "/index.html";
    string const CONTENT_LENGTH_HEADER = "Content-Length: ";

    socketstream ss;
    ss.open(HOST, 80);
    if (!ss)
        throw std::runtime_error("Faild to connect to ");
    if (ss) {
        ss << "GET " << PATH << " HTTP/1.1\r\n"
            << "Host: " << HOST << "\r\n"
            << "\r\n"
            << "\r\n"
            << std::flush;
    }

    long length;
    std::string response;
    while (getline(ss, response)) {
        if (response.find(CONTENT_LENGTH_HEADER) == 0) {
            length = sstream_cast<long>(response.substr(CONTENT_LENGTH_HEADER.length()));
            clog << "*CONTENT-LENGTH* " << length << endl;
        }
        cout << response << endl;
    }
    ss.close();

    return EXIT_SUCCESS;
} catch (std::exception const &e) {
    std::cerr << "[C++ Exception]: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
