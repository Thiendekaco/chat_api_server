#include <iostream>
#include <boost/asio.hpp>
#include "RestServer.h"
#include "TcpServer.h"


int main()
{
    try {
        // Create an io_context object
		// io_context in Boost.Asio is an important component used to manage and synchronise multi-threaded I/O tasks.
        // In this application, io_context is used to manage restfulApi as well as TCP/IP.
        boost::asio::io_context io_context;

        // Create a thread pool with 150 threads
        ThreadPool threadPool(150);

        // Create a tcp server object with the io_context, port 12345
        TcpServer tcpServer(io_context, 12345, threadPool);
        // Create a rest server object with the io_context, port 8080
        RestServer restServer(io_context, tcp::endpoint(tcp::v4(), 8080), threadPool);

        // Run the io_context object
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
