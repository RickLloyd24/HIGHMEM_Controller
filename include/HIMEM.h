#ifndef HIMEM_h
#define HIMEM_h

#include <Arduino.h>
//#include "esp_heap_caps.h"
#include "esp32/himem.h"
#include <esp_log.h>             // Required for ESP-IDF logging macros

#define MAX_HIMEM_FILENAME_LEN 40
#define HIMEM_FILE_HEADER_SIZE sizeof(struct_HIMEM_FileInfo)
#define MAX_HIMEM_FILES (ESP_HIMEM_BLKSZ / sizeof(struct_HIMEM_FileInfo) - 1)

// File Information Structure
struct struct_HIMEM_FileInfo {
    uint16_t ID;
    char filename[MAX_HIMEM_FILENAME_LEN + 1];
    uint32_t fileSize;
    uint16_t page;
    uint16_t offset;
};

struct struct_HIMEM_FileInfo;

namespace HIMEMLIB {
    // Error codes for HIMEM operations
    enum class HimemError {
        SUCCESS = 0,
        FILE_TOO_LARGE = -1,
        FILENAME_TOO_LONG = -2,
        MAX_HIMEM_FILES_REACHED = -3,
        INSUFFICIENT_MEMORY = -4,
        INVALID_ID = -5,
        INITIALIZATION_FAILED = -6
    };

    // Utility function to convert error codes to strings
    const char* errorToString(HimemError error);

    /**
     * High Memory (HIMEM) File System
     * Provides file storage and retrieval functionality using ESP32 HIMEM
     */
    class HIMEM {
    public:
        /**
         * Constructor
         */
        HIMEM();
        
        /**
         * Destructor - clean up allocated memory
         */
        ~HIMEM();
        // System Management
        void create();                                                     // Initialize HIMEM file system
        void destroy();                                                    // Deinitialize HIMEM file system
        void freeMemory();                                                 // Free all HIMEM resources
        unsigned long freespace();                                         // Get available HIMEM space

        // File Operations
        int writeFile(int id, String fileName, uint8_t* buf, uint32_t bytes);      // Write file, return file ID or negative error code
        uint32_t readFile(int id, String &fileName, uint8_t* buf);                 // Return number of bytes read, 0 on error
        int writeBaseline(int id, String fileName, uint8_t* buf, uint32_t bytes);  // Writes a baseline file to slot id
        void setBaseline(int id);                                                  // Sets baseline to the specified ID
                
        // File Information
        int getID(String filename);                                        // Get file ID by name, -1 if not found   
        uint32_t getFilesize(int id);                                      // Get file size by ID, 0 if not found    
        String getFileName(int id);                                        // Get file name by ID, empty string if not found
        
        // Memory Management
        void printMemoryStatus();                                          // Print current HIMEM usage status   
        boolean memoryTest();                                              // Test HIMEM functionality
        
    protected:
        esp_himem_handle_t memptr = nullptr;
        esp_himem_rangehandle_t rangeptr = nullptr;
        unsigned long himemSize = 0;
        unsigned int lastPage = 0;
        uint16_t fileIndex = 0;
        uint16_t cPage;
        uint16_t cOffset;
        uint8_t pageUsed;
        
        // Resource tracking for leak prevention
        bool isInitialized = false;
        bool memoryAllocated = false;
        bool rangeAllocated = false;
      
        struct_HIMEM_FileInfo getRecord(int id);
        void cleanupResources();

    };   
}



#endif