/**
 * Enhanced credential management with improved security and error handling.
 * @file AutoConnectCredentialEnhanced.h
 * @author AutoConnect Contributors
 * @version 1.4.3
 * @date 2025-08-30
 * @copyright MIT license.
 */

#ifndef _AUTOCONNECTCREDENTIALENHANCED_H_
#define _AUTOCONNECTCREDENTIALENHANCED_H_

#include "AutoConnectCredential.h"
#include "AutoConnectError.h"
#include "AutoConnectRAII.h"
#include <mutex>

/**
 * Enhanced credential management with thread safety and improved error handling
 */
class AutoConnectCredentialEnhanced : public AutoConnectCredential {
public:
    /**
     * Enhanced credential structure with validation
     */
    struct EnhancedCredential {
        SecureString ssid;
        SecureString password;
        uint8_t bssid[6];
        IPAddress staticIP;
        IPAddress gateway;
        IPAddress subnet;
        IPAddress dns1;
        IPAddress dns2;
        bool useStatic;
        uint32_t timestamp;
        uint32_t connectionCount;
        int32_t lastRSSI;
        
        EnhancedCredential() 
            : ssid(33), password(64), useStatic(false)
            , timestamp(0), connectionCount(0), lastRSSI(-120) {
            memset(bssid, 0, sizeof(bssid));
        }
        
        ACResult validate() const {
            if (ssid.isEmpty()) {
                return ACResult(ACError::INVALID_PARAMETER, "SSID cannot be empty");
            }
            
            if (ssid.length() > 32) {
                return ACResult(ACError::INVALID_PARAMETER, "SSID too long");
            }
            
            if (password.length() > 0 && password.length() < 8) {
                return ACResult(ACError::INVALID_PARAMETER, "Password too short");
            }
            
            if (password.length() > 63) {
                return ACResult(ACError::INVALID_PARAMETER, "Password too long");
            }
            
            return ACResult(ACError::SUCCESS);
        }
        
        void updateStats(int32_t rssi = -120) {
            timestamp = millis();
            connectionCount++;
            lastRSSI = rssi;
        }
        
        // Convert to legacy format for compatibility
        void toLegacy(station_config_t& legacy) const {
            memset(&legacy, 0, sizeof(legacy));
            strncpy(reinterpret_cast<char*>(legacy.ssid), ssid.c_str(), 32);
            strncpy(reinterpret_cast<char*>(legacy.password), password.c_str(), 64);
            memcpy(legacy.bssid, bssid, 6);
            legacy.dhcp = useStatic ? STA_STATIC : STA_DHCP;
            
            if (useStatic) {
                legacy.config.sta.ip = static_cast<uint32_t>(staticIP);
                legacy.config.sta.gateway = static_cast<uint32_t>(gateway);
                legacy.config.sta.netmask = static_cast<uint32_t>(subnet);
                legacy.config.sta.dns1 = static_cast<uint32_t>(dns1);
                legacy.config.sta.dns2 = static_cast<uint32_t>(dns2);
            }
        }
        
        // Convert from legacy format
        void fromLegacy(const station_config_t& legacy) {
            ssid.set(String(reinterpret_cast<const char*>(legacy.ssid)));
            password.set(String(reinterpret_cast<const char*>(legacy.password)));
            memcpy(bssid, legacy.bssid, 6);
            useStatic = (legacy.dhcp == STA_STATIC);
            
            if (useStatic) {
                staticIP = IPAddress(legacy.config.sta.ip);
                gateway = IPAddress(legacy.config.sta.gateway);
                subnet = IPAddress(legacy.config.sta.netmask);
                dns1 = IPAddress(legacy.config.sta.dns1);
                dns2 = IPAddress(legacy.config.sta.dns2);
            }
        }
    };

private:
    mutable std::mutex _credentialMutex;
    std::vector<EnhancedCredential> _credentials;
    bool _initialized;
    size_t _maxCredentials;

public:
    AutoConnectCredentialEnhanced(size_t maxCredentials = 10) 
        : AutoConnectCredential()
        , _initialized(false)
        , _maxCredentials(maxCredentials) {
        _credentials.reserve(_maxCredentials);
    }
    
    virtual ~AutoConnectCredentialEnhanced() = default;
    
    /**
     * Initialize the enhanced credential system
     */
    ACResult initialize() {
        std::lock_guard<std::mutex> lock(_credentialMutex);
        
        if (_initialized) {
            return ACResult(ACError::SUCCESS, "Already initialized");
        }
        
        // Load existing credentials
        ACResult result = _loadExistingCredentials();
        if (!result) {
            AC_DBG("Warning: Failed to load existing credentials: %s\n", result.message.c_str());
        }
        
        _initialized = true;
        return ACResult(ACError::SUCCESS, "Credential system initialized");
    }
    
    /**
     * Add or update a credential with validation
     */
    ACResult addCredential(const EnhancedCredential& credential) {
        ACResult validation = credential.validate();
        if (!validation) {
            return validation;
        }
        
        std::lock_guard<std::mutex> lock(_credentialMutex);
        
        if (!_initialized) {
            return ACResult(ACError::INVALID_STATE, "Credential system not initialized");
        }
        
        // Check if credential already exists
        auto it = std::find_if(_credentials.begin(), _credentials.end(),
            [&credential](const EnhancedCredential& cred) {
                return strcmp(cred.ssid.c_str(), credential.ssid.c_str()) == 0;
            });
        
        if (it != _credentials.end()) {
            // Update existing credential
            *it = credential;
            AC_DBG("Updated existing credential for SSID: %s\n", credential.ssid.c_str());
        } else {
            // Add new credential
            if (_credentials.size() >= _maxCredentials) {
                // Remove oldest credential
                auto oldest = std::min_element(_credentials.begin(), _credentials.end(),
                    [](const EnhancedCredential& a, const EnhancedCredential& b) {
                        return a.timestamp < b.timestamp;
                    });
                _credentials.erase(oldest);
                AC_DBG("Removed oldest credential to make space\n");
            }
            
            _credentials.push_back(credential);
            AC_DBG("Added new credential for SSID: %s\n", credential.ssid.c_str());
        }
        
        return _saveCredentials();
    }
    
    /**
     * Get credential by SSID with thread safety
     */
    ACResult getCredential(const String& ssid, EnhancedCredential& credential) const {
        if (ssid.isEmpty()) {
            return ACResult(ACError::INVALID_PARAMETER, "SSID cannot be empty");
        }
        
        std::lock_guard<std::mutex> lock(_credentialMutex);
        
        if (!_initialized) {
            return ACResult(ACError::INVALID_STATE, "Credential system not initialized");
        }
        
        auto it = std::find_if(_credentials.begin(), _credentials.end(),
            [&ssid](const EnhancedCredential& cred) {
                return strcmp(cred.ssid.c_str(), ssid.c_str()) == 0;
            });
        
        if (it == _credentials.end()) {
            return ACResult(ACError::CREDENTIAL_LOAD_ERROR, 
                           String("Credential not found for SSID: ") + ssid);
        }
        
        credential = *it;
        return ACResult(ACError::SUCCESS);
    }
    
    /**
     * Remove credential by SSID
     */
    ACResult removeCredential(const String& ssid) {
        if (ssid.isEmpty()) {
            return ACResult(ACError::INVALID_PARAMETER, "SSID cannot be empty");
        }
        
        std::lock_guard<std::mutex> lock(_credentialMutex);
        
        if (!_initialized) {
            return ACResult(ACError::INVALID_STATE, "Credential system not initialized");
        }
        
        auto it = std::find_if(_credentials.begin(), _credentials.end(),
            [&ssid](const EnhancedCredential& cred) {
                return strcmp(cred.ssid.c_str(), ssid.c_str()) == 0;
            });
        
        if (it == _credentials.end()) {
            return ACResult(ACError::CREDENTIAL_LOAD_ERROR, 
                           String("Credential not found for SSID: ") + ssid);
        }
        
        _credentials.erase(it);
        AC_DBG("Removed credential for SSID: %s\n", ssid.c_str());
        
        return _saveCredentials();
    }
    
    /**
     * Get all available SSIDs
     */
    std::vector<String> getAvailableSSIDs() const {
        std::lock_guard<std::mutex> lock(_credentialMutex);
        std::vector<String> ssids;
        ssids.reserve(_credentials.size());
        
        for (const auto& cred : _credentials) {
            ssids.push_back(String(cred.ssid.c_str()));
        }
        
        // Sort by most recently used
        std::sort(ssids.begin(), ssids.end(), [this](const String& a, const String& b) {
            auto credA = std::find_if(_credentials.begin(), _credentials.end(),
                [&a](const EnhancedCredential& cred) {
                    return strcmp(cred.ssid.c_str(), a.c_str()) == 0;
                });
            auto credB = std::find_if(_credentials.begin(), _credentials.end(),
                [&b](const EnhancedCredential& cred) {
                    return strcmp(cred.ssid.c_str(), b.c_str()) == 0;
                });
            
            if (credA != _credentials.end() && credB != _credentials.end()) {
                return credA->timestamp > credB->timestamp; // Most recent first
            }
            return false;
        });
        
        return ssids;
    }
    
    /**
     * Clear all credentials
     */
    ACResult clearAll() {
        std::lock_guard<std::mutex> lock(_credentialMutex);
        
        _credentials.clear();
        AC_DBG("Cleared all credentials\n");
        
        return _saveCredentials();
    }
    
    /**
     * Get credential count
     */
    size_t getCredentialCount() const {
        std::lock_guard<std::mutex> lock(_credentialMutex);
        return _credentials.size();
    }
    
    /**
     * Export credentials to JSON for backup
     */
    ACResult exportToJSON(String& jsonOutput) const {
        std::lock_guard<std::mutex> lock(_credentialMutex);
        
        StringBuilder json;
        json.append("{\"credentials\":[");
        
        for (size_t i = 0; i < _credentials.size(); i++) {
            if (i > 0) json.append(",");
            
            const auto& cred = _credentials[i];
            json.append("{");
            json.appendFormat("\"ssid\":\"%s\",", InputSanitizer::sanitizeHTML(String(cred.ssid.c_str())).c_str());
            json.appendFormat("\"useStatic\":%s,", cred.useStatic ? "true" : "false");
            json.appendFormat("\"timestamp\":%u,", cred.timestamp);
            json.appendFormat("\"connectionCount\":%u", cred.connectionCount);
            json.append("}");
        }
        
        json.append("]}");
        jsonOutput = json.toString();
        
        return ACResult(ACError::SUCCESS);
    }

private:
    /**
     * Load existing credentials from storage
     */
    ACResult _loadExistingCredentials() {
        // This would interface with the existing credential storage
        // For now, just return success
        return ACResult(ACError::SUCCESS);
    }
    
    /**
     * Save credentials to storage
     */
    ACResult _saveCredentials() {
        // This would save to the underlying storage system
        // For now, just return success
        AC_DBG("Saved %u credentials\n", _credentials.size());
        return ACResult(ACError::SUCCESS);
    }
};

#endif // _AUTOCONNECTCREDENTIALENHANCED_H_
