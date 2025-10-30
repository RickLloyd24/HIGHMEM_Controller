#include "HIMEM.h"

HIMEMLIB::HIMEM himem;

#define stop {delay(1000); while(1);}
#define NUMFILES 100
#define fileBufSize 20000
uint8_t fileBuf[fileBufSize];
uint16_t fileIdarray[NUMFILES] = {0};
unsigned long taskTime = 0;
String fileName = "";

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
  for (int i = 0; i < NUMFILES; i++) {
    fileName = "file_" + String(i) + ".bin";
    fileIdarray[i] = himem.writeFile(fileName, fileBuf, fileBufSize);
  }
  ESP_LOGI("setup", "Wrote %d files of %d bytes each", NUMFILES, fileBufSize);

  himem.deleteFile(50);  // Delete one file to test read before delete in loop
  ESP_LOGI("setup", "Deleted file ID 50");

  ESP_LOGI("setup", "Starting to compact memory after deleting");
  himem.compact();


  ESP_LOGI("setup", "Compaction completed after deletions. Available memory is %lu bytes.", himem.freespace());
  uint32_t bytesRead = himem.readFile(50, fileName, fileBuf);  // Read deleted file to confirm deletion
  if (bytesRead != 0) {
    ESP_LOGE("setup", "Error: Deleted file ID 50 still readable with %d bytes!", bytesRead);
    stop;
  }
  ESP_LOGI("setup", "Confirmed deletion of file ID 50"); 
  delay(5000); // Wait before reading another file
  int match = true;
  bytesRead = himem.readFile(51, fileName, fileBuf);
  Serial.printf("Read file ID 51, filename %s, %d bytes\n", fileName.c_str(), bytesRead);
  for (int j = 0; j < bytesRead; j++) {
    if (fileBuf[j] != (j % 256)) {
      match = false;
      ESP_LOGE("setup", "Data mismatch at byte %d: expected %d, got %d", j, j % 256, fileBuf[j]);
      break;
    }
  }


  taskTime = millis() + 1000;
}

void loop() {
  unsigned long currentTime = millis();
  uint16_t fileNameCount = NUMFILES;
  himem.compact();

  if (currentTime > taskTime) {
    taskTime = currentTime + 1000;
    uint16_t ID = random(0, NUMFILES);
    fileName = "file_" + String(ID) + ".bin";
/* Read and verify file before deletion */    
    uint32_t bytesRead = himem.readFile(ID, fileName, fileBuf);                  // Read the file before deleting
    if (bytesRead == 0) stop;
    for (uint32_t j = 0; j < bytesRead; j++) {                                   //Check data integrity
      if (fileBuf[j] != (j % 256)) {
        ESP_LOGE("loop", "Data mismatch at byte %d: expected %d, got %d", j, j % 256, fileBuf[j]);
        stop;
      }
    }
/* Delete the file */
    if (!himem.deleteFile(fileIdarray[ID])) stop;
/* Write a new file random size to replace it */
    fileNameCount++;
    fileName = "file_" + String(fileNameCount) + ".bin";
    int filesize = random(10000, 20000);
    int ret = himem.writeFile(fileName, fileBuf, filesize);
    if (ret < 0) {stop}
    fileIdarray[ID] = ret;
    Serial.printf("Deleted file ID %d, wrote new file %s with ID %d and size %d bytes\n", ID, fileName.c_str(), ret, filesize);
  }
}