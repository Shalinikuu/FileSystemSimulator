# File System Simulator

## Project Overview

The **File System Simulator** is a project aimed at simulating file system operations through a web interface, command-line interface (CLI), voice commands, and multi-user support. The backend is implemented in **C++** with the **Crow framework** for REST API communication, and the frontend is developed using **React.js**.

### Features
- **Web Interface (React.js)**: A user-friendly web-based interface for interacting with the file system.
- **Command-Line Interface (CLI)**: A CLI for executing file system commands such as `mkdir`, `cd`, `create`, `delete`, etc.
- **Voice Commands**: Integration with speech recognition for hands-free file operations.
- **Multi-User Support**: User authentication and access control for file system operations.

## Running the Backend

Follow these steps to run the backend of the File System Simulator:

## Prerequisites:

CMake installed.
MinGW or another C++ compiler.
Crow framework setup for C++.
OpenSSL installed and configured (see instructions below).
Steps to Run the Backend:

Delete the build folder (if it exists from a previous build attempt).

Open Command Prompt and navigate to the backend folder of your project.

## For example:

"YourFolderLocation"\FileSystemSimulator\backend
Generate the build system using CMake:

Run the following command:

cmake -B build -G "MinGW Makefiles"

## Build the project:

## After the CMake command finishes, build the project by running:

cmake --build build
This command will compile the code and create an executable file.

## Navigate to the build folder:

## Run the following command:

cd build
Run the executable:

Start the backend server by running the executable:

FileSystemSimulator\backend\build>FileSystem.exe
Access the Backend:

Your backend should now be running at:

http://localhost:8080/
You can now interact with the backend via REST API calls.

## Downloading and Setting Up OpenSSL

## Download OpenSSL:

Go to the OpenSSL website or a trusted mirror (e.g., Shining Light Productions).
Download the latest Win32/Win64 OpenSSL installer for your system.
Install OpenSSL:

Run the downloaded installer.
Choose the default installation directory or select a different location.
Complete the installation process.
Set System Variable:

Search for "Environment Variables" in the Windows search bar.
Click on "Edit the system environment variables."
Click on the "Environment Variables" button.
Under "System Variables," find1 the "Path" variable and click "Edit."   
1.
github.com
github.com
Click "New" and add the path to the OpenSSL bin directory (e.g., C:\Program Files\OpenSSL-Win64\bin).
Click "OK" on all windows to save the changes.
Verify Installation:

Open a new Command Prompt window.
Type openssl version and press Enter.
You should see the OpenSSL version information if it's installed correctly.

## Explanation

OpenSSL is a cryptography toolkit that provides secure communication and encryption capabilities. It's required for generating and verifying JWT tokens used for authentication in the File System Simulator.
Downloading and installing the OpenSSL installer provides the necessary libraries and tools.
Setting the OpenSSL bin directory in the system's "Path" variable allows you to use the openssl command from any location in the Command Prompt. This is required for running the backend server, which uses OpenSSL for JWT operations.