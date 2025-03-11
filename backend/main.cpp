#include "crow.h"
#include "auth.h"
#include "json.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <regex>
#include <stdexcept>
#include "globals.h"

namespace fs = std::filesystem;
std::string currentDirectory = "PBL_FS";
std::mutex fileMutex;

crow::SimpleApp app; // Global declaration

// Helper function for authentication
crow::response authenticated(const crow::request &req, std::function<crow::response(std::string)> handler)
{
    std::string username;
    if (!isAuthenticated(req, username))
    {
        return crow::response(401, "Unauthorized");
    }
    return handler(username);
}

// Ensure base directory exists
void ensureBaseDirectory()
{
    std::error_code ec;
    if (!fs::exists(currentDirectory))
    {
        if (fs::create_directory(currentDirectory, ec))
        {
            std::cout << "📁 Created base directory: " << currentDirectory << std::endl;
        }
        else
        {
            std::cerr << "❌ Failed to create base directory: " << ec.message() << std::endl;
        }
    }
}

// Input Validation Helper
bool isValidFileName(const std::string &name)
{
    if (name.empty())
        return false;
    std::regex pattern("^[a-zA-Z0-9_\\-\\.]+$");
    return std::regex_match(name, pattern);
}

// File System Operations
bool createDirectory(const std::string &folderName)
{
    std::string newPath;

    // Handle absolute and relative paths
    if (folderName.find("PBL_FS/") == 0)
    {
        newPath = folderName; // Absolute path
    }
    else
    {
        newPath = currentDirectory + "/" + folderName; // Append to current directory
    }

    // Use mkdir() to create the directory with permissions
    if (mkdir(newPath.c_str()) == 0)
    {
        std::cout << "✅ Directory created: " << newPath << std::endl;
        return true;
    }
    else
    {
        perror("❌ mkdir failed"); // Prints system error
        return false;
    }
}

crow::response removeDirectory(const crow::request &req, const std::string &folderName, const std::string &username)
{
    if (!isValidFileName(folderName))
    {
        return crow::response(400, "Invalid folder name");
    }

    std::string folderPath = userDirectories[username] + "/" + folderName; // Use user's current directory

    std::error_code ec;
    if (fs::remove_all(folderPath, ec))
    {
        return crow::response(200, "✅ Directory removed successfully"); // Success message
    }
    else
    {
        return crow::response(500, "❌ Failed to remove directory: " + ec.message()); // Failure message
    }
}

bool createFileInCurrentDirectory(const std::string &username, const std::string &fileName, const std::string &content)
{
    if (!isValidFileName(fileName))
        return false;

    // Ensure user stays inside their directory
    std::string userRoot = "PBL_FS/" + username;
    if (userDirectories[username].find(userRoot) != 0)
    { // Corrected line
        std::cerr << "❌ Access denied! Cannot create files outside user directory." << std::endl;
        return false;
    }

    std::string filePath = userDirectories[username] + "/" + fileName; // Corrected line
    std::ofstream file(filePath);

    if (file.is_open())
    {
        file << content;
        file.close();
        std::cout << "✅ File created: " << filePath << std::endl;
        return true;
    }

    std::cerr << "❌ Failed to create file: " << filePath << std::endl;
    return false;
}

std::string readFileInCurrentDirectory(const std::string &username, const std::string &fileName)
{
    if (!isValidFileName(fileName))
        return "";

    // Get the user's current working directory
    std::string userDirectory = userDirectories[username]; // Corrected line
    std::string filePath = userDirectory + "/" + fileName;

    std::cout << "📖 Attempting to read file: " << filePath << std::endl;

    // Check if the file exists
    if (!fs::exists(filePath))
    {
        std::cerr << "⚠️ File not found: " << filePath << std::endl;
        return "";
    }

    // Read the file
    std::ifstream file(filePath);
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        std::cout << "✅ File read successfully: " << filePath << std::endl;
        return buffer.str();
    }
    else
    {
        std::cerr << "❌ Failed to open file: " << filePath << std::endl;
        return "";
    }
}

// Delete file in the current directory
bool deleteFileInCurrentDirectory(const std::string &username, const std::string &fileName)
{
    if (!isValidFileName(fileName))
    {
        std::cout << "DEBUG: Invalid filename: " << fileName << std::endl;
        return false;
    }

    std::string userDirectory = userDirectories[username];
    std::string filePath = userDirectory + "/" + fileName;

    std::cout << "DEBUG: Attempting to delete file: " << filePath << std::endl;

    if (!fs::exists(filePath))
    {
        std::cout << "DEBUG: File not found: " << filePath << std::endl;
        return false;
    }

    std::error_code ec;
    if (fs::remove(filePath, ec))
    {
        std::cout << "DEBUG: File deleted successfully: " << filePath << std::endl;
        return true;
    }
    else
    {
        std::cerr << "DEBUG: Failed to delete file: " << ec.message() << std::endl;
        return false;
    }
}

// List directory contents
crow::response listDirectoryContents(const crow::request &req, const std::string &username)
{
    std::string userDir = userDirectories[username];
    nlohmann::json response = nlohmann::json::array();

    for (const auto &entry : fs::directory_iterator(userDir))
    {
        nlohmann::json entryInfo = {
            {"name", entry.path().filename().string()},
            {"type", entry.is_directory() ? "directory" : "file"}};
        response.push_back(entryInfo);
    }

    return crow::response(200, response.dump());
}

// Edit file in the current directory
crow::response editFileInCurrentDirectory(const crow::request &req, const std::string &username, const std::string &fileName)
{
    if (!isValidFileName(fileName))
    {
        return crow::response(400, "Invalid filename");
    }

    std::string userDir = userDirectories[username];
    std::string filePath = userDir + "/" + fileName;

    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << req.body; // Write the request body (new content) to the file
        file.close();
        return crow::response(200, "File edited successfully");
    }
    else
    {
        return crow::response(500, "Failed to open file for editing");
    }
}

// Append to file in the current directory
crow::response appendToFileInCurrentDirectory(const crow::request &req, const std::string &username, const std::string &fileName)
{
    if (!isValidFileName(fileName))
    {
        return crow::response(400, "Invalid filename");
    }

    std::string userDir = userDirectories[username];
    std::string filePath = userDir + "/" + fileName;

    std::ofstream file(filePath, std::ios_base::app); // Open in append mode
    if (file.is_open())
    {
        file << req.body; // Append the request body (new content) to the file
        file.close();
        return crow::response(200, "File appended successfully");
    }
    else
    {
        return crow::response(500, "Failed to open file for appending");
    }
}

// Change directory
bool changeDirectory(std::string &currentDirectory, const std::string &username, const std::string &newPath)
{
    std::filesystem::path userRoot = "PBL_FS/" + username;
    std::filesystem::path currentPath = currentDirectory;

    if (newPath == "..") // Go back one directory
    {
        if (currentPath == userRoot) // Prevent going above user root
        {
            std::cerr << "⚠️ You are already at the root directory!" << std::endl;
            return false;
        }
        currentDirectory = currentPath.parent_path().string();
        return true;
    }
    else
    {
        std::filesystem::path potentialPath = currentPath / newPath;

        if (std::filesystem::exists(potentialPath) && std::filesystem::is_directory(potentialPath))
        {
            currentDirectory = potentialPath.string();
            return true;
        }
        else
        {
            std::cerr << "❌ Directory does not exist: " << potentialPath << std::endl;
            return false;
        }
    }
}

bool moveUpDirectory(const std::string &username)
{
    if (userDirectories.find(username) == userDirectories.end())
    {
        userDirectories[username] = "PBL_FS/" + username;
    }

    std::string &userDir = userDirectories[username];
    std::string userRoot = "PBL_FS/" + username;

    if (userDir == userRoot)
    {
        std::cerr << "⚠️ Cannot move above root directory: " << userRoot << std::endl;
        return false;
    }

    userDir = fs::path(userDir).parent_path().string();
    return true;
}

// API Routes
int main()
{
    ensureBaseDirectory();
    app.loglevel(crow::LogLevel::Warning);

    // Sign up Route
    CROW_ROUTE(app, "/signup")
        .methods(crow::HTTPMethod::Post)([](const crow::request &req)
                                         {
                                             auto body = crow::json::load(req.body);
                                             if (!body || !body.has("username") || !body.has("password"))
                                                 return crow::response(400, "Invalid input");

                                             std::string username = body["username"].s();
                                             std::string password = body["password"].s();

                                             if (registerUser(username, password)) // Call registerUser from auth.cpp
                                             {
                                                 return crow::response(200, crow::json::wvalue({{"status", "success"}}));
                                             }
                                             else
                                             {
                                                 return crow::response(400, "Username already exists");
                                             } });

    // Login Route
    CROW_ROUTE(app, "/login")
        .methods(crow::HTTPMethod::Post)([](const crow::request &req)
                                         {
    auto body = crow::json::load(req.body);
    if (!body || !body.has("username") || !body.has("password")) 
    {
        return crow::response(400, "Invalid input");
    }

    std::string username = body["username"].s();
    std::string password = body["password"].s();

    if (loginUser(username, password)) 
    {
        std::string token = generateJWT(username);
        
        // Initialize user directory on login
        userDirectories[username] = "PBL_FS/" + username;
        changeDirectory(userDirectories[username], username, "");

        return crow::response(200, crow::json::wvalue({{"status", "success"}, {"token", token}, {"currentDir", userDirectories[username]}}));
    }
    else
    {
        return crow::response(401, "Invalid username or password");
    } });

    // Create folder inside user-specific directory
    CROW_ROUTE(app, "/mkdir/<string>")
        .methods(crow::HTTPMethod::Post)([](const crow::request &req, std::string foldername)
                                         {
        if (createDirectory(foldername)) {
            return crow::response(200, "✅ Directory created successfully");
        } else {
            return crow::response(500, "❌ Failed to create directory");
        } });

    // Delete folder inside user-specific directory
    CROW_ROUTE(app, "/rmdir/<string>")
        .methods(crow::HTTPMethod::Delete)([](const crow::request &req, std::string folderName)
                                           { return authenticated(req, [&](std::string username)
                                                                  { return removeDirectory(req, folderName, username); }); });

    // Create file in the current directory
    CROW_ROUTE(app, "/create-file/<string>")
        .methods(crow::HTTPMethod::Post)([](const crow::request &req, std::string fileName)
                                         { return authenticated(req, [&](std::string username)
                                                                {
            std::string content = req.body;

            bool success = createFileInCurrentDirectory(username, fileName, content);
            return crow::response(crow::json::wvalue({{"status", success ? "success" : "error"}})); }); });

    // Delete file inside user-specific directory
    CROW_ROUTE(app, "/delete-file/<string>")
        .methods(crow::HTTPMethod::Delete)([](const crow::request &req, std::string fileName)
                                           { return authenticated(req, [&](std::string username)
                                                                  {
            bool success = deleteFileInCurrentDirectory(username, fileName);
            return crow::response(crow::json::wvalue({{"status", success ? "success" : "error"}})); }); });

    // Read file inside user-specific directory
    CROW_ROUTE(app, "/read-file/<string>")
        .methods(crow::HTTPMethod::Get)([](const crow::request &req, std::string fileName)
                                        { return authenticated(req, [&](std::string username)
                                                               {
            std::string content = readFileInCurrentDirectory(username, fileName);
            if (content.empty()) {
                return crow::response(404, "File not found or empty");
            }
            return crow::response(content); }); });

    // List directory contents
    CROW_ROUTE(app, "/ls")
        .methods(crow::HTTPMethod::GET)([](const crow::request &req)
                                        { return authenticated(req, [&](std::string username)
                                                               { return listDirectoryContents(req, username); }); });

    // Edit file in the current directory
    CROW_ROUTE(app, "/edit-file/<string>")
        .methods(crow::HTTPMethod::PUT)([](const crow::request &req, std::string fileName)
                                        { return authenticated(req, [&](std::string username)
                                                               { return editFileInCurrentDirectory(req, username, fileName); }); });

    // Append to file in the current directory
    CROW_ROUTE(app, "/append-file/<string>")
        .methods(crow::HTTPMethod::PUT)([](const crow::request &req, std::string fileName) {
            return authenticated(req, [&](std::string username)
                                 { return appendToFileInCurrentDirectory(req, username, fileName); });
        });

    // Change Directory (cd)
    CROW_ROUTE(app, "/cd/<string>")
        .methods(crow::HTTPMethod::Post)([](const crow::request &req, std::string folderName)
                                         { return authenticated(req, [&](std::string username)
                                                                {
    // ✅ Remove local declaration of userDirectories
    if (userDirectories.find(username) == userDirectories.end()) {
        userDirectories[username] = "PBL_FS/" + username;
    }

    bool success = changeDirectory(userDirectories[username], username, folderName);

    return crow::json::wvalue({
        {"status", success ? "success" : "error"},
        {"currentDir", userDirectories[username]}
    }); }); });

    // Move Up (cd ..)
    CROW_ROUTE(app, "/cd..")
        .methods(crow::HTTPMethod::Post)([](const crow::request &req) { // ✅ Corrected
            return authenticated(req, [&](std::string username)
                                 { return crow::json::wvalue({{"status", moveUpDirectory(username) ? "success" : "error"}}); });
        });

    std::cout << "🚀 Server running at http://localhost:8080" << std::endl;
    app.port(8080).multithreaded().run();
}
