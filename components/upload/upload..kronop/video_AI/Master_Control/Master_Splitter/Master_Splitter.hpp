#ifndef MASTER_SPLITTER_HPP
#define MASTER_SPLITTER_HPP

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include "../Input_Collector/Input_Collector.hpp"

/**
 * Master_Splitter.hpp
 * Video Splitting and AI Distribution System
 * Splits videos into 5 equal chunks and sends to AI1-AI5 in sequence
 */

struct VideoChunk {
    int chunkIndex;           // 1-5
    std::string originalVideoId;
    std::string chunkPath;
    uint64_t chunkSize;
    uint64_t startPosition;
    uint64_t endPosition;
    int assignedAI;           // AI number (1-5)
    bool isProcessed;
};

struct SplittingJob {
    VideoInput videoInput;
    std::vector<VideoChunk> chunks;
    bool isCompleted;
    std::chrono::system_clock::time_point startTime;
};

class Master_Splitter {
public:
    Master_Splitter(Input_Collector& inputCollector);
    ~Master_Splitter();

    // Start/stop splitting
    void startSplitting();
    void stopSplitting();

    // Splitting operations
    bool splitVideo(const VideoInput& video);
    bool distributeChunksToAI(const std::vector<VideoChunk>& chunks);
    
    // Status and monitoring
    int getActiveJobs() const;
    bool isSplittingActive() const;
    void getSplittingStatus(std::string& status);

private:
    // References
    Input_Collector& inputCollector_;
    
    // Splitting thread
    std::thread splittingThread_;
    std::atomic<bool> splittingActive_;
    
    // Job management
    std::vector<SplittingJob> activeJobs_;
    mutable std::mutex jobsMutex_;
    std::condition_variable jobsCondition_;
    
    // Chunk storage
    std::string chunksDirectory_;
    
    // Logging
    std::ofstream logFile_;
    
    // Internal methods
    void splittingLoop();
    bool createVideoChunks(const VideoInput& video, std::vector<VideoChunk>& chunks);
    bool saveChunkToFile(const VideoInput& video, VideoChunk& chunk);
    void sendChunkToAI(const VideoChunk& chunk, int aiNumber);
    void logSplittingOperation(const VideoInput& video, const std::vector<VideoChunk>& chunks);
    std::string getChunkPath(const VideoInput& video, int chunkIndex);
    void cleanupJob(const SplittingJob& job);
};

#endif // MASTER_SPLITTER_HPP
