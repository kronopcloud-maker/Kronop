#include "AI1_Receiver.hpp"
#include <iostream>
#include <chrono>

AI1_Receiver::AI1_Receiver() : running_(false) {
    // Constructor
}

AI1_Receiver::~AI1_Receiver() {
    stopAsyncProcessing();
}

void AI1_Receiver::startAsyncProcessing() {
    if (running_) return;
    running_ = true;
    processingThread_ = std::thread(&AI1_Receiver::processingThread, this);
}

void AI1_Receiver::stopAsyncProcessing() {
    if (!running_) return;
    running_ = false;
    queueCV_.notify_all();
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
}

void AI1_Receiver::processingThread() {
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
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate processing
            
            // Add processed data to output queue
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                processedDataQueue_.push("AI1 Processed: " + videoPath);
            }
        }
    }
}

bool AI1_Receiver::isDataAvailable() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return !processedDataQueue_.empty();
}

std::string AI1_Receiver::getNextVideoData() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (processedDataQueue_.empty()) {
        return "";
    }
    std::string data = processedDataQueue_.front();
    processedDataQueue_.pop();
    return data;
}

void AI1_Receiver::enqueueVideo(const std::string& videoPath) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        videoQueue_.push(videoPath);
    }
    queueCV_.notify_one();
}
