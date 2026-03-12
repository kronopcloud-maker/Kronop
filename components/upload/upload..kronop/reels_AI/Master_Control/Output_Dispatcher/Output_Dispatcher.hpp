#ifndef OUTPUT_DISPATCHER_HPP
#define OUTPUT_DISPATCHER_HPP

#include <string>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <memory>

/**
 * Output_Dispatcher.hpp
 * Final Video Output and Server Dispatch System
 * Sends merged videos to server for final delivery
 */

struct VideoOutput {
    std::string videoId;
    std::string videoPath;
    std::string serverUrl;
    uint64_t fileSize;
    int sequenceIndex;
    std::chrono::system_clock::time_point processedTime;
    bool isDispatched;
};

struct DispatchJob {
    VideoOutput videoOutput;
    std::string serverResponse;
    bool isCompleted;
    std::chrono::system_clock::time_point dispatchTime;
    int retryCount;
};

class Output_Dispatcher {
public:
    Output_Dispatcher();
    ~Output_Dispatcher();

    // Start/stop dispatching
    void startDispatching();
    void stopDispatching();

    // Video output and dispatch operations
    bool receiveMergedVideo(const std::string& videoId, const std::string& videoPath, int sequenceIndex);
    bool dispatchToServer(const VideoOutput& videoOutput);
    
    // Server configuration
    void setServerUrl(const std::string& serverUrl);
    void setDispatchTimeout(int timeoutSeconds);
    
    // Status and monitoring
    int getQueueSize() const;
    int getDispatchedCount() const;
    int getFailedCount() const;
    bool isDispatchingActive() const;
    void getDispatchStatus(std::string& status);

private:
    // Dispatching thread
    std::thread dispatchingThread_;
    std::atomic<bool> dispatchingActive_;
    
    // Queue management
    std::queue<DispatchJob> dispatchQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    
    // Server configuration
    std::string serverUrl_;
    int dispatchTimeout_;
    int maxRetries_;
    
    // Statistics
    std::atomic<int> dispatchedCount_;
    std::atomic<int> failedCount_;
    
    // Logging
    std::ofstream logFile_;
    
    // Internal methods
    void dispatchingLoop();
    bool uploadToServer(const VideoOutput& videoOutput, std::string& response);
    bool validateVideoFile(const std::string& videoPath);
    void logDispatchOperation(const DispatchJob& job);
    void logServerResponse(const std::string& videoId, const std::string& response);
    std::string generateServerRequest(const VideoOutput& videoOutput);
    bool checkServerResponse(const std::string& response);
    void cleanupDispatchedJob(const DispatchJob& job);
    std::string getServerEndpoint();
    void handleDispatchFailure(DispatchJob& job);
};

#endif // OUTPUT_DISPATCHER_HPP
