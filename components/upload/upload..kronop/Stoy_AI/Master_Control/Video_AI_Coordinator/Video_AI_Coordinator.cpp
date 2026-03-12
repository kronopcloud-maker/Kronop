#include "Video_AI_Coordinator.hpp"
#include "../Input_Collector/Input_Collector.hpp"
#include "../Master_Splitter/Master_Splitter.hpp"
#include "../Merge_Master/Merge_Master.hpp"
#include "../Output_Dispatcher/Output_Dispatcher.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

Video_AI_Coordinator::Video_AI_Coordinator() 
    : systemActive_(false), systemInitialized_(false), nextSequenceIndex_(1), 
      maxConcurrentJobs_(5), completedJobsCount_(0), failedJobsCount_(0) {
    std::cout << "🎯 Video_AI_Coordinator initialized" << std::endl;
    
    // Open log file
    logFile_.open("Video_AI_Coordinator_Log.txt", std::ios::app);
    if (logFile_.is_open()) {
        logFile_ << "\n=== Video_AI_Coordinator Session Started ===" << std::endl;
    }
}

Video_AI_Coordinator::~Video_AI_Coordinator() {
    stopSystem();
    
    if (logFile_.is_open()) {
        logFile_ << "=== Video_AI_Coordinator Session Ended ===" << std::endl;
        logFile_.close();
    }
}

bool Video_AI_Coordinator::initializeSystem() {
    if (systemInitialized_) {
        std::cout << "⚠️ Video_AI_Coordinator: System already initialized" << std::endl;
        return true;
    }
    
    try {
        // Initialize components
        inputCollector_ = std::make_unique<Input_Collector>();
        masterSplitter_ = std::make_unique<Master_Splitter>(*inputCollector_);
        mergeMaster_ = std::make_unique<Merge_Master>();
        outputDispatcher_ = std::make_unique<Output_Dispatcher>();
        
        systemInitialized_ = true;
        
        std::cout << "✅ Video_AI_Coordinator: System initialized successfully" << std::endl;
        logSystemEvent("System Initialized", "All components created");
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Video_AI_Coordinator: Initialization failed: " << e.what() << std::endl;
        logSystemEvent("System Initialization Failed", e.what());
        return false;
    }
}

void Video_AI_Coordinator::startSystem() {
    if (!systemInitialized_) {
        std::cerr << "❌ Video_AI_Coordinator: System not initialized" << std::endl;
        return;
    }
    
    if (systemActive_) {
        std::cout << "⚠️ Video_AI_Coordinator: System already active" << std::endl;
        return;
    }
    
    // Start all components
    inputCollector_->startCollection();
    masterSplitter_->startSplitting();
    mergeMaster_->startMerging();
    outputDispatcher_->startDispatching();
    
    // Start monitoring thread
    systemActive_ = true;
    monitoringThread_ = std::thread(&Video_AI_Coordinator::monitoringLoop, this);
    
    std::cout << "🚀 Video_AI_Coordinator: System started successfully" << std::endl;
    logSystemEvent("System Started", "All components active");
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
    
    // Wait for monitoring thread
    if (monitoringThread_.joinable()) {
        monitoringThread_.join();
    }
    
    std::cout << "🛑 Video_AI_Coordinator: System stopped" << std::endl;
    logSystemEvent("System Stopped", "All components deactivated");
}

bool Video_AI_Coordinator::processVideo(const std::string& videoPath, const std::string& videoId) {
    if (!systemActive_) {
        std::cerr << "❌ Video_AI_Coordinator: System not active" << std::endl;
        return false;
    }
    
    if (!canAcceptNewJob()) {
        std::cerr << "❌ Video_AI_Coordinator: Maximum concurrent jobs reached" << std::endl;
        return false;
    }
    
    if (!validateVideoPath(videoPath)) {
        std::cerr << "❌ Video_AI_Coordinator: Invalid video path: " << videoPath << std::endl;
        return false;
    }
    
    // Create processing job
    std::string actualVideoId = videoId.empty() ? generateVideoId(videoPath) : videoId;
    VideoProcessingJob job = createJob(videoPath, actualVideoId);
    
    // Add to job tracking
    {
        std::lock_guard<std::mutex> lock(jobsMutex_);
        processingJobs_[actualVideoId] = job;
    }
    
    // Start processing by sending to input collector
    bool success = inputCollector_->receiveVideo(videoPath, actualVideoId);
    
    if (success) {
        updateJobStatus(actualVideoId, COLLECTING);
        std::cout << "📹 Video_AI_Coordinator: Video processing started - " << actualVideoId << std::endl;
        logSystemEvent("Video Processing Started", actualVideoId + " - " + videoPath);
    } else {
        handleJobFailure(actualVideoId, "Failed to receive video in Input_Collector");
    }
    
    return success;
}

bool Video_AI_Coordinator::processVideoSequence(const std::vector<std::string>& videoPaths) {
    std::cout << "🎬 Video_AI_Coordinator: Processing video sequence of " << videoPaths.size() << " videos" << std::endl;
    
    bool allSuccess = true;
    for (const auto& videoPath : videoPaths) {
        if (!processVideo(videoPath)) {
            allSuccess = false;
        }
        
        // Small delay between videos to ensure proper sequencing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return allSuccess;
}

void Video_AI_Coordinator::setServerUrl(const std::string& serverUrl) {
    if (outputDispatcher_) {
        outputDispatcher_->setServerUrl(serverUrl);
        logSystemEvent("Server URL Updated", serverUrl);
    }
}

void Video_AI_Coordinator::setProcessingTimeout(int timeoutSeconds) {
    if (outputDispatcher_) {
        outputDispatcher_->setDispatchTimeout(timeoutSeconds);
        logSystemEvent("Processing Timeout Updated", std::to_string(timeoutSeconds) + " seconds");
    }
}

void Video_AI_Coordinator::setMaxConcurrentJobs(int maxJobs) {
    maxConcurrentJobs_ = maxJobs;
    std::cout << "⚙️ Video_AI_Coordinator: Max concurrent jobs set to " << maxJobs << std::endl;
    logSystemEvent("Max Concurrent Jobs Updated", std::to_string(maxJobs));
}

void Video_AI_Coordinator::getSystemStatus(std::string& status) {
    if (!systemActive_) {
        status = "🛑 Video_AI_Coordinator: System Inactive";
        return;
    }
    
    std::string inputStatus, splitterStatus, mergeStatus, dispatchStatus;
    inputCollector_->getCollectionStatus(inputStatus);
    masterSplitter_->getSplittingStatus(splitterStatus);
    mergeMaster_->getMergingStatus(mergeStatus);
    outputDispatcher_->getDispatchStatus(dispatchStatus);
    
    status = "🎯 Video_AI_Coordinator: System Active\n";
    status += "  📥 " + inputStatus + "\n";
    status += "  🔪 " + splitterStatus + "\n";
    status += "  🔗 " + mergeStatus + "\n";
    status += "  📤 " + dispatchStatus + "\n";
    status += "  📊 Jobs: Active=" + std::to_string(getActiveJobs()) + 
              ", Completed=" + std::to_string(getCompletedJobs()) + 
              ", Failed=" + std::to_string(getFailedJobs());
}

int Video_AI_Coordinator::getActiveJobs() const {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    int active = 0;
    for (const auto& pair : processingJobs_) {
        if (pair.second.currentStatus != COMPLETED && 
            pair.second.currentStatus != ERROR) {
            active++;
        }
    }
    return active;
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
    
    // Sort by sequence index
    std::sort(history.begin(), history.end(), 
        [](const VideoProcessingJob& a, const VideoProcessingJob& b) {
            return a.sequenceIndex < b.sequenceIndex;
        });
    
    return history;
}

bool Video_AI_Coordinator::notifyChunkProcessed(const std::string& videoId, int chunkIndex, const std::string& processedPath) {
    // This method would be called by AI systems when chunks are processed
    ProcessedChunk chunk;
    chunk.chunkIndex = chunkIndex;
    chunk.originalVideoId = videoId;
    chunk.processedPath = processedPath;
    chunk.sourceAI = chunkIndex; // AI number matches chunk index
    chunk.isReceived = true;
    chunk.receivedTime = std::chrono::system_clock::now();
    
    // Get chunk size
    if (fs::exists(processedPath)) {
        chunk.processedSize = fs::file_size(processedPath);
    }
    
    // Send to merge master
    bool success = mergeMaster_->receiveProcessedChunk(chunk);
    
    if (success) {
        updateJobStatus(videoId, PROCESSING);
        std::cout << "✅ Video_AI_Coordinator: Chunk " << chunkIndex << " processed for " << videoId << std::endl;
    }
    
    return success;
}

bool Video_AI_Coordinator::notifyVideoMerged(const std::string& videoId, const std::string& mergedPath) {
    // Send merged video to output dispatcher
    int sequenceIndex = 0;
    {
        std::lock_guard<std::mutex> lock(jobsMutex_);
        auto it = processingJobs_.find(videoId);
        if (it != processingJobs_.end()) {
            sequenceIndex = it->second.sequenceIndex;
        }
    }
    
    bool success = outputDispatcher_->receiveMergedVideo(videoId, mergedPath, sequenceIndex);
    
    if (success) {
        updateJobStatus(videoId, DISPATCHING);
        std::cout << "✅ Video_AI_Coordinator: Video merged for " << videoId << std::endl;
    }
    
    return success;
}

bool Video_AI_Coordinator::notifyVideoDispatched(const std::string& videoId, bool success) {
    if (success) {
        handleJobCompletion(videoId, true);
        std::cout << "✅ Video_AI_Coordinator: Video dispatched successfully for " << videoId << std::endl;
    } else {
        handleJobFailure(videoId, "Failed to dispatch video to server");
    }
    
    return success;
}

void Video_AI_Coordinator::monitoringLoop() {
    while (systemActive_) {
        // Monitor job progress and handle timeouts
        cleanupCompletedJobs();
        
        // Check for stuck jobs
        std::lock_guard<std::mutex> lock(jobsMutex_);
        auto now = std::chrono::system_clock::now();
        
        for (auto& pair : processingJobs_) {
            VideoProcessingJob& job = pair.second;
            
            if (job.currentStatus != COMPLETED && job.currentStatus != ERROR) {
                auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - job.startTime);
                
                if (elapsed.count() > 30) { // 30 minute timeout
                    handleJobFailure(job.videoId, "Processing timeout");
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void Video_AI_Coordinator::updateJobStatus(const std::string& videoId, ProcessingStatus newStatus, const std::string& error) {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    
    auto it = processingJobs_.find(videoId);
    if (it != processingJobs_.end()) {
        it->second.currentStatus = newStatus;
        if (!error.empty()) {
            it->second.errorMessage = error;
        }
        
        if (newStatus == COMPLETED) {
            it->second.completionTime = std::chrono::system_clock::now();
        }
    }
}

VideoProcessingJob Video_AI_Coordinator::createJob(const std::string& videoPath, const std::string& videoId) {
    VideoProcessingJob job;
    job.videoId = videoId;
    job.originalPath = videoPath;
    job.finalPath = ""; // Will be set after dispatch
    job.sequenceIndex = nextSequenceIndex_++;
    job.currentStatus = IDLE;
    job.startTime = std::chrono::system_clock::now();
    
    return job;
}

void Video_AI_Coordinator::logSystemEvent(const std::string& event, const std::string& details) {
    if (logFile_.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile_ << "[" << std::ctime(&time_t) << "] " << event;
        if (!details.empty()) {
            logFile_ << " - " << details;
        }
        logFile_ << std::endl;
    }
}

void Video_AI_Coordinator::cleanupCompletedJobs() {
    std::lock_guard<std::mutex> lock(jobsMutex_);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> toRemove;
    
    for (const auto& pair : processingJobs_) {
        const VideoProcessingJob& job = pair.second;
        
        if (job.currentStatus == COMPLETED || job.currentStatus == ERROR) {
            auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - job.completionTime);
            if (elapsed.count() > 60) { // Keep for 1 hour
                toRemove.push_back(pair.first);
            }
        }
    }
    
    for (const auto& videoId : toRemove) {
        processingJobs_.erase(videoId);
    }
}

bool Video_AI_Coordinator::canAcceptNewJob() {
    return getActiveJobs() < maxConcurrentJobs_;
}

void Video_AI_Coordinator::processNextVideo() {
    // This method can be used for queue-based processing
    // Implementation depends on specific requirements
}

std::string Video_AI_Coordinator::generateVideoId(const std::string& videoPath) {
    std::string filename = fs::path(videoPath).stem().string();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return filename + "_" + std::to_string(timestamp);
}

bool Video_AI_Coordinator::validateVideoPath(const std::string& videoPath) {
    return fs::exists(videoPath) && fs::is_regular_file(videoPath);
}

void Video_AI_Coordinator::handleJobCompletion(const std::string& videoId, bool success) {
    updateJobStatus(videoId, COMPLETED);
    completedJobsCount_++;
    logSystemEvent("Job Completed", videoId);
}

void Video_AI_Coordinator::handleJobFailure(const std::string& videoId, const std::string& error) {
    updateJobStatus(videoId, ERROR, error);
    failedJobsCount_++;
    logSystemEvent("Job Failed", videoId + " - " + error);
}
