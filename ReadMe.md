# HIMEM Library

The ESP32 can direclty access external memories (PSRAM) that are <= 4 MiB.  Above 4 MiB (HIMEM) a bank switching scheme scheme is used.  HIMEM is accessed in 32 K banks.  This makes using HIMEM complicated.

This library simplifies using HIMEM. Features:
    * packs buffers/files smaller than 32k in HIMEM not wasting any space
    * allows buffers to be larger than 32k. Only limit is available space

I use this library as a FiFo to save camera frames and verify that motion was detected before writing the data to an SD card.

For a 15k file approximate HIMEM write time is 14 milliseconds and for an SD card write time is 62 milliseconds.

The maximum number of files that can be written is 629.  The filename can be upto 40 charactors.

## Code Example

#include "HIMEM.h"

HIMEMLIB::HIMEM himem;

uint8_t fileBuf[20000];
uint16_t fileId = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  ESP_LOGI("setup", "Start");
  
/* generate test data */
  for (int i = 0; i < fileBufSize; i++) {
    fileBuf[i] = i % 256;
  }
/* initialize HIMEM */
  himem.create();

/* write multiple files */
  for (int i = 0; i < 20; i++) {
    String fileName = "file_" + String(i) + ".bin";
    int id = himem.writeFile(fileName, fileBuf, fileBufSize);
    //ESP_LOGI("setup", "Wrote file %s with ID %d", fileName.c_str(), id);
  }
  ESP_LOGI("setup", "Wrote 20 files of %d bytes each", fileBufSize);
  ESP_LOGI("setup", "Available memory is %lu bytes.", himem.freespace());

/* read back and verify */
  bool match = true;
  for (int i = 0; i < 20; i++) {
    String fileName;
    int bytesRead = himem.readFile(i, fileName, fileBuf);
    for (int j = 0; j < bytesRead; j++) {
      if (fileBuf[j] != (j % 256)) {
        match = false;
        ESP_LOGE("setup", "Data mismatch at byte %d: expected %d, got %d", j, j % 256, fileBuf[j]);
        break;
      }
    }
  }
  if (match) {
    ESP_LOGI("setup", "Data verification successful for all files");
  }
  uint32_t filesize = himem.filesize(5);
  ESP_LOGI("setup", "Filesize for file ID 5 is %d bytes", filesize);
  ESP_LOGI("setup", "Filename for file ID 5 is %s", himem.fileName(5).c_str());
}

## Hardware Support

This library should support ESP32 boads with HIMEM.

 The library was tested on ESP32-CAM and Freenove WROVER.

## Similar libraries

none that I could find
