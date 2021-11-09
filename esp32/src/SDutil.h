/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */

#pragma once

#include <Arduino.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

class SDutilClass {
 public:
  SDutilClass();
  ~SDutilClass();
  int begin();
  void end();
  void listDir(const char *dirname, uint8_t levels);
  void createDir(const char *path);
  void removeDir(const char *path);
  void readFile(const char *path, char **pBuffer, size_t *pSize);
  void writeFile(const char *path, const char *message);
  void appendFile(const char *path, const char *message);
  void renameFile(const char *path1, const char *path2);
  void deleteFile(const char *path);

 private:
  bool _initialized;
  bool _accessing;
};

extern SDutilClass SDutil;
