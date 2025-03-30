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
#include <cstdlib>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
std::string currentDirectory = "PBL_FS";
std::mutex fileMutex;

crow::SimpleApp app; // Global declaration

// Global variable to store voice recognition status
std::string voiceRecognitionText = "";
bool voiceRecognitionCompleted = true;
std::string voiceStatusFile = "voice_status.txt";

// Helper function for authentication
crow::response authenticated(const crow::request &req, std::function<crow::response(std::string)> handler)
{
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0)
    {
        return crow::response(401, "Unauthorized: No token provided");
    }

    std::string token = authHeader.substr(7); // Remove "Bearer " prefix
    std::string username;
    if (verifyJWT(token, username))
    {
        // JWT is valid, proceed with username
    }
    else
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

// Remove directory
crow::response removeDirectory(const crow::request &req, const std::string &folderName, const std::string &username)
{
    if (!isValidFileName(folderName))
    {
        return crow::response(400, "Invalid folder name");
    }

    std::string folderPath = userDirectories[username] + "/" + folderName;

    std::error_code ec;
    if (fs::remove_all(folderPath, ec))
    {
        return crow::response(200, "✅ Directory removed successfully");
    }
    else
    {
        return crow::response(500, "❌ Failed to remove directory: " + ec.message());
    }
}

// Create file in the current directory
bool createFileInCurrentDirectory(const std::string &username, const std::string &fileName, const std::string &content)
{
    if (!isValidFileName(fileName))
        return false;

    // Ensure user stays inside their directory
    std::string userRoot = "PBL_FS/" + username;
    if (userDirectories[username].find(userRoot) != 0)
    {
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
    std::cout << "📂 Listing directory contents for: " << userDir << std::endl;
    
    // Check if the directory exists
    if (!fs::exists(userDir)) {
        std::cout << "❌ Directory does not exist: " << userDir << std::endl;
        // Create the directory if it doesn't exist
        std::error_code ec;
        fs::create_directories(userDir, ec);
        if (ec) {
            std::cout << "❌ Failed to create directory: " << ec.message() << std::endl;
            crow::response res(500, "Failed to access directory");
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        }
        std::cout << "✅ Created directory: " << userDir << std::endl;
    }
    
    nlohmann::json items = nlohmann::json::array();

    try {
        for (const auto &entry : fs::directory_iterator(userDir)) {
            nlohmann::json entryInfo = {
                {"name", entry.path().filename().string()},
                {"type", entry.is_directory() ? "directory" : "file"}};
            items.push_back(entryInfo);
            std::cout << "📄 Found " << (entry.is_directory() ? "directory" : "file") << ": " << entry.path().filename().string() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "❌ Error listing directory: " << e.what() << std::endl;
        crow::response res(500, "Error listing directory");
        res.set_header("Access-Control-Allow-Origin", "*");
        return res;
    }

    nlohmann::json response = {{"items", items}};
    std::cout << "📊 Sending directory listing: " << response.dump() << std::endl;
    
    crow::response res(200, response.dump());
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Content-Type", "application/json");
    return res;
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

// Helper function to read voice status from file
void updateVoiceStatusFromFile() {
    std::cout << "Reading voice status from file: " << voiceStatusFile << std::endl;
    std::ifstream file(voiceStatusFile);
    if (file.is_open()) {
        std::string line;
        if (std::getline(file, line)) {
            voiceRecognitionText = line;
            std::cout << "Read voice status: " << voiceRecognitionText << std::endl;
            
            // Check for completion markers in the status text
            bool isComplete = false;
            
            // Success cases
            if (line.find("File created") != std::string::npos ||
                line.find("Folder created") != std::string::npos ||
                line.find("SUCCESS") != std::string::npos ||
                line.find("Deleted:") != std::string::npos ||
                line.find("Navigated to:") != std::string::npos) {
                isComplete = true;
                std::cout << "Success action detected" << std::endl;
            }
            
            // Error cases
            else if (line.find("Failed to") != std::string::npos ||
                    line.find("ERROR") != std::string::npos ||
                    line.find("Error:") != std::string::npos ||
                    line.find("not understood") != std::string::npos) {
                isComplete = true;
                std::cout << "Error condition detected" << std::endl;
            }
            
            // Explicit completion
            else if (line.find("Command completed") != std::string::npos ||
                    line.find("Voice command stopped") != std::string::npos) {
                isComplete = true;
                std::cout << "Explicit completion marker detected" << std::endl;
            }
            
            // Reset when back to listening
            else if (line.find("Listening") != std::string::npos) {
                isComplete = false;
                std::cout << "Back to listening state" << std::endl;
            }
            
            voiceRecognitionCompleted = isComplete;
            std::cout << "Voice recognition completed: " << (voiceRecognitionCompleted ? "true" : "false") << std::endl;
        } else {
            std::cout << "Voice status file is empty" << std::endl;
        }
        file.close();
    } else {
        std::cout << "Failed to open voice status file: " << voiceStatusFile << std::endl;
    }
}

// API Routes
int main()
{
    ensureBaseDirectory();
    app.loglevel(crow::LogLevel::Warning);
    
    // Helper function to add CORS headers to response
    auto setCorsHeaders = [](crow::response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "*");
        res.set_header("Access-Control-Max-Age", "86400"); // 24 hours
        std::cout << "✅ CORS headers set for response" << std::endl;
    };
    
    // Global OPTIONS handler
    CROW_ROUTE(app, "/<path>")
        .methods(crow::HTTPMethod::OPTIONS)
        ([](const crow::request&, std::string path) {
            std::cout << "👉 Handling OPTIONS request for path: " << path << std::endl;
            crow::response res(204);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Max-Age", "86400");
            return res;
        });
    
    // Root OPTIONS handler
    CROW_ROUTE(app, "/")
        .methods(crow::HTTPMethod::OPTIONS)
        ([]() {
            std::cout << "👉 Handling OPTIONS request for root path" << std::endl;
            crow::response res(204);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Max-Age", "86400");
            return res;
        });
    
    // Specific OPTIONS handler for signup
    CROW_ROUTE(app, "/signup")
        .methods(crow::HTTPMethod::OPTIONS)
        ([](){
            std::cout << "👉 Handling OPTIONS request for signup" << std::endl;
            crow::response res(204);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Max-Age", "86400"); // 24 hours
            std::cout << "✅ CORS headers set for signup OPTIONS response" << std::endl;
            return res;
        });
    
    // Sign up Route
    CROW_ROUTE(app, "/signup")
        .methods(crow::HTTPMethod::Post)([](const crow::request &req)
                                         {
                                             std::cout << "👉 Received signup request with headers:" << std::endl;
                                             for (auto& h : req.headers) {
                                                 std::cout << " - " << h.first << ": " << h.second << std::endl;
                                             }
                                             std::cout << "Request body: " << req.body << std::endl;
                                             
                                             crow::response res;
                                             // Set CORS headers immediately - apply to all origins
                                             res.set_header("Access-Control-Allow-Origin", "*");
                                             res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
                                             res.set_header("Access-Control-Allow-Headers", "*");
                                             
                                             auto body = crow::json::load(req.body);
                                             if (!body || !body.has("username") || !body.has("password")) {
                                                 std::cout << "❌ Invalid signup data" << std::endl;
                                                 res.code = 400;
                                                 res.body = "Invalid input";
                                                 return res;
                                             }

                                             std::string username = body["username"].s();
                                             std::string password = body["password"].s();
                                             std::cout << "👤 Attempting to register user: " << username << std::endl;

                                             if (registerUser(username, password)) {
                                                 std::cout << "✅ User registered successfully: " << username << std::endl;
                                                 res.code = 200;
                                                 res.body = crow::json::wvalue({{"status", "success"}}).dump();
                                                 return res;
                                             } else {
                                                 std::cout << "❌ Username already exists: " << username << std::endl;
                                                 res.code = 400;
                                                 res.body = "Username already exists";
                                                 return res;
                                             } });

    // Specific OPTIONS handler for login
    CROW_ROUTE(app, "/login")
        .methods(crow::HTTPMethod::OPTIONS)
        ([](){
            std::cout << "👉 Handling OPTIONS request for login" << std::endl;
            crow::response res(204);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Max-Age", "86400"); // 24 hours
            std::cout << "✅ CORS headers set for login OPTIONS response" << std::endl;
            return res;
        });

    // Login Route
    CROW_ROUTE(app, "/login")
        .methods(crow::HTTPMethod::Post)([](const crow::request &req)
                                         {
            std::cout << "👉 Received login request" << std::endl;
            std::cout << "Request body: " << req.body << std::endl;
            
            crow::response res;
            // Set CORS headers immediately
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "*");
                                             
            auto body = crow::json::load(req.body);
            if (!body || !body.has("username") || !body.has("password")) {
                std::cout << "❌ Invalid login data" << std::endl;
                res.code = 400;
                res.body = "Invalid input";
                return res;
            }

            std::string username = body["username"].s();
            std::string password = body["password"].s();

            if (loginUser(username, password)) {
                std::string token = generateJWT(username);
                
                // Initialize user directory on login
                userDirectories[username] = "PBL_FS/" + username;
                changeDirectory(userDirectories[username], username, "");

                res.code = 200;
                res.body = crow::json::wvalue({
                    {"status", "success"}, 
                    {"token", token}, 
                    {"currentDir", userDirectories[username]}
                }).dump();
                return res;
            } else {
                res.code = 401;
                res.body = "Invalid username or password";
                return res;
            } });

    // Modify the authenticated function to add CORS headers
    auto authenticatedWithCors = [&setCorsHeaders](const crow::request &req, std::function<crow::response(std::string)> handler) {
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            crow::response res(401, "Unauthorized: No token provided");
            setCorsHeaders(res);
            return res;
        }

        std::string token = authHeader.substr(7); // Remove "Bearer " prefix
        std::string username;
        if (verifyJWT(token, username)) {
            // JWT is valid, proceed with username
            auto res = handler(username);
            setCorsHeaders(res);
            return res;
        } else {
            crow::response res(401, "Unauthorized");
            setCorsHeaders(res);
            return res;
        }
    };

    // Create folder inside user-specific directory
    CROW_ROUTE(app, "/mkdir/<string>")
        .methods(crow::HTTPMethod::Post)([&authenticatedWithCors](const crow::request &req, std::string foldername)
                                         { return authenticatedWithCors(req, [&](std::string username)
                                                                { return createDirectory(username + "/" + foldername) ? crow::response(200, "✅ Directory created") : crow::response(500, "❌ Failed"); }); });

    // Start voice command
    CROW_ROUTE(app, "/start-voice")
        .methods("POST"_method)([&](const crow::request &) {
            std::cout << "👉 Starting voice command" << std::endl;
            
            // Reset voice recognition status
            voiceRecognitionText = "Listening...";
            voiceRecognitionCompleted = false;
            
            // Write initial status to file
            std::ofstream file(voiceStatusFile);
            if (file.is_open()) {
                file << "Listening...";
                file.close();
                std::cout << "Status file initialized at: " << voiceStatusFile << std::endl;
            } else {
                std::cerr << "Failed to open status file for writing: " << voiceStatusFile << std::endl;
            }
            
            // Start the Python script for voice recognition
            std::string cmd = "start /B python ../voice_control.py";
            std::cout << "Running command: " << cmd << std::endl;
            system(cmd.c_str());
            
            crow::response res(200, "{\"message\": \"Voice command started\"}");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");
            return res;
        });

    // Get voice recognition status
    CROW_ROUTE(app, "/voice-status")
        .methods("GET"_method)([&]() {
            std::cout << "👉 Getting voice recognition status" << std::endl;
            
            // Update status from file
            updateVoiceStatusFromFile();
            
            // Check if the status is from the file and not the last read state
            std::ifstream freshCheck(voiceStatusFile);
            std::string freshStatus;
            if (freshCheck.is_open()) {
                if (std::getline(freshCheck, freshStatus)) {
                    std::cout << "Fresh status check: " << freshStatus << std::endl;
                    
                    // If the last status is "Command completed", mark as completed
                    if (freshStatus.find("Command completed") != std::string::npos ||
                        freshStatus.find("File created") != std::string::npos ||
                        freshStatus.find("Folder created") != std::string::npos ||
                        freshStatus.find("Deleted") != std::string::npos ||
                        freshStatus.find("Error:") != std::string::npos) {
                        voiceRecognitionCompleted = true;
                        voiceRecognitionText = freshStatus;
                        std::cout << "Detected completion in fresh check" << std::endl;
                    }
                }
                freshCheck.close();
            }
            
            nlohmann::json status = {
                {"text", voiceRecognitionText},
                {"completed", voiceRecognitionCompleted}
            };
            
            crow::response res(200, status.dump());
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");
            return res;
        });

    // OPTIONS handler for voice-status
    CROW_ROUTE(app, "/voice-status")
        .methods(crow::HTTPMethod::OPTIONS)
        ([](){
            std::cout << "👉 Handling OPTIONS request for voice-status" << std::endl;
            crow::response res(204);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Max-Age", "86400"); // 24 hours
            return res;
        });

    // For all other routes, use authenticated with cors
    // Delete folder inside user-specific directory
    CROW_ROUTE(app, "/rmdir/<string>")
        .methods(crow::HTTPMethod::Delete)([&authenticatedWithCors](const crow::request &req, std::string folderName)
                                           { return authenticatedWithCors(req, [&](std::string username)
                                                                  { return removeDirectory(req, folderName, username); }); });

    // Create file in the current directory
    CROW_ROUTE(app, "/create-file/<string>")
        .methods(crow::HTTPMethod::Post)([&authenticatedWithCors](const crow::request &req, std::string fileName)
                                         { return authenticatedWithCors(req, [&](std::string username)
                                                                {
            std::string content = req.body;

            bool success = createFileInCurrentDirectory(username, fileName, content);
            return crow::response(crow::json::wvalue({{"status", success ? "success" : "error"}})); }); });

    // Delete file inside user-specific directory
    CROW_ROUTE(app, "/delete-file/<string>")
        .methods(crow::HTTPMethod::Delete)([&authenticatedWithCors](const crow::request &req, std::string fileName)
                                           { return authenticatedWithCors(req, [&](std::string username)
                                                                  {
            bool success = deleteFileInCurrentDirectory(username, fileName);
            return crow::response(crow::json::wvalue({{"status", success ? "success" : "error"}})); }); });

    // Read file inside user-specific directory
    CROW_ROUTE(app, "/read-file/<string>")
        .methods(crow::HTTPMethod::Get)([&authenticatedWithCors](const crow::request &req, std::string fileName)
                                        { return authenticatedWithCors(req, [&](std::string username)
                                                               {
            std::string content = readFileInCurrentDirectory(username, fileName);
            if (content.empty()) {
                return crow::response(404, "File not found or empty");
            }
            return crow::response(content); }); });

    CROW_ROUTE(app, "/pwd")
        .methods(crow::HTTPMethod::GET)([&authenticatedWithCors](const crow::request &req)
                                        { return authenticatedWithCors(req, [&](std::string username)
                                                               { return crow::response(200, userDirectories[username]); }); });

    // List directory contents
    CROW_ROUTE(app, "/ls")
        .methods(crow::HTTPMethod::GET)([&authenticatedWithCors](const crow::request &req)
                                        { return authenticatedWithCors(req, [&](std::string username)
                                                               { return listDirectoryContents(req, username); }); });

    // Edit file in the current directory
    CROW_ROUTE(app, "/edit-file/<string>")
        .methods(crow::HTTPMethod::PUT)([&authenticatedWithCors](const crow::request &req, std::string fileName)
                                        { return authenticatedWithCors(req, [&](std::string username)
                                                               { return editFileInCurrentDirectory(req, username, fileName); }); });

    // Append to file in the current directory
    CROW_ROUTE(app, "/append-file/<string>")
        .methods(crow::HTTPMethod::PUT)([&authenticatedWithCors](const crow::request &req, std::string fileName)
                                        { return authenticatedWithCors(req, [&](std::string username)
                                                               { return appendToFileInCurrentDirectory(req, username, fileName); }); });

    // Change Directory (cd)
    CROW_ROUTE(app, "/cd/<string>")
        .methods(crow::HTTPMethod::Post)([&authenticatedWithCors](const crow::request &req, std::string folderName)
                                         { return authenticatedWithCors(req, [&](std::string username)
                                                                {
    // Remove local declaration of userDirectories
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
        .methods(crow::HTTPMethod::Post)([&authenticatedWithCors](const crow::request &req) {
            return authenticatedWithCors(req, [&](std::string username)
                                 { return crow::json::wvalue({{"status", moveUpDirectory(username) ? "success" : "error"}}); });
        });

    // Stop voice command
    CROW_ROUTE(app, "/stop-voice")
        .methods("POST"_method)([&](const crow::request &) {
            std::cout << "👉 Stopping voice command" << std::endl;
            
            // Reset voice recognition status
            voiceRecognitionText = "Voice command stopped";
            voiceRecognitionCompleted = true;
            
            // Write status to file
            std::ofstream file(voiceStatusFile);
            if (file.is_open()) {
                file << "Voice command stopped";
                file.close();
                std::cout << "Status file updated to stopped state" << std::endl;
            }
            
            // Try to kill any running voice command process
            system("taskkill /F /IM python.exe /T 2>nul");
            
            crow::response res(200, "{\"message\": \"Voice command stopped\"}");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");
            return res;
        });

    // OPTIONS handler for stop-voice
    CROW_ROUTE(app, "/stop-voice")
        .methods(crow::HTTPMethod::OPTIONS)
        ([](){
            std::cout << "👉 Handling OPTIONS request for stop-voice" << std::endl;
            crow::response res(204);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Max-Age", "86400"); // 24 hours
            return res;
        });

    // Add rename file/folder endpoint
    CROW_ROUTE(app, "/rename/<string>/<string>")
        .methods("POST"_method)
        ([](const crow::request &req, const std::string &oldName, const std::string &newName) {
            return authenticated(req, [&oldName, &newName](std::string username) {
                std::lock_guard<std::mutex> lock(fileMutex);
                
                if (!isValidFileName(oldName) || !isValidFileName(newName)) {
                    return crow::response(400, "Invalid file or folder name");
                }
                
                std::string userDirectory = userDirectories[username];
                std::string oldPath = userDirectory + "/" + oldName;
                std::string newPath = userDirectory + "/" + newName;
                
                // Check if old path exists
                if (!fs::exists(oldPath)) {
                    return crow::response(404, "File or folder not found");
                }
                
                // Check if new path already exists
                if (fs::exists(newPath)) {
                    return crow::response(409, "Destination already exists");
                }
                
                try {
                    fs::rename(oldPath, newPath);
                    std::cout << "✅ Renamed: " << oldPath << " to " << newPath << std::endl;
                    return crow::response(200, "Item renamed successfully");
                } catch (const std::exception &e) {
                    std::cerr << "❌ Error renaming item: " << e.what() << std::endl;
                    return crow::response(500, "Failed to rename item: " + std::string(e.what()));
                }
            });
        });

    std::cout << "🚀 Server running at http://localhost:8080" << std::endl;
    app.port(8080).multithreaded().run();
}
