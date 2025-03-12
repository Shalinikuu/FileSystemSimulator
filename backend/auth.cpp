#include "auth.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;
extern std::string currentDirectory;

// Define your secret key here (for testing purposes only)
const std::string SECRET_KEY = "YourSecretKey123"; // Replace with a strong, secure key

// Function to verify JWT Token
bool verifyJWT(const std::string &token, std::string &username)
{
    try
    {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{SECRET_KEY});
        verifier.verify(decoded);
        username = decoded.get_payload_claim("username").as_string();
        return true;
    }
    catch (const std::runtime_error &e) // FIXED
    {
        std::cerr << "JWT verification error: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in verifyJWT: " << e.what() << std::endl;
        return false;
    }
}

// Load or save user data with error handling
json loadUsers()
{
    std::ifstream file("database.json");
    json users;
    if (file.is_open())
    {
        try
        {
            file >> users;
        }
        catch (json::parse_error &e)
        {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            // Handle parse error (e.g., return empty json)
        }
    }
    else
    {
        std::cerr << "Error opening database.json" << std::endl;
        // Handle file open error (e.g., return empty json)
    }
    return users;
}

void saveUsers(const json &users)
{
    std::ofstream file("database.json");
    if (file.is_open())
    {
        file << users.dump(4);
    }
    else
    {
        std::cerr << "Error opening database.json for writing" << std::endl;
        // Handle file open error
    }
}

// Generate JWT Token with expiration
std::string generateJWT(const std::string &username)
{
    auto token = jwt::create()
                     .set_issuer("filesystem")
                     .set_type("JWT")
                     .set_payload_claim("username", jwt::claim(username))
                     .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(1)) // 1 hour expiration
                     .sign(jwt::algorithm::hs256{SECRET_KEY});
    return token;
}

// Function to check if the user is authenticated based on JWT token
bool isAuthenticated(const crow::request &req, std::string &username)
{
    // Extract the JWT token from the Authorization header
    auto authorization_header = req.get_header_value("Authorization");
    if (authorization_header.empty())
    {
        std::cerr << "No Authorization header provided." << std::endl;
        return false;
    }

    // Assuming the token is in the format "Bearer <token>"
    if (authorization_header.rfind("Bearer ", 0) == 0)
    {
        std::string token = authorization_header.substr(7); // Remove "Bearer " prefix
        return verifyJWT(token, username);                  // Use the verifyJWT function to validate the token
    }
    else
    {
        std::cerr << "Invalid Authorization header format." << std::endl;
        return false;
    }
}

// Register User
bool registerUser(const std::string &username, const std::string &password)
{
    json users = loadUsers();
    if (users.contains(username))
        return false; // User already exists

    users[username] = password;
    saveUsers(users);

    // ✅ Fix: Ensure the user directory follows the same structure as login
    std::string userDirectory = "PBL_FS/" + username;
    std::error_code ec;

    if (!fs::exists(userDirectory))
    {
        if (!fs::create_directory(userDirectory, ec))
        {
            std::cerr << "❌ Failed to create user directory: " << ec.message() << std::endl;
            return false;
        }
    }

    return true;
}

// Login User

bool loginUser(const std::string &username, const std::string &password)
{
    json users = loadUsers();
    if (!users.contains(username) || users[username].get<std::string>() != password)
    {
        return false; // Invalid username or password
    }

    std::string userDirectory = "PBL_FS/" + username;
    std::string originalDirectory = currentDirectory; // Save the current directory
    currentDirectory = userDirectory;

    if (!fs::exists(userDirectory))
    {
        std::cerr << "⚠️ User directory does not exist. Creating: " << userDirectory << std::endl;
        fs::create_directories(userDirectory);
    }

    std::cout << "✅ User logged in. Current directory: " << currentDirectory << std::endl;

    // 🔐 Generate and save token
    std::string token = generateJWT(username);

    // Save token to file
    std::ofstream tokenFile("token.txt", std::ios::out | std::ios::trunc);
    if (tokenFile.is_open())
    {
        tokenFile << token;
        tokenFile.close();
        std::cout << "🔐 Token saved to token.txt\n";
    }
    else
    {
        std::cerr << "❌ Failed to create token.txt! Trying to create manually...\n";

        // Try manually creating the file
        std::ofstream createFile("../token.txt");
        if (createFile.is_open())
        {
            createFile.close();
            std::cout << "✅ token.txt created successfully. Retrying token save...\n";

            // Retry writing token
            std::ofstream retryTokenFile("token.txt");
            if (retryTokenFile.is_open())
            {
                retryTokenFile << token;
                retryTokenFile.close();
                std::cout << "🔐 Token saved successfully!\n";
            }
            else
            {
                std::cerr << "❌ Still failed to save token!\n";
            }
        }
        else
        {
            std::cerr << "❌ Could not manually create token.txt!\n";
        }
    }

    // Restore the original directory
    currentDirectory = originalDirectory;

    return true;
}