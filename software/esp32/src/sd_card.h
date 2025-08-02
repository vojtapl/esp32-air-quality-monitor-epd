#pragma once
// https://github.com/espressif/arduino-esp32/blob/master/libraries/SD_MMC/examples/SDMMC_Test/SDMMC_Test.ino
#include "config_gpio.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <FS.h>
#include <SD_MMC.h>

#define USE_1_BIT_MODE true
#define USE_4_BIT_MODE false

bool setup_sd_card();
bool read_note_lookup(fs::FS &fs, const char *path, uint8_t *array);
bool write_note_lookup(fs::FS &fs, const char *path, const uint8_t *array);

bool setup_sd_card()
{
  if (!SD_MMC.setPins(CLK, CMD, D0, D1, D2, D3)) {
    Log.errorln("SD card pin change failed!");
    return false;
  }
  if (!SD_MMC.begin("/sdcard", USE_4_BIT_MODE)) {
    Log.errorln("Card Mount Failed.");
    Log.errorln("Make sure that all data pins have a 10k Ohm pull-up "
                "resistor to 3.3V");
    return false;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Log.errorln("No SD card attached though it is detected");
    return false;
  }
  return true;
}

void list_dir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Log.verboseln("Listing directory: %s", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Log.errorln("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Log.errorln("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        list_dir(fs, file.path(), levels - 1);
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

void create_dir(fs::FS &fs, const char *path)
{
  Log.verboseln("Creating Dir: %s", path);
  if (fs.mkdir(path)) {
    Log.verboseln("Dir created");
  } else {
    Log.errorln("mkdir failed");
  }
}

void remove_dir(fs::FS &fs, const char *path)
{
  Log.verboseln("Removing Dir: %s", path);
  if (fs.rmdir(path)) {
    Log.verboseln("Dir removed");
  } else {
    Log.errorln("rmdir failed");
  }
}

void read_file(fs::FS &fs, const char *path)
{
  Log.verboseln("Reading file: %s", path);

  File file = fs.open(path);
  if (!file) {
    Log.errorln("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
}

void write_file(fs::FS &fs, const char *path, const char *message)
{
  Log.verboseln("Writing file: %s", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Log.errorln("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Log.verboseln("File written");
  } else {
    Log.errorln("Write failed");
  }
}

void append_file(fs::FS &fs, const char *path, const char *message)
{
  Log.verboseln("Appending to file: %s", path);

  File file = fs.open(path, FILE_APPEND, true);
  if (!file) {
    Log.errorln("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Log.verboseln("Message appended");
  } else {
    Log.errorln("Append failed");
  }
}

void rename_file(fs::FS &fs, const char *path1, const char *path2)
{
  Log.verboseln("Renaming file %s to %s", path1, path2);
  if (fs.rename(path1, path2)) {
    Log.verboseln("File renamed");
  } else {
    Log.errorln("Rename failed");
  }
}

void delete_file(fs::FS &fs, const char *path)
{
  Log.verboseln("Deleting file: %s", path);
  if (fs.remove(path)) {
    Log.verboseln("File deleted");
  } else {
    Log.errorln("Delete failed");
  }
}
