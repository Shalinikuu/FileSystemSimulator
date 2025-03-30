import axios from 'axios';

// Use relative URLs to leverage the proxy in package.json
const API_URL = '';

// Create axios instance with default config
const api = axios.create({
  baseURL: API_URL,
  headers: {
    'Content-Type': 'application/json',
  },
  withCredentials: false // Important for CORS
});

// Add interceptor to add auth token to requests
api.interceptors.request.use(
  (config) => {
    const token = localStorage.getItem('token');
    if (token) {
      config.headers.Authorization = `Bearer ${token}`;
    }
    // Ensure content type is set for all requests
    config.headers['Content-Type'] = 'application/json';
    return config;
  },
  (error) => Promise.reject(error)
);

// Auth services
export const authService = {
  login: async (username, password) => {
    try {
      // Use axios for consistency with other calls
      const response = await api.post('/login', { username, password });
      if (response.data.token) {
        localStorage.setItem('token', response.data.token);
        localStorage.setItem('currentDir', response.data.currentDir);
      }
      return response.data;
    } catch (error) {
      console.error('Login error:', error);
      throw error;
    }
  },
  signup: async (username, password) => {
    try {
      // Use axios now that we have a proxy
      const response = await api.post('/signup', { username, password });
      return response.data;
    } catch (error) {
      console.error('Signup error:', error);
      throw error;
    }
  },
  logout: () => {
    localStorage.removeItem('token');
    localStorage.removeItem('currentDir');
  },
  isAuthenticated: () => {
    const token = localStorage.getItem('token');
    
    // If no token exists, user is not authenticated
    if (!token) {
      console.log('No auth token found');
      return false;
    }
    
    try {
      // Basic validation - check if token format is valid
      // A proper JWT should have 3 parts separated by dots
      const tokenParts = token.split('.');
      if (tokenParts.length !== 3) {
        console.log('Invalid token format');
        return false;
      }
      
      // More thorough validation would involve checking expiration
      // For simplicity, we're just checking format here
      
      return true;
    } catch (error) {
      console.error('Error validating token:', error);
      return false;
    }
  }
};

// File system services
export const fileSystemService = {
  // List directory contents
  listFiles: async () => {
    console.log("API CALL: Listing files");
    try {
      const response = await api.get('/ls');
      console.log("API RESPONSE - listFiles:", response);
      return response;
    } catch (error) {
      console.error("API ERROR - listFiles:", error);
      throw error;
    }
  },
  
  // Get current directory
  getCurrentDirectory: async () => {
    console.log("API CALL: Getting current directory");
    try {
      const response = await api.get('/pwd');
      console.log("API RESPONSE - getCurrentDirectory:", response);
      return response;
    } catch (error) {
      console.error("API ERROR - getCurrentDirectory:", error);
      throw error;
    }
  },
  
  // Create directory
  createDirectory: async (folderName) => {
    return await api.post(`/mkdir/${folderName}`);
  },
  
  // Delete directory
  deleteDirectory: async (folderName) => {
    return await api.delete(`/rmdir/${folderName}`);
  },
  
  // Create file
  createFile: async (fileName, content) => {
    return await api.post(`/create-file/${fileName}`, content);
  },
  
  // Read file
  readFile: async (fileName) => {
    return await api.get(`/read-file/${fileName}`);
  },
  
  // Edit file
  editFile: async (fileName, content) => {
    return await api.put(`/edit-file/${fileName}`, content);
  },
  
  // Append to file
  appendToFile: async (fileName, content) => {
    return await api.put(`/append-file/${fileName}`, content);
  },
  
  // Delete file
  deleteFile: async (fileName) => {
    return await api.delete(`/delete-file/${fileName}`);
  },
  
  // Rename item (file or directory)
  renameItem: async (oldName, newName) => {
    return await api.post(`/rename/${oldName}/${newName}`);
  },
  
  // Change directory
  changeDirectory: async (path) => {
    const response = await api.post(`/cd/${path}`);
    if (response.data.currentDir) {
      localStorage.setItem('currentDir', response.data.currentDir);
    }
    return response.data;
  },
  
  // Move up directory
  moveUpDirectory: async () => {
    return await api.post('/cd..');
  },
  
  // Start voice command
  startVoiceCommand: async () => {
    return await api.post('/start-voice');
  },
  
  // Stop voice command
  stopVoiceCommand: async () => {
    return await api.post('/stop-voice');
  },
  
  // Get voice recognition status
  getVoiceStatus: async () => {
    return await api.get('/voice-status');
  }
};

export default api;
