/**
 * RAII utilities and smart resource management for AutoConnect.
 * @file AutoConnectRAII.h
 * @author AutoConnect Contributors
 * @version 1.4.3
 * @date 2025-08-30
 * @copyright MIT license.
 */

#ifndef _AUTOCONNECTRAII_H_
#define _AUTOCONNECTRAII_H_

#include <Arduino.h>
#include <memory>
#include "AutoConnectFS.h"
#include "AutoConnectError.h"

/**
 * RAII wrapper for file operations with automatic cleanup
 */
class AutoFile {
private:
    File _file;
    bool _isOpen;

public:
    AutoFile(const String& path, const char* mode) 
        : _isOpen(false) {
        _file = AUTOCONNECT_APPLIED_FILESYSTEM.open(path, mode);
        _isOpen = static_cast<bool>(_file);
        if (!_isOpen) {
            AC_DBG("Failed to open file: %s\n", path.c_str());
        }
    }
    
    ~AutoFile() {
        if (_isOpen && _file) {
            _file.close();
            AC_DBG("File automatically closed\n");
        }
    }
    
    // Non-copyable
    AutoFile(const AutoFile&) = delete;
    AutoFile& operator=(const AutoFile&) = delete;
    
    // Movable
    AutoFile(AutoFile&& other) noexcept 
        : _file(std::move(other._file)), _isOpen(other._isOpen) {
        other._isOpen = false;
    }
    
    AutoFile& operator=(AutoFile&& other) noexcept {
        if (this != &other) {
            if (_isOpen && _file) {
                _file.close();
            }
            _file = std::move(other._file);
            _isOpen = other._isOpen;
            other._isOpen = false;
        }
        return *this;
    }
    
    operator bool() const { 
        return _isOpen && _file; 
    }
    
    File& get() { 
        return _file; 
    }
    
    const File& get() const { 
        return _file; 
    }
    
    size_t size() const {
        return _file ? _file.size() : 0;
    }
    
    String readString() {
        return _file ? _file.readString() : String();
    }
    
    size_t write(const String& str) {
        return _file ? _file.print(str) : 0;
    }
    
    void close() {
        if (_isOpen && _file) {
            _file.close();
            _isOpen = false;
        }
    }
};

/**
 * String builder to avoid String concatenation fragmentation
 */
class StringBuilder {
private:
    std::vector<String> _parts;
    size_t _estimatedSize;
    
public:
    StringBuilder() : _estimatedSize(0) {
        _parts.reserve(16); // Reserve space for common case
    }
    
    StringBuilder& append(const String& str) {
        _parts.push_back(str);
        _estimatedSize += str.length();
        return *this;
    }
    
    StringBuilder& append(const char* str) {
        if (str) {
            _parts.push_back(String(str));
            _estimatedSize += strlen(str);
        }
        return *this;
    }
    
    StringBuilder& append(const __FlashStringHelper* str) {
        _parts.push_back(String(str));
        _estimatedSize += strlen_P(reinterpret_cast<const char*>(str));
        return *this;
    }
    
    StringBuilder& appendFormat(const char* format, ...) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        return append(buffer);
    }
    
    String toString() const {
        String result;
        result.reserve(_estimatedSize + 16); // Add some padding
        
        for (const auto& part : _parts) {
            result += part;
        }
        
        return result;
    }
    
    void writeTo(Print& output) const {
        for (const auto& part : _parts) {
            output.print(part);
        }
    }
    
    void clear() {
        _parts.clear();
        _estimatedSize = 0;
    }
    
    size_t estimatedSize() const {
        return _estimatedSize;
    }
    
    bool isEmpty() const {
        return _parts.empty();
    }
};

/**
 * Secure string that zeros memory on destruction
 */
class SecureString {
private:
    char* _data;
    size_t _capacity;
    size_t _length;
    
public:
    explicit SecureString(size_t capacity = 64) 
        : _data(nullptr), _capacity(capacity), _length(0) {
        _data = new char[_capacity];
        memset(_data, 0, _capacity);
    }
    
    ~SecureString() {
        if (_data) {
            // Zero out memory before deallocation
            memset(_data, 0, _capacity);
            delete[] _data;
        }
    }
    
    // Non-copyable for security
    SecureString(const SecureString&) = delete;
    SecureString& operator=(const SecureString&) = delete;
    
    bool set(const String& str) {
        if (str.length() >= _capacity) {
            return false;
        }
        memset(_data, 0, _capacity);
        strncpy(_data, str.c_str(), str.length());
        _length = str.length();
        return true;
    }
    
    const char* c_str() const {
        return _data;
    }
    
    size_t length() const {
        return _length;
    }
    
    bool isEmpty() const {
        return _length == 0;
    }
    
    void clear() {
        memset(_data, 0, _capacity);
        _length = 0;
    }
};

/**
 * Memory pool for reducing fragmentation
 */
class MemoryPool {
private:
    uint8_t* _buffer;
    size_t _size;
    size_t _offset;
    
public:
    explicit MemoryPool(size_t size) 
        : _buffer(nullptr), _size(size), _offset(0) {
        _buffer = new uint8_t[_size];
        if (!_buffer) {
            AC_DBG("Failed to allocate memory pool of size %u\n", _size);
            _size = 0;
        }
    }
    
    ~MemoryPool() {
        delete[] _buffer;
    }
    
    void* allocate(size_t bytes) {
        // Align to 4-byte boundaries
        size_t aligned_bytes = (bytes + 3) & ~3;
        
        if (_offset + aligned_bytes > _size) {
            AC_DBG("Memory pool exhausted: requested %u, available %u\n", 
                   aligned_bytes, _size - _offset);
            return nullptr;
        }
        
        void* ptr = _buffer + _offset;
        _offset += aligned_bytes;
        return ptr;
    }
    
    void reset() {
        _offset = 0;
        memset(_buffer, 0, _size);
    }
    
    size_t available() const {
        return _size - _offset;
    }
    
    size_t used() const {
        return _offset;
    }
    
    bool isValid() const {
        return _buffer != nullptr;
    }
};

/**
 * Timeout helper with automatic checking
 */
class TimeoutHelper {
private:
    unsigned long _startTime;
    unsigned long _timeout;
    
public:
    TimeoutHelper(unsigned long timeoutMs) 
        : _startTime(millis()), _timeout(timeoutMs) {}
    
    bool isExpired() const {
        return (millis() - _startTime) >= _timeout;
    }
    
    unsigned long elapsed() const {
        return millis() - _startTime;
    }
    
    unsigned long remaining() const {
        unsigned long elapsed = this->elapsed();
        return elapsed >= _timeout ? 0 : (_timeout - elapsed);
    }
    
    void restart() {
        _startTime = millis();
    }
};

/**
 * Input sanitization utilities
 */
namespace InputSanitizer {
    String sanitizeHTML(const String& input) {
        String clean = input;
        clean.replace("&", "&amp;");
        clean.replace("<", "&lt;");
        clean.replace(">", "&gt;");
        clean.replace("\"", "&quot;");
        clean.replace("'", "&#x27;");
        return clean;
    }
    
    String sanitizeFilename(const String& input) {
        String clean;
        clean.reserve(input.length());
        
        for (size_t i = 0; i < input.length(); i++) {
            char c = input[i];
            if (isalnum(c) || c == '_' || c == '-' || c == '.') {
                clean += c;
            } else {
                clean += '_';
            }
        }
        
        // Ensure filename isn't empty and doesn't start with dot
        if (clean.isEmpty() || clean[0] == '.') {
            clean = "file_" + clean;
        }
        
        return clean;
    }
    
    bool isValidSSID(const String& ssid) {
        return ssid.length() > 0 && ssid.length() <= 32;
    }
    
    bool isValidPassword(const String& password) {
        return password.length() == 0 || (password.length() >= 8 && password.length() <= 63);
    }
    
    bool isValidHostname(const String& hostname) {
        if (hostname.length() == 0 || hostname.length() > 63) {
            return false;
        }
        
        for (size_t i = 0; i < hostname.length(); i++) {
            char c = hostname[i];
            if (!isalnum(c) && c != '-') {
                return false;
            }
        }
        
        return hostname[0] != '-' && hostname[hostname.length() - 1] != '-';
    }
}

#endif // _AUTOCONNECTRAII_H_
