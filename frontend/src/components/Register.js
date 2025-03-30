import React, { useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { authService } from '../api';
import './Login.css';

const Register = () => {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);
  const navigate = useNavigate();

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');

    if (password !== confirmPassword) {
      setError('Passwords do not match.');
      return;
    }

    setLoading(true);

    try {
      console.log('Attempting to register with:', { username, password });
      const result = await authService.signup(username, password);
      console.log('Registration response:', result);
      
      // Redirect to login after successful registration
      alert('Registration successful! Please login.');
      navigate('/');
    } catch (err) {
      console.error('Registration error details:', err);
      // Display more detailed error information
      let errorMessage = 'Registration failed. ';
      
      if (err.response) {
        // The request was made and the server responded with a status code
        // that falls out of the range of 2xx
        errorMessage += `Server responded with code ${err.response.status}: ${err.response.data || err.response.statusText}`;
        console.log('Error response:', err.response);
      } else if (err.request) {
        // The request was made but no response was received
        errorMessage += 'No response received from server. Please check if the backend server is running.';
        console.log('Error request:', err.request);
      } else {
        // Something happened in setting up the request that triggered an Error
        errorMessage += err.message || 'Unknown error occurred';
      }
      
      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="auth-container">
      <div className="auth-form">
        <h2>File System Explorer</h2>
        <h3>Create Account</h3>
        {error && <div className="error-message">{error}</div>}
        <form onSubmit={handleSubmit}>
          <div className="form-group">
            <label>Username</label>
            <input
              type="text"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              required
            />
          </div>
          <div className="form-group">
            <label>Password</label>
            <input
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              required
            />
          </div>
          <div className="form-group">
            <label>Confirm Password</label>
            <input
              type="password"
              value={confirmPassword}
              onChange={(e) => setConfirmPassword(e.target.value)}
              required
            />
          </div>
          <button type="submit" disabled={loading} className="auth-button">
            {loading ? 'Registering...' : 'Register'}
          </button>
        </form>
        <p className="auth-link">
          Already have an account? <Link to="/">Login</Link>
        </p>
      </div>
    </div>
  );
};

export default Register; 