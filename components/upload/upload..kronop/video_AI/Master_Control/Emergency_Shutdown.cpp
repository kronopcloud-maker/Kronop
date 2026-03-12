#include "Emergency_Shutdown.hpp"
#include "Video_AI_Coordinator.hpp"
#include "../Input_Collector/Input_Collector.hpp"
#include "../Master_Splitter/Master_Splitter.hpp"
#include "../Merge_Master/Merge_Master.hpp"
#include "../Output_Dispatcher/Output_Dispatcher.hpp"
#include <iostream>
#include <fstream>

EmergencyShutdown::EmergencyShutdown() 
    : emergencyTriggered_(false), shutdownInProgress_(false),
      inputCollector_(nullptr), masterSplitter_(nullptr), 
      mergeMaster_(nullptr), outputDispatcher_(nullptr),
      emergencyCount_(0) {
    
    lastEmergencyTime_ = std::chrono::system_clock::now();
    
    // Setup thermal callback
    thermalCallback_ = [this](const ThermalAlert& alert) {
        handleThermalAlert(alert);
    };
    
    std::cout << "🚨 Emergency Shutdown System initialized - Brain Department Safety" << std::endl;
}

EmergencyShutdown::~EmergencyShutdown() {
    resetEmergencyState();
    std::cout << "🚨 Emergency Shutdown System shutdown" << std::endl;
}

void EmergencyShutdown::registerComponents(void* inputCollector, void* splitter, 
                                          void* merger, void* dispatcher) {
    inputCollector_ = inputCollector;
    masterSplitter_ = splitter;
    mergeMaster_ = merger;
    outputDispatcher_ = dispatcher;
    
    std::cout << "🔗 Emergency Shutdown: All components registered" << std::endl;
}

void EmergencyShutdown::triggerEmergencyShutdown(const std::string& reason) {
    if (emergencyTriggered_) {
        std::cout << "⚠️ Emergency already in progress" << std::endl;
        return;
    }
    
    emergencyTriggered_ = true;
    emergencyCount_++;
    lastEmergencyTime_ = std::chrono::system_clock::now();
    
    // Start shutdown in separate thread
    shutdownThread_ = std::thread(&EmergencyShutdown::executeEmergencyShutdown, this, reason);
    
    std::cout << "🚨 EMERGENCY SHUTDOWN TRIGGERED: " << reason << std::endl;
    logEmergency(reason);
}

void EmergencyShutdown::triggerThermalEmergency(float temperature) {
    std::string reason = "Thermal emergency - Temperature: " + std::to_string(temperature) + "°C";
    triggerEmergencyShutdown(reason);
}

std::function<void(const ThermalAlert&)> EmergencyShutdown::getThermalCallback() {
    return thermalCallback_;
}

void EmergencyShutdown::resetEmergencyState() {
    emergencyTriggered_ = false;
    shutdownInProgress_ = false;
    
    if (shutdownThread_.joinable()) {
        shutdownThread_.join();
    }
    
    std::cout << "🔄 Emergency state reset" << std::endl;
}

void EmergencyShutdown::testEmergencyShutdown() {
    std::cout << "🧪 Testing emergency shutdown..." << std::endl;
    triggerEmergencyShutdown("Manual test");
}

void EmergencyShutdown::executeEmergencyShutdown(const std::string& reason) {
    shutdownInProgress_ = true;
    
    std::cout << "🛑 EXECUTING EMERGENCY SHUTDOWN: " << reason << std::endl;
    std::cout << "   Timeout: " << SHUTDOWN_TIMEOUT.count() << " seconds" << std::endl;
    
    auto startTime = std::chrono::system_clock::now();
    
    try {
        // Step 1: Stop accepting new jobs (highest priority)
        std::cout << "📥 Step 1: Stopping input collector..." << std::endl;
        if (inputCollector_) {
            shutdownComponent("Input_Collector", inputCollector_);
        }
        
        // Step 2: Stop splitting (stop creating new chunks)
        std::cout << "🔪 Step 2: Stopping master splitter..." << std::endl;
        if (masterSplitter_) {
            shutdownComponent("Master_Splitter", masterSplitter_);
        }
        
        // Step 3: Stop merging (wait for current merges to complete or timeout)
        std::cout << "🔗 Step 3: Stopping merge master..." << std::endl;
        if (mergeMaster_) {
            shutdownComponent("Merge_Master", mergeMaster_);
        }
        
        // Step 4: Stop output dispatch (stop uploads)
        std::cout << "📤 Step 4: Stopping output dispatcher..." << std::endl;
        if (outputDispatcher_) {
            shutdownComponent("Output_Dispatcher", outputDispatcher_);
        }
        
        auto endTime = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
        
        std::cout << "✅ Emergency shutdown completed in " << duration.count() << " seconds" << std::endl;
        
        // Log completion
        std::ofstream logFile("Emergency_Shutdown_Log.txt", std::ios::app);
        if (logFile.is_open()) {
            auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            logFile << "[" << std::ctime(&time_t) << "] EMERGENCY_SHUTDOWN_COMPLETED: " 
                   << reason << " (Duration: " << duration.count() << "s)" << std::endl;
            logFile.close();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error during emergency shutdown: " << e.what() << std::endl;
    }
    
    shutdownInProgress_ = false;
}

void EmergencyShutdown::shutdownComponent(const std::string& componentName, void* component) {
    if (!component) {
        std::cout << "⚠️ Component " << componentName << " is null" << std::endl;
        return;
    }
    
    try {
        auto componentStartTime = std::chrono::system_clock::now();
        
        // Component-specific shutdown
        if (componentName == "Input_Collector") {
            Input_Collector* collector = static_cast<Input_Collector*>(component);
            collector->stopCollection();
        }
        else if (componentName == "Master_Splitter") {
            Master_Splitter* splitter = static_cast<Master_Splitter*>(component);
            splitter->stopSplitting();
        }
        else if (componentName == "Merge_Master") {
            Merge_Master* merger = static_cast<Merge_Master*>(component);
            merger->stopMerging();
        }
        else if (componentName == "Output_Dispatcher") {
            Output_Dispatcher* dispatcher = static_cast<Output_Dispatcher*>(component);
            dispatcher->stopDispatching();
        }
        
        auto componentEndTime = std::chrono::system_clock::now();
        auto componentDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            componentEndTime - componentStartTime);
        
        std::cout << "✅ " << componentName << " stopped in " 
                  << componentDuration.count() << "ms" << std::endl;
        
        logComponentShutdown(componentName, true);
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to stop " << componentName << ": " << e.what() << std::endl;
        logComponentShutdown(componentName, false);
    }
}

void EmergencyShutdown::waitForComponentShutdown(const std::string& componentName, void* component) {
    // This could implement a timeout-based wait for graceful shutdown
    // For now, immediate shutdown is sufficient
}

void EmergencyShutdown::handleThermalAlert(const ThermalAlert& alert) {
    if (isThermalEmergency(alert)) {
        std::string reason = "Thermal emergency - " + alert.message;
        triggerThermalEmergency(alert.temperature);
    }
}

bool EmergencyShutdown::isThermalEmergency(const ThermalAlert& alert) {
    return alert.status == EMERGENCY || alert.status == CRITICAL;
}

void EmergencyShutdown::logEmergency(const std::string& reason) {
    std::ofstream logFile("Emergency_Shutdown_Log.txt", std::ios::app);
    if (logFile.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile << "[" << std::ctime(&time_t) << "] EMERGENCY_TRIGGERED: " 
               << reason << " (Count: " << emergencyCount_ << ")" << std::endl;
        logFile.close();
    }
}

void EmergencyShutdown::logComponentShutdown(const std::string& component, bool success) {
    std::ofstream logFile("Emergency_Shutdown_Log.txt", std::ios::app);
    if (logFile.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile << "[" << std::ctime(&time_t) << "] COMPONENT_SHUTDOWN: " 
               << component << " - " << (success ? "SUCCESS" : "FAILED") << std::endl;
        logFile.close();
    }
}
