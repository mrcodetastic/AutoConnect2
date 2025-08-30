/**
 * ArduinoJson 7 compatibility definitions for AutoConnect.
 * @file AutoConnectJsonDefs.h
 * @author hieromon@gmail.com, AutoConnect Contributors
 * @version  1.4.3
 * @date 2025-08-30
 * @copyright  MIT license.
 */

#ifndef _AUTOCONNECTJSONDEFS_H_
#define _AUTOCONNECTJSONDEFS_H_

#include <ArduinoJson.h>

// Ensure we're using ArduinoJson 7.x
#if ARDUINOJSON_VERSION_MAJOR < 7
#error "AutoConnect2 requires ArduinoJson version 7.0 or later. Please update your ArduinoJson library."
#endif

/**
 * ArduinoJson 7 unified definitions
 * Version 7 simplified the API and removed many compatibility macros
 */

// Document creation - ArduinoJson 7 uses JsonDocument directly
#define ArduinoJsonStaticBuffer           JsonDocument
#define ARDUINOJSON_CREATEOBJECT(doc)     doc.to<JsonObject>()
#define ARDUINOJSON_CREATEARRAY(doc)      doc.to<JsonArray>()

// Serialization functions - simplified in v7
#define ARDUINOJSON_PRETTYPRINT(doc, out) ({ size_t s = serializeJsonPretty(doc, out); s; })
#define ARDUINOJSON_PRINT(doc, out)       ({ size_t s = serializeJson(doc, out); s; })

// Object reference handling - v7 uses value semantics
#define ARDUINOJSON_OBJECT_REFMODIFY      const

// Type aliases for ArduinoJson 7
using ArduinoJsonObject = JsonObject;
using ArduinoJsonArray = JsonArray;

// Memory allocation strategy for ArduinoJson 7
#if defined(BOARD_HAS_PSRAM) && defined(ESP32)
// Custom allocator for PSRAM when available
struct PsramAllocator {
    void* allocate(size_t size) {
        if (psramFound()) {
            void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
            if (ptr) {
                AC_DBG("JSON buffer allocated in PSRAM: %u bytes\n", size);
                return ptr;
            }
        }
        // Fallback to regular heap
        AC_DBG("JSON buffer allocated in heap: %u bytes\n", size);
        return malloc(size);
    }
    
    void deallocate(void* pointer) {
        heap_caps_free(pointer);
    }
    
    void* reallocate(void* ptr, size_t new_size) {
        if (psramFound()) {
            void* new_ptr = heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
            if (new_ptr) return new_ptr;
        }
        return realloc(ptr, new_size);
    }
};

using ArduinoJsonBuffer = BasicJsonDocument<PsramAllocator>;
#define AUTOCONNECT_JSONBUFFER_PRIMITIVE_SIZE AUTOCONNECT_JSONPSRAM_SIZE
#else
// Standard heap allocation for ArduinoJson 7
using ArduinoJsonBuffer = JsonDocument;
#define AUTOCONNECT_JSONBUFFER_PRIMITIVE_SIZE AUTOCONNECT_JSONDOCUMENT_SIZE
#endif

// Size calculation helpers for ArduinoJson 7
// v7 doesn't need precise size calculations, but we provide estimates
#define JSON_OBJECT_SIZE(pairs) (24 + (pairs) * 32)  // Rough estimate for v7
#define JSON_ARRAY_SIZE(elements) (24 + (elements) * 16)  // Rough estimate for v7
#define JSON_STRING_SIZE(len) ((len) + 1)

// Error handling for ArduinoJson 7
inline bool isJsonError(DeserializationError error) {
    return error != DeserializationError::Ok;
}

inline const char* getJsonErrorString(DeserializationError error) {
    return error.c_str();
}

// Helper functions for ArduinoJson 7 compatibility
namespace AutoConnectJson {
    // Create a JSON document with appropriate size
    inline ArduinoJsonBuffer createDocument(size_t capacity = AUTOCONNECT_JSONBUFFER_PRIMITIVE_SIZE) {
        return ArduinoJsonBuffer(capacity);
    }
    
    // Parse JSON string with error checking
    inline ACResult parseJson(ArduinoJsonBuffer& doc, const String& json) {
        DeserializationError error = deserializeJson(doc, json);
        if (isJsonError(error)) {
            return ACResult(ACError::JSON_PARSE_ERROR, 
                           String("JSON parsing failed: ") + getJsonErrorString(error));
        }
        return ACResult(ACError::SUCCESS);
    }
    
    // Serialize JSON with error checking
    inline ACResult serializeJson(const ArduinoJsonBuffer& doc, String& output) {
        size_t size = measureJson(doc);
        if (size == 0) {
            return ACResult(ACError::JSON_PARSE_ERROR, "Empty JSON document");
        }
        
        output.reserve(size + 16); // Add some padding
        size_t written = ::serializeJson(doc, output);
        
        if (written != size) {
            return ACResult(ACError::JSON_PARSE_ERROR, "JSON serialization size mismatch");
        }
        
        return ACResult(ACError::SUCCESS);
    }
    
    // Estimate required capacity for JSON document
    inline size_t estimateJsonCapacity(size_t numObjects, size_t numArrays, size_t totalStringLength) {
        return JSON_OBJECT_SIZE(numObjects) + 
               JSON_ARRAY_SIZE(numArrays) + 
               JSON_STRING_SIZE(totalStringLength) + 
               512; // Safety margin for ArduinoJson 7
    }
    
    // Safe JSON object access
    template<typename T>
    inline T getJsonValue(const JsonObject& obj, const char* key, const T& defaultValue = T{}) {
        JsonVariant variant = obj[key];
        if (variant.isNull()) {
            return defaultValue;
        }
        return variant.as<T>();
    }
    
    // Safe JSON array access
    template<typename T>
    inline T getJsonArrayValue(const JsonArray& arr, size_t index, const T& defaultValue = T{}) {
        if (index >= arr.size()) {
            return defaultValue;
        }
        JsonVariant variant = arr[index];
        if (variant.isNull()) {
            return defaultValue;
        }
        return variant.as<T>();
    }
    
    // Check if JSON key exists and is not null
    inline bool hasJsonKey(const JsonObject& obj, const char* key) {
        return obj.containsKey(key) && !obj[key].isNull();
    }
    
    // Get JSON object memory usage
    inline size_t getJsonMemoryUsage(const ArduinoJsonBuffer& doc) {
        return doc.memoryUsage();
    }
    
    // Get JSON object capacity
    inline size_t getJsonCapacity(const ArduinoJsonBuffer& doc) {
        return doc.capacity();
    }
}

#endif // _AUTOCONNECTJSONDEFS_H_