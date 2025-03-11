#ifndef AUTH_H
#define AUTH_H

#include "crow.h"
#include <string>

// Function Declarations
bool isAuthenticated(const crow::request &req, std::string &username);
bool verifyJWT(const std::string &token, std::string &username);
std::string generateJWT(const std::string &username);
bool registerUser(const std::string &username, const std::string &password);
bool loginUser(const std::string &username, const std::string &password);

#endif // AUTH_H
