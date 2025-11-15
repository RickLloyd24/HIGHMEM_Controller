#include "HIMEM.h"
#include "SD_MMC.h"

HIMEMLIB::HIMEM himem;

#define stop {delay(1000); while(1);}
#define fileBufSize 15000
#define SD_MMC_D0          2           //Hardwired to SD card
#define SD_MMC_CLK        14           //Hardwired to SD card
#define SD_MMC_CMD        15           //Hardwired to SD card

uint8_t fileBuf[fileBufSize];
String fileName = "";

void setup() {
  unsigned long start, end;
  uint16_t ret = 0;
  Serial.begin(115200);
  delay(3000);
  ESP_LOGI("setup", "Start");

  /* generate test data */
  for (int i = 0; i < fileBufSize; i++) {
    fileBuf[i] = i % 256;
  }
/* initialize HIMEM */
  himem.create();
  
/* write files until out of space */ 
  int numberOfFiles = 0; 
  while (himem.freespace() > fileBufSize + HIMEM_FILE_HEADER_SIZE) {
    //Serial.printf("Writing file %d, %d bytes free\n", numberOfFiles, himem.freespace());
    fileName = "file_" + String(numberOfFiles) + ".bin";
    ret = himem.writeFile(numberOfFiles, fileName, fileBuf, fileBufSize);
    if (ret < 0) {
      ESP_LOGE("setup", "Failed to write file %s: %s, error code: %d", 
        fileName.c_str(), HIMEMLIB::errorToString(static_cast<HIMEMLIB::HimemError>(ret)), ret);
      stop;
    }
    numberOfFiles++;
    if (numberOfFiles > MAX_HIMEM_FILES) {
      ESP_LOGW("setup", "Ran out of HIMEM File space after writing %d files, maximum is %d files.", 
        numberOfFiles - 1, MAX_HIMEM_FILES);  
      break;  
    }
  }

  numberOfFiles--; // adjust for last failed write
  ESP_LOGI("setup", "Wrote %d files of %d bytes each", numberOfFiles, fileBufSize);
  himem.printMemoryStatus();

/* initialize SD_MMC */
    start = millis();
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
      ESP_LOGE("SD", "Micro SD Card Mount Failed #####");
      stop;
    }
    uint8_t cardType = SD_MMC.cardType();
    if(cardType == CARD_NONE){
        ESP_LOGE("SD", "No Micro SD Card attached #####");
        stop;
    }
  
    ESP_LOGI("SD", "SD_MMC Initialization Success! Free Space: %llu MB", 
      (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024));

ret = himem.getID("file_100.bin");
Serial.printf("HIMEM ID for file %s is %d\n", himem.getFileName(ret), ret);
/* write files in HIMEM to SD_MMC */ 
for (int i = 0; i < numberOfFiles; i++) {
  uint32_t fileSize = himem.getFilesize(i);
  if (fileSize > fileBufSize) {
    ESP_LOGE("setup", "Error: file larger than buffer, buffer: %d bytes, file: %d bytes for file ID %d", 
      fileBufSize, fileSize, i);
    stop;  
  }
  ret = himem.readFile(i, fileName, fileBuf);
  if (ret == 0) {
    ESP_LOGE("setup", "Failed to read file ID %d from HIMEM: %s, error code: %d", 
      i, HIMEMLIB::errorToString(static_cast<HIMEMLIB::HimemError>(ret)), ret);
    stop;
  }
  File file = SD_MMC.open("/" + fileName, FILE_WRITE);
  if(!file){
    ESP_LOGE("FILES", "Failed to open file for writing %s", fileName.c_str());
    stop;
  }
  if(file.write(fileBuf, fileBufSize) != fileBufSize){
    ESP_LOGE("FILES", "File Write failed %s", fileName.c_str());
    file.close();
    stop;
  }
  file.close();
}  
  ESP_LOGI("setup", "Wrote %d files from HIMEM to SD_MMC", numberOfFiles);

/* Free all HIMEM resources to start writing again */
  himem.freeMemory();

}

void loop() {
  // put your main code here, to run repeatedly:
}