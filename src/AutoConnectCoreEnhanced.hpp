/**
 * Enhanced AutoConnect core implementation with improved error handling and modern C++.
 * @file AutoConnectCoreEnhanced.hpp
 * @author AutoConnect Contributors
 * @version 1.4.3
 * @date 2025-08-30
 * @copyright MIT license.
 */

#ifndef _AUTOCONNECTCOREENHANCED_HPP_
#define _AUTOCONNECTCOREENHANCED_HPP_

#include "AutoConnectCore.hpp"

/**
 * Enhanced begin method with comprehensive error reporting
 */
template<typename T>
ACResult AutoConnectCore<T>::beginWithResult(void) {
    return beginWithResult(nullptr, nullptr, _apConfig.beginTimeout);
}

/**
 * Enhanced begin with detailed error information
 */
template<typename T>
ACResult AutoConnectCore<T>::beginWithResult(const char* ssid, const char* passphrase, unsigned long timeout) {
    AC_DBG("Enhanced begin called with SSID: %s\n", ssid ? ssid : "nullptr");
    
    // Check memory before starting
    if (!_checkMemoryAvailable(8192)) {
        return ACResult(ACError::MEMORY_INSUFFICIENT, "Insufficient memory to start AutoConnect");
    }
    
    // Validate parameters
    if (ssid) {
        ACResult validation = _validateSSID(String(ssid));
        if (!validation) return validation;
    }
    
    if (passphrase) {
        ACResult validation = _validatePassword(String(passphrase));
        if (!validation) return validation;
    }
    
    if (timeout > 300000) { // 5 minutes max
        return ACResult(ACError::INVALID_PARAMETER, "Timeout too large (max 5 minutes)");
    }
    
    // Update memory stats before operation
    _updateMemoryStats();
    
    // Call the original begin method
    bool success = begin(ssid, passphrase, timeout);
    
    if (!success) {
        // Determine the specific error based on portal status
        uint8_t status = portalStatus();
        
        if (status & AC_TIMEOUT) {
            return ACResult(ACError::WIFI_TIMEOUT, "WiFi connection timeout");
        } else if (status & AC_CAPTIVEPORTAL) {
            return ACResult(ACError::SUCCESS, "Captive portal started"); // This is actually success for portal mode
        } else {
            return ACResult(ACError::WIFI_CONNECT_FAILED, "WiFi connection failed");
        }
    }
    
    return ACResult(ACError::SUCCESS, "WiFi connection established");
}

/**
 * Enhanced configuration with validation
 */
template<typename T>
ACResult AutoConnectCore<T>::configWithValidation(T& config) {
    // Basic validation - this would be expanded based on config type
    std::lock_guard<std::mutex> lock(_configMutex);
    
    // Check memory impact
    if (!_checkMemoryAvailable(1024)) {
        return ACResult(ACError::MEMORY_INSUFFICIENT, "Insufficient memory for configuration");
    }
    
    // Store old config for rollback
    T oldConfig = _apConfig;
    
    // Apply new configuration
    bool success = this->config(config);
    
    if (!success) {
        // Rollback on failure
        _apConfig = oldConfig;
        return ACResult(ACError::INVALID_PARAMETER, "Configuration validation failed");
    }
    
    return ACResult(ACError::SUCCESS, "Configuration applied successfully");
}

/**
 * Connect to WiFi with detailed configuration
 */
template<typename T>
ACResult AutoConnectCore<T>::connectToWiFi(const NetworkConfig& networkConfig) {
    // Validate network configuration
    ACResult validation = networkConfig.validate();
    if (!validation) return validation;
    
    AC_DBG("Connecting to WiFi: %s\n", networkConfig.ssid.c_str());
    
    // Check memory
    if (!_checkMemoryAvailable(4096)) {
        return ACResult(ACError::MEMORY_INSUFFICIENT, "Insufficient memory for WiFi connection");
    }
    
    // Set hostname if provided
    if (!networkConfig.hostname.isEmpty()) {
        ACResult hostnameResult = setHostname(networkConfig.hostname);
        if (!hostnameResult) {
            AC_DBG("Warning: Failed to set hostname: %s\n", hostnameResult.message.c_str());
        }
    }
    
    // Configure static IP if requested
    if (networkConfig.useStaticIP) {
        ACResult ipResult = setStaticIP(networkConfig.staticIP, networkConfig.gateway, networkConfig.subnet);
        if (!ipResult) return ipResult;
        
        if (networkConfig.dns1.isSet()) {
            ACResult dnsResult = setDNS(networkConfig.dns1, networkConfig.dns2);
            if (!dnsResult) return dnsResult;
        }
    }
    
    // Attempt connection with retry logic
    TimeoutHelper timeout(networkConfig.connectionTimeoutMs);
    uint8_t retries = 0;
    
    while (retries < networkConfig.maxRetries && !timeout.isExpired()) {
        AC_DBG("Connection attempt %d/%d\n", retries + 1, networkConfig.maxRetries);
        
        ACResult result = beginWithResult(networkConfig.ssid.c_str(), 
                                         networkConfig.password.c_str(), 
                                         networkConfig.connectionTimeoutMs);
        
        if (result) {
            AC_DBG("WiFi connected successfully\n");
            return result;
        }
        
        retries++;
        if (retries < networkConfig.maxRetries) {
            delay(1000); // Wait between retries
        }
    }
    
    return ACResult(ACError::WIFI_CONNECT_FAILED, 
                   String("Failed to connect after ") + String(retries) + " attempts");
}

/**
 * Start captive portal with configuration
 */
template<typename T>
ACResult AutoConnectCore<T>::startCaptivePortal(const PortalConfig& portalConfig) {
    // Validate portal configuration
    ACResult validation = portalConfig.validate();
    if (!validation) return validation;
    
    AC_DBG("Starting captive portal: %s\n", portalConfig.apSSID.c_str());
    
    // Check memory for portal operations
    if (!_checkMemoryAvailable(8192)) {
        return ACResult(ACError::MEMORY_INSUFFICIENT, "Insufficient memory for captive portal");
    }
    
    // Configure portal settings
    std::lock_guard<std::mutex> lock(_configMutex);
    
    _apConfig.apid = portalConfig.apSSID;
    _apConfig.psk = portalConfig.apPassword;
    _apConfig.apip = static_cast<uint32_t>(portalConfig.apIP);
    _apConfig.gateway = static_cast<uint32_t>(portalConfig.apGateway);
    _apConfig.netmask = static_cast<uint32_t>(portalConfig.apSubnet);
    _apConfig.channel = portalConfig.channel;
    _apConfig.hidden = portalConfig.hidden ? 1 : 0;
    _apConfig.portalTimeout = portalConfig.timeoutMs;
    
    // Configure authentication if enabled
    if (portalConfig.enableAuth) {
        _apConfig.auth = AC_AUTH_DIGEST;
        _apConfig.username = portalConfig.authUsername;
        _apConfig.password = portalConfig.authPassword;
    }
    
    // Start portal
    bool success = begin();
    
    if (!success) {
        return ACResult(ACError::PORTAL_START_FAILED, "Failed to start captive portal");
    }
    
    return ACResult(ACError::SUCCESS, "Captive portal started successfully");
}

/**
 * Set hostname with validation
 */
template<typename T>
ACResult AutoConnectCore<T>::setHostname(const String& hostname) {
    ACResult validation = _validateHostname(hostname);
    if (!validation) return validation;
    
    AC_DBG("Setting hostname: %s\n", hostname.c_str());
    
    std::lock_guard<std::mutex> lock(_configMutex);
    _apConfig.hostName = hostname;
    
    // Apply hostname to WiFi
    SET_HOSTNAME(hostname.c_str());
    
    return ACResult(ACError::SUCCESS, "Hostname set successfully");
}

/**
 * Configure static IP
 */
template<typename T>
ACResult AutoConnectCore<T>::setStaticIP(const IPAddress& ip, const IPAddress& gateway, const IPAddress& subnet) {
    if (!ip.isSet() || !gateway.isSet() || !subnet.isSet()) {
        return ACResult(ACError::INVALID_PARAMETER, "Invalid IP configuration");
    }
    
    AC_DBG("Setting static IP: %s\n", ip.toString().c_str());
    
    std::lock_guard<std::mutex> lock(_configMutex);
    _apConfig.staip = static_cast<uint32_t>(ip);
    _apConfig.staGateway = static_cast<uint32_t>(gateway);
    _apConfig.staNetmask = static_cast<uint32_t>(subnet);
    
    return ACResult(ACError::SUCCESS, "Static IP configured");
}

/**
 * Configure DNS servers
 */
template<typename T>
ACResult AutoConnectCore<T>::setDNS(const IPAddress& dns1, const IPAddress& dns2) {
    if (!dns1.isSet()) {
        return ACResult(ACError::INVALID_PARAMETER, "Primary DNS cannot be empty");
    }
    
    AC_DBG("Setting DNS: %s, %s\n", dns1.toString().c_str(), 
           dns2.isSet() ? dns2.toString().c_str() : "none");
    
    std::lock_guard<std::mutex> lock(_configMutex);
    _apConfig.dns1 = static_cast<uint32_t>(dns1);
    if (dns2.isSet()) {
        _apConfig.dns2 = static_cast<uint32_t>(dns2);
    }
    
    return ACResult(ACError::SUCCESS, "DNS configured");
}

/**
 * Get current memory statistics
 */
template<typename T>
ACMemoryStats AutoConnectCore<T>::getMemoryStats() const {
    std::lock_guard<std::mutex> lock(_memoryMutex);
    ACMemoryStats stats = _memoryStats;
    stats.update(); // Get current values
    return stats;
}

/**
 * Log current memory usage
 */
template<typename T>
void AutoConnectCore<T>::logMemoryUsage() const {
    ACMemoryStats stats = getMemoryStats();
    AC_DBG("Memory stats: %s\n", stats.toString().c_str());
}

/**
 * Check if system is low on memory
 */
template<typename T>
bool AutoConnectCore<T>::isLowMemory(size_t threshold) const {
    return ESP.getFreeHeap() < threshold;
}

/**
 * Get current credential safely
 */
template<typename T>
bool AutoConnectCore<T>::getCurrentCredential(station_config_t* staConfig) const {
    if (!staConfig) return false;
    
    std::lock_guard<std::mutex> lock(_credentialMutex);
    memcpy(staConfig, &_credential, sizeof(station_config_t));
    return true;
}

/**
 * Get EEPROM used size
 */
template<typename T>
uint16_t AutoConnectCore<T>::getEEPROMUsedSize(void) const {
    return sizeof(station_config_t) + sizeof(T);
}

/**
 * Validate SSID
 */
template<typename T>
ACResult AutoConnectCore<T>::_validateSSID(const String& ssid) const {
    if (!InputSanitizer::isValidSSID(ssid)) {
        return ACResult(ACError::INVALID_PARAMETER, 
                       String("Invalid SSID: ") + ssid + " (length: " + String(ssid.length()) + ")");
    }
    return ACResult(ACError::SUCCESS);
}

/**
 * Validate password
 */
template<typename T>
ACResult AutoConnectCore<T>::_validatePassword(const String& password) const {
    if (!InputSanitizer::isValidPassword(password)) {
        return ACResult(ACError::INVALID_PARAMETER, 
                       String("Invalid password length: ") + String(password.length()));
    }
    return ACResult(ACError::SUCCESS);
}

/**
 * Validate hostname
 */
template<typename T>
ACResult AutoConnectCore<T>::_validateHostname(const String& hostname) const {
    if (!InputSanitizer::isValidHostname(hostname)) {
        return ACResult(ACError::INVALID_PARAMETER, 
                       String("Invalid hostname: ") + hostname);
    }
    return ACResult(ACError::SUCCESS);
}

/**
 * Check if enough memory is available
 */
template<typename T>
bool AutoConnectCore<T>::_checkMemoryAvailable(size_t required) const {
    size_t available = ESP.getFreeHeap();
    bool sufficient = available >= required;
    
    if (!sufficient) {
        AC_DBG("Insufficient memory: required %u, available %u\n", required, available);
    }
    
    return sufficient;
}

/**
 * Update memory statistics
 */
template<typename T>
void AutoConnectCore<T>::_updateMemoryStats() const {
    std::lock_guard<std::mutex> lock(_memoryMutex);
    _memoryStats.update();
    _freeHeapSize = _memoryStats.freeHeap;
}

/**
 * Build HTML using StringBuilder to avoid fragmentation
 */
template<typename T>
String AutoConnectCore<T>::_buildHTML(const std::vector<String>& parts) const {
    StringBuilder builder;
    
    for (const auto& part : parts) {
        builder.append(part);
    }
    
    return builder.toString();
}

#endif // _AUTOCONNECTCOREENHANCED_HPP_
