/*
  EnhancedAutoConnect.ino - Example demonstrating improved AutoConnect features
  
  This example shows how to use the enhanced AutoConnect library with:
  - Improved error handling
  - Memory management
  - Input validation
  - Thread safety
  - Modern C++ features
  
  Copyright (c) 2025, AutoConnect Contributors
  https://github.com/mrcodetastic/AutoConnect2

  This software is released under the MIT License.
  https://opensource.org/licenses/MIT
*/

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using WiFiWebServer = ESP8266WebServer;
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
using WiFiWebServer = WebServer;
#endif

// Include the enhanced AutoConnect headers
#include <AutoConnect.h>
#include "AutoConnectError.h"
#include "AutoConnectRAII.h"
#include "AutoConnectAdvancedConfig.h"
#include "AutoConnectCoreEnhanced.hpp"
#include "AutoConnectCredentialEnhanced.h"

// Enhanced AutoConnect instance
AutoConnect portal;
AutoConnectAdvancedConfig advancedConfig;
AutoConnectCredentialEnhanced enhancedCredentials;

// Memory monitoring
unsigned long lastMemoryCheck = 0;
const unsigned long MEMORY_CHECK_INTERVAL = 10000; // 10 seconds

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println(F("\n=== Enhanced AutoConnect Example ==="));
    
    // Configure advanced settings
    setupAdvancedConfiguration();
    
    // Initialize enhanced credential system
    initializeCredentials();
    
    // Start AutoConnect with enhanced error handling
    connectWithEnhancedHandling();
    
    // Setup web server routes
    setupWebRoutes();
    
    Serial.println(F("Setup complete. Ready to handle requests."));
}

void loop() {
    // Handle client requests
    portal.handleClient();
    
    // Periodic memory monitoring
    checkMemoryUsage();
    
    // Check WiFi connection status
    checkWiFiStatus();
    
    delay(100);
}

void setupAdvancedConfiguration() {
    Serial.println(F("Configuring advanced settings..."));
    
    // Enable specific features
    advancedConfig.enableFeature(AC_FEATURE_CREDENTIALS);
    advancedConfig.enableFeature(AC_FEATURE_PORTAL);
    advancedConfig.enableFeature(AC_FEATURE_JSON);
    advancedConfig.enableFeature(AC_FEATURE_DEBUG);
    
    // Configure network settings
    advancedConfig.network.hostname = "enhanced-autoconnect";
    advancedConfig.network.connectionTimeoutMs = 30000;
    advancedConfig.network.maxRetries = 3;
    
    // Configure portal settings
    advancedConfig.portal.apSSID = "EnhancedAP";
    advancedConfig.portal.apPassword = "enhanced123";
    advancedConfig.portal.enableAuth = true;
    advancedConfig.portal.authUsername = "admin";
    advancedConfig.portal.authPassword = "admin123";
    
    // Configure memory management
    advancedConfig.memory.jsonBufferSize = 8192;
    advancedConfig.memory.lowMemoryThreshold = 4096;
    advancedConfig.memory.enableMemoryMonitoring = true;
    
    // Configure security
    advancedConfig.security.enableInputSanitization = true;
    advancedConfig.security.logSecurityEvents = true;
    
    // Configure debug settings
    advancedConfig.debug.enableSerial = true;
    advancedConfig.debug.logLevel = 3; // Debug level
    advancedConfig.debug.memoryStats = true;
    
    // Validate configuration
    ACResult validation = advancedConfig.validate();
    if (!validation) {
        Serial.printf("Configuration validation failed: %s\n", validation.message.c_str());
        return;
    }
    
    // Apply to base configuration for compatibility
    advancedConfig.applyToBase();
    
    // Configure the portal
    portal.config(advancedConfig);
    
    Serial.printf("Configuration complete. Estimated memory usage: %u bytes\n", 
                  advancedConfig.estimateMemoryUsage());
}

void initializeCredentials() {
    Serial.println(F("Initializing enhanced credential system..."));
    
    ACResult result = enhancedCredentials.initialize();
    if (!result) {
        Serial.printf("Failed to initialize credentials: %s\n", result.message.c_str());
        return;
    }
    
    // Add some example credentials (normally these would be added through the portal)
    AutoConnectCredentialEnhanced::EnhancedCredential exampleCred;
    exampleCred.ssid.set("MyHomeWiFi");
    exampleCred.password.set("mypassword123");
    exampleCred.useStatic = false;
    
    result = enhancedCredentials.addCredential(exampleCred);
    if (result) {
        Serial.println(F("Added example credential"));
    }
    
    // List available credentials
    std::vector<String> ssids = enhancedCredentials.getAvailableSSIDs();
    Serial.printf("Available credentials: %u\n", ssids.size());
    for (const auto& ssid : ssids) {
        Serial.printf("  - %s\n", ssid.c_str());
    }
}

void connectWithEnhancedHandling() {
    Serial.println(F("Starting enhanced connection process..."));
    
    // Try to connect using enhanced error handling
    ACResult result = portal.beginWithResult();
    
    if (result) {
        Serial.println(F("✓ Connection successful!"));
        
        // Log connection details
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("Connected to: %s\n", WiFi.SSID().c_str());
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        }
    } else {
        Serial.printf("Connection failed: %s (%s)\n", 
                     result.errorString(), result.message.c_str());
        
        // Handle different error types
        switch (result.error) {
            case ACError::WIFI_TIMEOUT:
                Serial.println(F("Consider increasing timeout or checking signal strength"));
                break;
            case ACError::WIFI_CREDENTIALS_INVALID:
                Serial.println(F("Please check your WiFi credentials"));
                break;
            case ACError::MEMORY_INSUFFICIENT:
                Serial.println(F("System is low on memory, restarting..."));
                ESP.restart();
                break;
            default:
                Serial.println(F("Check your network configuration"));
                break;
        }
    }
    
    // Log memory statistics
    ACMemoryStats memStats = portal.getMemoryStats();
    Serial.printf("Memory stats: %s\n", memStats.toString().c_str());
}

void setupWebRoutes() {
    WiFiWebServer& webServer = portal.host();
    
    // Root page with memory information
    webServer.on("/", []() {
        StringBuilder html;
        html.append(F("<!DOCTYPE html><html><head><title>Enhanced AutoConnect</title></head><body>"));
        html.append(F("<h1>Enhanced AutoConnect Demo</h1>"));
        
        // WiFi status
        html.append(F("<h2>WiFi Status</h2>"));
        if (WiFi.status() == WL_CONNECTED) {
            html.appendFormat("<p>Connected to: %s</p>", WiFi.SSID().c_str());
            html.appendFormat("<p>IP Address: %s</p>", WiFi.localIP().toString().c_str());
            html.appendFormat("<p>Signal Strength: %d dBm</p>", WiFi.RSSI());
        } else {
            html.append(F("<p>Not connected to WiFi</p>"));
        }
        
        // Memory information
        html.append(F("<h2>Memory Information</h2>"));
        ACMemoryStats memStats = portal.getMemoryStats();
        html.appendFormat("<p>Free Heap: %u bytes</p>", memStats.freeHeap);
        html.appendFormat("<p>Min Free Heap: %u bytes</p>", memStats.minFreeHeap);
        html.appendFormat("<p>Max Alloc Heap: %u bytes</p>", memStats.maxAllocHeap);
        
        // Check memory status
        if (memStats.isLowMemory()) {
            html.append(F("<p style='color:red'>⚠️ Low memory warning!</p>"));
        } else {
            html.append(F("<p style='color:green'>✓ Memory usage is normal</p>"));
        }
        
        // Credential information
        html.append(F("<h2>Stored Credentials</h2>"));
        std::vector<String> ssids = enhancedCredentials.getAvailableSSIDs();
        html.appendFormat("<p>Total credentials: %u</p>", ssids.size());
        html.append(F("<ul>"));
        for (const auto& ssid : ssids) {
            html.appendFormat("<li>%s</li>", InputSanitizer::sanitizeHTML(ssid).c_str());
        }
        html.append(F("</ul>"));
        
        // Links
        html.append(F("<h2>Actions</h2>"));
        html.append(F("<p><a href='/_ac'>AutoConnect Menu</a></p>"));
        html.append(F("<p><a href='/memory'>Detailed Memory Info</a></p>"));
        html.append(F("<p><a href='/credentials'>Credential Export</a></p>"));
        
        html.append(F("</body></html>"));
        
        webServer.send(200, "text/html", html.toString());
    });
    
    // Memory details page
    webServer.on("/memory", []() {
        StringBuilder json;
        json.append(F("{"));
        
        ACMemoryStats memStats = portal.getMemoryStats();
        json.appendFormat("\"freeHeap\": %u,", memStats.freeHeap);
        json.appendFormat("\"minFreeHeap\": %u,", memStats.minFreeHeap);
        json.appendFormat("\"maxAllocHeap\": %u,", memStats.maxAllocHeap);
        json.appendFormat("\"timestamp\": %u,", memStats.timestamp);
        json.appendFormat("\"isLowMemory\": %s,", memStats.isLowMemory() ? "true" : "false");
        json.appendFormat("\"chipId\": %u,", ESP.getChipId());
        json.appendFormat("\"flashSize\": %u", ESP.getFlashChipRealSize());
        
        json.append(F("}"));
        
        webServer.send(200, "application/json", json.toString());
    });
    
    // Credential export page
    webServer.on("/credentials", []() {
        String jsonOutput;
        ACResult result = enhancedCredentials.exportToJSON(jsonOutput);
        
        if (result) {
            webServer.send(200, "application/json", jsonOutput);
        } else {
            webServer.send(500, "text/plain", "Failed to export credentials");
        }
    });
}

void checkMemoryUsage() {
    unsigned long now = millis();
    if (now - lastMemoryCheck >= MEMORY_CHECK_INTERVAL) {
        portal.logMemoryUsage();
        
        if (portal.isLowMemory(4096)) {
            Serial.println(F("⚠️ Low memory detected! Consider reducing features or restarting."));
        }
        
        lastMemoryCheck = now;
    }
}

void checkWiFiStatus() {
    static wl_status_t lastStatus = WL_IDLE_STATUS;
    wl_status_t currentStatus = WiFi.status();
    
    if (currentStatus != lastStatus) {
        switch (currentStatus) {
            case WL_CONNECTED:
                Serial.println(F("✓ WiFi connected"));
                break;
            case WL_DISCONNECTED:
                Serial.println(F("✗ WiFi disconnected"));
                break;
            case WL_CONNECTION_LOST:
                Serial.println(F("✗ WiFi connection lost"));
                break;
            case WL_CONNECT_FAILED:
                Serial.println(F("✗ WiFi connection failed"));
                break;
            default:
                Serial.printf("WiFi status: %d\n", currentStatus);
                break;
        }
        lastStatus = currentStatus;
    }
}
