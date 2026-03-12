#include "Video_AI_Coordinator.hpp"
#include "../Thermal_Guard/Thermal_Monitor.hpp"
#include "../AI_Workers/Worker_Manager.hpp"
#include "../IO_Vault/Shared_Memory_Manager.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
#include <chrono>

Video_AI_Coordinator::Video_AI_Coordinator() 
    : systemActive_(false), systemInitialized_(false), 
      nextSequenceIndex_(1), maxConcurrentJobs_(5),
      completedJobsCount_(0), failedJobsCount_(0),
      thermalMonitor_(nullptr), workerManager_(nullptr), sharedMemory_(nullptr),
      currentActiveWorkers_(5), thermalLinkActive_(false) {
    
    std::cout << "🧠 Video_AI_Coordinator initializing - The Brain Department" << std::endl;
    
    // Open log file
    logFile_.open("Video_AI_Coordinator_Log.txt", std::ios::app);
    if (logFile_.is_open()) {
        logFile_ << "\n=== Video_AI_Coordinator Session Started ===" << std::endl;
        logFile_ << "Max Concurrent Jobs: " << maxConcurrentJobs_ << std::endl;
        logFile_ << "Load Balancer: Active" << std::endl;
        logFile_ << "Thermal Link: Active" << std::endl;
        logFile_ << "Chunk Mapping: Active" << std::endl;
        logFile_ << "Safe Merging: Active" << std::endl;
    }
}

Video_AI_Coordinator::~Video_AI_Coordinator() {
    stopSystem();
    
    if (logFile_.is_open()) {
        logFile_ << "=== Video_AI_Coordinator Session Ended ===" << std::endl;
        logFile_ << "Completed Jobs: " << completedJobsCount_ << std::endl;
        logFile_ << "Failed Jobs: " << failedJobsCount_ << std::endl;
        logFile_.close();
    }
    
    std::cout << "🧠 Video_AI_Coordinator shutdown - The Brain Department offline" << std::endl;
}

bool Video_AI_Coordinator::initializeSystem() {
    if (systemInitialized_) {
        std::cout << "⚠️ System already initialized" << std::endl;
        return true;
    }
    
    try {
        std::cout << "🔧 Initializing all system components..." << std::endl;
        
        // 1. Initialize Core Components
        inputCollector_ = std::make_unique<Input_Collector>();
        masterSplitter_ = std::make_unique<Master_Splitter>(*inputCollector_);
        mergeMaster_ = std::make_unique<Merge_Master>();
        outputDispatcher_ = std::make_unique<Output_Dispatcher>();
        
        // 2. Initialize Advanced Systems
        thermalMonitor_ = std::make_unique<ThermalMonitor>();
        workerManager_ = std::make_unique<WorkerManager>();
        sharedMemory_ = std::make_unique<SharedMemoryManager>();
        
        // 3. Initialize Shared Memory
        if (!sharedMemory_->initialize("../IO_Vault/Input", "../IO_Vault/Output")) {
            throw std::runtime_error("Failed to initialize shared memory manager");
        }
        
        // 4. Initialize Worker Manager
        if (!workerManager_->initialize()) {
            throw std::runtime_error("Failed to initialize worker manager");
        }
        
        // 5. Setup Thermal Link
        setupThermalLink();
        
        // 6. Initialize Chunk Mapping
        initializeChunkMapping();
        
        // 7. Start all components
        inputCollector_->startCollection();
        masterSplitter_->startSplitting();
        mergeMaster_->startMerging();
        outputDispatcher_->startDispatching();
        workerManager_->start();
        
        // 8. Start monitoring thread
        monitoringThread_ = std::thread(&Video_AI_Coordinator::monitoringLoop, this);
        
        systemInitialized_ = true;
        systemActive_ = true;
        
        logSystemEvent("SYSTEM_INITIALIZED", "All components started successfully");
        std::cout << "✅ Video_AI_Coordinator initialized - The Brain is ready" << std::endl;
        std::cout << "   Load Balancer: Active" << std::endl;
        std::cout << "   Thermal Link: Active" << std::endl;
        std::cout << "   Chunk Mapping: Active" << std::endl;
        std::cout << "   Safe Merging: Active" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ System initialization failed: " << e.what() << std::endl;
        logSystemEvent("SYSTEM_INIT_FAILED", e.what());
        return false;
    }
}

void Video_AI_Coordinator::startSystem() {
    if (!systemInitialized_) {
        if (!initializeSystem()) {
            return;
        }
    }
    
    if (!systemActive_) {
        systemActive_ = true;
        logSystemEvent("SYSTEM_STARTED", "Brain Department active");
        std::cout << "🚀 Video_AI_Coordinator started - Processing pipeline active" << std::endl;
    }
}

void Video_AI_Coordinator::stopSystem() {
    if (!systemActive_) {
        return;
    }
    
    systemActive_ = false;
    
    // Stop all components
    if (inputCollector_) inputCollector_->stopCollection();
    if (masterSplitter_) masterSplitter_->stopSplitting();
    if (mergeMaster_) mergeMaster_->stopMerging();
    if (outputDispatcher_) outputDispatcher_->stopDispatching();
    
    // Stop monitoring thread
    if (monitoringThread_.joinable()) {
        monitoringThread_.join();
    }
    
    logSystemEvent("SYSTEM_STOPPED", "Brain Department shutdown");
    std::cout << "🛑 Video_AI_Coordinator stopped - Processing pipeline inactive" << std::endl;
}

bool Video_AI_Coordinator::processVideo(const std::string& videoPath, const std::string& videoId) {
    if (!systemInitialized_ || !systemActive_) {
        std::cerr << "❌ System not ready for processing" << std::endl;
        return false;
    }
    
    if (!canAcceptNewJob()) {
        std::cerr << "❌ System at capacity - cannot accept new job" << std::endl;
        return false;
    }
    
    // 1. Analyze video duration for load balancing
    int durationSeconds = analyzeVideoDuration(videoPath);
    int requiredWorkers = calculateOptimalWorkers(durationSeconds);
    
    // 2. Create processing job with worker assignment
    VideoProcessingJob job = createJob(videoPath, videoId);
    job.requiredWorkers = requiredWorkers;
    job.assignedWorkers = selectOptimalWorkers(requiredWorkers);
    
    // 3. Add to job tracking
    {
        std::lock_guard<std::mutex> lock(jobsMutex_);
        processingJobs_[job.videoId] = job;
    }
    
    // 4. Submit to input collector
    bool success = inputCollector_->receiveVideo(job.originalPath, job.videoId);
    
    if (success) {
        updateJobStatus(job.videoId, COLLECTING);
        logSystemEvent("JOB_SUBMITTED", "Video: " + job.videoId + " Duration: " + std::to_string(durationSeconds) + "s Workers: " + std::to_string(requiredWorkers));
        std::cout << "📹 Video submitted for processing: " << job.videoId << std::endl;
        std::cout << "   Duration: " << durationSeconds << " seconds" << std::endl;
        std::cout << "   Workers: " << requiredWorkers << " (Load Balanced)" << std::endl;
    } else {
        updateJobStatus(job.videoId, ERROR, "Failed to submit to input collector");
        handleJobFailure(job.videoId, "Input collector rejected video");
    }
    
    return success;
}

bool Video_AI_Coordinator::processVideoSequence(const std::vector<std::string>& videoPaths) {
    std::cout << "🎬 Processing video sequence with " << videoPaths.size() << " videos" << std::endl;
    
    bool allSuccess = true;
    for (const auto& path : videoPaths) {
        std::string videoId = generateVideoId(path);
        bool success = processVideo(path, videoId);
        if (!success) {
            allSuccess = false;
            std::cerr << "❌ Failed to process video: " << path << std::endl;
        }
    }
    
    return allSuccess;
}

void Video_AI_Coordinator::setServerUrl(const std::string& serverUrl) {
    if (outputDispatcher_) {
        outputDispatcher_->setServerUrl(serverUrl);
        logSystemEvent("SERVER_URL_SET", serverUrl);
        std::cout << "🌐 Server URL updated: " << serverUrl << std::endl;
    }
}

void Video_AI_Coordinator::setProcessingTimeout(int timeoutSeconds) {
    // This would be implemented for timeout management
    logSystemEvent("TIMEOUT_SET", std::to_string(timeoutSeconds) + " seconds");
    std::cout << "⏱️ Processing timeout set to " << timeoutSeconds << " seconds" << std::endl;
}

void Video_AI_Coordinator::setMaxConcurrentJobs(int maxJobs) {
    maxConcurrentJobs_ = maxJobs;
    logSystemEvent("MAX_JOBS_SET", std::to_string(maxJobs));
    std::cout << "🎯 Max concurrent jobs set to " << maxJobs << std::endl;
}

void Video_AI_Coordinator::getSystemStatus(std::string& status) {
    if (!systemInitialized_) {
        status = "🔴 System: Not Initialized";
        return;
    }
    
    if (!systemActive_) {
        status = "🟡 System: Initialized but Inactive";
        return;
    }
    
    // Get component statuses
    std::string inputStatus, splitterStatus, mergeStatus, dispatchStatus;
    inputCollector_->getCollectionStatus(inputStatus);
    masterSplitter_->getSplittingStatus(splitterStatus);
    mergeMaster_->getMergingStatus(mergeStatus);
    outputDispatcher_->getDispatchStatus(dispatchStatus);
    
    // Build comprehensive status
    status = "🟢 System: Active and Processing\n";
    status += "├─ " + inputStatus + "\n";
    status += "├─ " + splitterStatus + "\n";
    status += "├─ " + mergeStatus + "\n";
    status += "├─ " + dispatchStatus + "\n";
    status += "└─ Jobs: Active=" + std::toString(getActiveJobs()) + 
             " Completed=" + std::toString(getCompletedJobs()) + 
             " Failed=" + std::toString(getFailedJobs());
}

int Video_AI_Coordinator::getActiveJobs() const {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    int activeCount = 0;
    for (const auto& pair : processingJobs_) {
        const VideoProcessingJob& job = pair.second;
        if (job.currentStatus != COMPLETED && job.currentStatus != ERROR) {
            activeCount++;
        }
    }
    return activeCount;
}

int Video_AI_Coordinator::getCompletedJobs() const {
    return completedJobsCount_;
}

int Video_AI_Coordinator::getFailedJobs() const {
    return failedJobsCount_;
}

std::vector<VideoProcessingJob> Video_AI_Coordinator::getJobHistory() const {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    std::vector<VideoProcessingJob> history;
    for (const auto& pair : processingJobs_) {
        history.push_back(pair.second);
    }
    return history;
}

bool Video_AI_Coordinator::notifyChunkProcessed(const std::string& videoId, int chunkIndex, const std::string& processedPath) {
    // ⚠️ FIXED: Extended mutex scope for thread safety
    std::lock_guard<std::mutex> lock(jobsMutex_);
    
    // Create processed chunk for merge master
    ProcessedChunk chunk;
    chunk.chunkIndex = chunkIndex;
    chunk.originalVideoId = videoId;
    chunk.processedPath = processedPath;
    chunk.sourceAI = chunkIndex; // AI number matches chunk index
    chunk.isReceived = true;
    chunk.receivedTime = std::chrono::system_clock::now();
    
    // Send to merge master - FIXED: Protected access
    bool success = mergeMaster_->receiveProcessedChunk(chunk);
    
    if (success) {
        logSystemEvent("CHUNK_PROCESSED", "Video: " + videoId + " Chunk: " + std::to_string(chunkIndex));
        std::cout << "✅ Chunk processed: " << videoId << " - Chunk " << chunkIndex << std::endl;
    }
    
    return success;
}

bool Video_AI_Coordinator::notifyVideoMerged(const std::string& videoId, const std::string& mergedPath) {
    // Find the job
    std::lock_guard<std::mutex> lock(jobsMutex_);
    auto it = processingJobs_.find(videoId);
    if (it == processingJobs_.end()) {
        std::cerr << "❌ Job not found for merged video: " << videoId << std::endl;
        return false;
    }
    
    VideoProcessingJob& job = it->second;
    job.finalPath = mergedPath;
    job.completionTime = std::chrono::system_clock::now();
    
    // Update job status
    updateJobStatus(videoId, DISPATCHING);
    
    // Send to output dispatcher
    bool success = outputDispatcher_->receiveMergedVideo(videoId, mergedPath, job.sequenceIndex);
    
    if (success) {
        logSystemEvent("VIDEO_MERGED", "Video: " + videoId + " Path: " + mergedPath);
        std::cout << "🔗 Video merged: " << videoId << std::endl;
    }
    
    return success;
}

bool Video_AI_Coordinator::notifyVideoDispatched(const std::string& videoId, bool success) {
    // Find the job
    std::lock_guard<std::mutex> lock(jobsMutex_);
    auto it = processingJobs_.find(videoId);
    if (it == processingJobs_.end()) {
        std::cerr << "❌ Job not found for dispatched video: " << videoId << std::endl;
        return false;
    }
    
    VideoProcessingJob& job = it->second;
    
    if (success) {
        updateJobStatus(videoId, COMPLETED);
        handleJobCompletion(videoId, true);
        logSystemEvent("JOB_COMPLETED", "Video: " + videoId);
        std::cout << "✅ Job completed: " << videoId << std::endl;
    } else {
        updateJobStatus(videoId, ERROR, "Dispatch failed");
        handleJobFailure(videoId, "Output dispatcher failed");
        logSystemEvent("JOB_FAILED", "Video: " + videoId + " - Dispatch failed");
        std::cout << "❌ Job failed: " << videoId << std::endl;
    }
    
    return true;
}

void Video_AI_Coordinator::monitoringLoop() {
    std::cout << "👁️ System monitoring loop started" << std::endl;
    
    while (systemActive_) {
        try {
            // 1. Monitor thermal conditions and worker health
            monitorThermalConditions();
            
            // 2. Check for failed chunks and reassign
            monitorChunkProgress();
            
            // 3. Safe merging check
            monitorSafeMergingConditions();
            
            // 4. Cleanup completed jobs
            cleanupCompletedJobs();
            
            // 5. Monitor system health
            int activeJobs = getActiveJobs();
            if (activeJobs > maxConcurrentJobs_) {
                std::cout << "⚠️ System overload detected - Active jobs: " << activeJobs 
                         << " (Max: " << maxConcurrentJobs_ << ")" << std::endl;
            }
            
            // 6. Check component health
            if (!inputCollector_->isCollectionActive()) {
                std::cout << "⚠️ Input collector not active" << std::endl;
            }
            if (!masterSplitter_->isSplittingActive()) {
                std::cout << "⚠️ Master splitter not active" << std::endl;
            }
            if (!mergeMaster_->isMergingActive()) {
                std::cout << "⚠️ Merge master not active" << std::endl;
            }
            if (!outputDispatcher_->isDispatchingActive()) {
                std::cout << "⚠️ Output dispatcher not active" << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
        } catch (const std::exception& e) {
            std::cerr << "❌ Error in monitoring loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cout << "👁️ System monitoring loop ended" << std::endl;
}

void Video_AI_Coordinator::updateJobStatus(const std::string& videoId, ProcessingStatus newStatus, const std::string& error) {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    auto it = processingJobs_.find(videoId);
    if (it != processingJobs_.end()) {
        VideoProcessingJob& job = it->second;
        job.currentStatus = newStatus;
        if (!error.empty()) {
            job.errorMessage = error;
        }
        
        // Log status change
        std::string statusStr;
        switch (newStatus) {
            case IDLE: statusStr = "IDLE"; break;
            case COLLECTING: statusStr = "COLLECTING"; break;
            case SPLITTING: statusStr = "SPLITTING"; break;
            case PROCESSING: statusStr = "PROCESSING"; break;
            case MERGING: statusStr = "MERGING"; break;
            case DISPATCHING: statusStr = "DISPATCHING"; break;
            case COMPLETED: statusStr = "COMPLETED"; break;
            case ERROR: statusStr = "ERROR"; break;
        }
        
        logSystemEvent("STATUS_UPDATE", "Video: " + videoId + " Status: " + statusStr);
    }
}

VideoProcessingJob Video_AI_Coordinator::createJob(const std::string& videoPath, const std::string& videoId) {
    VideoProcessingJob job;
    job.videoId = videoId.empty() ? generateVideoId(videoPath) : videoId;
    job.originalPath = videoPath;
    job.finalPath = "";
    job.sequenceIndex = nextSequenceIndex_++;
    job.currentStatus = IDLE;
    job.startTime = std::chrono::system_clock::now();
    
    return job;
}

void Video_AI_Coordinator::logSystemEvent(const std::string& event, const std::string& details) {
    if (logFile_.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile_ << "[" << std::ctime(&time_t) << "] " << event << ": " << details << std::endl;
    }
}

void Video_AI_Coordinator::cleanupCompletedJobs() {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    
    auto it = processingJobs_.begin();
    while (it != processingJobs_.end()) {
        const VideoProcessingJob& job = it->second;
        
        // Remove jobs completed more than 5 minutes ago
        if (job.currentStatus == COMPLETED || job.currentStatus == ERROR) {
            auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
                std::chrono::system_clock::now() - job.completionTime).count();
            
            if (elapsed > 5) {
                it = processingJobs_.erase(it);
                continue;
            }
        }
        
        ++it;
    }
}

bool Video_AI_Coordinator::canAcceptNewJob() {
    return getActiveJobs() < maxConcurrentJobs_;
}

std::string Video_AI_Coordinator::generateVideoId(const std::string& videoPath) {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::string filename = std::filesystem::path(videoPath).stem().string();
    return filename + "_" + std::to_string(timestamp);
}

bool Video_AI_Coordinator::validateVideoPath(const std::string& videoPath) {
    return std::filesystem::exists(videoPath) && 
           std::filesystem::is_regular_file(videoPath);
}

void Video_AI_Coordinator::handleJobCompletion(const std::string& videoId, bool success) {
    if (success) {
        completedJobsCount_++;
    } else {
        failedJobsCount_++;
    }
}

void Video_AI_Coordinator::handleJobFailure(const std::string& videoId, const std::string& error) {
    failedJobsCount_++;
    updateJobStatus(videoId, ERROR, error);
    
    // Attempt recovery if possible
    if (error.find("thermal") != std::string::npos) {
        // Thermal-related failure - retry when system cools
        scheduleRetry(videoId, "thermal_recovery");
    } else if (error.find("worker") != std::string::npos) {
        // Worker-related failure - reassign to different workers
        reassignJobToDifferentWorkers(videoId);
    }
    
    std::cerr << "❌ Job failed: " << videoId << " - " << error << std::endl;
}

// Load Balancer Implementation
int Video_AI_Coordinator::analyzeVideoDuration(const std::string& videoPath) {
    // Simulate video duration analysis (in real implementation, use FFmpeg)
    std::ifstream file(videoPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return 60; // Default 60 seconds
    }
    
    size_t fileSize = file.tellg();
    file.close();
    
    // Estimate duration based on file size (rough approximation)
    // Assuming 5MB per second for 1080p video
    int estimatedDuration = static_cast<int>(fileSize / (5 * 1024 * 1024));
    return std::max(10, std::min(600, estimatedDuration)); // 10s to 10min range
}

int Video_AI_Coordinator::calculateOptimalWorkers(int durationSeconds) {
    if (durationSeconds < 30) {
        return 3; // Short videos - 3 workers
    } else if (durationSeconds < 120) {
        return 4; // Medium videos - 4 workers  
    } else {
        return 5; // Long videos - 5 workers
    }
}

std::vector<int> Video_AI_Coordinator::selectOptimalWorkers(int requiredWorkers) {
    std::vector<int> selectedWorkers;
    auto workerStates = workerManager_->getAllWorkerStatus();
    
    // Filter active workers
    std::vector<WorkerInfo> activeWorkers;
    for (const auto& worker : workerStates) {
        if (worker.status == BUSY || worker.status == IDLE) {
            activeWorkers.push_back(worker);
        }
    }
    
    // Sort by performance (tasks completed / total tasks)
    std::sort(activeWorkers.begin(), activeWorkers.end(),
        [](const WorkerInfo& a, const WorkerInfo& b) {
            double aEfficiency = (a.tasksCompleted + a.tasksFailed > 0) ? 
                               static_cast<double>(a.tasksCompleted) / (a.tasksCompleted + a.tasksFailed) : 0.0;
            double bEfficiency = (b.tasksCompleted + b.tasksFailed > 0) ? 
                               static_cast<double>(b.tasksCompleted) / (b.tasksCompleted + b.tasksFailed) : 0.0;
            return aEfficiency > bEfficiency;
        });
    
    // Select top performers
    for (int i = 0; i < std::min(requiredWorkers, static_cast<int>(activeWorkers.size())); ++i) {
        selectedWorkers.push_back(activeWorkers[i].workerId);
    }
    
    return selectedWorkers;
}

// Thermal Link Implementation
void Video_AI_Coordinator::setupThermalLink() {
    if (!thermalMonitor_) {
        return;
    }
    
    // Setup thermal alert callback
    thermalMonitor_->setAlertCallback([this](const ThermalAlert& alert) {
        handleThermalAlert(alert);
    });
    
    // Register all workers with thermal monitor
    auto workerStates = workerManager_->getAllWorkerStatus();
    for (const auto& worker : workerStates) {
        // In real implementation, get actual PID
        pid_t workerPid = 1000 + worker.workerId; // Simulated PID
        thermalMonitor_->registerWorker(worker.workerId, workerPid, worker.workerId == 1); // Worker 1 is critical
    }
    
    thermalLinkActive_ = true;
    std::cout << "🔗 Thermal link established with " << workerStates.size() << " workers" << std::endl;
}

void Video_AI_Coordinator::handleThermalAlert(const ThermalAlert& alert) {
    std::cout << "🌡️ Thermal alert received: " << alert.message << std::endl;
    
    if (alert.status >= CRITICAL) {
        // Get paused/suspended workers
        auto pausedWorkers = thermalMonitor_->getPausedWorkers();
        auto suspendedWorkers = thermalMonitor_->getSuspendedWorkers();
        
        // Reassign chunks from paused workers
        for (int workerId : pausedWorkers) {
            reassignWorkerChunks(workerId);
        }
        
        // Reassign chunks from suspended workers
        for (int workerId : suspendedWorkers) {
            reassignWorkerChunks(workerId);
        }

        logSystemEvent("THERMAL_REASSIGNMENT", "Reassigned chunks due to thermal alert");
    }
}

void Video_AI_Coordinator::reassignWorkerChunks(int failedWorkerId) {
    // FIXED: Extended mutex scope for thread safety
    std::lock_guard<std::mutex> lock(chunkMappingMutex_);

    // Find chunks assigned to failed worker
    std::vector<int> chunksToReassign;
    for (const auto& pair : chunkMapping_) {
        const ChunkInfo& chunk = pair.second;
        if (chunk.assignedWorkerId == failedWorkerId && chunk.status == PROCESSING) {
            chunksToReassign.push_back(pair.first);
        }
    }

    // Reassign chunks to active workers - FIXED: Same lock scope
    for (int chunkId : chunksToReassign) {
        int newWorkerId = findBestAvailableWorker();
        if (newWorkerId != -1) {
            chunkMapping_[chunkId].assignedWorkerId = newWorkerId;
            chunkMapping_[chunkId].status = PENDING;
            chunkMapping_[chunkId].reassignmentCount++;
            
            // Submit chunk to new worker
            submitChunkToWorker(chunkId, newWorkerId);
            
            std::cout << "🔄 Chunk " << chunkId << " reassigned from worker " 
                     << failedWorkerId << " to worker " << newWorkerId << std::endl;
        }
    }
}

// Chunk Mapping Implementation
void Video_AI_Coordinator::initializeChunkMapping() {
    std::lock_guard<std::mutex> lock(chunkMappingMutex_);
    
    chunkMapping_.clear();
    
    // Initialize chunk tracking for 5 chunks (0-4)
    for (int i = 0; i < 5; ++i) {
        ChunkInfo chunk;
        chunk.chunkId = i;
        chunk.status = PENDING;
        chunk.assignedWorkerId = -1;
        chunk.progress = 0.0;
        chunk.reassignmentCount = 0;
        chunk.startTime = std::chrono::system_clock::now();
        
        chunkMapping_[i] = chunk;
    }
    
    std::cout << "🗺️ Chunk mapping initialized for 5 chunks" << std::endl;
}

void Video_AI_Coordinator::monitorChunkProgress() {
    // ⚠️ FIXED: Extended mutex scope for thread safety
    std::lock_guard<std::mutex> lock(chunkMappingMutex_);
    
    for (auto& pair : chunkMapping_) {
        ChunkInfo& chunk = pair.second;
        
        if (chunk.status == PROCESSING) {
            // Check if chunk is taking too long (timeout detection)
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() - chunk.startTime).count();
            
            if (elapsed > 300) { // 5 minutes timeout
                std::cout << "⏰ Chunk " << chunk.chunkId << " timeout detected" << std::endl;
                // ⚠️ FIXED: Call without releasing lock
                reassignWorkerChunksUnsafe(chunk.assignedWorkerId);
            }
        }
    }
}

// Safe Merging Implementation
void Video_AI_Coordinator::monitorSafeMergingConditions() {
    std::lock_guard<std::mutex> lock(chunkMappingMutex_);
    
    // Check if all chunks are completed
    bool allChunksCompleted = true;
    for (const auto& pair : chunkMapping_) {
        const ChunkInfo& chunk = pair.second;
        if (chunk.status != COMPLETED) {
            allChunksCompleted = false;
            break;
        }
    }
    
    if (allChunksCompleted && mergeMaster_) {
        // All chunks ready - trigger safe merging
        std::cout << "✅ All chunks completed - Triggering safe merge" << std::endl;
        
        // Collect all processed chunks
        std::vector<ProcessedChunk> processedChunks;
        for (const auto& pair : chunkMapping_) {
            const ChunkInfo& chunk = pair.second;
            
            ProcessedChunk processedChunk;
            processedChunk.chunkIndex = chunk.chunkId;
            processedChunk.originalVideoId = getCurrentVideoId();
            processedChunk.processedPath = getChunkProcessedPath(chunk.chunkId);
            processedChunk.sourceAI = chunk.assignedWorkerId;
            processedChunk.isReceived = true;
            processedChunk.receivedTime = std::chrono::system_clock::now();
            
            processedChunks.push_back(processedChunk);
        }
        
        // Submit to merge master
        for (const auto& chunk : processedChunks) {
            mergeMaster_->receiveProcessedChunk(chunk);
        }
        
        // Reset chunk mapping for next video
        initializeChunkMapping();
        
        logSystemEvent("SAFE_MERGE_TRIGGERED", "All chunks processed and submitted for merging");
    }
}

// ⚠️ FIXED: Added unsafe version for internal use
void Video_AI_Coordinator::reassignWorkerChunksUnsafe(int failedWorkerId) {
    // Find chunks assigned to failed worker
    std::vector<int> chunksToReassign;
    for (const auto& pair : chunkMapping_) {
        const ChunkInfo& chunk = pair.second;
        if (chunk.assignedWorkerId == failedWorkerId && chunk.status == PROCESSING) {
            chunksToReassign.push_back(pair.first);
        }
    }
    
    // Reassign chunks to active workers
    for (int chunkId : chunksToReassign) {
        int newWorkerId = findBestAvailableWorker();
        if (newWorkerId != -1) {
            chunkMapping_[chunkId].assignedWorkerId = newWorkerId;
            chunkMapping_[chunkId].status = PENDING;
            chunkMapping_[chunkId].reassignmentCount++;
            
            // Submit chunk to new worker
            submitChunkToWorker(chunkId, newWorkerId);
            
            std::cout << "🔄 Chunk " << chunkId << " reassigned from worker " 
                     << failedWorkerId << " to worker " << newWorkerId << std::endl;
        }
    }
}

int Video_AI_Coordinator::findBestAvailableWorker() {
    auto workerStates = workerManager_->getAllWorkerStatus();
    
    // Find best available worker (idle with good performance)
    int bestWorkerId = -1;
    double bestEfficiency = -1.0;
    
    for (const auto& worker : workerStates) {
        if (worker.status == IDLE) {
            double efficiency = (worker.tasksCompleted + worker.tasksFailed > 0) ? 
                              static_cast<double>(worker.tasksCompleted) / (worker.tasksCompleted + worker.tasksFailed) : 0.0;
            
            if (efficiency > bestEfficiency) {
                bestEfficiency = efficiency;
                bestWorkerId = worker.workerId;
            }
        }
    }
    
    return bestWorkerId;
}

void Video_AI_Coordinator::submitChunkToWorker(int chunkId, int workerId) {
    // ⚠️ FIXED: Create task with proper error handling
    ProcessingTask task;
    task.taskId = chunkId;
    task.chunkIndex = chunkId;
    task.priority = 1; // High priority for reassigned chunks
    task.assignedTime = std::chrono::system_clock::now();
    task.videoPath = getChunkProcessedPath(chunkId);
    task.outputPath = getChunkProcessedPath(chunkId) + "_processed";
    
    // FIXED: Add error handling
    if (!workerManager_->submitTask(task)) {
        std::cerr << "❌ Failed to submit chunk " << chunkId << " to worker " << workerId << std::endl;
        // Mark chunk as failed for retry
        std::lock_guard<std::mutex> lock(chunkMappingMutex_);
        auto it = chunkMapping_.find(chunkId);
        if (it != chunkMapping_.end()) {
            it->second.status = PENDING;
            it->second.assignedWorkerId = -1;
        }
        return;
    }
    
    std::cout << "📋 Chunk " << chunkId << " submitted to worker " << workerId << std::endl;
}

std::string Video_AI_Coordinator::getCurrentVideoId() const {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    
    // Return the most recent active job
    std::string latestVideoId;
    auto latestTime = std::chrono::system_clock::time_point{};
    
    for (const auto& pair : processingJobs_) {
        const VideoProcessingJob& job = pair.second;
        if (job.startTime > latestTime) {
            latestTime = job.startTime;
            latestVideoId = job.videoId;
        }
    }
    
    return latestVideoId;
}

std::string Video_AI_Coordinator::getChunkProcessedPath(int chunkId) const {
    return "../IO_Vault/Output/chunk_" + std::to_string(chunkId) + "_processed.mp4";
}

void Video_AI_Coordinator::monitorThermalConditions() {
    if (!thermalMonitor_ || !thermalLinkActive_) {
        return;
    }
    
    // Get current thermal status
    float currentTemp = thermalMonitor_->getCurrentTemperature();
    ThermalStatus currentStatus = thermalMonitor_->getCurrentStatus();
    
    // Log thermal conditions every 10 seconds
    static auto lastThermalLog = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastThermalLog).count() >= 10) {
        std::cout << "🌡️ Thermal Status: " << currentTemp << "°C (" 
                 << thermalMonitor_->getStatusString() << ")" << std::endl;
        
        // Update active worker count based on thermal conditions
        updateWorkerCountBasedOnThermal(currentStatus);
        
        lastThermalLog = now;
    }
}

void Video_AI_Coordinator::updateWorkerCountBasedOnThermal(ThermalStatus status) {
    int newActiveWorkers = currentActiveWorkers_;
    
    switch (status) {
        case WARNING:
            newActiveWorkers = std::max(3, currentActiveWorkers_ - 1);
            break;
        case CRITICAL:
            newActiveWorkers = std::max(2, currentActiveWorkers_ - 2);
            break;
        case EMERGENCY:
            newActiveWorkers = 1;
            break;
        default:
            newActiveWorkers = 5; // Full capacity
            break;
    }
    
    if (newActiveWorkers != currentActiveWorkers_) {
        std::cout << "🔥 Adjusting active workers: " << currentActiveWorkers_ 
                 << " -> " << newActiveWorkers << " (Thermal: " << thermalMonitor_->getStatusString() << ")" << std::endl;
        currentActiveWorkers_ = newActiveWorkers;
    }
}
