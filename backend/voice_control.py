import speech_recognition as sr
import pyttsx3
import requests
import re
import threading
import os

BASE_URL = "http://localhost:8080/"
TOKEN_FILE_PATH = "token.txt"

command_map = {
    "create a folder": "mkdir/",
    "remove the folder": "rmdir/",
    "create a file": "create-file/",
    "delete the file": "delete-file/",
    "change directory to": "cd/",
    "go back": "cd/..",
    "show files": "ls",
    "current directory": "pwd",
    "append to file": "append-file/",
    "edit file": "edit-file/",
    "stop listening": "exit"
}

class SpeechEngine:
    def __init__(self):
        self.engine = pyttsx3.init()
        self.lock = threading.Lock()

    def speak(self, text):
        """Convert text to speech in a thread-safe manner."""
        def run_speech():
            with self.lock:
                self.engine.say(text)
                self.engine.runAndWait()

        threading.Thread(target=run_speech, daemon=True).start()

# Global instance of SpeechEngine
speech_engine = SpeechEngine()

def get_auth_token():
    """Retrieve the authentication token from the file."""
    if not os.path.exists(TOKEN_FILE_PATH):
        print(f"❌ Token file not found at {TOKEN_FILE_PATH}!")
        speech_engine.speak("Token not found.")
        return None

    with open(TOKEN_FILE_PATH, "r") as token_file:
        return token_file.read().strip()

def send_request(api_url, method="GET", data=None):
    """Send request to the file system API with authorization."""
    token = get_auth_token()
    if token is None:
        return

    headers = {"Authorization": f"Bearer {token}"}

    try:
        if method == "POST":
            response = requests.post(api_url, headers=headers, data=data)
        elif method == "DELETE":
            response = requests.delete(api_url, headers=headers)
        else:
            response = requests.get(api_url, headers=headers)

        if response.status_code == 200:
            print(f"✅ Success: {response.text}")
            speech_engine.speak("Command executed successfully.")
        else:
            print(f"❌ Error: {response.status_code} - {response.text}")
            speech_engine.speak("There was an error executing the command.")

    except requests.exceptions.RequestException as e:
        print(f"⚠️ Request failed: {e}")
        speech_engine.speak("Failed to connect to the server.")

def normalize_input(spoken_text):
    """Normalize the input by replacing words like 'dot' with '.'."""
    normalized_text = spoken_text.lower()
    normalized_text = normalized_text.replace("dot", ".")  # Replace "dot" with "."
    normalized_text = normalized_text.replace("underscore", "_")  # Replace "underscore" with "_"
    normalized_text = normalized_text.replace("space", " ")  # Replace "space" with actual space
    return normalized_text

def recognize_speech():
    """Recognize speech and execute commands in a loop."""
    recognizer = sr.Recognizer()
    with sr.Microphone() as source:
        recognizer.adjust_for_ambient_noise(source)
        print("\n🎤 Voice command system ready. Say a command...")

    while True:
        with sr.Microphone() as source:
            print("\n🎤 Speak a command:")
            audio = recognizer.listen(source)

        try:
            spoken_text = recognizer.recognize_google(audio)
            print(f"🗣️ You said: {spoken_text}")

            # Normalize the input to handle special characters
            normalized_spoken_text = normalize_input(spoken_text)

            for key in command_map:
                if key in normalized_spoken_text:
                    if key == "stop listening":
                        print("🛑 Stopping voice recognition.")
                        speech_engine.speak("Stopping voice recognition.")
                        return

                    param = normalized_spoken_text.replace(key, "").strip()
                    param = re.sub(r"\s+", "_", param)  # Replace spaces with underscores
                    api_url = BASE_URL + command_map[key] + param if "/" in command_map[key] else BASE_URL + command_map[key]

                    print(f"✅ API Call: {api_url}")
                    speech_engine.speak("Command received. Executing.")

                    if command_map[key] == "create-file/" or command_map[key] == "append-file/" or command_map[key] == "edit-file/":
                        # Ask for file content if creating, appending, or editing a file
                        if command_map[key] == "create-file/":
                            speech_engine.speak("What content would you like to write in the file?")
                            print("🎤 What content would you like to write in the file?")
                        elif command_map[key] == "append-file/":
                            speech_engine.speak("What content would you like to append to the file?")
                            print("🎤 What content would you like to append to the file?")
                        elif command_map[key] == "edit-file/":
                            speech_engine.speak("What content would you like to edit in the file?")
                            print("🎤 What content would you like to edit in the file?")

                        with sr.Microphone() as content_source:
                            audio_content = recognizer.listen(content_source)
                            try:
                                content_text = recognizer.recognize_google(audio_content)
                                print(f"📝 Content for the file: {content_text}")
                                data = content_text.encode()  # Use the spoken content as the file content
                                send_request(api_url, method="POST", data=data)
                            except sr.UnknownValueError:
                                print("⚠️ Could not understand the content.")
                                speech_engine.speak("Sorry, I didn't catch the content.")
                            except sr.RequestError:
                                print("⚠️ There was an issue with the speech service.")
                                speech_engine.speak("There was an issue with the speech service.")
                    else:
                        method = "POST" if command_map[key] in ["mkdir/", "cd/", "create-file/", "append-file/", "edit-file/"] else \
                                 "DELETE" if command_map[key] in ["delete-file/", "rmdir/"] else \
                                 "GET"

                        data = "Sample file content".encode() if command_map[key] in ["create-file/", "append-file/", "edit-file/"] else None
                        send_request(api_url, method, data)

                    break  # Stop checking once a command is found

            else:
                print("⚠️ Unknown command, please try again.")
                speech_engine.speak("Unknown command. Please try again.")

        except sr.UnknownValueError:
            speech_engine.speak("Sorry, I didn't catch that.")
        except sr.RequestError:
            speech_engine.speak("There was an issue with the speech service.")

if __name__ == "__main__":
    recognize_speech()
