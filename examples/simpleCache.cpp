#include "HIMEM.h"

HIMEMLIB::HIMEM himem;

#define stop {delay(1000); while(1);}
#define fileBufSize 20000
uint8_t fileBuf[fileBufSize];
uint16_t fileId = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.printf("Start\n");

/* generate test data */
  for (int i = 0; i < fileBufSize; i++) {
    fileBuf[i] = i % 256;
  }
/* initialize HIMEM */
  himem.create();

/* write multiple files */
  for (int i = 0; i < 20; i++) {
    String fileName = "file_" + String(i) + ".bin";
    int id = himem.writeFile(i, fileName, fileBuf, fileBufSize);
    //ESP_LOGI("setup", "Wrote file %s with ID %d", fileName.c_str(), id);
  }
  Serial.printf("Wrote 20 files of %d bytes each\n", fileBufSize);
  Serial.printf("Available memory is %lu bytes.\n", himem.freespace());
  himem.printMemoryStatus();

/* read back "First In First Out" order and verify */
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
  /* Free all HIMEM resources to start writing again */
  himem.freeMemory();

}

void loop() {
  // put your main code here, to run repeatedly:
}
