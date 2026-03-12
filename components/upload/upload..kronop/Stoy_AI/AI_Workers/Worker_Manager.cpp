#include "Worker_Manager.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <random>

// AIWorker Implementation

AIWorker::AIWorker(int workerId, Zero_Copy_IO_Manager* ioManager) 
    : workerId_(workerId), status_(IDLE), active_(false),
      ioManager_(ioManager), inputMapping_(nullptr), currentMemoryUsage_(0),
      tasksCompleted_(0), tasksFailed_(0), totalProcessingTime_(0.0),
      canProcessVideo_(true), canProcessImage_(true), canProcessAudio_(false) {
    
    startTime_ = std::chrono::system_clock::now();
    
    if (!ioManager_) {
        std::cerr << "❌ AI Worker " << workerId_ << " created without IO Manager - Zero-Copy disabled" << std::endl;
    } else {
        std::cout << "🤖 AI Worker " << workerId_ << " created with Zero-Copy IO Manager" << std::endl;
    }
}

AIWorker::~AIWorker() {
    shutdown();
    std::cout << "🤖 AI Worker " << workerId_ << " destroyed" << std::endl;
}

bool AIWorker::start() {
    if (active_) {
        return true;
    }
    
    active_ = true;
    status_ = IDLE;
    workerThread_ = std::thread(&AIWorker::workerLoop, this);
    
    std::cout << "🚀 AI Worker " << workerId_ << " started" << std::endl;
    return true;
}

void AIWorker::stop() {
    if (!active_) {
        return;
    }
    
    active_ = false;
    queueCondition_.notify_all();
    
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    status_ = SHUTDOWN;
    std::cout << "🛑 AI Worker " << workerId_ << " stopped" << std::endl;
}

void AIWorker::shutdown() {
    stop();
}

bool AIWorker::assignTask(const ProcessingTask& task) {
    if (!active_ || status_ != IDLE) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        taskQueue_.push(task);
    }
    
    queueCondition_.notify_one();
    return true;
}

bool AIWorker::hasCapacity() const {
    return active_ && status_ == IDLE && taskQueue_.empty();
}

WorkerInfo AIWorker::getWorkerInfo() const {
    WorkerInfo info;
    info.workerId = workerId_;
    info.status = status_;
    info.currentTask = (status_ == BUSY) ? "Processing" : "";
    info.lastActivity = std::chrono::system_clock::now();
    info.tasksCompleted = tasksCompleted_;
    info.tasksFailed = tasksFailed_;
    info.averageProcessingTime = getAverageProcessingTime();
    
    return info;
}

double AIWorker::getAverageProcessingTime() const {
    int totalTasks = tasksCompleted_ + tasksFailed_;
    return (totalTasks > 0) ? (totalProcessingTime_ / totalTasks) : 0.0;
}

void AIWorker::setCapabilities(bool video, bool image, bool audio) {
    canProcessVideo_ = video;
    canProcessImage_ = image;
    canProcessAudio_ = audio;
    
    std::cout << "🔧 AI Worker " << workerId_ << " capabilities set: "
              << "Video=" << canProcessVideo_ 
              << ", Image=" << canProcessImage_
              << ", Audio=" << canProcessAudio_ << std::endl;
}

bool AIWorker::canHandleTask(const ProcessingTask& task) const {
    // For now, all workers can handle video chunks
    return canProcessVideo_;
}

void AIWorker::workerLoop() {
    std::cout << "🔄 AI Worker " << workerId_ << " loop started" << std::endl;
    
    while (active_) {
        try {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Wait for task or shutdown signal
            queueCondition_.wait(lock, [this] {
                return !taskQueue_.empty() || !active_;
            });
            
            if (!active_) {
                break;
            }
            
            if (taskQueue_.empty()) {
                continue;
            }
            
            // Get next task
            ProcessingTask task = taskQueue_.front();
            taskQueue_.pop();
            lock.unlock();
            
            // Process the task
            status_ = BUSY;
            auto startTime = std::chrono::high_resolution_clock::now();
            
            bool success = false;
            try {
                success = processTask(task);
            } catch (const std::exception& e) {
                std::cerr << "❌ AI Worker " << workerId_ << " - Task processing exception: " << e.what() << std::endl;
                success = false;
            } catch (...) {
                std::cerr << "❌ AI Worker " << workerId_ << " - Unknown task processing exception" << std::endl;
                success = false;
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            double processingTime = std::chrono::duration<double, std::chrono::milliseconds::period>(
                endTime - startTime).count();
            
            // Update statistics
            updateStatistics(success, processingTime);
            
            status_ = IDLE;
            
            std::cout << "📋 AI Worker " << workerId_ << " completed task " << task.taskId 
                      << " (" << (success ? "SUCCESS" : "FAILED") << ", " 
                      << processingTime << "ms)" << std::endl;
                      
        } catch (const std::exception& e) {
            std::cerr << "❌ AI Worker " << workerId_ << " - Worker loop exception: " << e.what() << std::endl;
            status_ = IDLE;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (...) {
            std::cerr << "❌ AI Worker " << workerId_ << " - Unknown worker loop exception" << std::endl;
            status_ = IDLE;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    std::cout << "🔄 AI Worker " << workerId_ << " loop ended" << std::endl;
}

bool AIWorker::processTask(const ProcessingTask& task) {
    try {
        // Check memory limits before processing (Low Memory Footprint)
        if (!checkMemoryLimits()) {
            std::cerr << "❌ AI Worker " << workerId_ << " - Memory limit exceeded, skipping task" << std::endl;
            return false;
        }
        
        // Process video chunk (main functionality) - Zero-Copy
        if (task.videoPath.find(".mp4") != std::string::npos || 
            task.videoPath.find(".avi") != std::string::npos ||
            task.videoPath.find(".mov") != std::string::npos) {
            return processVideoChunkZeroCopy(task);
        }
        
        // Process image if needed - Zero-Copy
        if (task.videoPath.find(".jpg") != std::string::npos || 
            task.videoPath.find(".png") != std::string::npos) {
            return processImageEnhancementZeroCopy(task);
        }
        
        // Process audio if needed - Zero-Copy
        if (task.videoPath.find(".wav") != std::string::npos || 
            task.videoPath.find(".mp3") != std::string::npos) {
            return processAudioEnhancementZeroCopy(task);
        }
        
        std::cerr << "❌ AI Worker " << workerId_ << " - Unknown task type: " << task.videoPath << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ AI Worker " << workerId_ << " - Task processing error: " << e.what() << std::endl;
        return false;
    }
}

bool AIWorker::processVideoChunkZeroCopy(const ProcessingTask& task) {
    std::cout << "🎬 AI Worker " << workerId_ << " processing video chunk (Zero-Copy) " 
              << task.chunkIndex << " from " << task.videoPath << std::endl;
    
    if (!ioManager_) {
        std::cerr << "❌ Zero-Copy IO Manager not available" << std::endl;
        return false;
    }
    
    try {
        // Map input data for reading (Zero-Copy - no RAM copy)
        if (!mapInputData(task.videoPath)) {
            return false;
        }
        
        // Simulate video processing (in real implementation, this would process the mapped data directly)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + (rand() % 2000)));
        
        // Generate output data (simulated processed chunk)
        const size_t outputSize = 1024 * 1024; // 1MB simulated output
        std::vector<uint8_t> outputData(outputSize);
        for (size_t i = 0; i < outputSize; ++i) {
            outputData[i] = rand() % 256; // Random data simulation
        }
        
        // Write output directly to disk (Zero-Copy)
        if (!writeOutputDirect(outputData.data(), outputData.size(), task.outputPath)) {
            unmapInputData();
            return false;
        }
        
        // Unmap input data
        unmapInputData();
        
        std::cout << "✅ Zero-Copy video processing completed: " << task.outputPath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        unmapInputData();
        std::cerr << "❌ Zero-Copy video processing error: " << e.what() << std::endl;
        return false;
    }
}

bool AIWorker::processImageEnhancementZeroCopy(const ProcessingTask& task) {
    std::cout << "🖼️ AI Worker " << workerId_ << " enhancing image (Zero-Copy): " << task.videoPath << std::endl;
    
    if (!ioManager_) {
        std::cerr << "❌ Zero-Copy IO Manager not available" << std::endl;
        return false;
    }
    
    try {
        // Map image data for reading
        if (!mapInputData(task.videoPath)) {
            return false;
        }
        
        // Simulate image enhancement (direct processing on mapped data)
        std::this_thread::sleep_for(std::chrono::milliseconds(500 + (rand() % 1000)));
        
        // Generate enhanced output
        const size_t outputSize = 512 * 1024; // 512KB simulated enhanced image
        std::vector<uint8_t> outputData(outputSize);
        for (size_t i = 0; i < outputSize; ++i) {
            outputData[i] = rand() % 256;
        }
        
        // Write directly to disk
        if (!writeOutputDirect(outputData.data(), outputData.size(), task.outputPath)) {
            unmapInputData();
            return false;
        }
        
        unmapInputData();
        std::cout << "✅ Zero-Copy image enhancement completed: " << task.outputPath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        unmapInputData();
        std::cerr << "❌ Zero-Copy image enhancement error: " << e.what() << std::endl;
        return false;
    }
}

bool AIWorker::processAudioEnhancementZeroCopy(const ProcessingTask& task) {
    std::cout << "🎵 AI Worker " << workerId_ << " enhancing audio (Zero-Copy): " << task.videoPath << std::endl;
    
    if (!ioManager_) {
        std::cerr << "❌ Zero-Copy IO Manager not available" << std::endl;
        return false;
    }
    
    try {
        // Map audio data for reading
        if (!mapInputData(task.videoPath)) {
            return false;
        }
        
        // Simulate audio enhancement
        std::this_thread::sleep_for(std::chrono::milliseconds(800 + (rand() % 1200)));
        
        // Generate enhanced audio output
        const size_t outputSize = 2 * 1024 * 1024; // 2MB simulated enhanced audio
        std::vector<uint8_t> outputData(outputSize);
        for (size_t i = 0; i < outputSize; ++i) {
            outputData[i] = rand() % 256;
        }
        
        // Write directly to disk
        if (!writeOutputDirect(outputData.data(), outputData.size(), task.outputPath)) {
            unmapInputData();
            return false;
        }
        
        unmapInputData();
        std::cout << "✅ Zero-Copy audio enhancement completed: " << task.outputPath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        unmapInputData();
        std::cerr << "❌ Zero-Copy audio enhancement error: " << e.what() << std::endl;
        return false;
    }
}

// RAII Memory Mapping Helper Class - FIXED for proper cleanup
class MemoryMappingRAII {
private:
    Zero_Copy_IO_Manager* ioManager_;
    MappedRegion* mapping_;
    std::string filePath_;
    bool isMapped_;
    
public:
    MemoryMappingRAII(Zero_Copy_IO_Manager* ioManager) 
        : ioManager_(ioManager), mapping_(nullptr), isMapped_(false) {}
    
    ~MemoryMappingRAII() {
        // FIXED: RAII cleanup - always unmap in destructor
        unmap();
    }
    
    bool map(const std::string& filePath) {
        unmap(); // Ensure previous mapping is cleared
        
        if (!ioManager_) {
            return false;
        }
        
        mapping_ = ioManager_->mapFileForReading(filePath);
        if (mapping_) {
            filePath_ = filePath;
            isMapped_ = true;
            return true;
        }
        return false;
    }
    
    void unmap() {
        if (isMapped_ && mapping_ && ioManager_) {
            try {
                ioManager_->unmapFile(mapping_->filePath);
            } catch (...) {
                // Ignore exceptions in destructor
            }
            mapping_ = nullptr;
            filePath_.clear();
            isMapped_ = false;
        }
    }
    
    MappedRegion* get() const { return mapping_; }
    size_t size() const { return mapping_ ? mapping_->size : 0; }
    bool isMapped() const { return isMapped_; }
    
    // Prevent copying
    MemoryMappingRAII(const MemoryMappingRAII&) = delete;
    MemoryMappingRAII& operator=(const MemoryMappingRAII&) = delete;
    
    // Allow moving
    MemoryMappingRAII(MemoryMappingRAII&& other) noexcept 
        : ioManager_(other.ioManager_), mapping_(other.mapping_), 
          filePath_(std::move(other.filePath_)), isMapped_(other.isMapped_) {
        other.mapping_ = nullptr;
        other.isMapped_ = false;
    }
    
    MemoryMappingRAII& operator=(MemoryMappingRAII&& other) noexcept {
        if (this != &other) {
            unmap();
            ioManager_ = other.ioManager_;
            mapping_ = other.mapping_;
            filePath_ = std::move(other.filePath_);
            isMapped_ = other.isMapped_;
            other.mapping_ = nullptr;
            other.isMapped_ = false;
        }
        return *this;
    }
};

// Zero-Copy helpers - FIXED with proper RAII
bool AIWorker::mapInputData(const std::string& inputPath) {
    // FIXED: Use local RAII instance for automatic cleanup
    if (!currentMapping_) {
        currentMapping_ = std::make_unique<MemoryMappingRAII>(ioManager_);
    }
    
    if (!currentMapping_->map(inputPath)) {
        std::cerr << "❌ Failed to map input file: " << inputPath << std::endl;
        return false;
    }
    
    inputMapping_ = currentMapping_->get();
    currentMemoryUsage_ = currentMapping_->size();
    
    std::cout << "🗺️ Mapped input file: " << inputPath << " (" << currentMemoryUsage_ << " bytes)" << std::endl;
    return true;
}

void AIWorker::unmapInputData() {
    // FIXED: RAII handles cleanup automatically
    if (currentMapping_) {
        currentMapping_->unmap();
        currentMemoryUsage_ = 0;
        inputMapping_ = nullptr;
        std::cout << "🗺️ Unmapped input file" << std::endl;
    }
}

bool AIWorker::writeOutputDirect(void* data, size_t size, const std::string& outputPath) {
    if (!ioManager_) {
        return false;
    }
    
    // Queue block write request (Direct I/O)
    BlockWriteRequest request;
    request.workerId = workerId_;
    request.chunkIndex = 0; // Not used for output
    request.data = data;
    request.dataSize = size;
    request.targetPath = outputPath;
    
    return ioManager_->queueBlockWrite(request);
}

bool AIWorker::checkMemoryLimits() const {
    if (!ioManager_) {
        return true; // No limits if no IO manager
    }
    
    const size_t maxMemoryPerWorker = 50 * 1024 * 1024; // 50MB per worker
    return currentMemoryUsage_ <= maxMemoryPerWorker;
}

void AIWorker::updateStatistics(bool success, double processingTime) {
    if (success) {
        tasksCompleted_++;
    } else {
        tasksFailed_++;
    }
    
    totalProcessingTime_ += processingTime;
}

// WorkerManager Implementation

WorkerManager::WorkerManager(Zero_Copy_IO_Manager* ioManager) 
    : ioManager_(ioManager), totalMemoryUsage_(0),
      nextWorkerId_(1), activeWorkers_(0), managerActive_(false),
      totalTasksAssigned_(0), totalTasksCompleted_(0), totalTasksFailed_(0) {
    
    if (!ioManager_) {
        std::cerr << "❌ Worker Manager created without IO Manager - Zero-Copy disabled" << std::endl;
    } else {
        std::cout << "🏭 Worker Manager initializing with Zero-Copy IO Manager" << std::endl;
        
        // Set up memory pressure callback
        ioManager_->setMemoryPressureCallback([this]() {
            this->handleMemoryPressure();
        });
        
        // Set up alert callback for disk space warnings
        ioManager_->setAlertCallback([this](const std::string& message) {
            std::cout << "🚨 WORKER_MANAGER ALERT: " << message << std::endl;
        });
    }
}

WorkerManager::~WorkerManager() {
    shutdown();
    std::cout << "🏭 Worker Manager destroyed" << std::endl;
}

bool WorkerManager::initialize() {
    // Create initial worker pool
    for (int i = 0; i < maxWorkers_; ++i) {
        if (!addWorker()) {
            std::cerr << "❌ Failed to create worker " << i + 1 << std::endl;
            return false;
        }
    }
    
    std::cout << "✅ Worker Manager initialized with " << workers_.size() << " workers" << std::endl;
    return true;
}

void WorkerManager::start() {
    if (managerActive_) {
        return;
    }
    
    managerActive_ = true;
    managerThread_ = std::thread(&WorkerManager::managerLoop, this);
    
    // Start all workers
    for (auto& worker : workers_) {
        worker->start();
    }
    
    std::cout << "🚀 Worker Manager started - Factory operational" << std::endl;
}

void WorkerManager::stop() {
    if (!managerActive_) {
        return;
    }
    
    managerActive_ = false;
    tasksCondition_.notify_all();
    
    if (managerThread_.joinable()) {
        managerThread_.join();
    }
    
    // Stop all workers
    for (auto& worker : workers_) {
        worker->stop();
    }
    
    std::cout << "🛑 Worker Manager stopped - Factory offline" << std::endl;
}

void WorkerManager::shutdown() {
    stop();
    
    // Shutdown all workers
    for (auto& worker : workers_) {
        worker->shutdown();
    }
    
    workers_.clear();
    workerStatus_.clear();
    activeWorkers_ = 0;
    
    std::cout << "🔌 Worker Manager shutdown - Factory closed" << std::endl;
}

bool WorkerManager::submitTask(const ProcessingTask& task) {
    if (!managerActive_) {
        std::cerr << "❌ Worker Manager not active" << std::endl;
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        pendingTasks_.push(task);
        totalTasksAssigned_++;
    }
    
    tasksCondition_.notify_one();
    
    std::cout << "📋 Task submitted: " << task.taskId << " (Priority: " << task.priority << ")" << std::endl;
    return true;
}

bool WorkerManager::submitTaskBatch(const std::vector<ProcessingTask>& tasks) {
    bool allSuccess = true;
    
    for (const auto& task : tasks) {
        if (!submitTask(task)) {
            allSuccess = false;
        }
    }
    
    std::cout << "📦 Task batch submitted: " << tasks.size() << " tasks" << std::endl;
    return allSuccess;
}

bool WorkerManager::addWorker() {
    if (workers_.size() >= maxWorkers_) {
        std::cerr << "❌ Maximum workers reached" << std::endl;
        return false;
    }
    
    int workerId = nextWorkerId_++;
    auto worker = std::make_unique<AIWorker>(workerId);
    
    // Set worker capabilities (all workers can process video)
    worker->setCapabilities(true, true, false);
    
    workers_.push_back(std::move(worker));
    activeWorkers_++;
    
    std::cout << "➕ Worker added: " << workerId << " (Total: " << workers_.size() << ")" << std::endl;
    return true;
}

bool WorkerManager::removeWorker(int workerId) {
    auto it = std::find_if(workers_.begin(), workers_.end(),
        [workerId](const std::unique_ptr<AIWorker>& w) {
            return w->getWorkerInfo().workerId == workerId;
        });
    
    if (it == workers_.end()) {
        std::cerr << "❌ Worker not found: " << workerId << std::endl;
        return false;
    }
    
    // Shutdown and remove worker
    (*it)->shutdown();
    workers_.erase(it);
    activeWorkers_--;
    
    std::cout << "➖ Worker removed: " << workerId << " (Remaining: " << workers_.size() << ")" << std::endl;
    return true;
}

bool WorkerManager::replaceFailedWorker(int failedWorkerId) {
    std::cout << "🔄 Replacing failed worker: " << failedWorkerId << std::endl;
    
    if (!removeWorker(failedWorkerId)) {
        return false;
    }
    
    return addWorker();
}

std::vector<WorkerInfo> WorkerManager::getAllWorkerStatus() const {
    std::vector<WorkerInfo> status;
    
    for (const auto& worker : workers_) {
        status.push_back(worker->getWorkerInfo());
    }
    
    return status;
}

WorkerInfo WorkerManager::getWorkerStatus(int workerId) const {
    for (const auto& worker : workers_) {
        WorkerInfo info = worker->getWorkerInfo();
        if (info.workerId == workerId) {
            return info;
        }
    }
    
    return WorkerInfo{}; // Return empty if not found
}

int WorkerManager::getActiveWorkerCount() const {
    int count = 0;
    for (const auto& worker : workers_) {
        if (worker->getStatus() == BUSY) {
            count++;
        }
    }
    return count;
}

int WorkerManager::getAvailableWorkerCount() const {
    int count = 0;
    for (const auto& worker : workers_) {
        if (worker->hasCapacity()) {
            count++;
        }
    }
    return count;
}

double WorkerManager::getSystemEfficiency() const {
    int totalTasks = totalTasksCompleted_ + totalTasksFailed_;
    return (totalTasks > 0) ? (static_cast<double>(totalTasksCompleted_) / totalTasks * 100.0) : 0.0;
}

void WorkerManager::managerLoop() {
    std::cout << "👨‍💼 Worker Manager loop started" << std::endl;
    
    while (managerActive_) {
        try {
            // Assign pending tasks to available workers
            assignPendingTasks();
            
            // Monitor worker health
            monitorWorkerHealth();
            
            // Update system statistics
            updateSystemStatistics();
            
            // Optimize worker distribution periodically
            static auto lastOptimization = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastOptimization).count() >= 30) {
                optimizeWorkerDistribution();
                lastOptimization = now;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        } catch (const std::exception& e) {
            std::cerr << "❌ Error in manager loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cout << "👨‍💼 Worker Manager loop ended" << std::endl;
}

void WorkerManager::assignPendingTasks() {
    std::unique_lock<std::mutex> lock(tasksMutex_);
    
    while (!pendingTasks_.empty()) {
        ProcessingTask task = pendingTasks_.front();
        
        // FIXED: Atomic task assignment - find and assign under same lock
        int workerId = -1;
        AIWorker* targetWorker = nullptr;
        
        // Find best available worker under lock - get worker snapshot first
        for (auto& worker : workers_) {
            WorkerInfo info = worker->getWorkerInfo();
            if (info.status == IDLE && !info.isTerminated) {
                workerId = info.workerId;
                targetWorker = worker.get();
                break;
            }
        }
        
        if (workerId == -1 || !targetWorker) {
            break; // No available workers
        }
        
        // FIXED: Assign task immediately under same lock
        if (targetWorker->assignTask(task)) {
            pendingTasks_.pop();
            std::cout << "📋 Task " << task.taskId << " assigned to Worker " << workerId << std::endl;
        } else {
            // Assignment failed, break to prevent infinite loop
            break;
        }
    }
}

void WorkerManager::monitorWorkerHealth() {
    for (auto& worker : workers_) {
        WorkerInfo info = worker->getWorkerInfo();
        
        // Check for failed workers
        if (info.status == FAILED) {
            std::cout << "⚠️ Worker " << info.workerId << " failed - attempting replacement" << std::endl;
            handleWorkerFailure(info.workerId);
        }
        
        // Update worker status map
        workerStatus_[info.workerId] = info;
    }
}

void WorkerManager::handleWorkerFailure(int workerId) {
    // Replace failed worker with new one
    replaceFailedWorker(workerId);
}

void WorkerManager::updateSystemStatistics() {
    // Aggregate statistics from all workers
    int completed = 0;
    int failed = 0;
    
    for (const auto& worker : workers_) {
        completed += worker->getTasksCompleted();
        failed += worker->getTasksFailed();
    }
    
    totalTasksCompleted_ = completed;
    totalTasksFailed_ = failed;
}

bool WorkerManager::isWorkerHealthy(const WorkerInfo& worker) const {
    return worker.status != FAILED && worker.status != SHUTDOWN;
}

void WorkerManager::optimizeWorkerDistribution() {
    // Check if we need to rebalance tasks
    int availableWorkers = getAvailableWorkerCount();
    int pendingTasks = pendingTasks_.size();
    
    if (pendingTasks > availableWorkers * 2) {
        std::cout << "⚖️ High task load detected - considering worker optimization" << std::endl;
        // Could implement more sophisticated optimization here
    }
}

// ⚠️ FIXED: Added new function to avoid race condition
int WorkerManager::selectBestWorkerFromSnapshot(const ProcessingTask& task, const std::vector<WorkerInfo>& workerSnapshot) {
    // Simple round-robin selection for now
    // Could implement more sophisticated load balancing
    
    for (const auto& worker : workerSnapshot) {
        if (worker.status == IDLE && !worker.isTerminated) {
            return worker.workerId;
        }
    }
    
    return -1; // No available worker
}

int WorkerManager::selectBestWorker(const ProcessingTask& task) {
    // ⚠️ DEPRECATED: Use selectBestWorkerFromSnapshot instead
    std::vector<WorkerInfo> workerSnapshot = getAllWorkerStatus();
    return selectBestWorkerFromSnapshot(task, workerSnapshot);
}

void WorkerManager::rebalanceTasks() {
    // Implement task rebalancing logic if needed
    std::cout << "⚖️ Rebalancing tasks across workers" << std::endl;
}

// Memory management (Low Memory Footprint)
bool WorkerManager::isMemoryPressureHigh() const {
    if (!ioManager_) {
        return false;
    }
    
    // Check if total memory usage exceeds 80% of max
    return totalMemoryUsage_ > (ioManager_->getMaxMemoryUsage() * 0.8);
}

void WorkerManager::handleMemoryPressure() {
    std::cout << "🧠 Memory pressure detected - Worker Manager taking action" << std::endl;
    
    // Update memory usage from all workers
    updateMemoryUsage();
    
    // If memory pressure is high, pause some workers
    if (isMemoryPressureHigh()) {
        std::cout << "⚠️ High memory pressure - pausing non-critical workers" << std::endl;
        
        // Could implement worker pausing logic here
        // For now, just log the issue
        std::cout << "🧠 Current memory usage: " << (totalMemoryUsage_ / 1024 / 1024) << "MB" << std::endl;
    }
}

void WorkerManager::updateMemoryUsage() {
    totalMemoryUsage_ = 0;
    
    // Sum memory usage from all workers
    for (const auto& worker : workers_) {
        totalMemoryUsage_ += worker->getMemoryUsage();
    }
    
    // Update IO Manager if available
    if (ioManager_) {
        // IO Manager tracks its own memory, but we can log here
        std::cout << "🧠 Total worker memory: " << (totalMemoryUsage_ / 1024 / 1024) << "MB" << std::endl;
    }
}
