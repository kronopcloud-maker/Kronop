#ifndef AI2_RECEIVER_HPP
#define AI2_RECEIVER_HPP

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class AI2_Receiver {
public:
    AI2_Receiver();
    ~AI2_Receiver();

    // Asynchronous receive data from AI 2 folder
    void startAsyncProcessing();
    void stopAsyncProcessing();
    bool isDataAvailable();
    std::string getNextVideoData();

    // Add video to processing queue
    void enqueueVideo(const std::string& videoPath);

private:
    void processingThread();
    
    std::queue<std::string> videoQueue_;
    std::queue<std::string> processedDataQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    std::thread processingThread_;
    std::atomic<bool> running_;
};

#endif // AI2_RECEIVER_HPP
