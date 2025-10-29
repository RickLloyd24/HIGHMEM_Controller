#include "HIMEM.h"

// Configuration Constants
#define MAX_BUFFERS 100
#define MAX_FILENAME_LEN 40

// File Information Structure
struct struct_FileInfo {
    uint16_t ID;
    char filename[MAX_FILENAME_LEN];
    uint32_t fileSize;
    uint16_t page;
    uint16_t offset;
};

// Calculate maximum files that can fit in one block
#define MAX_FILES (ESP_HIMEM_BLKSZ / sizeof(struct_FileInfo) - 1)

// Global Variables (shared across all HIMEM instances)
struct_FileInfo* records = nullptr;        // Pointer to file records array

// HIMEM Hardware Handles
esp_himem_handle_t memptr = nullptr;        // Handle to allocated HIMEM
esp_himem_rangehandle_t rangeptr = nullptr; // Handle to memory mapping range

// Memory Layout Tracking
unsigned long himemSize = 0;               // Total HIMEM size available
unsigned int lastPage = 0;                 // Last page number (reserved for records)
uint16_t fileIndex = 0;                    // Current number of stored files
uint16_t cPage = 0;                        // Current page for new writes
uint16_t cOffset = 0;                      // Current offset within page

namespace HIMEMLIB {

    /**
     * Constructor - Initialize HIMEM file system
     */
    HIMEM::HIMEM() {
        esp_log_level_set("HIMEM", ESP_LOG_DEBUG);
    }
    
    /**
     * Destructor - Clean up allocated memory resources
     */
    HIMEM::~HIMEM() {
        // TODO: Implement proper cleanup of HIMEM resources
        // esp_himem_free(memptr);
        // esp_himem_free_map_range(rangeptr);
    }
    /* ----------------------------------------------------------- 
    * HIMEM Initialization
    ----------------------------------------------------------------*/    
    void HIMEM::init() {
        if (esp_himem_get_phys_size() == 0) {
            ESP_LOGE("init", "HIMEM not initialized make sure -D BOARD_HAS_PSRAM is set");
            return;
        }
        himemSize = esp_himem_get_free_size() - ESP_HIMEM_BLKSZ;  //Reserve one block for file records
        if (himemSize <= ESP_HIMEM_BLKSZ) {
            ESP_LOGE("init", "Not enough HIMEM available, only %lu bytes", himemSize);
            return;
        }   
        ESP_ERROR_CHECK(esp_himem_alloc(himemSize, &memptr));
        ESP_ERROR_CHECK(esp_himem_alloc_map_range(ESP_HIMEM_BLKSZ, &rangeptr));
        lastPage = himemSize / ESP_HIMEM_BLKSZ - 1;

        ESP_LOGI("init", "HIMEM total size: %lu bytes", himemSize);
        ESP_LOGI("init", "HIMEM available pages: %u, each page is %d bytes", lastPage - 1, ESP_HIMEM_BLKSZ);
        ESP_LOGI("init", "HIMEM reserved size: %u bytes", esp_himem_reserved_area_size());
    }
    /* ----------------------------------------------------------- 
    * Write File from HIMEM
    * @param fileName - file name output to String
    * @param buf - buffer to read file data into
    * @param bytes - number of bytes to write
    * @return file Id number, -1 on error
    ----------------------------------------------------------------*/
    int HIMEM::writeFile(String fileName, uint8_t* buf, uint32_t bytes) {
    /* Check for Errors */
        if (fileName.length() >= MAX_FILENAME_LEN) {
            ESP_LOGE("writeFile", "File %s name too long, max is %d characters", 
                fileName.c_str(), MAX_FILENAME_LEN - 1);
            return -1;
        }
        if (fileIndex + 1 >= MAX_FILES) {
            ESP_LOGE("writeFile", "Maximum number of files reached %d", MAX_FILES);
            return -1;
        }
        if (himemSize - cPage * ESP_HIMEM_BLKSZ - cOffset < bytes) {
            ESP_LOGE("writeFile", "File is larger than available HIMEM");
            return -1;
        }
    /* Save File Information */
        ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, lastPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&records));
        records[fileIndex].ID = fileIndex;
        records[fileIndex].fileSize = bytes;
        strncpy(records[fileIndex].filename, fileName.c_str(), MAX_FILENAME_LEN);
        records[fileIndex].page = cPage;
        records[fileIndex].offset = cOffset;
        ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, records, ESP_HIMEM_BLKSZ));
        fileIndex++;
    /* Write File to HIMEM */
        uint32_t bytesToWrite = bytes;
        uint32_t bufferOffset = 0;
        
        while (bytesToWrite > 0) {
            uint8_t* ptr = nullptr;
            ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, cPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&ptr));
            
            uint32_t availableInPage = ESP_HIMEM_BLKSZ - cOffset;
            uint32_t chunkSize = (bytesToWrite <= availableInPage) ? bytesToWrite : availableInPage;
            
            memcpy(ptr + cOffset, buf + bufferOffset, chunkSize);
            ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, ptr, ESP_HIMEM_BLKSZ));
            
            bytesToWrite -= chunkSize;
            bufferOffset += chunkSize;
            cOffset += chunkSize;
            
            // Move to next page if current page is full
            if (cOffset >= ESP_HIMEM_BLKSZ) {
                cPage++;
                cOffset = 0;
            }
        }
        return (fileIndex - 1);
    }
    /* ----------------------------------------------------------- 
    * Read File from HIMEM
    * @param id - id assigned when file was created
    * @param fileName - file name output to String
    * @param buf - buffer to read file data into
    * @return number of bytes read, 0 on error
    ----------------------------------------------------------------*/
    uint32_t HIMEM::readFile(int id, String &fileName, uint8_t* buf) {
    /* Check for Errors */
        if (id < 0 || id >= fileIndex) {
            ESP_LOGE("readFile", "Invalid file ID %d", id);
            return 0;
        }
    /* Retrieve File Information */
        ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, lastPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&records));
        fileName = String(records[id].filename);
        uint32_t fileSize = records[id].fileSize;
        uint16_t page = records[id].page;
        uint16_t offset = records[id].offset;
        ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, records, ESP_HIMEM_BLKSZ));

    /* Read File from HIMEM */
        uint8_t *ptr = nullptr;
        ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, page*ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&ptr));
        if (offset + fileSize <= ESP_HIMEM_BLKSZ) {
            memcpy(buf, ptr + offset, fileSize - 1);
            ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, ptr, ESP_HIMEM_BLKSZ));
        }
        else {
            uint16_t firstChunk = ESP_HIMEM_BLKSZ - offset;
            memcpy(buf, ptr + offset, firstChunk - 1);
            ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, ptr, ESP_HIMEM_BLKSZ));
            page++; uint16_t remaining = fileSize - firstChunk;
            if (remaining > ESP_HIMEM_BLKSZ) {
                while (remaining > ESP_HIMEM_BLKSZ) {
                    ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, page*ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&ptr));
                    memcpy(buf + firstChunk,  ptr, ESP_HIMEM_BLKSZ);
                    ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, ptr, ESP_HIMEM_BLKSZ));
                    remaining -= ESP_HIMEM_BLKSZ;
                    firstChunk = firstChunk + ESP_HIMEM_BLKSZ;
                    page++;
                }
                ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, page*ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&ptr));
                memcpy(buf + firstChunk, ptr, remaining);
                ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, ptr, ESP_HIMEM_BLKSZ));
            }
            ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, page*ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&ptr));
            memcpy(buf + firstChunk, ptr, remaining);
            ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, ptr, ESP_HIMEM_BLKSZ));
        }
        return fileSize;
    }

    unsigned long HIMEM::freespace(void) {
        unsigned long avail = himemSize - (unsigned long)cPage * ESP_HIMEM_BLKSZ - cOffset;
        return avail;
    }

    uint32_t HIMEM::getFilesize(int id) {
        struct_FileInfo info = getRecord(id);
        return info.fileSize;
    }
    String HIMEM::getFileName(int id) {
        struct_FileInfo info = getRecord(id);
        return String(info.filename);
    }
    uint16_t HIMEM::getID(String filename) {
        // Not yet implemented
        return 0;
    }

    boolean HIMEM::deleteFile(int id) {
        // Not yet implemented
        return false;
    }
    void HIMEM::compact(void) {
        // Not yet implemented
    }

    boolean HIMEM::memoryTest(){
        // Not yet implemented
        return true;
    }
    

    struct_FileInfo HIMEM::getRecord(int id){
        struct_FileInfo info;
        Serial.printf("fileIndex: %d, requested id: %d\n", fileIndex, id);  
        if (id >= 0 && id < fileIndex) {
            ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, lastPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&records));
            info = records[id];
            Serial.printf("Info File size: %d, record file size: %d\n", info.fileSize, records[id].fileSize);
            Serial.printf("Info File name: %s, record file name: %s\n", info.filename, records[id].filename);
            ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, records, ESP_HIMEM_BLKSZ));
        }
        return info;
    }

} // namespace HIMEMLIB

