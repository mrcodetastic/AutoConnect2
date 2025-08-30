/**
 * AutoConnect error handling and result types.
 * @file AutoConnectError.h
 * @author AutoConnect Contributors
 * @version 1.4.3
 * @date 2025-08-30
 * @copyright MIT license.
 */

#ifndef _AUTOCONNECTERROR_H_
#define _AUTOCONNECTERROR_H_

#include <Arduino.h>

/**
 * Comprehensive error enumeration for AutoConnect operations
 */
enum class ACError : uint8_t {
    SUCCESS = 0,
    WIFI_CONNECT_FAILED,
    WIFI_TIMEOUT,
    WIFI_CREDENTIALS_INVALID,
    JSON_PARSE_ERROR,
    JSON_BUFFER_OVERFLOW,
    FILESYSTEM_ERROR,
    FILESYSTEM_NOT_MOUNTED,
    FILE_NOT_FOUND,
    FILE_READ_ERROR,
    FILE_WRITE_ERROR,
    MEMORY_ALLOCATION_FAILED,
    MEMORY_INSUFFICIENT,
    TIMEOUT_EXCEEDED,
    INVALID_PARAMETER,
    INVALID_STATE,
    PORTAL_START_FAILED,
    WEBSERVER_ERROR,
    DNS_SERVER_ERROR,
    CREDENTIAL_STORE_ERROR,
    CREDENTIAL_LOAD_ERROR,
    UNKNOWN_ERROR
};

/**
 * Result wrapper for AutoConnect operations
 */
struct ACResult {
    ACError error;
    String message;
    
    ACResult(ACError err = ACError::SUCCESS, const String& msg = String()) 
        : error(err), message(msg) {}
    
    // Implicit conversion to bool for easy checking
    explicit operator bool() const { 
        return error == ACError::SUCCESS; 
    }
    
    bool isSuccess() const { 
        return error == ACError::SUCCESS; 
    }
    
    bool isError() const { 
        return error != ACError::SUCCESS; 
    }
    
    const char* errorString() const {
        switch (error) {
            case ACError::SUCCESS: return "Success";
            case ACError::WIFI_CONNECT_FAILED: return "WiFi connection failed";
            case ACError::WIFI_TIMEOUT: return "WiFi connection timeout";
            case ACError::WIFI_CREDENTIALS_INVALID: return "Invalid WiFi credentials";
            case ACError::JSON_PARSE_ERROR: return "JSON parsing error";
            case ACError::JSON_BUFFER_OVERFLOW: return "JSON buffer overflow";
            case ACError::FILESYSTEM_ERROR: return "Filesystem error";
            case ACError::FILESYSTEM_NOT_MOUNTED: return "Filesystem not mounted";
            case ACError::FILE_NOT_FOUND: return "File not found";
            case ACError::FILE_READ_ERROR: return "File read error";
            case ACError::FILE_WRITE_ERROR: return "File write error";
            case ACError::MEMORY_ALLOCATION_FAILED: return "Memory allocation failed";
            case ACError::MEMORY_INSUFFICIENT: return "Insufficient memory";
            case ACError::TIMEOUT_EXCEEDED: return "Timeout exceeded";
            case ACError::INVALID_PARAMETER: return "Invalid parameter";
            case ACError::INVALID_STATE: return "Invalid state";
            case ACError::PORTAL_START_FAILED: return "Portal start failed";
            case ACError::WEBSERVER_ERROR: return "Web server error";
            case ACError::DNS_SERVER_ERROR: return "DNS server error";
            case ACError::CREDENTIAL_STORE_ERROR: return "Credential store error";
            case ACError::CREDENTIAL_LOAD_ERROR: return "Credential load error";
            default: return "Unknown error";
        }
    }
};

/**
 * Memory statistics structure for monitoring
 */
struct ACMemoryStats {
    size_t freeHeap;
    size_t minFreeHeap;
    size_t maxAllocHeap;
    uint32_t timestamp;
    
    ACMemoryStats() {
        update();
    }
    
    void update() {
        freeHeap = ESP.getFreeHeap();
#ifdef ESP32
        minFreeHeap = ESP.getMinFreeHeap();
        maxAllocHeap = ESP.getMaxAllocHeap();
#else
        minFreeHeap = freeHeap;
        maxAllocHeap = freeHeap;
#endif
        timestamp = millis();
    }
    
    bool isLowMemory(size_t threshold = 4096) const {
        return freeHeap < threshold;
    }
    
    String toString() const {
        return String("Free: ") + String(freeHeap) + 
               ", Min: " + String(minFreeHeap) + 
               ", Max: " + String(maxAllocHeap);
    }
};

/**
 * Parameter validation helper macros
 */
#define AC_VALIDATE_PARAM(condition, error) \
    do { \
        if (!(condition)) { \
            AC_DBG("Parameter validation failed: %s\n", #condition); \
            return ACResult(error, String("Parameter validation failed: ") + #condition); \
        } \
    } while(0)

#define AC_VALIDATE_PARAM_BOOL(condition) \
    do { \
        if (!(condition)) { \
            AC_DBG("Parameter validation failed: %s\n", #condition); \
            return false; \
        } \
    } while(0)

#define AC_VALIDATE_NOT_NULL(ptr) \
    AC_VALIDATE_PARAM((ptr) != nullptr, ACError::INVALID_PARAMETER)

#define AC_VALIDATE_STRING_LENGTH(str, maxLen) \
    AC_VALIDATE_PARAM((str).length() <= (maxLen), ACError::INVALID_PARAMETER)

/**
 * Memory monitoring helper
 */
#define AC_CHECK_MEMORY(threshold) \
    do { \
        if (ESP.getFreeHeap() < (threshold)) { \
            AC_DBG("Low memory warning: %u bytes free\n", ESP.getFreeHeap()); \
        } \
    } while(0)

#endif // _AUTOCONNECTERROR_H_
