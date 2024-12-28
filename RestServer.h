#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include "ThreadPool.h"

#ifndef RESTSERVER_H
#define RESTSERVER_H

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class RestServer {
public:
    RestServer(net::io_context& ioc, tcp::endpoint endpoint, ThreadPool& threadPool);

private:
    void doAccept();
    void handleRequest(std::shared_ptr<tcp::socket> socket);
    void fail(beast::error_code ec, char const* what);

    tcp::acceptor acceptor_;
    ThreadPool& threadPool_;
};

#endif // RESTSERVER_H