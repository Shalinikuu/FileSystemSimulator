# File System Simulator

## Project Overview

The **File System Simulator** is a project aimed at simulating file system operations through a web interface, command-line interface (CLI), voice commands, and multi-user support. The backend is implemented in **C++** with the **Crow framework** for REST API communication, and the frontend is developed using **React.js**.

### Features
- **Web Interface (React.js)**: A user-friendly web-based interface for interacting with the file system.
- **Command-Line Interface (CLI)**: A CLI for executing file system commands such as `mkdir`, `cd`, `create`, `delete`, etc.
- **Voice Commands**: Integration with speech recognition for hands-free file operations.
- **Multi-User Support**: User authentication and access control for file system operations.

## Running the Backend

Follow these steps to run the backend of the **File System Simulator**:

### Prerequisites:
- **CMake** installed.
- **MinGW** or another C++ compiler.
- **Crow** framework setup for C++.

### Steps to Run the Backend:

1. **Delete the `build` folder** (if it exists from a previous build attempt).

2. **Open Command Prompt** and navigate to the backend folder of your project.

   For example:

   ```
   "YourFolderLocation"\FileSystemSimulator\backend
   ```

3. **Generate the build system using CMake**:

   Run the following command:

   ```
   cmake -B build -G "MinGW Makefiles"
   ```

4. **Build the project**:

   After the CMake command finishes, build the project by running:

   ```
   cmake --build build
   ```

   This command will compile the code and create an executable file.

5. **Navigate to the build folder**:

   Run the following command:

   ```
   cd build
   ```

6. **Run the executable**:

   Start the backend server by running the executable:

   ```
   FileSystemSimulator\backend\build>FileSystem.exe
   ```

7. **Access the Backend**:

   Your backend should now be running at:

   ```
   http://localhost:8080/
   ```

   You can now interact with the backend via REST API calls.
