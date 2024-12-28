cmake_minimum_required(VERSION 3.30)
project(chat_api_server)

set(CMAKE_CXX_STANDARD 20)

# Integrate vcpkg
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# Find the required packages
find_package(Boost REQUIRED COMPONENTS beast asio)
find_package(SOCI REQUIRED COMPONENTS core postgresql)
find_package(PostgreSQL REQUIRED)

# Add the executable
add_executable(chat_api_server main.cpp
        ThreadPool.cpp
        ThreadPool.h
        TcpServer.cpp
        TcpServer.h
        RestServer.cpp
        RestServer.h
        DatabaseManager.cpp
        DatabaseManager.h
)

# Link the required libraries
target_link_libraries(chat_api_server PRIVATE Boost::beast Boost::asio SOCI::soci_core SOCI::soci_postgresql PostgreSQL::PostgreSQL)