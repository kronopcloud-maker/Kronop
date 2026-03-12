#ifndef MERGE_MASTER_HPP
#define MERGE_MASTER_HPP

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <map>

/**
 * Merge_Master.hpp
 * Video Merging System
 * Merges 5 processed chunks back into final video in correct sequence (1,2,3,4,5)
 */

struct ProcessedChunk {
    int chunkIndex;           // 1-5
    std::string originalVideoId;
    std::string processedPath;
    uint64_t processedSize;
    int sourceAI;             // AI that processed this chunk (1-5)
    bool isReceived;
    std::chrono::system_clock::time_point receivedTime;
};

struct MergeJob {
    std::string videoId;
    std::map<int, ProcessedChunk> chunks; // Key: chunkIndex (1-5)
    std::string outputPath;
    bool isCompleted;
    std::chrono::system_clock::time_point startTime;
    int sequenceIndex;
};

class Merge_Master {
public:
    Merge_Master();
    ~Merge_Master();

    // Start/stop merging
    void startMerging();
    void stopMerging();

    // Chunk reception and merging
    bool receiveProcessedChunk(const ProcessedChunk& chunk);
    bool mergeVideoChunks(const std::string& videoId);
    
    // Status and monitoring
    int getPendingJobs() const;
    int getCompletedJobs() const;
    bool isMergingActive() const;
    void getMergingStatus(std::string& status);

private:
    // Merging thread
    std::thread mergingThread_;
    std::atomic<bool> mergingActive_;
    
    // Job management
    std::map<std::string, MergeJob> mergeJobs_;
    mutable std::mutex jobsMutex_;
    std::condition_variable jobsCondition_;
    
    // Output directory
    std::string outputDirectory_;
    
    // Statistics
    std::atomic<int> completedJobsCount_;
    
    // Logging
    std::ofstream logFile_;
    
    // Internal methods
    void mergingLoop();
    bool allChunksReceived(const MergeJob& job);
    bool performMerge(const MergeJob& job);
    bool mergeChunkFiles(const MergeJob& job);
    void logMergeOperation(const MergeJob& job);
    void cleanupCompletedJob(const std::string& videoId);
    std::string getOutputPath(const std::string& videoId);
    bool validateChunkSequence(const MergeJob& job);
    void waitForMissingChunks(MergeJob& job);
};

#endif // MERGE_MASTER_HPP
