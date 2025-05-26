import React, { useState, useEffect, useRef } from 'react';
import './Terminal.css';
import { fileSystemService } from '../api';

const Terminal = ({ currentPath, onCommandExecuted }) => {
    const [commandHistory, setCommandHistory] = useState([]);
    const [currentCommand, setCurrentCommand] = useState('');
    const [historyIndex, setHistoryIndex] = useState(-1);
    const [outputHistory, setOutputHistory] = useState([{ text: `Welcome to File System Terminal\nCurrent directory: ${currentPath}\n`, type: 'system' }]);
    const terminalRef = useRef(null);
    const inputRef = useRef(null);

    useEffect(() => {
        if (terminalRef.current) {
            terminalRef.current.scrollTop = terminalRef.current.scrollHeight;
        }
        if (inputRef.current) {
            inputRef.current.focus();
        }
    }, [outputHistory]);

    // Update output when currentPath changes
    useEffect(() => {
        if (currentPath) {
            setOutputHistory(prev => [...prev, { text: `Current directory: ${currentPath}`, type: 'system' }]);
        }
    }, [currentPath]);

    const executeCommand = async (command) => {
        const trimmedCommand = command.trim();
        if (!trimmedCommand) return;

        // Add command to history
        setCommandHistory(prev => [...prev, trimmedCommand]);
        setOutputHistory(prev => [...prev, { text: `$ ${trimmedCommand}`, type: 'command' }]);

        // Get the current token
        const token = localStorage.getItem('token');
        if (!token) {
            setOutputHistory(prev => [...prev, { text: 'Error: Not authenticated. Please log in.', type: 'error' }]);
            return;
        }

        // Parse command
        const [cmd, ...args] = trimmedCommand.split(' ');

        try {
            let response;
            let shouldRefresh = false;

            switch (cmd.toLowerCase()) {
                case 'ls':
                    response = await fileSystemService.listFiles();
                    shouldRefresh = true;
                    break;
                case 'cd':
                    if (!args[0]) {
                        setOutputHistory(prev => [...prev, { text: 'Usage: cd <directory>', type: 'error' }]);
                        return;
                    }
                    response = await fileSystemService.changeDirectory(args[0]);
                    shouldRefresh = true;
                    break;
                case 'mkdir':
                    if (!args[0]) {
                        setOutputHistory(prev => [...prev, { text: 'Usage: mkdir <directory>', type: 'error' }]);
                        return;
                    }
                    response = await fileSystemService.createDirectory(args[0]);
                    shouldRefresh = true;
                    break;
                case 'touch':
                case 'create':
                    if (!args[0]) {
                        setOutputHistory(prev => [...prev, { text: 'Usage: create <filename>', type: 'error' }]);
                        return;
                    }
                    response = await fileSystemService.createFile(args[0], '');
                    shouldRefresh = true;
                    break;
                case 'rm':
                    if (!args[0]) {
                        setOutputHistory(prev => [...prev, { text: 'Usage: rm <filename>', type: 'error' }]);
                        return;
                    }
                    response = await fileSystemService.deleteFile(args[0]);
                    shouldRefresh = true;
                    break;
                case 'rmdir':
                    if (!args[0]) {
                        setOutputHistory(prev => [...prev, { text: 'Usage: rmdir <directory>', type: 'error' }]);
                        return;
                    }
                    response = await fileSystemService.deleteDirectory(args[0]);
                    shouldRefresh = true;
                    break;
                case 'pwd':
                    response = await fileSystemService.getCurrentDirectory();
                    break;
                case 'clear':
                    setOutputHistory([{ text: `Welcome to File System Terminal\nCurrent directory: ${currentPath}\n`, type: 'system' }]);
                    return;
                case 'help':
                    const helpText = `
Available commands:
  ls              - List directory contents
  cd <dir>        - Change directory
  mkdir <dir>     - Create directory
  touch/create <file> - Create file
  rm <file>       - Remove file
  rmdir <dir>     - Remove directory
  pwd             - Print working directory
  clear           - Clear terminal
  help            - Show this help message
`;
                    setOutputHistory(prev => [...prev, { text: helpText, type: 'output' }]);
                    return;
                default:
                    setOutputHistory(prev => [...prev, { text: `Command not found: ${cmd}. Type 'help' for available commands.`, type: 'error' }]);
                    return;
            }

            if (response && response.data) {
                setOutputHistory(prev => [...prev, { 
                    text: typeof response.data === 'string' ? response.data : JSON.stringify(response.data, null, 2), 
                    type: 'output' 
                }]);

                // Call the refresh callback if the command was successful and should trigger a refresh
                if (shouldRefresh && onCommandExecuted) {
                    onCommandExecuted();
                }
            }

        } catch (error) {
            setOutputHistory(prev => [...prev, { text: `Error: ${error.message}`, type: 'error' }]);
        }
    };

    const handleKeyDown = (e) => {
        if (e.key === 'Enter') {
            executeCommand(currentCommand);
            setCurrentCommand('');
            setHistoryIndex(-1);
        } else if (e.key === 'ArrowUp') {
            e.preventDefault();
            if (commandHistory.length > 0) {
                const newIndex = historyIndex + 1;
                if (newIndex < commandHistory.length) {
                    setHistoryIndex(newIndex);
                    setCurrentCommand(commandHistory[commandHistory.length - 1 - newIndex]);
                }
            }
        } else if (e.key === 'ArrowDown') {
            e.preventDefault();
            if (historyIndex > 0) {
                const newIndex = historyIndex - 1;
                setHistoryIndex(newIndex);
                setCurrentCommand(commandHistory[commandHistory.length - 1 - newIndex]);
            } else if (historyIndex === 0) {
                setHistoryIndex(-1);
                setCurrentCommand('');
            }
        }
    };

    return (
        <div className="terminal-container">
            <div className="terminal-header">
                <span>Terminal</span>
                <button className="close-button" onClick={() => document.querySelector('.terminal-wrapper').style.display = 'none'}>×</button>
            </div>
            <div className="terminal-window" ref={terminalRef}>
                {outputHistory.map((output, index) => (
                    <div key={index} className={`terminal-line ${output.type}`}>
                        {output.text}
                    </div>
                ))}
                <div className="terminal-input-line">
                    <span className="prompt">$</span>
                    <input
                        ref={inputRef}
                        type="text"
                        value={currentCommand}
                        onChange={(e) => setCurrentCommand(e.target.value)}
                        onKeyDown={handleKeyDown}
                        className="terminal-input"
                        spellCheck="false"
                        autoFocus
                    />
                </div>
            </div>
        </div>
    );
};

export default Terminal; 