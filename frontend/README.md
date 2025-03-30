# File System Simulator Frontend

This is the frontend for the File System Simulator project, built with React.

## Prerequisites

- Node.js (v14 or later)
- npm or yarn package manager

## Installation

1. Install dependencies:

```bash
npm install
# or
yarn install
```

## Running the Application

1. Start the development server:

```bash
npm start
# or
yarn start
```

This will start the frontend on http://localhost:3000.

Make sure the backend server is running on http://localhost:8080 before using the application.

## Features

- User authentication (login/signup)
- File and folder creation
- File reading and editing
- Directory navigation
- File and folder deletion
- Current path tracking

## Project Structure

- `src/api.js` - API service for communication with the backend
- `src/app.js` - Main application component
- `src/components/` - React components
  - `Login.js` - Login component
  - `Register.js` - Registration component
  - `FileExplorer.js` - Main file explorer component 