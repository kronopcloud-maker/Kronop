#include "Input_Manager.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

InputManager::InputManager(AI1_Receiver& ai1, AI2_Receiver& ai2, AI3_Receiver& ai3, AI4_Receiver& ai4, AI5_Receiver& ai5)
    : ai1Receiver_(ai1), ai2Receiver_(ai2), ai3Receiver_(ai3), ai4Receiver_(ai4), ai5Receiver_(ai5), running_(false),
      currentAICounter_(0), ai1Enabled_(true), ai2Enabled_(true), ai3Enabled_(true), ai4Enabled_(true), ai5Enabled_(true),
      nextVideoIndex_(1), pauseSignal_(false), tempFolder_("Kronop_Temp") {
}

InputManager::~InputManager() {
    stopProcessing();
}

void InputManager::startProcessing() {
    if (running_) return;
    running_ = true;

    // Start AI receivers
    ai1Receiver_.startAsyncProcessing();
    if (ai2Enabled_) ai2Receiver_.startAsyncProcessing();
    if (ai3Enabled_) ai3Receiver_.startAsyncProcessing();
    if (ai4Enabled_) ai4Receiver_.startAsyncProcessing();
    if (ai5Enabled_) ai5Receiver_.startAsyncProcessing();

    processingThread_ = std::thread(&InputManager::processingThread, this);
    preFetchVideos();
}

void InputManager::stopProcessing() {
    if (!running_) return;
    running_ = false;

    // Stop AI receivers
    ai1Receiver_.stopAsyncProcessing();
    if (ai2Enabled_) ai2Receiver_.stopAsyncProcessing();
    if (ai3Enabled_) ai3Receiver_.stopAsyncProcessing();
    if (ai4Enabled_) ai4Receiver_.stopAsyncProcessing();
    if (ai5Enabled_) ai5Receiver_.stopAsyncProcessing();

    queueCV_.notify_all();
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
}

void InputManager::enqueueVideo(const std::string& videoPath) {
    // Gatekeeper: Hard limit of 5 videos
    int count = countVideosInFolder(tempFolder_);
    if (count >= 5) {
        std::cout << "Gatekeeper: Hard limit 5 reached, cannot enqueue new video" << std::endl;
        return;
    }

    int index = nextVideoIndex_++;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        videoQueue_.push({index, videoPath});
    }
    queueCV_.notify_one();
}


void InputManager::setEnabledAIs(bool enableAI1, bool enableAI2, bool enableAI3, bool enableAI4, bool enableAI5) {
    ai1Enabled_ = enableAI1;
    ai2Enabled_ = enableAI2;
    ai3Enabled_ = enableAI3;
    ai4Enabled_ = enableAI4;
    ai5Enabled_ = enableAI5;
}

void InputManager::processingThread() {
    while (running_) {
        // Check pause signal
        {
            std::unique_lock<std::mutex> pauseLock(pauseMutex_);
            if (pauseSignal_) {
                pauseCV_.wait(pauseLock, [this]{return !pauseSignal_ || !running_;});
                if (!running_) break;
            }
        }

        int videoIndex = -1;
        std::string videoPath;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCV_.wait(lock, [this]() { return !videoQueue_.empty() || !running_; });

            if (!running_) break;

            if (!videoQueue_.empty()) {
                auto [index, path] = videoQueue_.front();
                videoIndex = index;
                videoPath = path;
                videoQueue_.pop();
            }
        }

        if (!videoPath.empty()) {
            AIUnit assignedAI = getNextAI();
            assignVideoToAI(videoIndex, videoPath, assignedAI);
        }
    }
}

AIUnit InputManager::getNextAI() {
    // Round robin logic
    const int totalEnabledAIs = (ai1Enabled_ ? 1 : 0) + (ai2Enabled_ ? 1 : 0) + (ai3Enabled_ ? 1 : 0) + (ai4Enabled_ ? 1 : 0) + (ai5Enabled_ ? 1 : 0);

    if (totalEnabledAIs == 0) return AI_NONE;

    AIUnit assignedAI = AI_NONE;
    int attempts = 0;

    do {
        currentAICounter_ = (currentAICounter_ % 5) + 1; // 1, 2, 3, 4, 5 cycle
        assignedAI = static_cast<AIUnit>(currentAICounter_);

        if ((assignedAI == AI_1 && ai1Enabled_) ||
            (assignedAI == AI_2 && ai2Enabled_) ||
            (assignedAI == AI_3 && ai3Enabled_) ||
            (assignedAI == AI_4 && ai4Enabled_) ||
            (assignedAI == AI_5 && ai5Enabled_)) {
            return assignedAI;
        }

        attempts++;
    } while (attempts < 5);

    // Fallback to AI_1 if no other AI is enabled
    return ai1Enabled_ ? AI_1 : AI_NONE;
}

void InputManager::assignVideoToAI(int index, const std::string& videoPath, AIUnit ai) {
    std::string processedData;

    switch (ai) {
        case AI_1:
            ai1Receiver_.enqueueVideo(videoPath);
            // Wait for processing (simulate)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (ai1Receiver_.isDataAvailable()) {
                processedData = ai1Receiver_.getNextVideoData();
            }
            break;
        case AI_2:
            ai2Receiver_.enqueueVideo(videoPath);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (ai2Receiver_.isDataAvailable()) {
                processedData = ai2Receiver_.getNextVideoData();
            }
            break;
        case AI_3:
            ai3Receiver_.enqueueVideo(videoPath);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (ai3Receiver_.isDataAvailable()) {
                processedData = ai3Receiver_.getNextVideoData();
            }
            break;
        case AI_4:
            ai4Receiver_.enqueueVideo(videoPath);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (ai4Receiver_.isDataAvailable()) {
                processedData = ai4Receiver_.getNextVideoData();
            }
            break;
        case AI_5:
            ai5Receiver_.enqueueVideo(videoPath);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (ai5Receiver_.isDataAvailable()) {
                processedData = ai5Receiver_.getNextVideoData();
            }
            break;
        default:
            return;
    }

    // Save processed video to cache folder
    std::string filePath = tempFolder_ + "/" + std::to_string(index) + ".mp4";
    std::ofstream file(filePath, std::ios::binary);
    if (file) {
        file.write(processedData.data(), processedData.size());
        file.close();
        std::cout << "Video " << videoPath << " processed by AI-" << static_cast<int>(ai) << " and saved to " << filePath << std::endl;
    } else {
        std::cerr << "Failed to save video to " << filePath << std::endl;
    }
}

void InputManager::preFetchVideos() {
    // Pre-fetch next 5 videos to AI-1, AI-2, AI-3, AI-4, AI-5 when user starts
    enqueueVideo("pre-fetch-1");
    if (ai2Enabled_) enqueueVideo("pre-fetch-2");
    if (ai3Enabled_) enqueueVideo("pre-fetch-3");
    if (ai4Enabled_) enqueueVideo("pre-fetch-4");
    if (ai5Enabled_) enqueueVideo("pre-fetch-5");
    std::cout << "Pre-fetching next videos to AIs" << std::endl;
}

int InputManager::countVideosInFolder(const std::string& folder) {
    int count = 0;
    try {
        for (const auto& entry : fs::directory_iterator(folder)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mp4") {
                count++;
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Ignore errors
    }
    return count;
}
