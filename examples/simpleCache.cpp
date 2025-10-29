#include "HIMEM.h"

HIMEMLIB::HIMEM himem;

#define fileBufSize 20000
uint8_t fileBuf[fileBufSize];
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
  himem.init();

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

void loop() {
  // put your main code here, to run repeatedly:
}
