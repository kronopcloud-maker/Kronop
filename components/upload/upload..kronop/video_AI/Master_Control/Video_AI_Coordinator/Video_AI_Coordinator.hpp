#ifndef VIDEO_AI_COORDINATOR_HPP
#define VIDEO_AI_COORDINATOR_HPP

#include "../Input_Collector/Input_Collector.hpp"
#include "../Master_Splitter/Master_Splitter.hpp"
#include "../Merge_Master/Merge_Master.hpp"
#include "../Output_Dispatcher/Output_Dispatcher.hpp"
#include "../../IO_Vault/Zero_Copy_IO_Manager.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>

/**
 * Video_AI_Coordinator.hpp
 * Main Coordinator for Video-AI Processing Pipeline
 * Manages: Input_Collector -> Master_Splitter -> Merge_Master -> Output_Dispatcher
 */

enum ProcessingStatus {
    IDLE,
    COLLECTING,
    SPLITTING,
    PROCESSING,
    MERGING,
    DISPATCHING,
    COMPLETED,
    ERROR
};

struct VideoProcessingJob {
    std::string videoId;
    std::string originalPath;
    std::string finalPath;
    int sequenceIndex;
    ProcessingStatus currentStatus;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point completionTime;
    std::string errorMessage;
};

class Video_AI_Coordinator {
public:
    Video_AI_Coordinator();
    ~Video_AI_Coordinator();

    // System control
    bool initializeSystem();
    void startSystem();
    void stopSystem();
    
    // Video processing
    bool processVideo(const std::string& videoPath, const std::string& videoId = "");
    bool processVideoSequence(const std::vector<std::string>& videoPaths);
    
    // Configuration
    void setServerUrl(const std::string& serverUrl);
    void setProcessingTimeout(int timeoutSeconds);
    void setMaxConcurrentJobs(int maxJobs);
    
    // Status and monitoring
    void getSystemStatus(std::string& status);
    int getActiveJobs() const;
    int getCompletedJobs() const;
    int getFailedJobs() const;
    std::vector<VideoProcessingJob> getJobHistory() const;
    
    // AI integration callbacks
    bool notifyChunkProcessed(const std::string& videoId, int chunkIndex, const std::string& processedPath);
    bool notifyVideoMerged(const std::string& videoId, const std::string& mergedPath);
    bool notifyVideoDispatched(const std::string& videoId, bool success);

private:
    // Component instances
    std::unique_ptr<Input_Collector> inputCollector_;
    std::unique_ptr<Master_Splitter> masterSplitter_;
    std::unique_ptr<Merge_Master> mergeMaster_;
    std::unique_ptr<Output_Dispatcher> outputDispatcher_;
    
    // System state
    std::atomic<bool> systemActive_;
    std::atomic<bool> systemInitialized_;
    
    // Job management
    std::map<std::string, VideoProcessingJob> processingJobs_;
    mutable std::mutex jobsMutex_;
    std::atomic<int> nextSequenceIndex_;
    std::atomic<int> maxConcurrentJobs_;
    
    // Statistics
    std::atomic<int> completedJobsCount_;
    std::atomic<int> failedJobsCount_;
    
    // Monitoring thread
    std::thread monitoringThread_;
    
    // Logging
    std::ofstream logFile_;
    
    // Internal methods
    void monitoringLoop();
    void updateJobStatus(const std::string& videoId, ProcessingStatus newStatus, const std::string& error = "");
    VideoProcessingJob createJob(const std::string& videoPath, const std::string& videoId);
    void logSystemEvent(const std::string& event, const std::string& details = "");
    void cleanupCompletedJobs();
    bool canAcceptNewJob();
    void processNextVideo();
    std::string generateVideoId(const std::string& videoPath);
    bool validateVideoPath(const std::string& videoPath);
    void handleJobCompletion(const std::string& videoId, bool success);
    void handleJobFailure(const std::string& videoId, const std::string& error);
};

#endif // VIDEO_AI_COORDINATOR_HPP
