#ifndef INPUT_COLLECTOR_HPP
#define INPUT_COLLECTOR_HPP

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>

/**
 * Input_Collector.hpp
 * Video Input Collection System
 * Receives and queues video files for processing
 */

struct VideoInput {
    std::string videoPath;
    std::string videoId;
    int sequenceIndex;
    uint64_t fileSize;
    std::chrono::system_clock::time_point receivedTime;
};

class Input_Collector {
public:
    Input_Collector();
    ~Input_Collector();

    // Start/stop collection
    void startCollection();
    void stopCollection();

    // Video input methods
    bool receiveVideo(const std::string& videoPath, const std::string& videoId = "");
    bool getNextVideo(VideoInput& video);
    
    // Status and monitoring
    int getQueueSize() const;
    bool isCollectionActive() const;
    void getCollectionStatus(std::string& status);

private:
    // Video queue
    std::queue<VideoInput> videoQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    
    // Collection thread
    std::thread collectionThread_;
    std::atomic<bool> collectionActive_;
    
    // Sequence management
    std::atomic<int> nextSequenceIndex_;
    
    // Logging
    std::ofstream logFile_;
    
    // Internal methods
    void collectionLoop();
    bool validateVideoFile(const std::string& videoPath);
    void logVideoReceived(const VideoInput& video);
    std::string generateVideoId(const std::string& videoPath);
};

#endif // INPUT_COLLECTOR_HPP
