#ifndef EMERGENCY_BRAKE_HPP
#define EMERGENCY_BRAKE_HPP

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>

/**
 * Emergency Brake System - Thermal Guard's Safety Mechanism
 * AI Workers को PAUSE (SIGSTOP) करने और State save करने का system
 */

enum WorkerState {
    RUNNING = 0,
    PAUSED = 1,
    SUSPENDED = 2,
    CRASHED = 3
};

struct WorkerInfo {
    pid_t processId;
    int workerId;
    WorkerState currentState;
    std::chrono::system_clock::time_point lastPauseTime;
    std::string savedStatePath;
    bool isCritical;  // Critical worker - never suspend
};

struct ThermalAction {
    float temperature;
    int maxWorkersAllowed;
    std::string actionDescription;
    bool shouldSaveState;
};

class EmergencyBrake {
private:
    // Worker management
    std::map<int, WorkerInfo> activeWorkers_;
    std::mutex workersMutex_;
    std::atomic<bool> emergencyActive_;
    
    // Thermal thresholds
    const float SEVERE_THRESHOLD = 40.0f;   // 40°C - Pause 2 workers
    const float CRITICAL_THRESHOLD = 45.0f; // 45°C - Keep only 1 worker
    const float EMERGENCY_THRESHOLD = 50.0f; // 50°C - Emergency shutdown
    
    // Action plan based on temperature
    std::vector<ThermalAction> thermalActions_;
    
    // State management
    std::string stateSaveDirectory_;
    std::atomic<int> pausedWorkersCount_;
    std::atomic<int> suspendedWorkersCount_;
    
    // Safety statistics
    std::atomic<int> totalPauses_;
    std::atomic<int> totalSuspensions_;
    std::atomic<int> totalResumes_;
    std::atomic<int> stateSaveFailures_;
    
    // Communication with Master_Control
    std::function<void(int, WorkerState)> stateChangeCallback_;
    
    // Callbacks for handshake and fail-safe
    std::function<bool(int)> checkWorkerPausedCallback_;
    std::function<void(int)> notifyFailedWorkerCallback_;
    
public:
    EmergencyBrake();
    ~EmergencyBrake();
    
    // Initialization
    bool initialize(const std::string& stateSaveDir);
    void shutdown();
    
    // Worker registration
    bool registerWorker(int workerId, pid_t processId, bool isCritical = false);
    bool unregisterWorker(int workerId);
    
    // Core emergency actions
    bool executeThermalAction(float currentTemperature);
    bool pauseWorkers(int maxWorkersToKeep);
    bool suspendWorkers(int maxWorkersToKeep);
    bool resumeWorkers(const std::vector<int>& workerIds);
    bool emergencyShutdown();
    
    // State management
    bool saveWorkerState(int workerId);
    bool restoreWorkerState(int workerId);
    std::string getWorkerStatePath(int workerId) const;
    
    // Process control
    bool pauseProcess(pid_t pid);
    bool resumeProcess(pid_t pid);
    bool isProcessRunning(pid_t pid);
    
    // Status and monitoring
    std::vector<WorkerInfo> getWorkerStates() const;
    int getActiveWorkerCount() const;
    int getPausedWorkerCount() const { return pausedWorkersCount_; }
    int getSuspendedWorkerCount() const { return suspendedWorkersCount_; }
    
    // Statistics
    int getTotalPauses() const { return totalPauses_; }
    int getTotalSuspensions() const { return totalSuspensions_; }
    int getTotalResumes() const { return totalResumes_; }
    int getStateSaveFailures() const { return stateSaveFailures_; }
    
    // Master_Control integration
    void setStateChangeCallback(std::function<void(int, WorkerState)> callback);
    void setWorkerPausedCallback(std::function<bool(int)> callback);
    void setFailedWorkerCallback(std::function<void(int)> callback);
    void notifyWorkerStateChange(int workerId, WorkerState newState);
    
    // Safety checks
    bool isSystemHealthy() const;
    bool canAcceptNewWorkers() const;
    float getSystemLoad() const;

private:
    // Internal methods
    void setupThermalActions();
    std::vector<int> selectWorkersToPause(int maxWorkersToKeep);
    std::vector<int> selectWorkersToSuspend(int maxWorkersToKeep);
    bool executeSignalAction(pid_t pid, int signal);
    
    // State serialization
    bool serializeWorkerState(const WorkerInfo& worker);
    bool deserializeWorkerState(int workerId, WorkerInfo& worker);
    
    // Safety and recovery
    bool validateWorkerState(const WorkerInfo& worker);
    void handleWorkerCrash(int workerId);
    void performHealthCheck();
    
    // Logging
    void logEmergencyAction(const std::string& action, float temperature);
    void logWorkerStateChange(int workerId, WorkerState oldState, WorkerState newState);
    void logStateOperation(const std::string& operation, int workerId, bool success);
};

#endif // EMERGENCY_BRAKE_HPP
