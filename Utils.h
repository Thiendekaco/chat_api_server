#pragma once
#include <string>
#include <jwt-cpp/jwt.h>

#ifndef UTILS_H
#define UTILS_H


class Utils {
public:
    static std::string hashPassword(const std::string& password);
    static bool checkPassword(const std::string& password, const std::string& hash);
    static std::string generateToken(const std::string& email);
    static std::string generateSalt(size_t length);
};


#endif //UTILS_H
