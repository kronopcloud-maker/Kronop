#ifndef WORKER_MANAGER_HPP
#define WORKER_MANAGER_HPP

#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

/**
 * AI Workers Factory - Processing Department
 * ये सिर्फ 'मज़दूर' हैं। जब Master_Control से आदेश मिलेगा, तभी ये जागेंगे
 */

enum WorkerStatus {
    IDLE = 0,
    BUSY = 1,
    FAILED = 2,
    SHUTDOWN = 3,
    // ⚠️ FIXED: Added TERMINATED state for graceful shutdown tracking
    TERMINATED = 4
};

struct ProcessingTask {
    int taskId;
    std::string videoPath;
    int chunkIndex;
    std::string outputPath;
    std::chrono::system_clock::time_point assignedTime;
    int priority;  // 1=high, 2=normal, 3=low
};

struct WorkerInfo {
    int workerId;
    WorkerStatus status;
    std::string currentTask;
    std::chrono::system_clock::time_point lastActivity;
    int tasksCompleted;
    int tasksFailed;
    double averageProcessingTime;
    // ⚠️ FIXED: Added TERMINATED state for proper cleanup tracking
    bool isTerminated = false;
};

class AIWorker {
private:
    int workerId_;
    std::atomic<WorkerStatus> status_;
    std::thread workerThread_;
    std::queue<ProcessingTask> taskQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::atomic<bool> active_;
    
    // Zero-Copy additions
    Zero_Copy_IO_Manager* ioManager_;  // Direct disk access
    MappedRegion* inputMapping_;       // Memory-mapped input
    std::unique_ptr<class MemoryMappingRAII> currentMapping_;  // RAII memory management
    size_t currentMemoryUsage_;        // Track memory footprint
    
    // Worker statistics
    std::atomic<int> tasksCompleted_;
    std::atomic<int> tasksFailed_;
    std::chrono::system_clock::time_point startTime_;
    double totalProcessingTime_;
    
    // Processing capabilities
    bool canProcessVideo_;
    bool canProcessImage_;
    bool canProcessAudio_;
    
public:
    AIWorker(int workerId, Zero_Copy_IO_Manager* ioManager = nullptr);
    ~AIWorker();
    
    // Worker lifecycle
    bool start();
    void stop();
    void shutdown();
    
    // Task management (Zero-Copy)
    bool assignTask(const ProcessingTask& task);
    bool hasCapacity() const;
    WorkerStatus getStatus() const { return status_; }
    
    // Statistics
    WorkerInfo getWorkerInfo() const;
    int getTasksCompleted() const { return tasksCompleted_; }
    int getTasksFailed() const { return tasksFailed_; }
    double getAverageProcessingTime() const;
    size_t getMemoryUsage() const { return currentMemoryUsage_; }
    
    // Capabilities
    void setCapabilities(bool video, bool image, bool audio);
    bool canHandleTask(const ProcessingTask& task) const;

private:
    void workerLoop();
    bool processTask(const ProcessingTask& task);
    bool processVideoChunkZeroCopy(const ProcessingTask& task);
    bool processImageEnhancementZeroCopy(const ProcessingTask& task);
    bool processAudioEnhancementZeroCopy(const ProcessingTask& task);
    void updateStatistics(bool success, double processingTime);
    
    // Zero-Copy helpers
    bool mapInputData(const std::string& inputPath);
    void unmapInputData();
    bool writeOutputDirect(void* data, size_t size, const std::string& outputPath);
    bool checkMemoryLimits() const;
};

class WorkerManager {
private:
    // Worker pool
    std::vector<std::unique_ptr<AIWorker>> workers_;
    std::map<int, WorkerInfo> workerStatus_;
    
    // Task management
    std::queue<ProcessingTask> pendingTasks_;
    std::mutex tasksMutex_;
    std::condition_variable tasksCondition_;
    std::atomic<bool> managerActive_;
    
    // Load balancing
    std::atomic<int> nextWorkerId_;
    std::atomic<int> activeWorkers_;
    const int maxWorkers_ = 5;
    
    // Manager thread
    std::thread managerThread_;
    
    // Statistics
    std::atomic<int> totalTasksAssigned_;
    std::atomic<int> totalTasksCompleted_;
    std::atomic<int> totalTasksFailed_;
    
    // Zero-Copy IO Manager (No RAM buffering)
    Zero_Copy_IO_Manager* ioManager_;
    size_t totalMemoryUsage_;        // Track total memory across all workers
    
public:
    WorkerManager(Zero_Copy_IO_Manager* ioManager);
    ~WorkerManager();
    
    // Manager lifecycle
    bool initialize();
    void start();
    void stop();
    void shutdown();
    
    // Task assignment (Zero-Copy)
    bool submitTask(const ProcessingTask& task);
    bool submitTaskBatch(const std::vector<ProcessingTask>& tasks);
    
    // Worker management
    bool addWorker();
    bool removeWorker(int workerId);
    bool replaceFailedWorker(int failedWorkerId);
    
    // Status and monitoring
    std::vector<WorkerInfo> getAllWorkerStatus() const;
    WorkerInfo getWorkerStatus(int workerId) const;
    int getActiveWorkerCount() const;
    int getAvailableWorkerCount() const;
    size_t getTotalMemoryUsage() const { return totalMemoryUsage_; }
    
    // Statistics
    int getTotalTasksAssigned() const { return totalTasksAssigned_; }
    int getTotalTasksCompleted() const { return totalTasksCompleted_; }
    int getTotalTasksFailed() const { return totalTasksFailed_; }
    double getSystemEfficiency() const;
    
    // Load balancing
    void optimizeWorkerDistribution();
    int selectBestWorker(const ProcessingTask& task);
    void rebalanceTasks();
    
    // Memory management (Low Memory Footprint)
    bool isMemoryPressureHigh() const;
    void handleMemoryPressure();

private:
    void managerLoop();
    void assignPendingTasks();
    void monitorWorkerHealth();
    void handleWorkerFailure(int workerId);
    void updateSystemStatistics();
    bool isWorkerHealthy(const WorkerInfo& worker) const;
    void updateMemoryUsage();
};

#endif // WORKER_MANAGER_HPP
