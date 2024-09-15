import os
import zipfile
from platformio import fs

# Define the directory and files you want to include
DATA_DIR = "data/wwwroot"
INCLUDE_FILES = ["index.html", "js/main.js"]  # Files to include

def create_zip():
    zip_filename = "spiffs.zip"
    with zipfile.ZipFile(zip_filename, 'w') as zipf:
        for root, _, files in os.walk(DATA_DIR):
            for file in files:
                if file in INCLUDE_FILES:
                    file_path = os.path.join(root, file)
                    zipf.write(file_path, os.path.relpath(file_path, DATA_DIR))
    return zip_filename

def upload_zip(zip_filename):
    with open(zip_filename, 'rb') as f:
        fs.upload_data(f.read(), 'spiffs')

def main():
    zip_filename = create_zip()
    upload_zip(zip_filename)
    os.remove(zip_filename)

if __name__ == "__main__":
    main()
