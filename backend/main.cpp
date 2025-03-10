#include "crow.h"
#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
std::string currentDirectory = "PBL_FS";

// Ensure base directory exists
void ensureBaseDirectory()
{
    std::error_code ec;
    if (!fs::exists(currentDirectory))
    {
        if (fs::create_directory(currentDirectory, ec))
        {
            std::cout << "Created base directory: " << currentDirectory << std::endl;
        }
        else
        {
            std::cerr << "Failed to create base directory: " << currentDirectory << " Error: " << ec.message() << std::endl;
        }
    }
}

// Function to create a directory
bool createDirectory(const std::string &folderName)
{
    fs::path dirPath = currentDirectory + "/" + folderName;
    if (fs::exists(dirPath))
        return false;
    return fs::create_directory(dirPath);
}

// Function to delete a directory
bool deleteDirectory(const std::string &folderName)
{
    fs::path dirPath = currentDirectory + "/" + folderName;
    return fs::remove_all(dirPath) > 0;
}

// Function to list directory contents
std::string listContents()
{
    if (!fs::exists(currentDirectory))
        return "Error: Base directory not found!";

    std::string output;
    for (const auto &entry : fs::directory_iterator(currentDirectory))
    {
        output += (fs::is_directory(entry) ? "[DIR] " : "[FILE] ") + entry.path().filename().string() + "\n";
    }
    return output.empty() ? "Directory is empty" : output;
}

// Function to create a file
bool createFile(const std::string &fileName)
{
    std::ofstream file(currentDirectory + "/" + fileName);
    return file.good();
}

// Function to write to a file
bool writeFile(const std::string &fileName, const std::string &content)
{
    std::ofstream file(currentDirectory + "/" + fileName, std::ios::app);
    if (file)
    {
        file << content << "\n";
        return true;
    }
    return false;
}

// Function to read a file
std::string readFile(const std::string &fileName)
{
    std::ifstream file(currentDirectory + "/" + fileName);
    if (!file)
        return "Error: File not found!";

    std::string line, output;
    while (getline(file, line))
    {
        output += line + "\n";
    }
    return output.empty() ? "File is empty" : output;
}

// Function to delete a file
bool deleteFile(const std::string &fileName)
{
    return fs::remove(currentDirectory + "/" + fileName);
}

// Function to rename a file or directory
bool renameFileOrDirectory(const std::string &oldName, const std::string &newName)
{
    fs::path oldPath = currentDirectory + "/" + oldName;
    fs::path newPath = currentDirectory + "/" + newName;
    if (!fs::exists(oldPath) || fs::exists(newPath))
        return false;
    std::error_code ec;
    fs::rename(oldPath, newPath, ec);
    return !ec;
}

int main()
{
    crow::SimpleApp app;
    ensureBaseDirectory(); // Ensure PBL_FS exists

    app.loglevel(crow::LogLevel::Warning);

    CROW_ROUTE(app, "/mkdir/<string>").methods("POST"_method)([](const crow::request &, std::string folderName)
                                                              { return crow::json::wvalue({{"status", createDirectory(folderName) ? "success" : "error"}}); });

    CROW_ROUTE(app, "/rmdir/<string>").methods("DELETE"_method)([](const crow::request &, std::string folderName)
                                                                { return crow::json::wvalue({{"status", deleteDirectory(folderName) ? "success" : "error"}}); });

    CROW_ROUTE(app, "/ls").methods("GET"_method)([]
                                                 { return crow::json::wvalue({{"status", "success"}, {"contents", listContents()}}); });

    CROW_ROUTE(app, "/create/<string>").methods("POST"_method)([](const crow::request &, std::string fileName)
                                                               { return crow::json::wvalue({{"status", createFile(fileName) ? "success" : "error"}}); });

    CROW_ROUTE(app, "/write/<string>").methods("POST"_method)([](const crow::request &req, std::string fileName)
                                                              { return crow::json::wvalue({{"status", writeFile(fileName, req.body) ? "success" : "error"}}); });

    CROW_ROUTE(app, "/read/<string>").methods("GET"_method)([](const crow::request &, std::string fileName)
                                                            { return crow::json::wvalue({{"status", "success"}, {"content", readFile(fileName)}}); });

    CROW_ROUTE(app, "/delete/<string>").methods("DELETE"_method)([](const crow::request &, std::string fileName)
                                                                 { return crow::json::wvalue({{"status", deleteFile(fileName) ? "success" : "error"}}); });

    CROW_ROUTE(app, "/rename/<string>/<string>").methods("PUT"_method)([](const crow::request &, std::string oldName, std::string newName)
                                                                       { return crow::json::wvalue({{"status", renameFileOrDirectory(oldName, newName) ? "success" : "error"}}); });

    std::cout << "Server running at http://localhost:8080" << std::endl;
    app.port(8080).multithreaded().run();
}
