#include "HIMEM.h"

HIMEMLIB::HIMEM himem;

#define fileBufSize 200000
uint16_t fileId = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  ESP_LOGI("setup", "Start");
  ESP_LOGE("setup", "Example Error");
  ESP_LOGW("setup", "Example Warning");

  // Check if PSRAM is available and initialized
  if (!psramInit()) {
    ESP_LOGE("setup", "PSRAM not available or failed to initialize!");
    return;
  }
  ESP_LOGI("setup", "PSRAM initialized successfully.");

  // Print initial PSRAM and heap free memory
  ESP_LOGI("setup", "Free PSRAM before allocation: %d", ESP.getFreePsram());

  // Allocate a large buffer using malloc.
  // With PSRAM enabled, large allocations automatically go to PSRAM.
  size_t bufferSize = 200000; // 200 KB
  uint8_t* psramBuffer = (uint8_t*)malloc(bufferSize);

  if (psramBuffer == NULL) {
    ESP_LOGE("setup", "Failed to allocate memory using malloc!");
  } else {
    ESP_LOGI("setup", "Successfully allocated %d bytes using malloc.", bufferSize);
  }  

// Fill the buffer with some data to demonstrate usage
  for (size_t i = 0; i < bufferSize; i++) {
    psramBuffer[i] = (uint8_t)(i % 256);
  }
  ESP_LOGI("setup", "Buffer filled with data.");

/* initialize HIMEM */
  himem.init();

/* write multiple files */
  for (int i = 0; i < 5; i++) {
    String fileName = "file_" + String(i) + ".bin";
    int id = himem.writeFile(fileName, psramBuffer, fileBufSize);
  }
  ESP_LOGI("setup", "Wrote 5 files of %d bytes each", fileBufSize);
  ESP_LOGI("setup", "Available memory is %lu bytes.", himem.freespace());

/* read back and verify */
  bool match = true;
  for (int i = 0; i < 5; i++) {
    String fileName;
    int bytesRead = himem.readFile(i, fileName, psramBuffer);
    for (int j = 0; j < bytesRead; j++) {
      if (psramBuffer[j] != (j % 256)) {
        match = false;
        ESP_LOGE("setup", "Data mismatch at byte %d: expected %d, got %d", j, j % 256, psramBuffer[j]);
        break;
      }
    }
  }
  if (match) {
    ESP_LOGI("setup", "Data verification successful for all files");
  }

}

void loop() {
  // put your main code here, to run repeatedly:
}
