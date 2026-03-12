#include "Emergency_Brake.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

EmergencyBrake::EmergencyBrake() 
    : emergencyActive_(false), pausedWorkersCount_(0), suspendedWorkersCount_(0),
      totalPauses_(0), totalSuspensions_(0), totalResumes_(0), stateSaveFailures_(0) {
    
    setupThermalActions();
    
    std::cout << "🛑 Emergency Brake System initializing - Thermal Guard Safety" << std::endl;
    std::cout << "   Severe Threshold: " << SEVERE_THRESHOLD << "°C (Pause 2 workers)" << std::endl;
    std::cout << "   Critical Threshold: " << CRITICAL_THRESHOLD << "°C (Keep 1 worker)" << std::endl;
    std::cout << "   Emergency Threshold: " << EMERGENCY_THRESHOLD << "°C (Shutdown all)" << std::endl;
}

EmergencyBrake::~EmergencyBrake() {
    shutdown();
    std::cout << "🛑 Emergency Brake System destroyed" << std::endl;
}

bool EmergencyBrake::initialize(const std::string& stateSaveDir) {
    stateSaveDirectory_ = stateSaveDir;
    
    // Create state save directory
    try {
        if (!fs::exists(stateSaveDirectory_)) {
            fs::create_directories(stateSaveDirectory_);
        }
        
        // Test directory permissions
        std::string testFile = stateSaveDirectory_ + "/test_write.tmp";
        std::ofstream test(testFile);
        if (!test.is_open()) {
            std::cerr << "❌ Cannot write to state save directory: " << stateSaveDirectory_ << std::endl;
            return false;
        }
        test.close();
        fs::remove(testFile);
        
        std::cout << "✅ Emergency Brake initialized - State directory: " << stateSaveDirectory_ << std::endl;
        return true;
        
    } catch (const fs::filesystem_error& e) {
        std::cerr << "❌ Failed to create state directory: " << e.what() << std::endl;
        return false;
    }
}

void EmergencyBrake::shutdown() {
    emergencyActive_ = false;
    
    // Resume all paused workers before shutdown
    std::vector<int> allWorkers;
    {
        std::lock_guard<std::mutex> lock(workersMutex_);
        for (const auto& pair : activeWorkers_) {
            allWorkers.push_back(pair.first);
        }
    }
    
    if (!allWorkers.empty()) {
        resumeWorkers(allWorkers);
    }
    
    std::cout << "🛑 Emergency Brake shutdown - All workers resumed" << std::endl;
}

bool EmergencyBrake::registerWorker(int workerId, pid_t processId, bool isCritical) {
    std::lock_guard<std::mutex> lock(workersMutex_);
    
    // Check if worker already exists
    if (activeWorkers_.find(workerId) != activeWorkers_.end()) {
        std::cerr << "⚠️ Worker " << workerId << " already registered" << std::endl;
        return false;
    }
    
    // Validate process
    if (!isProcessRunning(processId)) {
        std::cerr << "❌ Process " << processId << " is not running" << std::endl;
        return false;
    }
    
    WorkerInfo worker;
    worker.processId = processId;
    worker.workerId = workerId;
    worker.currentState = RUNNING;
    worker.lastPauseTime = std::chrono::system_clock::now();
    worker.savedStatePath = getWorkerStatePath(workerId);
    worker.isCritical = isCritical;
    
    activeWorkers_[workerId] = worker;
    
    std::cout << "➕ Worker registered: " << workerId << " (PID: " << processId 
              << ", Critical: " << (isCritical ? "Yes" : "No") << ")" << std::endl;
    
    return true;
}

bool EmergencyBrake::unregisterWorker(int workerId) {
    std::lock_guard<std::mutex> lock(workersMutex_);
    
    auto it = activeWorkers_.find(workerId);
    if (it == activeWorkers_.end()) {
        std::cerr << "❌ Worker " << workerId << " not found" << std::endl;
        return false;
    }
    
    // Resume worker before unregistering
    if (it->second.currentState != RUNNING) {
        resumeProcess(it->second.processId);
    }
    
    activeWorkers_.erase(it);
    
    std::cout << "➖ Worker unregistered: " << workerId << std::endl;
    return true;
}

bool EmergencyBrake::executeThermalAction(float currentTemperature) {
    std::cout << "🌡️ Executing thermal action for temperature: " << currentTemperature << "°C" << std::endl;
    
    // Find appropriate action based on temperature
    ThermalAction* action = nullptr;
    for (auto& act : thermalActions_) {
        if (currentTemperature >= act.temperature) {
            action = &act;
            break;
        }
    }
    
    if (!action) {
        std::cout << "✅ No action needed - Temperature normal" << std::endl;
        return true;
    }
    
    emergencyActive_ = true;
    logEmergencyAction(action->actionDescription, currentTemperature);
    
    bool success = false;
    
    if (currentTemperature >= EMERGENCY_THRESHOLD) {
        // Emergency shutdown
        success = emergencyShutdown();
    } else if (currentTemperature >= CRITICAL_THRESHOLD) {
        // Critical - keep only 1 worker
        success = suspendWorkers(action->maxWorkersAllowed);
    } else if (currentTemperature >= SEVERE_THRESHOLD) {
        // Severe - pause 2 workers
        success = pauseWorkers(action->maxWorkersAllowed);
    }
    
    emergencyActive_ = false;
    return success;
}

bool EmergencyBrake::pauseWorkers(int maxWorkersToKeep) {
    std::cout << "⏸️ Pausing workers - Keeping max " << maxWorkersToKeep << " active" << std::endl;
    
    std::vector<int> workersToPause = selectWorkersToPause(maxWorkersToKeep);
    
    if (workersToPause.empty()) {
        std::cout << "ℹ️ No workers to pause" << std::endl;
        return true;
    }
    
    bool allSuccess = true;
    
    for (int workerId : workersToPause) {
        std::lock_guard<std::mutex> lock(workersMutex_);
        auto it = activeWorkers_.find(workerId);
        if (it == activeWorkers_.end() || it->second.isCritical) {
            continue;
        }
        
        WorkerInfo& worker = it->second;
        
        // Save state before pausing
        if (!saveWorkerState(workerId)) {
            std::cerr << "❌ Failed to save state for worker " << workerId << std::endl;
            stateSaveFailures_++;
            allSuccess = false;
            continue;
        }
        
        // Reliable SIGSTOP with handshake and retry
        bool workerPaused = false;
        int retryCount = 0;
        const int maxRetries = 3;
        const std::chrono::milliseconds checkInterval(50);
        
        while (!workerPaused && retryCount < maxRetries) {
            // Send SIGSTOP signal
            if (!pauseProcess(worker.processId)) {
                std::cerr << "❌ Failed to send SIGSTOP to worker " << workerId << std::endl;
                allSuccess = false;
                break;
            }
            
            // Handshake Check: Wait 50ms for worker to update Paused status
            std::this_thread::sleep_for(checkInterval);
            
            // Check if worker acknowledged by updating status in IO_Vault
            if (checkWorkerPausedCallback_ && checkWorkerPausedCallback_(workerId)) {
                workerPaused = true;
                std::cout << "✅ Worker " << workerId << " acknowledged pause (attempt " << (retryCount + 1) << ")" << std::endl;
            } else {
                retryCount++;
                if (retryCount < maxRetries) {
                    std::cout << "⏳ Worker " << workerId << " not paused, retrying... (attempt " << (retryCount + 1) << "/" << maxRetries << ")" << std::endl;
                }
            }
        }
        
        if (workerPaused) {
            worker.currentState = PAUSED;
            worker.lastPauseTime = std::chrono::system_clock::now();
            pausedWorkersCount_++;
            totalPauses_++;
            
            notifyWorkerStateChange(workerId, PAUSED);
            logWorkerStateChange(workerId, RUNNING, PAUSED);
            
            std::cout << "⏸️ Worker " << workerId << " paused successfully (PID: " << worker.processId << ")" << std::endl;
        } else {
            // ⚠️ FIXED: Resource cleanup before SIGKILL
            std::cout << "🚨 Worker " << workerId << " failed to respond after " << maxRetries << " attempts - Performing safe cleanup" << std::endl;
            
            // Step 1: Save worker state before termination
            if (!saveWorkerState(workerId)) {
                std::cerr << "❌ Failed to save worker state before termination" << std::endl;
            }
            
            // Step 2: Notify Video_AI_Coordinator about impending failure
            if (notifyFailedWorkerCallback_) {
                notifyFailedWorkerCallback_(workerId);
            }
            
            // Step 3: Attempt graceful termination first
            if (kill(worker.processId, SIGTERM) == 0) {
                std::cout << "� Sent SIGTERM to worker " << workerId << " - Waiting for graceful shutdown" << std::endl;
                
                // Wait 2 seconds for graceful shutdown
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                // Check if process still exists
                if (!isProcessRunning(worker.processId)) {
                    std::cout << "✅ Worker " << workerId << " terminated gracefully" << std::endl;
                    worker.currentState = TERMINATED;
                    allSuccess = true;
                } else {
                    std::cout << "⚠️ Worker " << workerId << " still running, forcing SIGKILL" << std::endl;
                }
            }
            
            // Step 4: Force kill if still running
            if (worker.currentState != TERMINATED) {
                if (kill(worker.processId, SIGKILL) == 0) {
                    std::cout << "💀 Worker " << workerId << " force killed (PID: " << worker.processId << ")" << std::endl;
                    worker.currentState = CRASHED;
                    
                    // Step 5: Final cleanup notification
                    if (notifyFailedWorkerCallback_) {
                        notifyFailedWorkerCallback_(workerId);
                    }
                } else {
                    std::cerr << "❌ Failed to force kill worker " << workerId << std::endl;
                }
            }
            
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

bool EmergencyBrake::suspendWorkers(int maxWorkersToKeep) {
    std::cout << "🛑 Suspending workers - Keeping max " << maxWorkersToKeep << " active" << std::endl;
    
    std::vector<int> workersToSuspend = selectWorkersToSuspend(maxWorkersToKeep);
    
    if (workersToSuspend.empty()) {
        std::cout << "ℹ️ No workers to suspend" << std::endl;
        return true;
    }
    
    bool allSuccess = true;
    
    for (int workerId : workersToSuspend) {
        std::lock_guard<std::mutex> lock(workersMutex_);
        auto it = activeWorkers_.find(workerId);
        if (it == activeWorkers_.end() || it->second.isCritical) {
            continue;
        }
        
        WorkerInfo& worker = it->second;
        
        // Save state before suspending
        if (!saveWorkerState(workerId)) {
            std::cerr << "❌ Failed to save state for worker " << workerId << std::endl;
            stateSaveFailures_++;
            allSuccess = false;
            continue;
        }
        
        // Suspend the process
        if (pauseProcess(worker.processId)) {  // SIGSTOP for suspension
            worker.currentState = SUSPENDED;
            worker.lastPauseTime = std::chrono::system_clock::now();
            suspendedWorkersCount_++;
            totalSuspensions_++;
            
            notifyWorkerStateChange(workerId, SUSPENDED);
            logWorkerStateChange(workerId, RUNNING, SUSPENDED);
            
            std::cout << "🛑 Worker " << workerId << " suspended (PID: " << worker.processId << ")" << std::endl;
        } else {
            std::cerr << "❌ Failed to suspend worker " << workerId << std::endl;
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

bool EmergencyBrake::resumeWorkers(const std::vector<int>& workerIds) {
    std::cout << "▶️ Resuming workers" << std::endl;
    
    bool allSuccess = true;
    
    for (int workerId : workerIds) {
        std::lock_guard<std::mutex> lock(workersMutex_);
        auto it = activeWorkers_.find(workerId);
        if (it == activeWorkers_.end()) {
            continue;
        }
        
        WorkerInfo& worker = it->second;
        
        if (worker.currentState == RUNNING) {
            continue; // Already running
        }
        
        // Resume the process
        if (resumeProcess(worker.processId)) {
            // Restore state if available
            restoreWorkerState(workerId);
            
            WorkerState oldState = worker.currentState;
            worker.currentState = RUNNING;
            
            if (oldState == PAUSED) {
                pausedWorkersCount_--;
            } else if (oldState == SUSPENDED) {
                suspendedWorkersCount_--;
            }
            
            totalResumes_++;
            notifyWorkerStateChange(workerId, RUNNING);
            logWorkerStateChange(workerId, oldState, RUNNING);
            
            std::cout << "▶️ Worker " << workerId << " resumed (PID: " << worker.processId << ")" << std::endl;
        } else {
            std::cerr << "❌ Failed to resume worker " << workerId << std::endl;
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

bool EmergencyBrake::emergencyShutdown() {
    std::cout << "🚨 EMERGENCY SHUTDOWN - Suspending all non-critical workers" << std::endl;
    
    std::vector<int> allWorkers;
    {
        std::lock_guard<std::mutex> lock(workersMutex_);
        for (const auto& pair : activeWorkers_) {
            if (!pair.second.isCritical) {
                allWorkers.push_back(pair.first);
            }
        }
    }
    
    return suspendWorkers(0); // Keep 0 workers running
}

bool EmergencyBrake::saveWorkerState(int workerId) {
    std::lock_guard<std::mutex> lock(workersMutex_);
    auto it = activeWorkers_.find(workerId);
    if (it == activeWorkers_.end()) {
        return false;
    }
    
    return serializeWorkerState(it->second);
}

bool EmergencyBrake::restoreWorkerState(int workerId) {
    std::lock_guard<std::mutex> lock(workersMutex_);
    auto it = activeWorkers_.find(workerId);
    if (it == activeWorkers_.end()) {
        return false;
    }
    
    WorkerInfo worker;
    if (deserializeWorkerState(workerId, worker)) {
        // Restore critical information
        it->second.lastPauseTime = worker.lastPauseTime;
        it->second.savedStatePath = worker.savedStatePath;
        return true;
    }
    
    return false;
}

std::string EmergencyBrake::getWorkerStatePath(int workerId) const {
    return stateSaveDirectory_ + "/worker_" + std::to_string(workerId) + "_state.json";
}

bool EmergencyBrake::pauseProcess(pid_t pid) {
    return executeSignalAction(pid, SIGSTOP);
}

bool EmergencyBrake::resumeProcess(pid_t pid) {
    return executeSignalAction(pid, SIGCONT);
}

bool EmergencyBrake::isProcessRunning(pid_t pid) {
    // Send signal 0 to check if process exists
    return executeSignalAction(pid, 0);
}

std::vector<WorkerInfo> EmergencyBrake::getWorkerStates() const {
    std::lock_guard<std::mutex> lock(workersMutex_);
    
    std::vector<WorkerInfo> states;
    for (const auto& pair : activeWorkers_) {
        states.push_back(pair.second);
    }
    
    return states;
}

int EmergencyBrake::getActiveWorkerCount() const {
    std::lock_guard<std::mutex> lock(workersMutex_);
    
    int count = 0;
    for (const auto& pair : activeWorkers_) {
        if (pair.second.currentState == RUNNING) {
            count++;
        }
    }
    
    return count;
}

void EmergencyBrake::setStateChangeCallback(std::function<void(int, WorkerState)> callback) {
    stateChangeCallback_ = callback;
    std::cout << "📞 State change callback registered" << std::endl;
}

void EmergencyBrake::setWorkerPausedCallback(std::function<bool(int)> callback) {
    checkWorkerPausedCallback_ = callback;
    std::cout << "🔄 Worker paused callback registered" << std::endl;
}

void EmergencyBrake::setFailedWorkerCallback(std::function<void(int)> callback) {
    notifyFailedWorkerCallback_ = callback;
    std::cout << "🚨 Failed worker callback registered" << std::endl;
}

void EmergencyBrake::notifyWorkerStateChange(int workerId, WorkerState newState) {
    if (stateChangeCallback_) {
        stateChangeCallback_(workerId, newState);
    }
}

bool EmergencyBrake::isSystemHealthy() const {
    return (stateSaveFailures_ < 5) && (getActiveWorkerCount() > 0);
}

bool EmergencyBrake::canAcceptNewWorkers() const {
    return !emergencyActive_ && isSystemHealthy();
}

float EmergencyBrake::getSystemLoad() const {
    int totalWorkers = activeWorkers_.size();
    if (totalWorkers == 0) return 0.0f;
    
    int activeWorkers = getActiveWorkerCount();
    return static_cast<float>(activeWorkers) / totalWorkers;
}

void EmergencyBrake::setupThermalActions() {
    thermalActions_.clear();
    
    // Critical action - keep only 1 worker
    ThermalAction critical;
    critical.temperature = CRITICAL_THRESHOLD;
    critical.maxWorkersAllowed = 1;
    critical.actionDescription = "Critical temperature - Keep only 1 worker running";
    critical.shouldSaveState = true;
    thermalActions_.push_back(critical);
    
    // Severe action - pause 2 workers
    ThermalAction severe;
    severe.temperature = SEVERE_THRESHOLD;
    severe.maxWorkersAllowed = 3; // Keep 3 out of 5 running
    severe.actionDescription = "Severe temperature - Pause 2 workers";
    severe.shouldSaveState = true;
    thermalActions_.push_back(severe);
    
    // Emergency action - shutdown all
    ThermalAction emergency;
    emergency.temperature = EMERGENCY_THRESHOLD;
    emergency.maxWorkersAllowed = 0;
    emergency.actionDescription = "Emergency temperature - Shutdown all workers";
    emergency.shouldSaveState = true;
    thermalActions_.push_back(emergency);
}

std::vector<int> EmergencyBrake::selectWorkersToPause(int maxWorkersToKeep) {
    std::vector<int> workersToPause;
    
    std::lock_guard<std::mutex> lock(workersMutex_);
    
    std::vector<std::pair<int, WorkerInfo>> runningWorkers;
    for (const auto& pair : activeWorkers_) {
        if (pair.second.currentState == RUNNING && !pair.second.isCritical) {
            runningWorkers.push_back(pair);
        }
    }
    
    // Sort by last pause time (pause least recently used first)
    std::sort(runningWorkers.begin(), runningWorkers.end(),
        [](const auto& a, const auto& b) {
            return a.second.lastPauseTime < b.second.lastPauseTime;
        });
    
    // Select workers to pause (keep the most recently used)
    int workersToPauseCount = runningWorkers.size() - maxWorkersToKeep;
    for (int i = 0; i < workersToPauseCount && i < runningWorkers.size(); ++i) {
        workersToPause.push_back(runningWorkers[i].first);
    }
    
    return workersToPause;
}

std::vector<int> EmergencyBrake::selectWorkersToSuspend(int maxWorkersToKeep) {
    std::vector<int> workersToSuspend;
    
    std::lock_guard<std::mutex> lock(workersMutex_);
    
    std::vector<std::pair<int, WorkerInfo>> runningWorkers;
    for (const auto& pair : activeWorkers_) {
        if (pair.second.currentState == RUNNING && !pair.second.isCritical) {
            runningWorkers.push_back(pair);
        }
    }
    
    // For suspension, pause all except maxWorkersToKeep
    int workersToSuspendCount = runningWorkers.size() - maxWorkersToKeep;
    for (int i = 0; i < workersToSuspendCount && i < runningWorkers.size(); ++i) {
        workersToSuspend.push_back(runningWorkers[i].first);
    }
    
    return workersToSuspend;
}

bool EmergencyBrake::executeSignalAction(pid_t pid, int signal) {
    int result = kill(pid, signal);
    
    if (result == 0) {
        return true;
    } else if (errno == ESRCH) {
        // Process doesn't exist
        return false;
    } else {
        std::cerr << "❌ Signal action failed: " << strerror(errno) << std::endl;
        return false;
    }
}

bool EmergencyBrake::serializeWorkerState(const WorkerInfo& worker) {
    try {
        std::ofstream file(worker.savedStatePath);
        if (!file.is_open()) {
            return false;
        }
        
        // Serialize worker state as JSON
        file << "{\n";
        file << "  \"workerId\": " << worker.workerId << ",\n";
        file << "  \"processId\": " << worker.processId << ",\n";
        file << "  \"currentState\": " << static_cast<int>(worker.currentState) << ",\n";
        file << "  \"isCritical\": " << (worker.isCritical ? "true" : "false") << ",\n";
        
        // Timestamp
        auto time_t = std::chrono::system_clock::to_time_t(worker.lastPauseTime);
        file << "  \"lastPauseTime\": " << time_t << ",\n";
        
        file << "  \"savedAt\": " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "\n";
        file << "}\n";
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to serialize worker state: " << e.what() << std::endl;
        return false;
    }
}

bool EmergencyBrake::deserializeWorkerState(int workerId, WorkerInfo& worker) {
    try {
        std::string path = getWorkerStatePath(workerId);
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        // Simple JSON parsing (in production, use proper JSON library)
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("\"workerId\"") != std::string::npos) {
                // Extract workerId
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    worker.workerId = std::stoi(line.substr(pos + 1));
                }
            }
            // Add other fields as needed
        }
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to deserialize worker state: " << e.what() << std::endl;
        return false;
    }
}

void EmergencyBrake::logEmergencyAction(const std::string& action, float temperature) {
    std::ofstream logFile("Emergency_Brake_Log.txt", std::ios::app);
    if (logFile.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile << "[" << std::ctime(&time_t) << "] EMERGENCY_ACTION: " << action 
               << " (Temperature: " << temperature << "°C)" << std::endl;
        logFile.close();
    }
}

void EmergencyBrake::logWorkerStateChange(int workerId, WorkerState oldState, WorkerState newState) {
    std::ofstream logFile("Worker_State_Log.txt", std::ios::app);
    if (logFile.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        
        const char* stateNames[] = {"RUNNING", "PAUSED", "SUSPENDED", "CRASHED"};
        
        logFile << "[" << std::ctime(&time_t) << "] WORKER_STATE: " << workerId
               << " " << stateNames[oldState] << " -> " << stateNames[newState] << std::endl;
        logFile.close();
    }
}

void EmergencyBrake::logStateOperation(const std::string& operation, int workerId, bool success) {
    std::ofstream logFile("State_Operation_Log.txt", std::ios::app);
    if (logFile.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile << "[" << std::ctime(&time_t) << "] STATE_OP: " << operation
               << " Worker " << workerId << " - " << (success ? "SUCCESS" : "FAILED") << std::endl;
        logFile.close();
    }
}
