#include "HIMEM.h"

HIMEMLIB::HIMEM himem;

#define MAX_FILES 100
#define fileBufSize 20000
uint8_t fileBuf[fileBufSize];
uint16_t fileIdarray[MAX_FILES] = {0};
unsigned long taskTime = 0;

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
  for (int i = 0; i < MAX_FILES; i++) {
    String fileName = "file_" + String(i) + ".bin";
    fileIdarray[i] = himem.writeFile(fileName, fileBuf, fileBufSize);
  }
  taskTime = millis() + 1000;
}

void loop() {
    unsigned long currentTime = millis();
    uint16_t fileNameCount = MAX_FILES;
    himem.compact();

    if (currentTime > taskTime) {
      taskTime = currentTime + 1000;
      uint16_t ID = random(0, MAX_FILES);
      himem.deleteFile(fileIdarray[ID]);
      fileNameCount++;
      String fileName = "file_" + String(fileNameCount) + ".bin";
      int filesize = random(10000, 20000);
      fileIdarray[ID] = himem.writeFile(fileName, fileBuf, filesize);
    }

  // put your main code here, to run repeatedly:
}