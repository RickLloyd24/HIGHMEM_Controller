#include "HIMEM.h"

HIMEMLIB::HIMEM himem;

#define stop {delay(1000); while(1);}
#define fileBufSize 20000
uint8_t fileBuf[fileBufSize];
uint16_t fileId = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Start");

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
  }
  Serial.printf("Wrote 20 files of %d bytes each\n", fileBufSize);
  himem.printMemoryStatus();

/* read back "First In First Out" order and verify */
  bool match = true;
  for (int i = 0; i < 20; i++) {
    String fileName = "";
    int bytesRead = himem.getFilesize(i);
    if (bytesRead == 0) {
      ESP_LOGW("setup", "Failed to get filesize for file ID %d", i);
      match = false;
      continue;
    }
    if (bytesRead > fileBufSize) {
      ESP_LOGW("setup", "Filesize mismatch for file ID %d: expected %d, got %d", i, fileBufSize, bytesRead);
      match = false;
      continue;
    }
    himem.readFile(i, fileName, fileBuf);
    for (int j = 0; j < bytesRead; j++) {
      if (fileBuf[j] != (j % 256)) {
        match = false;
        ESP_LOGE("setup", "Data mismatch at byte %d: expected %d, got %d", j, j % 256, fileBuf[j]);
        break;
      }
    }
  }
  if (match) {
    Serial.println("Data verification successful for all files");
  }
  /* Free all HIMEM resources to start writing again */
  himem.freeMemory();

}

void loop() {
  // put your main code here, to run repeatedly:
}
