import React, { useState, useEffect, useCallback } from 'react';
import { useNavigate } from 'react-router-dom';
import { fileSystemService, authService } from '../api';
import { FaFolder, FaFile, FaArrowUp, FaTrash, FaEdit, FaPen, FaSave, FaMicrophone, FaCheck, FaSun, FaMoon } from 'react-icons/fa';
import './FileExplorer.css';

const FileExplorer = () => {
  const [files, setFiles] = useState([]);
  const [currentDir, setCurrentDir] = useState('');
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [showCreateFolder, setShowCreateFolder] = useState(false);
  const [showCreateFile, setShowCreateFile] = useState(false);
  const [newFolderName, setNewFolderName] = useState('');
  const [newFileName, setNewFileName] = useState('');
  const [newFileContent, setNewFileContent] = useState('');
  const [selectedFile, setSelectedFile] = useState(null);
  const [fileContent, setFileContent] = useState('');
  const [editMode, setEditMode] = useState(false);
  const [voiceActive, setVoiceActive] = useState(false);
  const [voiceText, setVoiceText] = useState('');
  const [recognitionInterval, setRecognitionInterval] = useState(null);
  const [successNotification, setSuccessNotification] = useState(null);
  const [darkMode, setDarkMode] = useState(false);
  const [itemToRename, setItemToRename] = useState(null);
  const [newItemName, setNewItemName] = useState('');
  const navigate = useNavigate();

  const handleLogout = () => {
    try {
      console.log('Logging out...');
      // First clear the auth data
      authService.logout();
      console.log('Auth data cleared');
      
      // Force navigation to login page and reload to reset state
      console.log('Redirecting to login page');
      window.location.href = '/';
    } catch (err) {
      console.error('Logout error:', err);
      // Fallback navigation
      navigate('/');
    }
  };

  const fetchFiles = useCallback(async () => {
    try {
      setLoading(true);
      setError('');
      
      // Set a timeout to prevent being stuck in loading state forever
      const timeoutId = setTimeout(() => {
        console.log('Fetch files timeout reached, forcing loading to false');
        setLoading(false);
        setError('Request timed out. Please try again.');
      }, 10000); // 10 second timeout
      
      // Fetch current directory first
      console.log('Fetching current directory...');
      const dirResponse = await fileSystemService.getCurrentDirectory();
      console.log('Current directory response:', dirResponse);
      
      if (dirResponse.data) {
        setCurrentDir(dirResponse.data);
        console.log('Current directory set to:', dirResponse.data);
      } else {
        console.warn('Unexpected current directory response format:', dirResponse);
      }
      
      // Then fetch files in that directory
      console.log('Fetching files in directory...');
      const response = await fileSystemService.listFiles();
      console.log('List files raw response:', response);
      
      // Clear the timeout since we got a response
      clearTimeout(timeoutId);
      
      // Check if the response has an items property
      if (response.data && response.data.items) {
        console.log('Files list received:', response.data.items);
        setFiles(response.data.items);
      } else {
        // Try to parse the response if it's a string
        try {
          if (typeof response.data === 'string') {
            const parsedData = JSON.parse(response.data);
            if (parsedData.items) {
              console.log('Files list parsed from string:', parsedData.items);
              setFiles(parsedData.items);
              return;
            }
          }
        } catch (parseErr) {
          console.error('Failed to parse response data:', parseErr);
        }
        
        // Handle the case when the response doesn't have the expected format
        console.error('Unexpected response format:', response.data);
        setFiles([]);
        setError('Unexpected response format from server');
      }
    } catch (err) {
      console.error('Error fetching files:', err);
      setError('Failed to load files: ' + (err.response?.data || err.message));
      if (err.response?.status === 401) {
        // Handle unauthorized by redirecting to login
        console.warn('Unauthorized access detected, redirecting to login');
        authService.logout();
        navigate('/');
      }
    } finally {
      setLoading(false);
    }
  }, [navigate]);

  useEffect(() => {
    // Check if user is authenticated before trying to fetch files
    if (!authService.isAuthenticated()) {
      console.log('User not authenticated, redirecting to login');
      navigate('/');
      return;
    }
    
    fetchFiles();
    
    // Set up initial theme based on local storage
    const savedTheme = localStorage.getItem('darkMode');
    if (savedTheme === 'true') {
      setDarkMode(true);
      document.body.classList.add('dark-mode');
    }
  }, [fetchFiles, navigate]);

  // Separate useEffect for welcome notification to prevent it from showing on every render
  useEffect(() => {
    // Show welcome notification only on initial load
    const hasShownWelcome = sessionStorage.getItem('welcomeShown');
    if (!hasShownWelcome) {
      setTimeout(() => {
        setSuccessNotification("Welcome to File System Simulator!");
        setTimeout(() => setSuccessNotification(null), 5000);
        sessionStorage.setItem('welcomeShown', 'true');
      }, 1000);
    }
  }, []);

  const handleStartVoiceCommand = async () => {
    try {
      setError('');
      
      // If already listening, stop the voice command
      if (voiceActive) {
        console.log('Stopping voice command...');
        
        // Call the stop endpoint
        try {
          await fileSystemService.stopVoiceCommand();
          console.log('Voice command stopped via API');
        } catch (err) {
          console.error('Error stopping voice command:', err);
        }
        
        setVoiceActive(false);
        setVoiceText('Voice command stopped');
        
        // Clear any existing polling interval
        if (recognitionInterval) {
          console.log('Clearing polling interval');
          clearInterval(recognitionInterval);
          setRecognitionInterval(null);
        }
        
        // Clear voice text after a delay
        setTimeout(() => {
          setVoiceText('');
        }, 2000);
        
        return;
      }
      
      // Start new voice command session
      setVoiceActive(true);
      setVoiceText('Listening...');
      
      // Start voice command on backend
      console.log('Initiating voice command on backend...');
      const response = await fileSystemService.startVoiceCommand();
      console.log('Voice command response:', response);
      
      // Poll for voice recognition updates
      console.log('Setting up polling for voice status updates...');
      const interval = setInterval(async () => {
        try {
          // Only continue polling if still in voice active mode
          if (!voiceActive) {
            console.log('Voice recognition no longer active, stopping polling');
            clearInterval(interval);
            setRecognitionInterval(null);
            return;
          }
          
          console.log('Polling for voice status...');
          const voiceStatusResponse = await fileSystemService.getVoiceStatus();
          console.log('Voice status response:', voiceStatusResponse.data);
          
          if (voiceStatusResponse.data && voiceStatusResponse.data.text) {
            const currentStatus = voiceStatusResponse.data.text;
            
            // Debug the exact status received
            console.log('RECEIVED STATUS TEXT:', JSON.stringify(currentStatus));
            
            // PRIORITY 1: Check for special formatted messages first (exact match format)
            if (currentStatus.startsWith('FOLDER_CREATED_') || 
                currentStatus.startsWith('FILE_CREATED_') ||
                currentStatus.startsWith('RENAME_SUCCESS_')) {
              
              let actionType = 'unknown';
              let itemName = '';
              let displayMessage = '';
              
              if (currentStatus.startsWith('FOLDER_CREATED_')) {
                actionType = 'folder-create';
                itemName = currentStatus.replace('FOLDER_CREATED_', '');
                displayMessage = `Folder "${itemName}" created successfully!`;
              } else if (currentStatus.startsWith('FILE_CREATED_')) {
                actionType = 'file-create';
                itemName = currentStatus.replace('FILE_CREATED_', '');
                displayMessage = `File "${itemName}" created successfully!`;
              } else if (currentStatus.startsWith('RENAME_SUCCESS_')) {
                actionType = 'rename';
                // Format is RENAME_SUCCESS_oldname_TO_newname
                const renameParts = currentStatus.replace('RENAME_SUCCESS_', '').split('_TO_');
                if (renameParts.length === 2) {
                  displayMessage = `"${renameParts[0]}" renamed to "${renameParts[1]}" successfully!`;
                } else {
                  displayMessage = "Item renamed successfully!";
                }
              }
              
              // Use actionType for analytics or logging purposes
              console.log('SUCCESS DETECTED - SPECIAL FORMAT:', displayMessage, 'Action type:', actionType);
              
              // Force refresh files
              await fetchFiles();
              
              // Set success notification with formatted message
              setSuccessNotification(displayMessage);
              setTimeout(() => setSuccessNotification(null), 6000);
            }
            // PRIORITY 2: Standard format success messages
            else if (currentStatus.includes('SUCCESS') || 
                currentStatus.includes('success') ||
                currentStatus.includes('Folder created') || 
                currentStatus.includes('File created')) {
              
              console.log('SUCCESS DETECTED - STANDARD FORMAT:', currentStatus);
              
              // Force refresh files
              await fetchFiles();
              
              // Set success notification with the exact message
              setSuccessNotification(currentStatus);
              setTimeout(() => setSuccessNotification(null), 6000);
            }
            
            // Always update the voice text display
            setVoiceText(currentStatus);
            
            // If the command is completed, restart voice recognition
            if (voiceStatusResponse.data.completed) {
              console.log('Voice command completed, starting a new listening session');
              await fileSystemService.startVoiceCommand();
            }
          }
        } catch (err) {
          console.error('Error getting voice status:', err);
        }
      }, 300); // Poll frequently for better responsiveness
      
      setRecognitionInterval(interval);
      
    } catch (err) {
      console.error('Failed to start voice command:', err);
      setError('Failed to start voice command');
      setVoiceActive(false);
      setVoiceText('');
    }
  };

  // Clean up interval on component unmount
  useEffect(() => {
    return () => {
      if (recognitionInterval) {
        clearInterval(recognitionInterval);
      }
    };
  }, [recognitionInterval]);

  const handleNavigate = async (path) => {
    try {
      console.log('Navigating to:', path);
      const response = await fileSystemService.changeDirectory(path);
      console.log('Navigation response:', response);
      
      // Reset file state
      setSelectedFile(null);
      setFileContent('');
      setEditMode(false);
      
      // Refresh the file list
      await fetchFiles();
    } catch (err) {
      console.error(`Failed to navigate to ${path}:`, err);
      setError(`Failed to navigate to ${path}: ${err.response?.data || err.message}`);
    }
  };

  const handleNavigateUp = async () => {
    try {
      console.log('Navigating up...');
      const response = await fileSystemService.moveUpDirectory();
      console.log('Navigate up response:', response);
      
      // Reset file state
      setSelectedFile(null);
      setFileContent('');
      setEditMode(false);
      
      // Refresh the file list
      await fetchFiles();
    } catch (err) {
      console.error('Failed to navigate up:', err);
      setError(`Failed to navigate up: ${err.response?.data || err.message}`);
    }
  };

  const handleCreateFolder = async () => {
    if (!newFolderName.trim()) {
      setError('Folder name cannot be empty');
      return;
    }
    
    try {
      setError('');
      console.log('Creating folder:', newFolderName);
      const response = await fileSystemService.createDirectory(newFolderName);
      console.log('Create folder response:', response);
      setNewFolderName('');
      setShowCreateFolder(false);
      
      // Refresh the file list
      await fetchFiles();
    } catch (err) {
      console.error('Failed to create folder:', err);
      setError('Failed to create folder: ' + (err.response?.data || err.message));
    }
  };

  const handleCreateFile = async () => {
    if (!newFileName.trim()) {
      setError('File name cannot be empty');
      return;
    }
    
    try {
      setError('');
      console.log('Creating file:', newFileName, 'with content:', newFileContent);
      const response = await fileSystemService.createFile(newFileName, newFileContent);
      console.log('Create file response:', response);
      setNewFileName('');
      setNewFileContent('');
      setShowCreateFile(false);
      
      // Refresh the file list
      await fetchFiles();
    } catch (err) {
      console.error('Failed to create file:', err);
      setError('Failed to create file: ' + (err.response?.data || err.message));
    }
  };

  const handleDeleteItem = async (name, isDirectory) => {
    try {
      if (isDirectory) {
        console.log('Deleting directory:', name);
        const response = await fileSystemService.deleteDirectory(name);
        console.log('Delete directory response:', response);
      } else {
        console.log('Deleting file:', name);
        const response = await fileSystemService.deleteFile(name);
        console.log('Delete file response:', response);
      }
      
      // Refresh the file list
      await fetchFiles();
      
      if (selectedFile === name) {
        setSelectedFile(null);
        setFileContent('');
        setEditMode(false);
      }
    } catch (err) {
      console.error(`Failed to delete ${isDirectory ? 'folder' : 'file'}:`, err);
      setError(`Failed to delete ${isDirectory ? 'folder' : 'file'}: ${err.response?.data || err.message}`);
    }
  };

  const handleFileClick = async (fileName) => {
    try {
      setError('');
      console.log('Reading file:', fileName);
      setSelectedFile(fileName);
      setEditMode(false);
      
      const response = await fileSystemService.readFile(fileName);
      console.log('Read file response status:', response.status);
      setFileContent(response.data);
    } catch (err) {
      console.error('Failed to read file:', err);
      setError('Failed to read file: ' + (err.response?.data || err.message));
      setFileContent('');
    }
  };

  const handleSaveFile = async () => {
    try {
      setError('');
      console.log('Saving file:', selectedFile);
      const response = await fileSystemService.editFile(selectedFile, fileContent);
      console.log('Save file response:', response);
      setEditMode(false);
      
      // Optionally refresh the file list after saving
      await fetchFiles();
    } catch (err) {
      console.error('Failed to save file:', err);
      setError('Failed to save file: ' + (err.response?.data || err.message));
    }
  };

  const toggleDarkMode = () => {
    setDarkMode(prevMode => {
      const newMode = !prevMode;
      localStorage.setItem('darkMode', newMode);
      
      if (newMode) {
        document.body.classList.add('dark-mode');
      } else {
        document.body.classList.remove('dark-mode');
      }
      
      return newMode;
    });
  };

  // Enhanced function to specifically handle voice commands
  // This function is defined for future expansion of voice command features
  // and can be called from handleStartVoiceCommand as needed
  // eslint-disable-next-line no-unused-vars
  const processVoiceCommandStatus = async (status) => {
    console.log('Processing voice command status:', status);
    
    // Explicit check for SUCCESS messages (case insensitive)
    if (status.includes('SUCCESS') || status.includes('success')) {
      console.log('SUCCESS DETECTED - Setting notification:', status);
      setSuccessNotification(status);
      setTimeout(() => setSuccessNotification(null), 8000); // longer display
      await fetchFiles();
      return true;
    }
    
    // Check for creation/action commands
    if (status.includes('created') || 
        status.includes('deleted') || 
        status.includes('opened') ||
        status.includes('navigated')) {
      
      console.log('ACTION DETECTED IN VOICE COMMAND:', status);
      
      // Show success notification
      setSuccessNotification(status);
      setTimeout(() => setSuccessNotification(null), 5000);
      
      // Refresh files to show changes
      await fetchFiles();
      return true;
    }
    return false;
  };

  // Add a more robust useEffect to handle success notifications
  useEffect(() => {
    if (successNotification) {
      console.log('SUCCESS NOTIFICATION SET TO:', successNotification);
      
      // Keep only one notification active at a time - clear any existing timeouts
      if (window.successNotificationTimeout) {
        console.log('Clearing existing timeout');
        clearTimeout(window.successNotificationTimeout);
      }
      
      // Store the new timeout for later reference
      window.successNotificationTimeout = setTimeout(() => {
        console.log('Success notification timeout fired, clearing notification');
        setSuccessNotification(null);
      }, 8000);  // Increased to 8 seconds for better visibility
      
      // Check if the notification element exists
      setTimeout(() => {
        const notificationElement = document.querySelector('.success-notification');
        if (notificationElement) {
          console.log('Success notification found in DOM:', notificationElement);
        } else {
          console.error('SUCCESS NOTIFICATION NOT FOUND IN DOM!');
        }
      }, 100);  // Short delay to check after render
    } else {
      console.log('Success notification cleared');
    }
    
    // Cleanup on unmount
    return () => {
      if (window.successNotificationTimeout) {
        clearTimeout(window.successNotificationTimeout);
      }
    };
  }, [successNotification]);

  // Set notification function for testing from console
  useEffect(() => {
    // Expose a function to test setting notifications from the console
    window.testNotification = (message) => {
      console.log(`Setting test notification: ${message}`);
      setSuccessNotification(message);
      setTimeout(() => setSuccessNotification(null), 5000);
    };
    
    return () => {
      delete window.testNotification;
    };
  }, []);

  const handleRenameItem = async () => {
    if (!itemToRename || !newItemName.trim()) {
      return;
    }
    
    try {
      setError('');
      console.log(`Renaming ${itemToRename} to ${newItemName}`);
      
      // Call the API to rename the item
      const response = await fileSystemService.renameItem(itemToRename, newItemName);
      console.log('Rename response:', response);
      
      // Set success notification
      setSuccessNotification(`Renamed ${itemToRename} to ${newItemName} successfully`);
      setTimeout(() => setSuccessNotification(null), 5000);
      
      // Reset state
      setItemToRename(null);
      setNewItemName('');
      
      // Refresh the file list
      await fetchFiles();
    } catch (err) {
      console.error('Error renaming item:', err);
      setError('Failed to rename item: ' + (err.response?.data || err.message));
    }
  };

  return (
    <>
      {/* Success notification positioned outside the main container for maximum visibility */}
      {successNotification && (
        <div className="success-notification">
          <FaCheck /> {successNotification}
        </div>
      )}
      
      <div className={`file-explorer ${darkMode ? 'dark-mode' : ''}`}>
        <div className="explorer-header">
          <h2>File System Explorer</h2>
          <div className="header-actions">
            <button 
              className="theme-toggle-btn" 
              onClick={toggleDarkMode} 
              title={darkMode ? "Switch to Light Mode" : "Switch to Dark Mode"}
            >
              {darkMode ? <FaSun /> : <FaMoon />}
            </button>
            
            <button 
              onClick={handleStartVoiceCommand} 
              className={`action-btn ${voiceActive ? 'active' : ''}`}
              title={voiceActive ? "Stop Voice Command" : "Start Voice Command"}
            >
              <FaMicrophone /> {voiceActive ? 'Stop' : 'Voice'}
            </button>
            
            <button 
              onClick={handleLogout} 
              className="action-btn"
              title="Logout"
            >
              Logout
            </button>
          </div>
        </div>
        
        {error && <div className="error-message">{error}</div>}
        
        {voiceText && (
          <div className="voice-text-container">
            <div className={`voice-text ${
              voiceText.includes("Listening") ? "listening" :
              voiceText.includes("Recognized:") ? "recognized" :
              voiceText.includes("ERROR") || voiceText.includes("Failed") || voiceText.includes("Error") ? "error" :
              voiceText.includes("SUCCESS") || voiceText.includes("created") || voiceText.includes("Navigated") || voiceText.includes("Deleted") ? "success" :
              voiceText.includes("Processing") ? "processing" :
              ""
            }`}>
              <FaMicrophone className="voice-icon" /> {voiceText}
            </div>
          </div>
        )}
        
        <div className="path-navigation">
          <div className="current-path">
            <strong>Current Path:</strong> {currentDir}
          </div>
          <button onClick={handleNavigateUp} className="nav-btn">
            <FaArrowUp /> Up
          </button>
        </div>
        
        <div className="actions">
          <button onClick={() => setShowCreateFolder(true)} className="action-btn">
            <FaFolder /> New Folder
          </button>
          <button onClick={() => setShowCreateFile(true)} className="action-btn">
            <FaFile /> New File
          </button>
        </div>
        
        {showCreateFolder && (
          <div className="modal">
            <div className="modal-content">
              <h3>Create New Folder</h3>
              <input
                type="text"
                placeholder="Folder Name"
                value={newFolderName}
                onChange={(e) => setNewFolderName(e.target.value)}
              />
              <div className="modal-actions">
                <button onClick={handleCreateFolder}>Create</button>
                <button onClick={() => setShowCreateFolder(false)}>Cancel</button>
              </div>
            </div>
          </div>
        )}
        
        {showCreateFile && (
          <div className="modal">
            <div className="modal-content">
              <h3>Create New File</h3>
              <input
                type="text"
                placeholder="File Name"
                value={newFileName}
                onChange={(e) => setNewFileName(e.target.value)}
              />
              <textarea
                placeholder="File Content"
                value={newFileContent}
                onChange={(e) => setNewFileContent(e.target.value)}
              />
              <div className="modal-actions">
                <button onClick={handleCreateFile}>Create</button>
                <button onClick={() => setShowCreateFile(false)}>Cancel</button>
              </div>
            </div>
          </div>
        )}
        
        {itemToRename && (
          <div className="modal-backdrop">
            <div className="modal-content">
              <h3>
                {/* Determine if it's a file or folder based on the file list */}
                {files.find(item => item.name === itemToRename)?.type === 'directory' 
                  ? <><FaFolder className="modal-icon folder" /> Rename Folder</>
                  : <><FaFile className="modal-icon file" /> Rename File</>
                }
              </h3>
              <div className="modal-form">
                <div className="form-group">
                  <label>Current Name:</label>
                  <div className="current-name">{itemToRename}</div>
                </div>
                <div className="form-group">
                  <label>New Name:</label>
                  <input
                    type="text"
                    value={newItemName}
                    onChange={(e) => setNewItemName(e.target.value)}
                    placeholder="Enter new name"
                    autoFocus
                  />
                </div>
              </div>
              <div className="modal-actions">
                <button 
                  onClick={handleRenameItem}
                  disabled={!newItemName.trim() || newItemName === itemToRename}
                >
                  Rename
                </button>
                <button onClick={() => {
                  setItemToRename(null);
                  setNewItemName('');
                }}>Cancel</button>
              </div>
            </div>
          </div>
        )}
        
        <div className="explorer-container">
          <div className="file-list">
            <h3>Files and Folders</h3>
            {loading ? (
              <div className="loading">Loading...</div>
            ) : (
              <ul>
                {files.length === 0 ? (
                  <li className="empty-message">No files or folders found</li>
                ) : (
                  files.map((item) => (
                    <li key={item.name} className={`file-item ${selectedFile === item.name ? 'selected' : ''}`}>
                      <div 
                        className="file-name"
                        onClick={() => item.type === 'directory' 
                          ? handleNavigate(item.name) 
                          : handleFileClick(item.name)
                        }
                      >
                        {item.type === 'directory' ? <FaFolder className="icon folder" /> : <FaFile className="icon file" />}
                        {item.name}
                      </div>
                      <div className="file-actions">
                        <button 
                          onClick={(e) => {
                            e.stopPropagation();
                            setItemToRename(item.name);
                            setNewItemName(item.name);
                          }}
                          className="file-action-btn rename"
                          title="Rename"
                        >
                          <FaPen />
                        </button>
                        <button 
                          onClick={(e) => {
                            e.stopPropagation();
                            handleDeleteItem(item.name, item.type === 'directory');
                          }}
                          className="file-action-btn delete"
                          title="Delete"
                        >
                          <FaTrash />
                        </button>
                      </div>
                    </li>
                  ))
                )}
              </ul>
            )}
          </div>
          
          <div className="file-preview">
            <h3>
              {selectedFile ? (
                <>
                  <span>File: {selectedFile}</span>
                  <div className="file-preview-actions">
                    {editMode ? (
                      <button onClick={handleSaveFile} className="edit-btn save">
                        <FaSave /> Save
                      </button>
                    ) : (
                      <button onClick={() => setEditMode(true)} className="edit-btn">
                        <FaEdit /> Edit
                      </button>
                    )}
                  </div>
                </>
              ) : (
                'Select a file to preview'
              )}
            </h3>
            {selectedFile && (
              editMode ? (
                <textarea
                  className="file-content-editor"
                  value={fileContent}
                  onChange={(e) => setFileContent(e.target.value)}
                />
              ) : (
                <pre className="file-content">{fileContent}</pre>
              )
            )}
          </div>
        </div>
      </div>
    </>
  );
};

export default FileExplorer; 