#include "AI4_Receiver.hpp"
#include <iostream>
#include <chrono>

AI4_Receiver::AI4_Receiver() : running_(false) {
    // Constructor
}

AI4_Receiver::~AI4_Receiver() {
    stopAsyncProcessing();
}

void AI4_Receiver::startAsyncProcessing() {
    if (running_) return;
    running_ = true;
    processingThread_ = std::thread(&AI4_Receiver::processingThread, this);
}

void AI4_Receiver::stopAsyncProcessing() {
    if (!running_) return;
    running_ = false;
    queueCV_.notify_all();
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
}

void AI4_Receiver::processingThread() {
    while (running_) {
        std::string videoPath;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCV_.wait(lock, [this]() { return !videoQueue_.empty() || !running_; });
            
            if (!running_) break;
            
            if (!videoQueue_.empty()) {
                videoPath = videoQueue_.front();
                videoQueue_.pop();
            }
        }
        
        if (!videoPath.empty()) {
            // Process video asynchronously (simulated processing time)
            std::this_thread::sleep_for(std::chrono::milliseconds(140)); // Simulate processing
            
            // Add processed data to output queue
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                processedDataQueue_.push("AI4 Processed: " + videoPath);
            }
        }
    }
}

bool AI4_Receiver::isDataAvailable() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return !processedDataQueue_.empty();
}

std::string AI4_Receiver::getNextVideoData() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (processedDataQueue_.empty()) {
        return "";
    }
    std::string data = processedDataQueue_.front();
    processedDataQueue_.pop();
    return data;
}

void AI4_Receiver::enqueueVideo(const std::string& videoPath) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        videoQueue_.push(videoPath);
    }
    queueCV_.notify_one();
}
