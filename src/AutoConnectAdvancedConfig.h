/**
 * Advanced configuration structure for AutoConnect with enhanced features.
 * @file AutoConnectAdvancedConfig.h
 * @author AutoConnect Contributors
 * @version 1.4.3
 * @date 2025-08-30
 * @copyright MIT license.
 */

#ifndef _AUTOCONNECTADVANCEDCONFIG_H_
#define _AUTOCONNECTADVANCEDCONFIG_H_

#include "AutoConnectConfigBase.h"
#include "AutoConnectError.h"

/**
 * Feature flags to enable/disable functionality for memory optimization
 */
#define AC_FEATURE_OTA          (1U << 0)
#define AC_FEATURE_UPDATE       (1U << 1)
#define AC_FEATURE_FILESYSTEM   (1U << 2)
#define AC_FEATURE_JSON         (1U << 3)
#define AC_FEATURE_CREDENTIALS  (1U << 4)
#define AC_FEATURE_PORTAL       (1U << 5)
#define AC_FEATURE_TICKER       (1U << 6)
#define AC_FEATURE_DEBUG        (1U << 7)

// Default features for most use cases
#define AC_FEATURES_DEFAULT     (AC_FEATURE_CREDENTIALS | AC_FEATURE_PORTAL | AC_FEATURE_JSON)
#define AC_FEATURES_MINIMAL     (AC_FEATURE_CREDENTIALS | AC_FEATURE_PORTAL)
#define AC_FEATURES_FULL        (0xFFU)

/**
 * Network configuration with validation
 */
struct NetworkConfig {
    String ssid;
    String password;
    String hostname;
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;
    bool useStaticIP;
    bool validateCertificates;
    uint32_t connectionTimeoutMs;
    uint8_t maxRetries;
    
    NetworkConfig() 
        : useStaticIP(false)
        , validateCertificates(false)
        , connectionTimeoutMs(30000)
        , maxRetries(3) {}
    
    ACResult validate() const {
        if (!InputSanitizer::isValidSSID(ssid)) {
            return ACResult(ACError::INVALID_PARAMETER, "Invalid SSID");
        }
        
        if (!InputSanitizer::isValidPassword(password)) {
            return ACResult(ACError::INVALID_PARAMETER, "Invalid password");
        }
        
        if (!hostname.isEmpty() && !InputSanitizer::isValidHostname(hostname)) {
            return ACResult(ACError::INVALID_PARAMETER, "Invalid hostname");
        }
        
        if (connectionTimeoutMs < 5000 || connectionTimeoutMs > 300000) {
            return ACResult(ACError::INVALID_PARAMETER, "Connection timeout out of range (5-300 seconds)");
        }
        
        return ACResult(ACError::SUCCESS);
    }
};

/**
 * Portal configuration with security settings
 */
struct PortalConfig {
    String apSSID;
    String apPassword;
    IPAddress apIP;
    IPAddress apGateway;
    IPAddress apSubnet;
    uint8_t channel;
    bool hidden;
    bool enableAuth;
    String authRealm;
    String authUsername;
    String authPassword;
    uint32_t timeoutMs;
    uint16_t port;
    
    PortalConfig()
        : apIP(172, 217, 28, 1)
        , apGateway(172, 217, 28, 1)
        , apSubnet(255, 255, 255, 0)
        , channel(1)
        , hidden(false)
        , enableAuth(false)
        , authRealm("AutoConnect")
        , timeoutMs(0)
        , port(80) {}
    
    ACResult validate() const {
        if (!InputSanitizer::isValidSSID(apSSID)) {
            return ACResult(ACError::INVALID_PARAMETER, "Invalid AP SSID");
        }
        
        if (!InputSanitizer::isValidPassword(apPassword)) {
            return ACResult(ACError::INVALID_PARAMETER, "Invalid AP password");
        }
        
        if (channel < 1 || channel > 13) {
            return ACResult(ACError::INVALID_PARAMETER, "Invalid WiFi channel (1-13)");
        }
        
        if (port < 80 || port > 65535) {
            return ACResult(ACError::INVALID_PARAMETER, "Invalid port number");
        }
        
        return ACResult(ACError::SUCCESS);
    }
};

/**
 * Memory management configuration
 */
struct MemoryConfig {
    size_t jsonBufferSize;
    size_t maxStringLength;
    size_t lowMemoryThreshold;
    bool enableMemoryMonitoring;
    bool enableGarbageCollection;
    uint32_t gcIntervalMs;
    
    MemoryConfig()
        : jsonBufferSize(8192)
        , maxStringLength(4096)
        , lowMemoryThreshold(4096)
        , enableMemoryMonitoring(true)
        , enableGarbageCollection(false)
        , gcIntervalMs(30000) {}
    
    ACResult validate() const {
        if (jsonBufferSize < 1024 || jsonBufferSize > 32768) {
            return ACResult(ACError::INVALID_PARAMETER, "JSON buffer size out of range (1-32KB)");
        }
        
        if (maxStringLength > jsonBufferSize / 2) {
            return ACResult(ACError::INVALID_PARAMETER, "Max string length too large for JSON buffer");
        }
        
        return ACResult(ACError::SUCCESS);
    }
};

/**
 * Security configuration
 */
struct SecurityConfig {
    bool enableInputSanitization;
    bool enableCSRFProtection;
    bool enableRateLimiting;
    uint32_t maxRequestsPerMinute;
    bool logSecurityEvents;
    bool strictSSL;
    
    SecurityConfig()
        : enableInputSanitization(true)
        , enableCSRFProtection(false)
        , enableRateLimiting(false)
        , maxRequestsPerMinute(60)
        , logSecurityEvents(true)
        , strictSSL(false) {}
};

/**
 * Debug and logging configuration
 */
struct DebugConfig {
    bool enableSerial;
    bool enableFile;
    String logFilePath;
    size_t maxLogFileSize;
    uint8_t logLevel; // 0=Error, 1=Warn, 2=Info, 3=Debug, 4=Trace
    bool timestampLogs;
    bool memoryStats;
    
    DebugConfig()
        : enableSerial(false)
        , enableFile(false)
        , logFilePath("/autoconnect.log")
        , maxLogFileSize(1024 * 1024)
        , logLevel(2)
        , timestampLogs(true)
        , memoryStats(false) {}
};

/**
 * Advanced AutoConnect configuration extending the base
 */
class AutoConnectAdvancedConfig : public AutoConnectConfigBase {
public:
    // Feature control
    uint32_t enabledFeatures;
    
    // Configuration sections
    NetworkConfig network;
    PortalConfig portal;
    MemoryConfig memory;
    SecurityConfig security;
    DebugConfig debug;
    
    // Performance settings
    uint32_t taskStackSize;
    uint8_t taskPriority;
    uint32_t watchdogTimeoutMs;
    bool enableDeepSleep;
    uint32_t deepSleepDurationUs;
    
    // File system settings
    bool formatFSOnFail;
    size_t maxFileSize;
    uint16_t maxFiles;
    
    AutoConnectAdvancedConfig(uint32_t features = AC_FEATURES_DEFAULT)
        : AutoConnectConfigBase()
        , enabledFeatures(features)
        , taskStackSize(4096)
        , taskPriority(1)
        , watchdogTimeoutMs(30000)
        , enableDeepSleep(false)
        , deepSleepDurationUs(30000000) // 30 seconds
        , formatFSOnFail(false)
        , maxFileSize(1024 * 1024)
        , maxFiles(50)
    {
        // Set reasonable defaults for portal
        portal.apSSID = String(F(AUTOCONNECT_APID));
        portal.apPassword = String(F(AUTOCONNECT_PSK));
    }
    
    /**
     * Validate the entire configuration
     */
    ACResult validate() const {
        ACResult result;
        
        result = network.validate();
        if (!result) return result;
        
        result = portal.validate();
        if (!result) return result;
        
        result = memory.validate();
        if (!result) return result;
        
        // Cross-validation
        if (enabledFeatures & AC_FEATURE_FILESYSTEM) {
            if (maxFileSize < 1024) {
                return ACResult(ACError::INVALID_PARAMETER, "Max file size too small");
            }
        }
        
        if (enabledFeatures & AC_FEATURE_DEBUG) {
            if (debug.enableFile && !(enabledFeatures & AC_FEATURE_FILESYSTEM)) {
                return ACResult(ACError::INVALID_PARAMETER, "File logging requires filesystem feature");
            }
        }
        
        return ACResult(ACError::SUCCESS);
    }
    
    /**
     * Check if a feature is enabled
     */
    bool hasFeature(uint32_t feature) const {
        return (enabledFeatures & feature) != 0;
    }
    
    /**
     * Enable a feature
     */
    void enableFeature(uint32_t feature) {
        enabledFeatures |= feature;
    }
    
    /**
     * Disable a feature
     */
    void disableFeature(uint32_t feature) {
        enabledFeatures &= ~feature;
    }
    
    /**
     * Get memory usage estimate for this configuration
     */
    size_t estimateMemoryUsage() const {
        size_t usage = sizeof(*this);
        
        usage += memory.jsonBufferSize;
        
        if (hasFeature(AC_FEATURE_FILESYSTEM)) {
            usage += 2048; // FS overhead
        }
        
        if (hasFeature(AC_FEATURE_OTA)) {
            usage += 4096; // OTA overhead
        }
        
        if (hasFeature(AC_FEATURE_PORTAL)) {
            usage += 8192; // Web server overhead
        }
        
        return usage;
    }
    
    /**
     * Apply configuration to base class for compatibility
     */
    void applyToBase() {
        // Copy relevant settings to base class
        if (!portal.apSSID.isEmpty()) {
            apid = portal.apSSID;
        }
        if (!portal.apPassword.isEmpty()) {
            psk = portal.apPassword;
        }
        
        apip = static_cast<uint32_t>(portal.apIP);
        gateway = static_cast<uint32_t>(portal.apGateway);
        netmask = static_cast<uint32_t>(portal.apSubnet);
        channel = portal.channel;
        hidden = portal.hidden ? 1 : 0;
        
        if (!network.hostname.isEmpty()) {
            hostName = network.hostname;
        }
        
        beginTimeout = network.connectionTimeoutMs;
        portalTimeout = portal.timeoutMs;
        
        ticker = hasFeature(AC_FEATURE_TICKER);
    }
};

#endif // _AUTOCONNECTADVANCEDCONFIG_H_
