import React, { useState, useEffect } from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate, useNavigate } from 'react-router-dom';
import './App.css';
import Login from './components/Login';
import Register from './components/Register';
import FileExplorer from './components/FileExplorer';
import { authService } from './api';

// Wrapper component to handle auth check and redirection
const ProtectedRoute = ({ children }) => {
  const [isChecking, setIsChecking] = useState(true);
  const [isAuthorized, setIsAuthorized] = useState(false);
  const navigate = useNavigate();
  
  useEffect(() => {
    // Check authentication status
    const checkAuth = () => {
      const isAuth = authService.isAuthenticated();
      setIsAuthorized(isAuth);
      setIsChecking(false);
      
      if (!isAuth) {
        // Redirect to login if not authenticated
        navigate('/');
      }
    };
    
    checkAuth();
  }, [navigate]);
  
  // Show loading while checking
  if (isChecking) {
    return <div className="loading-auth">Checking authentication...</div>;
  }
  
  // Render children only if authorized
  return isAuthorized ? children : <Navigate to="/" />;
};

function App() {
  const [isAuthenticated, setIsAuthenticated] = useState(false);

  useEffect(() => {
    // Check if user is already authenticated on initial load
    const authStatus = authService.isAuthenticated();
    console.log("Auth status on load:", authStatus);
    setIsAuthenticated(authStatus);
    
    // Initialize dark mode from localStorage
    const savedTheme = localStorage.getItem('darkMode');
    if (savedTheme === 'true') {
      document.body.classList.add('dark-mode');
    } else {
      document.body.classList.remove('dark-mode');
    }
    
    // Add event listener for storage changes (for logout in other tabs)
    const handleStorageChange = (event) => {
      // Handle auth status change
      if (event.key === 'token') {
        const currentAuthStatus = authService.isAuthenticated();
        console.log("Auth status changed:", currentAuthStatus);
        setIsAuthenticated(currentAuthStatus);
      }
      
      // Handle theme change
      if (event.key === 'darkMode') {
        if (event.newValue === 'true') {
          document.body.classList.add('dark-mode');
        } else {
          document.body.classList.remove('dark-mode');
        }
      }
    };
    
    window.addEventListener('storage', handleStorageChange);
    
    // Clean up
    return () => {
      window.removeEventListener('storage', handleStorageChange);
    };
  }, []);

  return (
    <Router>
      <div className="App">
        <Routes>
          <Route 
            path="/" 
            element={isAuthenticated ? <Navigate to="/files" /> : <Login setIsAuthenticated={setIsAuthenticated} />} 
          />
          <Route 
            path="/register" 
            element={<Register />} 
          />
          <Route 
            path="/files" 
            element={
              <ProtectedRoute>
                <FileExplorer />
              </ProtectedRoute>
            } 
          />
          {/* Default redirect for any unmatched routes */}
          <Route path="*" element={<Navigate to="/" />} />
        </Routes>
      </div>
    </Router>
  );
}

export default App;
