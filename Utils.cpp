#include "Utils.h"
#include <argon2.h>
#include <stdexcept>
#include <chrono>

std::string Utils::hashPassword(const std::string& password) {
    return password;
}

bool Utils::checkPassword(const std::string& password, const std::string& hash) {
    return true;
}

std::string Utils::generateToken(const std::string& email) {
    auto token = jwt::create()
        .set_issuer("auth0")
        .set_type("JWS")
        .set_payload_claim("email", jwt::claim(email))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{ 24 })
        .sign(jwt::algorithm::hs256{ "secret" });
    return token;
}