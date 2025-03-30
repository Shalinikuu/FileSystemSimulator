import speech_recognition as sr
import os
import sys
import time
import requests
import pyttsx3  # Text-to-speech library

# Initialize text-to-speech engine
engine = pyttsx3.init()

# Define the status file path (one directory up from this script)
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
status_file = os.path.join(parent_dir, "voice_status.txt")
# Also define the file in the location where the C++ program is looking
backup_status_file = os.path.join(parent_dir, "backend", "build", "../voice_status.txt")

print(f"Script directory: {script_dir}")
print(f"Parent directory: {parent_dir}")
print(f"Status file path: {status_file}")
print(f"Backup status file path: {backup_status_file}")

# API base URL
API_BASE_URL = "http://localhost:8080"

# Function to speak text aloud and update status
def speak_and_update(text):
    print(f"Speaking: {text}")
    update_status(text)
    engine.say(text)
    engine.runAndWait()
    
# Update voice recognition status
def update_status(text):
    try:
        # Update the main status file
        with open(status_file, "w") as f:
            f.write(text)
        print(f"Status updated to: '{text}' in main file")
        
        # Also update the backup file location
        try:
            os.makedirs(os.path.dirname(backup_status_file), exist_ok=True)
            with open(backup_status_file, "w") as f:
                f.write(text)
            print(f"Status also updated in backup file")
        except Exception as e:
            print(f"Could not update backup file: {e}")
    except Exception as e:
        print(f"Error updating status file: {str(e)}")

# Get the token from token.txt
def get_token():
    # Try multiple possible locations for token.txt
    token_paths = [
        os.path.join(script_dir, "build", "token.txt"),  # backend/build/token.txt
        os.path.join(parent_dir, "backend", "build", "token.txt"),  # FileSystemSimulator/backend/build/token.txt
        os.path.join(script_dir, "token.txt"),  # backend/token.txt
        os.path.join(parent_dir, "token.txt")  # FileSystemSimulator/token.txt
    ]
    
    # Try each path
    for token_file in token_paths:
        print(f"Looking for token at: {token_file}")
        if os.path.exists(token_file):
            with open(token_file, "r") as f:
                token = f.read().strip()
                print(f"Token found: {token[:10]}...")
                return token
    
    print("Token file not found at any expected location")
    # Fallback: Use a test token just to demonstrate functionality
    test_token = "test_token_for_demonstration"
    print(f"Using test token: {test_token}")
    return test_token

# API call to create a folder
def create_folder(folder_name):
    token = get_token()
    if not token:
        print("No authentication token found")
        return False
    
    url = f"{API_BASE_URL}/mkdir/{folder_name}"
    headers = {"Authorization": f"Bearer {token}"}
    
    try:
        response = requests.post(url, headers=headers)
        print(f"Create folder API response: {response.status_code} - {response.text}")
        return response.status_code == 200
    except Exception as e:
        print(f"Error creating folder: {e}")
        return False

# API call to create a file
def create_file(file_name, content=""):
    token = get_token()
    if not token:
        print("No authentication token found")
        return False
    
    url = f"{API_BASE_URL}/create-file/{file_name}"
    headers = {"Authorization": f"Bearer {token}"}
    
    try:
        response = requests.post(url, headers=headers, data=content)
        print(f"Create file API response: {response.status_code} - {response.text}")
        return response.status_code == 200
    except Exception as e:
        print(f"Error creating file: {e}")
        return False

# API call to delete an item
def delete_item(item_name):
    token = get_token()
    if not token:
        print("No authentication token found")
        return False
    
    # Try to delete as file first
    try:
        url = f"{API_BASE_URL}/delete-file/{item_name}"
        headers = {"Authorization": f"Bearer {token}"}
        response = requests.delete(url, headers=headers)
        print(f"Delete file API response: {response.status_code} - {response.text}")
        if response.status_code == 200:
            return True
    except Exception as e:
        print(f"Error deleting file: {e}")
    
    # If file delete fails, try as folder
    try:
        url = f"{API_BASE_URL}/rmdir/{item_name}"
        headers = {"Authorization": f"Bearer {token}"}
        response = requests.delete(url, headers=headers)
        print(f"Delete folder API response: {response.status_code} - {response.text}")
        return response.status_code == 200
    except Exception as e:
        print(f"Error deleting folder: {e}")
        return False

# API call to navigate to a folder
def navigate_to_folder(folder_name):
    token = get_token()
    if not token:
        print("No authentication token found")
        return False
    
    url = f"{API_BASE_URL}/cd/{folder_name}"
    headers = {"Authorization": f"Bearer {token}"}
    
    try:
        response = requests.post(url, headers=headers)
        print(f"Navigate API response: {response.status_code} - {response.text}")
        return response.status_code == 200
    except Exception as e:
        print(f"Error navigating to folder: {e}")
        return False

# Function to listen for file content
def listen_for_file_content():
    r = sr.Recognizer()
    content = ""
    
    speak_and_update("What content would you like to add to this file? Please speak now.")
    
    with sr.Microphone() as source:
        print("Listening for file content...")
        # Adjust for ambient noise
        r.adjust_for_ambient_noise(source)
        
        try:
            # Listen for speech with longer timeout for content
            audio = r.listen(source, timeout=10, phrase_time_limit=10)
            
            update_status("Processing your file content...")
            
            try:
                # Recognize speech using Google Speech Recognition
                content = r.recognize_google(audio)
                update_status(f"Content recognized: {content}")
                print(f"File content recognized: {content}")
                return content
                
            except sr.UnknownValueError:
                speak_and_update("Could not understand the file content. Using empty file.")
                return ""
                
            except sr.RequestError as e:
                speak_and_update(f"Could not process file content; {str(e)}")
                return ""
                
        except sr.WaitTimeoutError:
            speak_and_update("Did not hear any content. Using empty file.")
            return ""
            
        except Exception as e:
            speak_and_update(f"Error while listening for content: {str(e)}")
            return ""

# Function to process a single voice command
def process_voice_command():
    r = sr.Recognizer()
    
    # Use microphone as source
    with sr.Microphone() as source:
        print("Listening for voice commands...")
        # Adjust for ambient noise
        r.adjust_for_ambient_noise(source)
        
        # Let the user know we're actively listening
        speak_and_update("Listening. Please speak your command now.")
        print("Ready for voice input")
        
        try:
            # Listen for speech with shorter timeout for better responsiveness
            audio = r.listen(source, timeout=8, phrase_time_limit=5)
            
            update_status("Processing your command...")
            
            try:
                # Recognize speech using Google Speech Recognition
                voice_command = r.recognize_google(audio).lower()
                update_status(f"Recognized: {voice_command}")
                print(f"Voice command recognized: {voice_command}")
                
                # Process the command
                if "create folder" in voice_command or "create a folder" in voice_command or "make folder" in voice_command or "make a folder" in voice_command or "make directory" in voice_command:
                    folder_name = voice_command
                    for phrase in ["create folder", "create a folder", "make folder", "make a folder", "make directory"]:
                        folder_name = folder_name.replace(phrase, "")
                    folder_name = folder_name.strip()
                    
                    speak_and_update(f"Creating folder: {folder_name}")
                    time.sleep(1)  # Small delay for UI feedback
                    if create_folder(folder_name):
                        # Use a crystal clear success message format for the frontend to detect
                        success_message = f"FOLDER_CREATED_{folder_name}"
                        update_status(success_message)
                        speak_and_update(f"Success! Folder {folder_name} has been created.")
                        time.sleep(1)
                    else:
                        error_message = f"ERROR_CREATING_FOLDER_{folder_name}"
                        update_status(error_message)
                        speak_and_update(f"Sorry, I couldn't create the folder {folder_name}.")
                        time.sleep(1)
                    
                elif "create file" in voice_command:
                    file_name = voice_command.replace("create file", "").strip()
                    speak_and_update(f"Creating file: {file_name}")
                    
                    # Ask for file content
                    file_content = listen_for_file_content()
                    
                    # Create the file with the content
                    if create_file(file_name, file_content):
                        success_message = f"FILE_CREATED_{file_name}"
                        update_status(success_message)
                        speak_and_update(f"Success! File {file_name} has been created with your content.")
                        time.sleep(1)
                    else:
                        error_message = f"ERROR_CREATING_FILE_{file_name}"
                        update_status(error_message)
                        speak_and_update(f"Sorry, I couldn't create the file {file_name}.")
                        time.sleep(1)
                    
                elif "open folder" in voice_command or "go to folder" in voice_command or "navigate to" in voice_command:
                    folder_name = voice_command
                    for phrase in ["open folder", "go to folder", "navigate to"]:
                        folder_name = folder_name.replace(phrase, "")
                    folder_name = folder_name.strip()
                    
                    speak_and_update(f"Opening folder: {folder_name}")
                    if navigate_to_folder(folder_name):
                        speak_and_update(f"Navigated to folder {folder_name}.")
                        time.sleep(1)
                    else:
                        speak_and_update(f"Failed to navigate to folder {folder_name}.")
                        time.sleep(1)
                    
                elif "delete" in voice_command or "remove" in voice_command:
                    item_name = voice_command
                    for phrase in ["delete", "remove"]:
                        item_name = item_name.replace(phrase, "")
                    item_name = item_name.strip()
                    
                    speak_and_update(f"Deleting: {item_name}")
                    if delete_item(item_name):
                        speak_and_update(f"Successfully deleted {item_name}.")
                        time.sleep(1)
                    else:
                        speak_and_update(f"Failed to delete {item_name}.")
                        time.sleep(1)
                    
                elif "rename" in voice_command:
                    # Match patterns like "rename X to Y" or just "rename X Y"
                    parts = voice_command.replace("rename", "").strip()
                    old_name = ""
                    new_name = ""
                    
                    # Split by "to" if it exists
                    if " to " in parts:
                        name_parts = parts.split(" to ")
                        if len(name_parts) == 2:
                            old_name = name_parts[0].strip()
                            new_name = name_parts[1].strip()
                    else:
                        # Try to split by space and take first and last parts
                        words = parts.split()
                        if len(words) >= 2:
                            old_name = words[0]
                            new_name = words[-1]
                    
                    # Check if both names are valid
                    if old_name and new_name:
                        speak_and_update(f"Renaming {old_name} to {new_name}")
                        if rename_item(old_name, new_name):
                            success_message = f"RENAME_SUCCESS_{old_name}_TO_{new_name}"
                            update_status(success_message)
                            speak_and_update(f"Success! {old_name} has been renamed to {new_name}.")
                            time.sleep(1)
                        else:
                            error_message = f"ERROR_RENAMING_{old_name}_TO_{new_name}"
                            update_status(error_message)
                            speak_and_update(f"Sorry, I couldn't rename {old_name} to {new_name}. Make sure the item exists and the new name is valid.")
                            time.sleep(1)
                    else:
                        speak_and_update("Sorry, I couldn't understand the rename command. Please use the format 'rename X to Y'.")
                        time.sleep(1)
                    
                else:
                    speak_and_update(f"Command not recognized: '{voice_command}'. Please try again.")
                    time.sleep(1)
                
                return True
                    
            except sr.UnknownValueError:
                speak_and_update("Voice command not understood. Please try again.")
                print("Voice command not understood")
                time.sleep(1)
                return True
                
            except sr.RequestError as e:
                speak_and_update(f"Speech service error; {str(e)}")
                print(f"Could not request results; {str(e)}")
                time.sleep(1)
                return False
                
        except sr.WaitTimeoutError:
            print("Timeout occurred while waiting for phrase")
            speak_and_update("I didn't hear anything. Please try again.")
            return True
            
        except Exception as e:
            print(f"Error during listening: {e}")
            speak_and_update(f"Error during listening: {str(e)}")
            time.sleep(1)
            return False

# Add a rename item API function
def rename_item(old_name, new_name):
    token = get_token()
    if not token:
        print("No authentication token found")
        return False
    
    # Try to rename as file first
    try:
        # For files, we need to read the file first, then create a new one, then delete the old one
        file_content = ""
        try:
            url = f"{API_BASE_URL}/read-file/{old_name}"
            headers = {"Authorization": f"Bearer {token}"}
            response = requests.get(url, headers=headers)
            if response.status_code == 200:
                file_content = response.text
                print(f"File read successful, content length: {len(file_content)}")
        except Exception as e:
            print(f"Error reading file to rename: {e}")
            # Continue with empty content if reading fails
        
        # Create new file with same content
        create_result = create_file(new_name, file_content)
        if create_result:
            # Delete old file
            url = f"{API_BASE_URL}/delete-file/{old_name}"
            headers = {"Authorization": f"Bearer {token}"}
            response = requests.delete(url, headers=headers)
            print(f"Delete old file response: {response.status_code} - {response.text}")
            return response.status_code == 200
    except Exception as e:
        print(f"Error renaming file: {e}")
    
    # If file rename fails, try as folder
    try:
        # For folders, we need to:
        # 1. Create new folder
        folder_result = create_folder(new_name)
        if not folder_result:
            return False
            
        # 2. Get contents of old folder
        url = f"{API_BASE_URL}/ls"
        headers = {"Authorization": f"Bearer {token}"}
        
        # Save current directory
        pwd_response = requests.get(f"{API_BASE_URL}/pwd", headers=headers)
        original_dir = pwd_response.text
        
        # Navigate to the old folder
        cd_response = requests.post(f"{API_BASE_URL}/cd/{old_name}", headers=headers)
        if cd_response.status_code != 200:
            return False
            
        # Get file list
        ls_response = requests.get(url, headers=headers)
        items = []
        if ls_response.status_code == 200:
            try:
                items = ls_response.json().get("items", [])
            except:
                print("Failed to parse folder contents")
                
        # Return to original directory
        requests.post(f"{API_BASE_URL}/cd/{original_dir}", headers=headers)
            
        # For simplicity, we'll just handle empty folders for now
        # A complete implementation would recursively copy all contents
        # But this could get complex with the current API
        
        # 3. Delete old folder (only for empty folders for now)
        if len(items) == 0:
            url = f"{API_BASE_URL}/rmdir/{old_name}"
            response = requests.delete(url, headers=headers)
            return response.status_code == 200
            
        # If folder has contents, inform user
        return False
            
    except Exception as e:
        print(f"Error renaming folder: {e}")
        return False
        
    return False

# Main function for continuous voice recognition
def main():
    try:
        # Start with listening
        update_status("Listening...")
        
        # Continue listening until manually stopped
        while True:
            # Process a single voice command
            success = process_voice_command()
            
            # If failed completely, exit the loop
            if not success:
                break
                
            # Mark command as completed before listening again
            update_status("Command completed")
            time.sleep(1)
            update_status("Listening...")
            
    except KeyboardInterrupt:
        print("Voice recognition stopped by user")
        update_status("Voice recognition stopped")
    except Exception as e:
        print(f"Error in main voice recognition loop: {str(e)}")
        update_status(f"Error: {str(e)}")
    
    print("Voice recognition completed")

if __name__ == "__main__":
    main() 