#include "HIMEM.h"

// Configuration Constants


// Global Variables (shared across all HIMEM instances)
struct_HIMEM_FileInfo* records = nullptr;        // Pointer to file records array

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
     * Convert HimemError to human-readable string
     */
    const char* errorToString(HimemError error) {
        switch (error) {
            case HimemError::SUCCESS: return "Success";
            case HimemError::FILE_TOO_LARGE: return "File too large";
            case HimemError::FILENAME_TOO_LONG: return "Filename too long";
            case HimemError::MAX_HIMEM_FILES_REACHED: return "Maximum files reached";
            case HimemError::INSUFFICIENT_MEMORY: return "Insufficient memory";
            case HimemError::INVALID_ID: return "Invalid file ID";
            case HimemError::INITIALIZATION_FAILED: return "Initialization failed";
            default: return "Unknown error";
        }
    }

    /**
     * Constructor - Initialize HIMEM file system
     */
    HIMEM::HIMEM() : isInitialized(false), memoryAllocated(false), rangeAllocated(false) {
        esp_log_level_set("HIMEM", ESP_LOG_DEBUG);
        // Initialize member variables to safe values
        memptr = nullptr;
        rangeptr = nullptr;
        himemSize = 0;
        lastPage = 0;
        fileIndex = 0;
        cPage = 0;
        cOffset = 0;
    }
    
    /**
     * Destructor - Clean up allocated memory resources
     */
    HIMEM::~HIMEM() {
        ESP_LOGI("HIMEM", "Destructor called, cleaning up resources");
        cleanupResources();
    }
    /* ----------------------------------------------------------- 
    * HIMEM Initialization
    ----------------------------------------------------------------*/    
    void HIMEM::create() {
        // Cleanup any existing resources first
        if (isInitialized) {
            ESP_LOGW("HIMEM", "Already initialized, cleaning up previous resources");
            cleanupResources();
        }
        
        if (esp_himem_get_phys_size() == 0) {
            ESP_LOGE("create", "HIMEM not initialized make sure -D BOARD_HAS_PSRAM is set");
            return;
        }
        himemSize = esp_himem_get_free_size() - ESP_HIMEM_BLKSZ;  //Reserve one block for file records
        if (himemSize <= ESP_HIMEM_BLKSZ) {
            ESP_LOGE("create", "Not enough HIMEM available, only %lu bytes", himemSize);
            return;
        }   
        
        esp_err_t ret = esp_himem_alloc(himemSize, &memptr);
        if (ret != ESP_OK) {
            ESP_LOGE("create", "Failed to allocate HIMEM: %s", esp_err_to_name(ret));
            return;
        }
        memoryAllocated = true;
        
        ret = esp_himem_alloc_map_range(ESP_HIMEM_BLKSZ, &rangeptr);
        if (ret != ESP_OK) {
            ESP_LOGE("create", "Failed to allocate map range: %s", esp_err_to_name(ret));
            cleanupResources();
            return;
        }
        rangeAllocated = true;
        
        lastPage = himemSize / ESP_HIMEM_BLKSZ - 1;
        isInitialized = true;

        ESP_LOGI("create", "HIMEM free space: %lu bytes", freespace());
        ESP_LOGI("create", "Maximun Number of Files/buffers: %d", MAX_HIMEM_FILES);
    }

    /**
     * Clean up all allocated HIMEM resources
     */
    void HIMEM::destroy() {
        ESP_LOGI("HIMEM", "Manual cleanup requested");
        cleanupResources();
    }

    /**
     * Internal cleanup function to prevent memory leaks
     */
    void HIMEM::cleanupResources() {
        // Free map range if allocated
        if (rangeAllocated && rangeptr != nullptr) {
            esp_err_t ret = esp_himem_free_map_range(rangeptr);
            if (ret != ESP_OK) {
                ESP_LOGE("cleanup", "Failed to free map range: %s", esp_err_to_name(ret));
            } else {
                ESP_LOGI("cleanup", "Map range freed successfully");
            }
            rangeptr = nullptr;
            rangeAllocated = false;
        }

        // Free HIMEM if allocated
        if (memoryAllocated && memptr != nullptr) {
            esp_err_t ret = esp_himem_free(memptr);
            if (ret != ESP_OK) {
                ESP_LOGE("cleanup", "Failed to free HIMEM: %s", esp_err_to_name(ret));
            } else {
                ESP_LOGI("cleanup", "HIMEM freed successfully");
            }
            memptr = nullptr;
            memoryAllocated = false;
        }

        // Reset state variables
        isInitialized = false;
        himemSize = 0;
        lastPage = 0;
        fileIndex = 0;
        cPage = 0;
        cOffset = 0;
        records = nullptr;
        
        ESP_LOGI("cleanup", "All resources cleaned up successfully");
    }
    /* ----------------------------------------------------------- 
    * Write File to HIMEM
    * @param fileName - file name output to String
    * @param buf - buffer with data to write 
    * @param bytes - number of bytes to write
    * @return file Id number, negative on error
    ----------------------------------------------------------------*/
    int HIMEM::writeFile(String fileName, uint8_t* buf, uint32_t bytes) {
    /* Check for Initialization and Safety */
        if (!isInitialized) {
            ESP_LOGE("writeFile", "HIMEM not initialized");
            return static_cast<int>(HimemError::INITIALIZATION_FAILED);
        }
        if (buf == nullptr) {
            ESP_LOGE("writeFile", "Buffer pointer is null");
            return static_cast<int>(HimemError::INVALID_ID);
        }
        if (bytes == 0) {
            ESP_LOGE("writeFile", "Cannot write 0 bytes");
            return static_cast<int>(HimemError::FILE_TOO_LARGE);
        }
        
    /* Check for Errors */
        if (fileName.length() >= MAX_HIMEM_FILENAME_LEN) {
            ESP_LOGE("writeFile", "File %s name too long, max is %d characters", 
                fileName.c_str(), MAX_HIMEM_FILENAME_LEN - 1);
            return static_cast<int>(HimemError::FILENAME_TOO_LONG);
        }
        if (freespace() < bytes) {
            ESP_LOGE("writeFile", "File is larger than available HIMEM");
            return static_cast<int>(HimemError::INSUFFICIENT_MEMORY);
        }
    /* Save File Information */
        int slot = fileIndex;
        ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, lastPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&records));      
        records[slot].ID = slot;
        records[slot].fileSize = bytes;
        fileName.toCharArray(records[slot].filename, fileName.length() + 1);
        records[slot].page = cPage;
        records[slot].offset = cOffset;
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
        return (slot);
    }

    /* ----------------------------------------------------------- 
    * Read File from HIMEM
    * @param id - id assigned when file was create()d
    * @param fileName - file name output to String
    * @param buf - buffer to read file data into
    * @return number of bytes read, 0 on error
    ----------------------------------------------------------------*/
    uint32_t HIMEM::readFile(int id, String &fileName, uint8_t* buf) {
    /* Check for Initialization and Safety */
        if (!isInitialized) {
            ESP_LOGE("readFile", "HIMEM not initialized");
            return 0;
        }
        if (buf == nullptr) {
            ESP_LOGE("readFile", "Buffer is null");
            return 0;
        }
        
    /* Check for Errors */
        if (id < 0 || id >= fileIndex) {
            ESP_LOGE("readFile", "Invalid file ID %d", id);
            return 0;
        }
    /* Locate File Record */
        int slot = id;
        ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, lastPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&records));
        if ( records[slot].ID != id ) {
            ESP_LOGE("readFile", "File ID mismatch expected ID %d, got ID %d", id, records[slot].ID);
            ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, records, ESP_HIMEM_BLKSZ));
            return 0;
        }
    /* Retrieve File Information */
        fileName = String(records[slot].filename);
        uint32_t fileSize = records[slot].fileSize;
        uint16_t currentPage = records[slot].page;
        uint16_t currentOffset = records[slot].offset;
        ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, records, ESP_HIMEM_BLKSZ));

        uint32_t bufferOffset = 0;
        uint32_t bytesToRead = fileSize;
    /* Read File from HIMEM */
        while (bytesToRead > 0) {
            uint8_t* ptr = nullptr;
            ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, currentPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&ptr));
            
            uint32_t availableInPage = ESP_HIMEM_BLKSZ - currentOffset;
            uint32_t chunkSize = (bytesToRead <= availableInPage) ? bytesToRead : availableInPage;
            
            memcpy(buf + bufferOffset, ptr + currentOffset, chunkSize);
            ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, ptr, ESP_HIMEM_BLKSZ));
            
            bytesToRead -= chunkSize;
            bufferOffset += chunkSize;
            
            // Move to next page if more data to read
            if (bytesToRead > 0) {
                currentPage++;
                currentOffset = 0;  // Start from beginning of next page
            }
        }
        return fileSize;
    }

    unsigned long HIMEM::freespace(void) {
        if (!isInitialized) {
            ESP_LOGW("freespace", "HIMEM not initialized");
            return 0;
        }
        unsigned long avail = himemSize - ((unsigned long)cPage * ESP_HIMEM_BLKSZ) - ESP_HIMEM_BLKSZ - cOffset;
        return avail;
    }

    /**
     * Reset file system without deallocating HIMEM
     */
    void HIMEM::freeMemory(void) {
        if (!isInitialized) {
            ESP_LOGW("freeMemory", "HIMEM not initialized");
            return;
        }

        //ESP_LOGI("freeMemory", "File system reset complete, freed %d files", fileIndex);   
        fileIndex = 0;
        cPage = 0;
        cOffset = 0;
        records = nullptr;
    }

    uint32_t HIMEM::getFilesize(int id) {
        if (!isInitialized) {
            ESP_LOGW("getFilesize", "HIMEM not initialized");
            return 0;
        }
        struct_HIMEM_FileInfo info = getRecord(id);
        return info.fileSize;
    }
    
    String HIMEM::getFileName(int id) {
        if (!isInitialized) {
            ESP_LOGW("getFileName", "HIMEM not initialized");
            return String("");
        }
        struct_HIMEM_FileInfo info = getRecord(id);
        return String(info.filename);
    }

    int HIMEM::getID(String filename) {
        if (!isInitialized) {
            ESP_LOGW("getID", "HIMEM not initialized");
            return 0;
        }
        int flag = -1;
        ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, lastPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&records));
        for (int i = 0; i < fileIndex; i++) {
            for (int j = 0; j <= filename.length(); j++) {
                // Compare characters one by one
                if (records[i].filename[j] != filename.charAt(j)) {
                    break; // Mismatch found, break inner loop
                }
                // If we reach the null terminator in both strings, they match
                if (records[i].filename[j] == '\0' && j == filename.length()) {
                    flag = i;
                    break;
                }
            }
            if (flag != -1) {
                break; // Match found, break outer loop
            }
        }
        if (flag == -1) {
            ESP_LOGW("getID", "File %s not found", filename.c_str());
        }
        ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, records, ESP_HIMEM_BLKSZ));
        return flag;
    }

    boolean HIMEM::memoryTest(){
        // Not yet implemented
        return true;
    }

    /**
     * Print detailed memory status for debugging memory leaks
     */
    void HIMEM::printMemoryStatus() {
        ESP_LOGI("MemStatus", "=== HIMEM Memory Status ===");
        ESP_LOGI("MemStatus", "Initialized: %s", isInitialized ? "YES" : "NO");
        ESP_LOGI("MemStatus", "Memory Allocated: %s", memoryAllocated ? "YES" : "NO");
        ESP_LOGI("MemStatus", "Range Allocated: %s", rangeAllocated ? "YES" : "NO");
        ESP_LOGI("MemStatus", "Memory Handle: %p", memptr);
        ESP_LOGI("MemStatus", "Range Handle: %p", rangeptr);
        
        if (isInitialized) {
            ESP_LOGI("MemStatus", "Total HIMEM Size: %lu bytes", himemSize);
            ESP_LOGI("MemStatus", "Current Files: %d / %d", fileIndex, MAX_HIMEM_FILES);
            ESP_LOGI("MemStatus", "Current Page: %d / %d", cPage, lastPage);
            ESP_LOGI("MemStatus", "Current Offset: %d bytes", cOffset);
            ESP_LOGI("MemStatus", "Free Space: %lu bytes", freespace());
            ESP_LOGI("MemStatus", "Memory Usage: %.1f%%", 
                (float)((cPage * ESP_HIMEM_BLKSZ + cOffset) * 100) / himemSize);
        }
        
        // Check ESP-IDF HIMEM stats
        ESP_LOGI("MemStatus", "ESP HIMEM Physical Size: %u bytes", esp_himem_get_phys_size());
        ESP_LOGI("MemStatus", "ESP HIMEM Free Size: %u bytes", esp_himem_get_free_size());
        ESP_LOGI("MemStatus", "ESP HIMEM Reserved: %u bytes", esp_himem_reserved_area_size());
        ESP_LOGI("MemStatus", "=== End Memory Status ===");
    }
    

    struct_HIMEM_FileInfo HIMEM::getRecord(int id){
        struct_HIMEM_FileInfo info = {}; // Initialize to zero
        
        if (!isInitialized) {
            ESP_LOGW("getRecord", "HIMEM not initialized");
            return info;
        }
        
        //Serial.printf("fileIndex: %d, requested id: %d\n", fileIndex, id);  
        if (id >= 0 && id < fileIndex) {
            ESP_ERROR_CHECK(esp_himem_map(memptr, rangeptr, lastPage * ESP_HIMEM_BLKSZ, 0, ESP_HIMEM_BLKSZ, 0, (void**)&records));
            info = records[id];
            //Serial.printf("ID: %d, Name: %s, Size: %u bytes, Page: %d, Offset: %d\n", 
            //    records[id].ID, records[id].filename, records[id].fileSize, records[id].page, records[id].offset);

            ESP_ERROR_CHECK(esp_himem_unmap(rangeptr, records, ESP_HIMEM_BLKSZ));
        }
        return info;
    }

} // namespace HIMEMLIB

