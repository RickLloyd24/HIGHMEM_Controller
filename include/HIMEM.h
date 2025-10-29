#ifndef HIMEM_h
#define HIMEM_h

#include <Arduino.h>
//#include "esp_heap_caps.h"
#include "esp32/himem.h"
#include <esp_log.h>             // Required for ESP-IDF logging macros

struct struct_FileInfo;

namespace HIMEMLIB {
    /**
     * Motion detection from JPEG image
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
        // Intialize HIMEM
        void init();
        int writeFile(String fileName, uint8_t* buf, uint32_t bytes);
        uint32_t readFile(int id, String &fileName, uint8_t* buf);
        boolean deleteFile(int id);
        unsigned long freespace();
        void freeMemory();
        uint16_t getID(String filename);
        uint32_t getFilesize(int id);
        String getFileName(int id);
        boolean memoryTest();
        void compact();
        
        // Configuration settings
    protected:
        esp_himem_handle_t memptr = nullptr;
        esp_himem_rangehandle_t rangeptr = nullptr;
        unsigned long himemSize = 0;
        unsigned int lastPage = 0;
        uint16_t fileIndex = 0;
        uint16_t cPage;
        uint16_t cOffset;
      
        struct_FileInfo getRecord(int id);

    };   
}



#endif