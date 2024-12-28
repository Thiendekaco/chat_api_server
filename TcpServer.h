#include <boost/asio.hpp>
#include "ThreadPool.h"

#ifndef TCPSERVER_H
#define TCPSERVER_H

using boost::asio::ip::tcp;

class TcpServer {
public:
    TcpServer(boost::asio::io_context& io_context, short port, ThreadPool& threadPool);

private:
    void doAccept();
    void handleClient();

    tcp::acceptor acceptor_; // Member variable to accept incoming connections
    ThreadPool& threadPool_;
    std::shared_ptr<tcp::socket> socket_; // Member variable for the socket
};

#endif // TCPSERVER_H