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
  Serial.printf("Generated test data of %d bytes\n", fileBufSize);
  delay(1000);

  /* initialize HIMEM */
  himem.create();
  Serial.printf("HIMEM initialized\n");
  delay(1000);

/* write multiple baseline files */
  for (int i = 0; i < 3; i++) {
    String fileName = "Baseline_" + String(i) + ".jpg";
    int ret = himem.writeBaseline(i, fileName, fileBuf, fileBufSize);
    ESP_LOGI("setup", "Wrote baseline file %s with ID %d, return page %d", fileName.c_str(), i, ret);
  }

  /* set file number 2 as the baseline */  
  int ret = himem.setBaseline(2, fileBuf, fileBufSize);                //need to provide buffer and size to setBaseline
  if (ret < 0) {
    ESP_LOGE("setup", "Failed to set baseline to file ID 2, return value %d", ret);
    stop;
  } else {
    ESP_LOGI("setup", "Set baseline to file ID 2, return value %d", ret);
  }

/* Write multiple files starting with file 0  */
  for (int i = 0; i < 100; i++) {
    String fileName = "file_" + String(i) + ".bin";
    int ret = himem.writeFile(i, fileName, fileBuf, fileBufSize);
  }

/* read back files starting with the baseline */
  bool match = true;
  for (int i = 0; i < 3; i++) {
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
    ESP_LOGI("setup", "Read and verified file ID %d: %s (%d bytes)", i, fileName.c_str(), bytesRead);
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