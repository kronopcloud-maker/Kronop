#ifndef EMERGENCY_SHUTDOWN_HPP
#define EMERGENCY_SHUTDOWN_HPP

#include <functional>
#include <thread>
#include <atomic>
#include <chrono>
#include "../Thermal_Guard/Thermal_Monitor.hpp"

/**
 * Emergency Shutdown System - Brain Department's Safety Mechanism
 * चुटकी बजाते ही 3-4 टर्मिनल्स को शटडाउन करने का सिस्टम
 */

class EmergencyShutdown {
private:
    // Shutdown triggers
    std::atomic<bool> emergencyTriggered_;
    std::atomic<bool> shutdownInProgress_;
    
    // Thermal alert callback
    std::function<void(const ThermalAlert&)> thermalCallback_;
    
    // Shutdown thread
    std::thread shutdownThread_;
    
    // Component references (will be set by coordinator)
    void* inputCollector_;
    void* masterSplitter_;
    void* mergeMaster_;
    void* outputDispatcher_;
    
    // Shutdown timeout
    const std::chrono::seconds SHUTDOWN_TIMEOUT = std::chrono::seconds(10);
    
    // Statistics
    std::atomic<int> emergencyCount_;
    std::chrono::system_clock::time_point lastEmergencyTime_;
    
public:
    EmergencyShutdown();
    ~EmergencyShutdown();
    
    // Component registration
    void registerComponents(void* inputCollector, void* splitter, 
                          void* merger, void* dispatcher);
    
    // Emergency triggers
    void triggerEmergencyShutdown(const std::string& reason);
    void triggerThermalEmergency(float temperature);
    
    // Thermal alert callback
    std::function<void(const ThermalAlert&)> getThermalCallback();
    
    // Status
    bool isEmergencyTriggered() const { return emergencyTriggered_; }
    bool isShutdownInProgress() const { return shutdownInProgress_; }
    int getEmergencyCount() const { return emergencyCount_; }
    
    // Manual control
    void resetEmergencyState();
    void testEmergencyShutdown();

private:
    // Core shutdown logic
    void executeEmergencyShutdown(const std::string& reason);
    void shutdownComponent(const std::string& componentName, void* component);
    void waitForComponentShutdown(const std::string& componentName, void* component);
    
    // Thermal monitoring
    void handleThermalAlert(const ThermalAlert& alert);
    bool isThermalEmergency(const ThermalAlert& alert);
    
    // Logging
    void logEmergency(const std::string& reason);
    void logComponentShutdown(const std::string& component, bool success);
};

#endif // EMERGENCY_SHUTDOWN_HPP
