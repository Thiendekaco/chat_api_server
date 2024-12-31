#include <iostream>
#include <boost/asio.hpp>
#include "RestServer.h"
#include "TcpServer.h"


int main()
{
    try {
        boost::asio::io_context io_context;

        ThreadPool threadPool(150);

        TcpServer tcpServer(io_context, 12345, threadPool);
        RestServer restServer(io_context, tcp::endpoint(tcp::v4(), 8080), threadPool);

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
