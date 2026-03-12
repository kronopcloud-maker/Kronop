#include "Thermal_Monitor.hpp"
#include "Emergency_Brake.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>

ThermalMonitor::ThermalMonitor() 
    : monitoringActive_(false), currentTemperature_(25.0f), currentStatus_(NORMAL),
      javaVM_(nullptr), thermalManagerClass_(nullptr), getTemperatureMethod_(nullptr),
      warningCount_(0), criticalCount_(0), emergencyCount_(0), peakTemperature_(0.0f),
      emergencyBrake_(nullptr) {
    
    // Initialize emergency brake system
    emergencyBrake_ = std::make_unique<EmergencyBrake>();
    
    LOGI("🔥 Thermal Guard initializing - Firefighter Department ready");
    LOGI("   Warning Threshold: %.1f°C", WARNING_THRESHOLD);
    LOGI("   Critical Threshold: %.1f°C", CRITICAL_THRESHOLD);
    LOGI("   Emergency Threshold: %.1f°C", EMERGENCY_THRESHOLD);
    LOGI("   Monitoring Interval: 500ms");
}

ThermalMonitor::~ThermalMonitor() {
    stopMonitoring();
    cleanupJNI();
    LOGI("🔥 Thermal Guard shutdown - Firefighter Department offline");
}

bool ThermalMonitor::initialize(JavaVM* vm) {
    javaVM_ = vm;
    
    // Initialize JNI for Android thermal API
    if (!initializeJNI()) {
        LOGW("⚠️  JNI initialization failed, using fallback temperature reading");
    }
    
    // Initialize emergency brake system
    std::string stateDir = "/tmp/thermal_guard_states";
    if (!emergencyBrake_->initialize(stateDir)) {
        LOGE("❌ Failed to initialize emergency brake system");
        return false;
    }
    emergencyBrakeInitialized_ = true;
    
    // Test temperature reading
    float testTemp = readThermalSensor();
    if (testTemp < 0 || testTemp > 100) {
        LOGE("❌ Invalid temperature reading: %.1f°C", testTemp);
        return false;
    }
    
    currentTemperature_ = testTemp;
    currentStatus_ = calculateStatus(testTemp);
    peakTemperature_ = testTemp;
    
    // Setup emergency brake callback
    emergencyBrake_->setStateChangeCallback([this](int workerId, int newState) {
        LOGI("🔄 Worker %d state changed to %d", workerId, newState);
        // Notify Master_Control if needed
        if (alertCallback_) {
            ThermalAlert alert;
            alert.status = (newState == 1 || newState == 2) ? CRITICAL : NORMAL;
            alert.temperature = currentTemperature_;
            alert.message = "Worker state changed due to thermal action";
            alert.timestamp = std::chrono::system_clock::now();
            alertCallback_(alert);
        }
    });
    
    LOGI("✅ Thermal Guard initialized - Current: %.1f°C (%s)", 
         testTemp, getStatusString().c_str());
    LOGI("   Emergency Brake: Ready");
    LOGI("   Monitoring Interval: 500ms");
    
    return true;
}

void ThermalMonitor::startMonitoring() {
    if (monitoringActive_) {
        LOGW("⚠️  Thermal monitoring already active");
        return;
    }
    
    monitoringActive_ = true;
    monitoringThread_ = std::thread(&ThermalMonitor::monitoringLoop, this);
    
    LOGI("🚀 Thermal Guard monitoring started - Firefighter on duty");
}

void ThermalMonitor::stopMonitoring() {
    if (!monitoringActive_) {
        return;
    }
    
    monitoringActive_ = false;
    if (monitoringThread_.joinable()) {
        monitoringThread_.join();
    }
    
    LOGI("🛑 Thermal Guard monitoring stopped - Firefighter off duty");
}

float ThermalMonitor::getCurrentTemperature() const {
    return currentTemperature_;
}

ThermalStatus ThermalMonitor::getCurrentStatus() const {
    return currentStatus_;
}

void ThermalMonitor::setAlertCallback(std::function<void(const ThermalAlert&)> callback) {
    alertCallback_ = callback;
    LOGI("📞 Alert callback registered with Master_Control");
}

void ThermalMonitor::sendAlert(ThermalStatus status, float temperature, const std::string& message) {
    ThermalAlert alert;
    alert.status = status;
    alert.temperature = temperature;
    alert.message = message;
    alert.timestamp = std::chrono::system_clock::now();
    
    // Update statistics
    switch (status) {
        case WARNING:
            warningCount_++;
            break;
        case CRITICAL:
            criticalCount_++;
            break;
        case EMERGENCY:
            emergencyCount_++;
            break;
        default:
            break;
    }
    
    // Log the alert
    logAlert(alert);
    
    // Execute emergency brake actions
    if (emergencyBrakeInitialized_ && emergencyBrake_) {
        bool actionSuccess = emergencyBrake_->executeThermalAction(temperature);
        if (actionSuccess) {
            LOGI("🛑 Emergency brake action executed successfully");
        } else {
            LOGE("❌ Emergency brake action failed");
        }
    }
    
    // Send to Master_Control
    if (alertCallback_) {
        alertCallback_(alert);
    }
}

std::string ThermalMonitor::getStatusString() const {
    switch (currentStatus_) {
        case NORMAL: return "NORMAL";
        case WARNING: return "WARNING";
        case CRITICAL: return "CRITICAL";
        case EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

void ThermalMonitor::monitoringLoop() {
    LOGI("🔄 Thermal monitoring loop started");
    
    while (monitoringActive_) {
        try {
            // Read current temperature
            float temperature = readThermalSensor();
            currentTemperature_ = temperature;
            
            // Update peak temperature
            if (temperature > peakTemperature_) {
                peakTemperature_ = temperature;
            }
            
            // Calculate new status
            ThermalStatus newStatus = calculateStatus(temperature);
            ThermalStatus oldStatus = currentStatus_.exchange(newStatus);
            
            // Log temperature
            logTemperature(temperature, newStatus);
            
            // Check for status changes and send alerts
            if (newStatus != oldStatus) {
                std::string message;
                switch (newStatus) {
                    case WARNING:
                        message = "Temperature rising - Reduce processing load";
                        break;
                    case CRITICAL:
                        message = "Critical temperature - Emergency measures activated";
                        break;
                    case EMERGENCY:
                        message = "EMERGENCY - Immediate shutdown required";
                        break;
                    default:
                        message = "Temperature returned to normal";
                        break;
                }
                
                sendAlert(newStatus, temperature, message);
            }
            
            // Dynamic monitoring interval based on status
            auto interval = getMonitoringInterval(newStatus);
            std::this_thread::sleep_for(interval);
            
        } catch (const std::exception& e) {
            LOGE("❌ Error in thermal monitoring: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    LOGI("🔄 Thermal monitoring loop ended");
}

float ThermalMonitor::readThermalSensor() {
    // Try Android thermal API first
    float temp = readAndroidThermalAPI();
    if (temp > 0 && temp < 100) {
        return temp;
    }
    
    // Fallback to CPU temperature
    temp = readCpuTemperature();
    if (temp > 0 && temp < 100) {
        return temp;
    }
    
    // Final fallback - simulate for demo
    return simulateTemperature();
}

float ThermalMonitor::simulateTemperature() {
    static float simulatedTemp = 25.0f;
    static auto lastUpdate = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count();
    
    if (elapsed > 0) {
        // Simulate temperature rise under load
        simulatedTemp += 0.1f * elapsed;
        simulatedTemp = std::min(simulatedTemp, 50.0f);  // Cap at 50°C
        lastUpdate = now;
    }
    
    return simulatedTemp;
}

float ThermalMonitor::readLinuxThermalZones() {
    float temperature = 0.0f;
    
    // Try different thermal zone paths
    std::vector<std::string> thermalPaths = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp",
        "/sys/devices/virtual/thermal/thermal_zone0/temp",
        "/sys/class/hwmon/hwmon0/temp1_input"
    };
    
    for (const auto& path : thermalPaths) {
        std::ifstream file(path);
        if (file.is_open()) {
            int tempMilliCelsius;
            file >> tempMilliCelsius;
            file.close();
            
            temperature = tempMilliCelsius / 1000.0f; // Convert from milli-Celsius to Celsius
            LOGI("🌡️ Linux thermal zone reading from %s: %.1f°C", path.c_str(), temperature);
    }
    
    return -1.0f;
}

ThermalStatus ThermalMonitor::calculateStatus(float temperature) {
    if (temperature >= EMERGENCY_THRESHOLD) {
        return EMERGENCY;
    } else if (temperature >= CRITICAL_THRESHOLD) {
        return CRITICAL;
    } else if (temperature >= WARNING_THRESHOLD) {
        return WARNING;
    } else {
        return NORMAL;
    }
}

std::chrono::milliseconds ThermalMonitor::getMonitoringInterval(ThermalStatus status) {
    switch (status) {
        case EMERGENCY:
        case CRITICAL:
            return CRITICAL_INTERVAL;  // 0.5 seconds
        case WARNING:
            return std::chrono::milliseconds(1000);  // 1 second
        default:
            return NORMAL_INTERVAL;  // 2 seconds
    }
}

bool ThermalMonitor::initializeJNI() {
    if (!javaVM_) {
        return false;
    }
    
    try {
        JNIEnv* env;
        if (javaVM_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            return false;
        }
        
        // Get Android thermal manager class
        jclass localClass = env->FindClass("android/os/PowerManager");
        if (!localClass) {
            return false;
        }
        
        thermalManagerClass_ = reinterpret_cast<jclass>(env->NewGlobalRef(localClass));
        env->DeleteLocalRef(localClass);
        
        // Get method ID for temperature reading
        getTemperatureMethod_ = env->GetStaticMethodID(thermalManagerClass_, 
                                                      "getCurrentThermalStatus", "()F");
        
        return (getTemperatureMethod_ != nullptr);
        
    } catch (...) {
        return false;
    }
}

void ThermalMonitor::cleanupJNI() {
    if (javaVM_ && thermalManagerClass_) {
        try {
            JNIEnv* env;
            if (javaVM_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
                env->DeleteGlobalRef(thermalManagerClass_);
            }
        } catch (...) {
            // Ignore cleanup errors
        }
    }
    
    thermalManagerClass_ = nullptr;
    getTemperatureMethod_ = nullptr;
}

void ThermalMonitor::logTemperature(float temperature, ThermalStatus status) {
    static auto lastLog = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastLog).count();
    
    // Log every 10 seconds or on status change
    if (elapsed >= 10 || status != NORMAL) {
        LOGI("🌡️  Temperature: %.1f°C [%s]", temperature, getStatusString().c_str());
        lastLog = now;
    }
}

void ThermalMonitor::logAlert(const ThermalAlert& alert) {
    auto time_t = std::chrono::system_clock::to_time_t(alert.timestamp);
    
    switch (alert.status) {
        case WARNING:
            LOGW("⚠️  THERMAL WARNING: %.1f°C - %s", alert.temperature, alert.message.c_str());
            break;
        case CRITICAL:
            LOGE("🔥 THERMAL CRITICAL: %.1f°C - %s", alert.temperature, alert.message.c_str());
            break;
        case EMERGENCY:
            LOGE("🚨 THERMAL EMERGENCY: %.1f°C - %s", alert.temperature, alert.message.c_str());
            break;
        default:
            LOGI("✅ THERMAL NORMAL: %.1f°C - %s", alert.temperature, alert.message.c_str());
            break;
    }
}

// Worker management functions
bool ThermalMonitor::registerWorker(int workerId, pid_t processId, bool isCritical) {
    if (!emergencyBrakeInitialized_ || !emergencyBrake_) {
        LOGE("❌ Emergency brake not initialized");
        return false;
    }
    
    bool success = emergencyBrake_->registerWorker(workerId, processId, isCritical);
    if (success) {
        LOGI("➕ Worker registered with thermal guard: %d (PID: %d, Critical: %s)", 
             workerId, processId, isCritical ? "Yes" : "No");
    }
    return success;
}

bool ThermalMonitor::unregisterWorker(int workerId) {
    if (!emergencyBrakeInitialized_ || !emergencyBrake_) {
        return false;
    }
    
    bool success = emergencyBrake_->unregisterWorker(workerId);
    if (success) {
        LOGI("➖ Worker unregistered from thermal guard: %d", workerId);
    }
    return success;
}

bool ThermalMonitor::initializeEmergencyBrake(const std::string& stateSaveDir) {
    if (!emergencyBrake_) {
        emergencyBrake_ = std::make_unique<EmergencyBrake>();
    }
    
    bool success = emergencyBrake_->initialize(stateSaveDir);
    emergencyBrakeInitialized_ = success;
    
    if (success) {
        LOGI("✅ Emergency brake initialized with state directory: %s", stateSaveDir.c_str());
    } else {
        LOGE("❌ Failed to initialize emergency brake");
    }
    
    return success;
}

std::vector<int> ThermalMonitor::getPausedWorkers() const {
    if (!emergencyBrakeInitialized_ || !emergencyBrake_) {
        return {};
    }
    
    std::vector<int> pausedWorkers;
    auto workerStates = emergencyBrake_->getWorkerStates();
    
    for (const auto& worker : workerStates) {
        if (worker.currentState == PAUSED) {
            pausedWorkers.push_back(worker.workerId);
        }
    }
    
    return pausedWorkers;
}

std::vector<int> ThermalMonitor::getSuspendedWorkers() const {
    if (!emergencyBrakeInitialized_ || !emergencyBrake_) {
        return {};
    }
    
    std::vector<int> suspendedWorkers;
    auto workerStates = emergencyBrake_->getWorkerStates();
    
    for (const auto& worker : workerStates) {
        if (worker.currentState == SUSPENDED) {
            suspendedWorkers.push_back(worker.workerId);
        }
    }
    
    return suspendedWorkers;
}

bool ThermalMonitor::resumeWorker(int workerId) {
    if (!emergencyBrakeInitialized_ || !emergencyBrake_) {
        return false;
    }
    
    std::vector<int> workersToResume = {workerId};
    bool success = emergencyBrake_->resumeWorkers(workersToResume);
    
    if (success) {
        LOGI("▶️ Worker resumed by thermal guard: %d", workerId);
    } else {
        LOGE("❌ Failed to resume worker: %d", workerId);
    }
    
    return success;
}
