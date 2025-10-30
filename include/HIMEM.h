#ifndef HIMEM_h
#define HIMEM_h

#include <Arduino.h>
//#include "esp_heap_caps.h"
#include "esp32/himem.h"
#include <esp_log.h>             // Required for ESP-IDF logging macros

struct struct_FileInfo;

namespace HIMEMLIB {
    // Error codes for HIMEM operations
    enum class HimemError {
        SUCCESS = 0,
        FILE_TOO_LARGE = -1,
        FILENAME_TOO_LONG = -2,
        MAX_FILES_REACHED = -3,
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
        void init();
        void cleanup();
        boolean memoryTest();
        void freeMemory();
        
        // File Operations
        int writeFile(String fileName, uint8_t* buf, uint32_t bytes);
        uint32_t readFile(int id, String &fileName, uint8_t* buf);
        boolean deleteFile(int id);
        
        // File Information
        uint16_t getID(String filename);
        uint32_t getFilesize(int id);
        String getFileName(int id);
        
        // Memory Management
        unsigned long freespace();
        void compact();
        void printMemoryStatus();
        
        // Configuration settings
    protected:
        esp_himem_handle_t memptr = nullptr;
        esp_himem_rangehandle_t rangeptr = nullptr;
        unsigned long himemSize = 0;
        unsigned int lastPage = 0;
        uint16_t fileIndex = 0;
        uint16_t cPage;
        uint16_t cOffset;
        
        // Resource tracking for leak prevention
        bool isInitialized = false;
        bool memoryAllocated = false;
        bool rangeAllocated = false;
      
        struct_FileInfo getRecord(int id);
        void cleanupResources();

    };   
}



#endif