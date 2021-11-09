// copy of "Examples > SD(esp32) > SD_Test.ino

#include "SDutil.h"

SDutilClass SDutil;

SDutilClass::SDutilClass() : _initialized(false), _accessing(false) {}
SDutilClass::~SDutilClass() {}

int SDutilClass::begin() {
  if (!SD.begin()) {
    Serial.println("[SDutil.begin] Card Mount Failed");
    return 0;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("[SDutil.begin] No SD card attached");
    return 0;
  }

  Serial.printf("[SDutil.begin] A valid SD card is found. Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
  _initialized = true;
  return 1;
}

void SDutilClass::end() {}

void SDutilClass::listDir(const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);
  File root = SD.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void SDutilClass::createDir(const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (SD.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void SDutilClass::removeDir(const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (SD.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void SDutilClass::readFile(const char *path, char **pBuffer, size_t *pSize) {
  Serial.printf("[SDutil.readFile] Reading file: %s\n", path);

  File file = SD.open(path);
  if (!file) {
    Serial.println("[SDutil.readFile] Failed to open file for reading");
    return;
  }
  *pSize = file.size();
  *pBuffer = new char[*pSize];
  file.readBytes(*pBuffer, *pSize);
  file.close();
  Serial.printf("[SDutil.readFile] Successfully read %d bytes from file.\n", *pSize);
}

void SDutilClass::writeFile(const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void SDutilClass::appendFile(const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void SDutilClass::renameFile(const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (SD.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void SDutilClass::deleteFile(const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (SD.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
