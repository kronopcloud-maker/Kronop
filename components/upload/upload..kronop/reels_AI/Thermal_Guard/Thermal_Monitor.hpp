#ifndef THERMAL_MONITOR_HPP
#define THERMAL_MONITOR_HPP

#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <android/log.h>
#include <jni.h>
#include <memory>

#define LOG_TAG "Thermal_Guard"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declaration
class EmergencyBrake;

/**
 * Thermal Guard - Firefighter Department
 * ऐप का Personal Cooling System
 * सिर्फ गर्मी पर नज़र रखना, किसी और काम से मतलब नहीं
 * हर 500ms पर तापमान चेक करो
 * Android NDK Thermal API (AThermalManager) का इस्तेमाल करो
 */

enum ThermalStatus {
    NORMAL = 0,      // < 35°C - All systems go
    WARNING = 1,     // 35-40°C - Reduce load
    CRITICAL = 2,    // 40-45°C - Emergency measures
    EMERGENCY = 3    // > 45°C - Shutdown required
};

struct ThermalAlert {
    ThermalStatus status;
    float temperature;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

class ThermalMonitor {
private:
    // Core monitoring components
    std::thread monitoringThread_;
    std::atomic<bool> monitoringActive_;
    std::atomic<float> currentTemperature_;
    std::atomic<ThermalStatus> currentStatus_;
    
    // Temperature thresholds
    const float WARNING_THRESHOLD = 35.0f;    // 35°C
    const float CRITICAL_THRESHOLD = 40.0f;   // 40°C  
    const float EMERGENCY_THRESHOLD = 45.0f;  // 45°C
    
    // Alert callback to Master_Control
    std::function<void(const ThermalAlert&)> alertCallback_;
    
    // Monitoring intervals (500ms for high-frequency monitoring)
    const std::chrono::milliseconds NORMAL_INTERVAL = std::chrono::milliseconds(500);  // 500ms
    const std::chrono::milliseconds CRITICAL_INTERVAL = std::chrono::milliseconds(250); // 250ms
    
    // Android thermal API (JNI)
    JavaVM* javaVM_;
    jclass thermalManagerClass_;
    jmethodID getTemperatureMethod_;
    
    // Emergency brake system
    std::unique_ptr<EmergencyBrake> emergencyBrake_;
    
    // Statistics
    std::atomic<int> warningCount_;
    std::atomic<int> criticalCount_;
    std::atomic<int> emergencyCount_;
    float peakTemperature_;
    
    // Worker management integration
    bool emergencyBrakeInitialized_;
    
public:
    ThermalMonitor();
    ~ThermalMonitor();
    
    // Core monitoring functions
    bool initialize(JavaVM* vm);
    void startMonitoring();
    void stopMonitoring();
    
    // Worker registration for emergency brake
    bool registerWorker(int workerId, pid_t processId, bool isCritical = false);
    bool unregisterWorker(int workerId);
    
    // Temperature reading
    float getCurrentTemperature() const;
    ThermalStatus getCurrentStatus() const;
    
    // Alert system
    void setAlertCallback(std::function<void(const ThermalAlert&)> callback);
    void sendAlert(ThermalStatus status, float temperature, const std::string& message);
    
    // Emergency brake integration
    bool initializeEmergencyBrake(const std::string& stateSaveDir);
    std::vector<int> getPausedWorkers() const;
    std::vector<int> getSuspendedWorkers() const;
    bool resumeWorker(int workerId);
    
    // Statistics
    int getWarningCount() const { return warningCount_; }
    int getCriticalCount() const { return criticalCount_; }
    int getEmergencyCount() const { return emergencyCount_; }
    float getPeakTemperature() const { return peakTemperature_; }
    
    // Status check
    bool isMonitoringActive() const { return monitoringActive_; }
    std::string getStatusString() const;

private:
    // Core monitoring loop
    void monitoringLoop();
    
    // Temperature reading methods
    float readThermalSensor();
    float readAndroidThermalAPI();
    float readCpuTemperature();  // Fallback method
    
    // Status determination
    ThermalStatus calculateStatus(float temperature);
    std::chrono::milliseconds getMonitoringInterval(ThermalStatus status);
    
    // JNI helper methods
    bool initializeJNI();
    void cleanupJNI();
    
    // Logging
    void logTemperature(float temperature, ThermalStatus status);
    void logAlert(const ThermalAlert& alert);
};

#endif // THERMAL_MONITOR_HPP
