#include "Input_Collector.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

Input_Collector::Input_Collector() 
    : collectionActive_(false), nextSequenceIndex_(1) {
    std::cout << "📥 Input_Collector initialized" << std::endl;
    
    // Open log file
    logFile_.open("Input_Collector_Log.txt", std::ios::app);
    if (logFile_.is_open()) {
        logFile_ << "\n=== Input_Collector Session Started ===" << std::endl;
    }
}

Input_Collector::~Input_Collector() {
    stopCollection();
    
    if (logFile_.is_open()) {
        logFile_ << "=== Input_Collector Session Ended ===" << std::endl;
        logFile_.close();
    }
}

void Input_Collector::startCollection() {
    if (collectionActive_) {
        std::cout << "⚠️ Input_Collector: Collection already active" << std::endl;
        return;
    }
    
    collectionActive_ = true;
    collectionThread_ = std::thread(&Input_Collector::collectionLoop, this);
    
    std::cout << "🚀 Input_Collector: Collection started" << std::endl;
    if (logFile_.is_open()) {
        logFile_ << "Collection started" << std::endl;
    }
}

void Input_Collector::stopCollection() {
    if (!collectionActive_) {
        return;
    }
    
    collectionActive_ = false;
    queueCondition_.notify_all();
    
    if (collectionThread_.joinable()) {
        collectionThread_.join();
    }
    
    std::cout << "🛑 Input_Collector: Collection stopped" << std::endl;
    if (logFile_.is_open()) {
        logFile_ << "Collection stopped" << std::endl;
    }
}

bool Input_Collector::receiveVideo(const std::string& videoPath, const std::string& videoId) {
    // Validate video file
    if (!validateVideoFile(videoPath)) {
        std::cerr << "❌ Input_Collector: Invalid video file: " << videoPath << std::endl;
        return false;
    }
    
    // Create video input structure
    VideoInput video;
    video.videoPath = videoPath;
    video.videoId = videoId.empty() ? generateVideoId(videoPath) : videoId;
    video.sequenceIndex = nextSequenceIndex_++;
    video.fileSize = fs::file_size(videoPath);
    video.receivedTime = std::chrono::system_clock::now();
    
    // Add to queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        videoQueue_.push(video);
    }
    
    queueCondition_.notify_one();
    
    // Log the received video
    logVideoReceived(video);
    
    std::cout << "📹 Input_Collector: Video received - " << video.videoId 
              << " (Seq: " << video.sequenceIndex << ", Size: " 
              << (video.fileSize / 1024 / 1024) << " MB)" << std::endl;
    
    return true;
}

bool Input_Collector::getNextVideo(VideoInput& video) {
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    // Wait for video if queue is empty and collection is active
    queueCondition_.wait(lock, [this] { 
        return !videoQueue_.empty() || !collectionActive_; 
    });
    
    if (!collectionActive_ && videoQueue_.empty()) {
        return false;
    }
    
    if (videoQueue_.empty()) {
        return false;
    }
    
    video = videoQueue_.front();
    videoQueue_.pop();
    
    return true;
}

int Input_Collector::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return static_cast<int>(videoQueue_.size());
}

bool Input_Collector::isCollectionActive() const {
    return collectionActive_;
}

void Input_Collector::getCollectionStatus(std::string& status) {
    if (collectionActive_) {
        status = "📥 Input_Collector: Active (Queue: " + std::to_string(getQueueSize()) + " videos)";
    } else {
        status = "🛑 Input_Collector: Inactive";
    }
}

void Input_Collector::collectionLoop() {
    while (collectionActive_) {
        // Collection monitoring logic can be added here
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool Input_Collector::validateVideoFile(const std::string& videoPath) {
    // Check if file exists
    if (!fs::exists(videoPath)) {
        std::cerr << "❌ Video file does not exist: " << videoPath << std::endl;
        return false;
    }
    
    // Check file extension
    std::string extension = fs::path(videoPath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension != ".mp4" && extension != ".avi" && extension != ".mov" && extension != ".mkv") {
        std::cerr << "❌ Unsupported video format: " << extension << std::endl;
        return false;
    }
    
    // Check file size (must be > 0)
    uint64_t fileSize = fs::file_size(videoPath);
    if (fileSize == 0) {
        std::cerr << "❌ Video file is empty: " << videoPath << std::endl;
        return false;
    }
    
    return true;
}

void Input_Collector::logVideoReceived(const VideoInput& video) {
    if (logFile_.is_open()) {
        auto time_t = std::chrono::system_clock::to_time_t(video.receivedTime);
        logFile_ << "Video Received: " << video.videoId 
                << " | Path: " << video.videoPath 
                << " | Sequence: " << video.sequenceIndex
                << " | Size: " << (video.fileSize / 1024 / 1024) << " MB"
                << " | Time: " << std::ctime(&time_t);
    }
}

std::string Input_Collector::generateVideoId(const std::string& videoPath) {
    // Generate video ID from filename and timestamp
    std::string filename = fs::path(videoPath).stem().string();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return filename + "_" + std::to_string(timestamp);
}
