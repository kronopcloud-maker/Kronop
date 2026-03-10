#ifndef AI4_RECEIVER_HPP
#define AI4_RECEIVER_HPP

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class AI4_Receiver {
public:
    AI4_Receiver();
    ~AI4_Receiver();

    // Asynchronous receive data from AI 4 folder
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

#endif // AI4_RECEIVER_HPP
